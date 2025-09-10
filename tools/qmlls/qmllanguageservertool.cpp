// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtQmlLS/private/qqmllanguageserver_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qcoreapplication.h>
#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtQmlToolingSettings/private/qqmltoolingutils_p.h>
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
    StdinReader()
        : m_streamReader(
                [](const QByteArray &, const QByteArray &) { /* just a header, do nothing */ },
                [this](const QByteArray &) {
                    // stop reading until we are sure that the server is not shutting down
                    m_isReading = false;

                    // message body
                    m_shouldSendData = true;
                },
                [this](QtMsgType, QString) {
                    // there was an error
                    m_shouldSendData = true;
                },
                QHttpMessageStreamParser::UNBUFFERED)
    {
    }

    void sendData()
    {
        const bool isEndOfMessage = !m_isReading && !m_hasEof;
        const qsizetype toSend = m_bytesInBuf;
        m_bytesInBuf = 0;
        const QByteArray dataToSend(m_buffer, toSend);
        emit receivedData(dataToSend, isEndOfMessage);
    }

private:
    const static constexpr qsizetype s_bufSize = 1024;
    qsizetype m_bytesInBuf = 0;
    char m_buffer[2 * s_bufSize] = {};
    QHttpMessageStreamParser m_streamReader;
    /*!
    \internal
    Indicates if the current message is not read out entirely.
    */
    bool m_isReading = true;
    /*!
    \internal
    Indicates if an EOF was encountered. No more data can be read after an EOF.
    */
    bool m_hasEof = false;
    /*!
    \internal
    Indicates whether sendData() should be called or not.
    */
    bool m_shouldSendData = false;
signals:
    void receivedData(const QByteArray &data, bool canRequestMoreData);
    void eof();
