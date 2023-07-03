// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtQmlLS/private/qqmllanguageserver_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qcoreapplication.h>
#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qmutex.h>
#include <QtCore/QMutexLocker>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qrunnable.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/qtimer.h>

#include <QtJsonRpc/private/qhttpmessagestreamparser_p.h>

#include <QtQmlCompiler/private/qqmljsresourcefilemapper_p.h>
#include <QtQmlCompiler/private/qqmljscompiler_p.h>
#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <QtQmlCompiler/private/qqmljsscope_p.h>
#include <QtQmlCompiler/private/qqmljsimporter_p.h>
#if QT_CONFIG(commandlineparser)
#    include <QtCore/qcommandlineparser.h>
#endif

#ifndef QT_BOOTSTRAPPED
#    include <QtCore/qlibraryinfo.h>
#endif

#include <iostream>
#ifdef Q_OS_WIN32
#    include <fcntl.h>
#    include <io.h>
#endif

using namespace QmlLsp;

QFile *logFile = nullptr;
QBasicMutex *logFileLock = nullptr;

class StdinReader : public QObject
{
    Q_OBJECT
public:
    void run()
    {
        auto guard = qScopeGuard([this]() { emit eof(); });
        const constexpr qsizetype bufSize = 1024;
        qsizetype bytesInBuf = 0;
        char bufferData[2 * bufSize];
        char *buffer = static_cast<char *>(bufferData);

        auto trySend = [this, &bytesInBuf, buffer]() {
            if (bytesInBuf == 0)
                return;
            qsizetype toSend = bytesInBuf;
            bytesInBuf = 0;
            QByteArray dataToSend(buffer, toSend);
            emit receivedData(dataToSend);
        };
        QHttpMessageStreamParser streamParser(
                [](const QByteArray &, const QByteArray &) { /* just a header, do nothing */ },
                [&trySend](const QByteArray &) {
                    // message body
                    trySend();
                },
                [&trySend](QtMsgType, QString) {
                    // there was an error
                    trySend();
                },
                QHttpMessageStreamParser::UNBUFFERED);

        while (std::cin.get(buffer[bytesInBuf])) { // should poll/select and process events
            qsizetype readNow = std::cin.readsome(buffer + bytesInBuf + 1, bufSize) + 1;
            QByteArray toAdd(buffer + bytesInBuf, readNow);
            bytesInBuf += readNow;
            if (bytesInBuf >= bufSize)
                trySend();
            streamParser.receiveData(toAdd);
        }
        trySend();
    }
signals:
    void receivedData(const QByteArray &data);
    void eof();
};

// To debug:
//
// * simple logging can be redirected to a file
//   passing -l <file> to the qmlls command
//
// * more complex debugging can use named pipes:
//
//     mkfifo qmllsIn
//     mkfifo qmllsOut
//
// this together with a qmllsEcho script that can be defined as
//
//     #!/bin/sh
//     cat -u < ~/qmllsOut &
//     cat -u > ~/qmllsIn
//
// allows to use qmllsEcho as lsp server, and still easily start
// it in a terminal
//
//     qmlls < ~/qmllsIn > ~/qmllsOut
//
// * statup can be slowed down to have the time to attach via the
//   -w <nSeconds> flag.

int main(int argv, char *argc[])
{
#ifdef Q_OS_WIN32
    // windows does not open stdin/stdout in binary mode by default
    int err = _setmode(_fileno(stdout), _O_BINARY);
    if (err == -1)
        perror("Cannot set mode for stdout");
    err = _setmode(_fileno(stdin), _O_BINARY);
    if (err == -1)
        perror("Cannot set mode for stdin");
#endif

    QHashSeed::setDeterministicGlobalSeed();
    QCoreApplication app(argv, argc);
    QCoreApplication::setApplicationName("qmlls");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QCommandLineParser parser;
    QQmlToolingSettings settings(QLatin1String("qmlls"));
    parser.setApplicationDescription(QLatin1String(R"(QML languageserver)"));

    parser.addHelpOption();
    QCommandLineOption waitOption(QStringList() << "w"
                                                << "wait",
                                  QLatin1String("Waits the given number of seconds before startup"),
                                  QLatin1String("waitSeconds"));
    parser.addOption(waitOption);

    QCommandLineOption verboseOption(
            QStringList() << "v"
                          << "verbose",
            QLatin1String("Outputs extra information on the operations being performed"));
    parser.addOption(verboseOption);

    QCommandLineOption logFileOption(QStringList() << "l"
                                                   << "log-file",
                                     QLatin1String("Writes logging to the given file"),
                                     QLatin1String("logFile"));
    parser.addOption(logFileOption);

    QString buildDir = QStringLiteral(u"buildDir");
    QCommandLineOption buildDirOption(
            QStringList() << "b"
                          << "build-dir",
            QLatin1String("Adds a build dir to look up for qml information"), buildDir);
    parser.addOption(buildDirOption);
    settings.addOption(buildDir);

    QCommandLineOption writeDefaultsOption(
            QStringList() << "write-defaults",
            QLatin1String("Writes defaults settings to .qmlls.ini and exits (Warning: This "
                          "will overwrite any existing settings and comments!)"));
    parser.addOption(writeDefaultsOption);

    QCommandLineOption ignoreSettings(QStringList() << "ignore-settings",
                                      QLatin1String("Ignores all settings files and only takes "
                                                    "command line options into consideration"));
    parser.addOption(ignoreSettings);

    parser.process(app);

    if (parser.isSet(writeDefaultsOption)) {
        return settings.writeDefaults() ? 0 : 1;
    }
    if (parser.isSet(logFileOption)) {
        QString fileName = parser.value(logFileOption);
        qInfo() << "will log to" << fileName;
        logFile = new QFile(fileName);
        logFileLock = new QMutex;
        logFile->open(QFile::WriteOnly | QFile::Truncate | QFile::Text);
        qInstallMessageHandler([](QtMsgType t, const QMessageLogContext &, const QString &msg) {
            QMutexLocker l(logFileLock);
            logFile->write(QString::number(int(t)).toUtf8());
            logFile->write(" ");
            logFile->write(msg.toUtf8());
            logFile->write("\n");
            logFile->flush();
        });
    }
    if (parser.isSet(verboseOption))
        QLoggingCategory::setFilterRules("qt.languageserver*.debug=true\n");
    if (parser.isSet(waitOption)) {
        int waitSeconds = parser.value(waitOption).toInt();
        if (waitSeconds > 0)
            qDebug() << "waiting";
        QThread::sleep(waitSeconds);
        qDebug() << "starting";
    }
    QMutex writeMutex;
    QQmlLanguageServer qmlServer(
            [&writeMutex](const QByteArray &data) {
                QMutexLocker l(&writeMutex);
                std::cout.write(data.constData(), data.size());
                std::cout.flush();
            },
            (parser.isSet(ignoreSettings) ? nullptr : &settings));

    const QStringList envPaths =
            qEnvironmentVariable("QMLLS_BUILD_DIRS").split(u',', Qt::SkipEmptyParts);
    for (const QString &envPath : envPaths) {
        QFileInfo info(envPath);
        if (!info.exists()) {
            qWarning() << "Argument" << buildDir << "passed via QMLLS_BUILD_DIRS does not exist.";
        } else if (!info.isDir()) {
            qWarning() << "Argument" << buildDir
                       << "passed via QMLLS_BUILD_DIRS is not a directory.";
        }
    }

    QStringList buildDirs;
    if (parser.isSet(buildDirOption)) {
        buildDirs = parser.values(buildDirOption);
        for (const QString &buildDir : buildDirs) {
            QFileInfo info(buildDir);
            if (!info.exists()) {
                qWarning() << "Argument" << buildDir << "passed to --build-dir does not exist.";
            } else if (!info.isDir()) {
                qWarning() << "Argument" << buildDir << "passed to --build-dir is not a directory.";
            }
        }
        qmlServer.codeModel()->setBuildPathsForRootUrl(QByteArray(), buildDirs);
    }

    if (!buildDirs.isEmpty()) {
        qInfo() << "Using the build directories passed via the --build-dir option:"
                << buildDirs.join(", ");
    } else if (!envPaths.isEmpty()) {
        qInfo() << "Using the build directories passed via the QMLLS_BUILD_DIRS environment "
                   "variable"
                << buildDirs.join(", ");
    } else {
        qInfo() << "Using the build directories found in the .qmlls.ini file. Your build folder "
                   "might not be found if no .qmlls.ini files are present in the root source "
                   "folder.";
    }

    if (buildDirs.isEmpty() && envPaths.isEmpty()) {
        qInfo() << "Build directory path omitted: Your source folders will be searched for "
                   ".qmlls.ini files.";
    }
    StdinReader r;
    QObject::connect(&r, &StdinReader::receivedData,
                     qmlServer.server(), &QLanguageServer::receiveData);
    QObject::connect(&r, &StdinReader::eof, &app, [&app]() {
        QTimer::singleShot(100, &app, []() {
            QCoreApplication::processEvents();
            QCoreApplication::exit();
        });
    });
    QThreadPool::globalInstance()->start([&r]() { r.run(); });
    app.exec();
    return qmlServer.returnValue();
}

#include "qmllanguageservertool.moc"