public slots:
    void readNextMessage()
    {
        if (m_hasEof)
            return;
        m_isReading = true;
        // Try to fill up the buffer as much as possible before calling the queued signal:
        // each loop iteration might read only one character from std::in in the worstcase, this
        // happens for example on macos.
        while (m_isReading) {
            // block while waiting for some data
            if (!std::cin.get(m_buffer[m_bytesInBuf])) {
                m_hasEof = true;
                emit eof();
                return;
            }
            // see if more data is available and fill the buffer with it
            qsizetype readNow = std::cin.readsome(m_buffer + m_bytesInBuf + 1, s_bufSize) + 1;
            QByteArray toAdd(m_buffer + m_bytesInBuf, readNow);
            m_bytesInBuf += readNow;
            m_streamReader.receiveData(toAdd);

            m_shouldSendData |= m_bytesInBuf >= s_bufSize;
            if (std::exchange(m_shouldSendData, false))
                sendData();
        }
    }
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

    QString qmlImportPath = QStringLiteral(u"importPaths");
    QCommandLineOption qmlImportPathOption(
            QStringList() << "I", QLatin1String("Look for QML modules in the specified directory"),
            qmlImportPath);
    parser.addOption(qmlImportPathOption);
    settings.addOption(qmlImportPath);

    QCommandLineOption environmentOption(
            QStringList() << "E",
            QLatin1String("Use the QML_IMPORT_PATH environment variable to look for QML Modules"));
    parser.addOption(environmentOption);

    QCommandLineOption writeDefaultsOption(
            QStringList() << "write-defaults",
            QLatin1String("Writes defaults settings to .qmlls.ini and exits (Warning: This "
                          "will overwrite any existing settings and comments!)"));
    parser.addOption(writeDefaultsOption);

    QCommandLineOption ignoreSettings(QStringList() << "ignore-settings",
                                      QLatin1String("Ignores all settings files and only takes "
                                                    "command line options into consideration"));
    parser.addOption(ignoreSettings);

    QCommandLineOption noCMakeCallsOption(
            QStringList() << "no-cmake-calls",
            QLatin1String("Disables automatic CMake rebuilds and C++ file watching."));
    parser.addOption(noCMakeCallsOption);
    settings.addOption("no-cmake-calls", "false");

    QCommandLineOption docDir(
            { { "d", "p", "doc-dir" },
              QLatin1String("Documentation path to use for the documentation hints feature"),
              "path",
              QString() });
    parser.addOption(docDir);
    settings.addOption("docDir");

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

    if (parser.isSet(docDir))
        qmlServer.codeModel()->setDocumentationRootPath(parser.value(docDir).toUtf8());

    const bool disableCMakeCallsViaEnvironment =
            qmlGetConfigOption<bool, qmlConvertBoolConfigOption>("QMLLS_NO_CMAKE_CALLS");

    if (disableCMakeCallsViaEnvironment || parser.isSet(noCMakeCallsOption)) {
        if (disableCMakeCallsViaEnvironment) {
            qWarning() << "Disabling CMake calls via QMLLS_NO_CMAKE_CALLS environment variable.";
        } else {
            qWarning() << "Disabling CMake calls via command line switch.";
        }

        qmlServer.codeModel()->disableCMakeCalls();
    }

    if (parser.isSet(buildDirOption)) {
        const QStringList dirs =
                QQmlToolingUtils::getAndWarnForInvalidDirsFromOption(parser, buildDirOption);

        qInfo().nospace().noquote()
                << "Using build directories passed by -b: \"" << dirs.join(u"\", \""_s) << "\".";

        qmlServer.codeModel()->setBuildPathsForRootUrl(QByteArray(), dirs);
    } else if (QStringList dirsFromEnv =
                       QQmlToolingUtils::getAndWarnForInvalidDirsFromEnv("QMLLS_BUILD_DIRS");
               !dirsFromEnv.isEmpty()) {

        // warn now at qmlls startup that those directories will be used later in qqmlcodemodel when
        // searching for build folders.
        qInfo().nospace().noquote() << "Using build directories passed from environment variable "
                                       "\"QMLLS_BUILD_DIRS\": \""
                                    << dirsFromEnv.join(u"\", \""_s) << "\".";

    } else {
        qInfo() << "Using the build directories found in the .qmlls.ini file. Your build folder "
                   "might not be found if no .qmlls.ini files are present in the root source "
                   "folder.";
    }
    QStringList importPaths{ QLibraryInfo::path(QLibraryInfo::QmlImportsPath) };
    if (parser.isSet(qmlImportPathOption)) {
        const QStringList pathsFromOption =
                QQmlToolingUtils::getAndWarnForInvalidDirsFromOption(parser, qmlImportPathOption);
        qInfo().nospace().noquote() << "Using import directories passed by -I: \""
                                    << pathsFromOption.join(u"\", \""_s) << "\".";
        importPaths << pathsFromOption;
    }
    if (parser.isSet(environmentOption)) {
        if (const QStringList dirsFromEnv =
                    QQmlToolingUtils::getAndWarnForInvalidDirsFromEnv(u"QML_IMPORT_PATH"_s);
            !dirsFromEnv.isEmpty()) {
            qInfo().nospace().noquote()
                    << "Using import directories passed from environment variable "
                       "\"QML_IMPORT_PATH\": \""
                    << dirsFromEnv.join(u"\", \""_s) << "\".";
            importPaths << dirsFromEnv;
        }

        if (const QStringList dirsFromEnv2 =
                    QQmlToolingUtils::getAndWarnForInvalidDirsFromEnv(u"QML2_IMPORT_PATH"_s);
            !dirsFromEnv2.isEmpty()) {
            qInfo().nospace().noquote()
                    << "Using import directories passed from the deprecated environment variable "
                       "\"QML2_IMPORT_PATH\": \""
                    << dirsFromEnv2.join(u"\", \""_s) << "\".";
            importPaths << dirsFromEnv2;
        }
    }
    qmlServer.codeModel()->setImportPaths(importPaths);

    StdinReader r;
    QThread workerThread;
    r.moveToThread(&workerThread);
    QObject::connect(&r, &StdinReader::receivedData,
                     qmlServer.server(), &QLanguageServer::receiveData);
    QObject::connect(qmlServer.server(), &QLanguageServer::readNextMessage, &r,
                     &StdinReader::readNextMessage);
    auto exit = [&app, &workerThread]() {
        workerThread.quit();
        workerThread.wait();
        QTimer::singleShot(100, &app, []() {
            QCoreApplication::processEvents();
            QCoreApplication::exit();
        });
    };
    QObject::connect(&r, &StdinReader::eof, &app, exit);
    QObject::connect(qmlServer.server(), &QLanguageServer::exit, exit);

    emit r.readNextMessage();
    workerThread.start();
    app.exec();
    workerThread.quit();
    workerThread.wait();
    return qmlServer.returnValue();
}

#include "qmllanguageservertool.moc"
