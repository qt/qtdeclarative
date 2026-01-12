// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qmllsmain_p.h"

#include <private/qhttpmessagestreamparser_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmljscompiler_p.h>
#include <private/qqmljsimporter_p.h>
#include <private/qqmljslogger_p.h>
#include <private/qqmljsresourcefilemapper_p.h>
#include <private/qqmljsscope_p.h>
#include <private/qqmllanguageserver_p.h>
#include <private/qqmltoolingsettings_p.h>
#include <private/qqmltoolingutils_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qcoreapplication.h>
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

#if QT_CONFIG(commandlineparser)
#  include <QtCore/qcommandlineparser.h>
#endif

#ifndef QT_BOOTSTRAPPED
#  include <QtCore/qlibraryinfo.h>
#endif

#include <iostream>
#ifdef Q_OS_WIN32
#  include <fcntl.h>
#  include <io.h>
#endif

QT_BEGIN_NAMESPACE

namespace QmlLsp {

QFile *logFile = nullptr;
QBasicMutex *logFileLock = nullptr;

class AbstractReader : public QObject
{
    Q_OBJECT
public:
    virtual ~AbstractReader() { }

protected:
    virtual void readNextMessageImpl() = 0;
signals:
    void receivedData(const QByteArray &data, bool canRequestMoreData);
    void eof();
public slots:
    void readNextMessage() { readNextMessageImpl(); }
};

class StdinReader final : public AbstractReader
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

protected:
    void readNextMessageImpl() override
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
                qInfo() << "Received EOF, stopping...";
                m_hasEof = true;
                emit eof();
                return;
            }
            // see if more data is available and fill the buffer with it
            qsizetype readNow = std::cin.readsome(m_buffer + m_bytesInBuf + 1, s_bufSize) + 1;
            QByteArray toAdd(m_buffer + m_bytesInBuf, readNow);
            m_bytesInBuf += readNow;
            m_streamReader.receiveData(std::move(toAdd));

            m_shouldSendData |= m_bytesInBuf >= s_bufSize;
            if (std::exchange(m_shouldSendData, false))
                sendData();
        }
    }
};

/*!
   \internal
   FileReader allows to read JsonRPC commands from a file. This is useful when the JsonRPC
   commands can't be read from stdin, for example when running qmlls in the macos profiler.
 */
class FileReader final : public AbstractReader
{
    Q_OBJECT
public:
    FileReader(const QString &fileName) : m_file(fileName)
    {
        bool isOpen = m_file.open(QFile::ReadOnly);
        Q_ASSERT(isOpen);
    }

private:
    QFile m_file;

protected:
    void readNextMessageImpl() override
    {
        if (!m_file.isOpen())
            return;
        emit receivedData(m_file.readAll(), false);
        // emit eof will shutdown qmlls before it manages to complete the preceding requests, so
        // don't emit eof when reading from a file.
        // also, just read the content of the file once, so close it now
        m_file.close();
    }
};

static QStringList collectImportPaths(const QCommandLineParser &parser,
                                      const QCommandLineOption &qmlImportPathOption,
                                      const QCommandLineOption &environmentOption,
                                      const QCommandLineOption &qmlImportNoDefault)
{
    QStringList importPaths;
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

    // add as default fallback at the end
    if (!parser.isSet(qmlImportNoDefault))
        importPaths << QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    return importPaths;
}

static int cmakeJobs(const QCommandLineParser &parser, const QCommandLineOption &cmakeJobsOption)
{
    auto parseAndWarn = [](const QString &valueString, const QString &infoMessage,
                           const QString &warningMessage) {
        bool ok = false;
        if (valueString == QQmlCodeModel::s_maxCMakeJobs) {
            const int value = QThread::idealThreadCount();
            qInfo().noquote() << infoMessage.arg("max (%1)"_L1.arg(QString::number(value)));
            return value;
        }

        const int value = valueString.toInt(&ok);
        if (!ok || value < 1) {
            qInfo().noquote() << warningMessage.arg(valueString);
            return QQmlCodeModel::s_defaultCMakeJobs;
        }
        qInfo().noquote() << infoMessage.arg(QString::number(value));
        return value;
    };

    if (parser.isSet(cmakeJobsOption)) {
        return parseAndWarn(
                parser.value(cmakeJobsOption), "Using %1 jobs for CMake, set via --cmake-jobs."_L1,
                "Value \"%1\" passed to --cmake-jobs is not a number greater than 0 and not "
                "\"max\", using default value of 1 instead."_L1);
    }

    if (!qEnvironmentVariableIsSet("QMLLS_CMAKE_JOBS")) {
        qInfo() << "Using 1 job for CMake, you can increase that value with the QMLLS_CMAKE_JOBS "
                   "environment variable or the --cmake-jobs option.";
        return QQmlCodeModel::s_defaultCMakeJobs;
    }
    return parseAndWarn(
            qEnvironmentVariable("QMLLS_CMAKE_JOBS"),
            "Using %1 jobs for CMake, set via QMLLS_CMAKE_JOBS environment variable."_L1,
            "Value \"%1\" passed to QMLLS_CMAKE_JOBS is not a number greater than 0 and not "
            "\"max\", using default value of 1 instead."_L1);
}

static bool prepareCMakeFeature(const QCommandLineParser &parser,
                                const QCommandLineOption &noCMakeCallsOption)
{
    if (qmlGetConfigOption<bool, qmlConvertBoolConfigOption>("QMLLS_NO_CMAKE_CALLS")) {
        qWarning() << "Disabling CMake calls via QMLLS_NO_CMAKE_CALLS environment variable.";
        return false;
    }
    if (parser.isSet(noCMakeCallsOption)) {
        qWarning() << "Disabling CMake calls via command line switch.";
        return false;
    }
    return true;
}

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

int qmllsMain(int argv, char *argc[])
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

    QCommandLineParser parser;
    QQmlToolingSharedSettings settings("qmlls"_L1);
    parser.setApplicationDescription("QML languageserver"_L1);

    parser.addHelpOption();
    QCommandLineOption waitOption(QStringList() << "w"_L1
                                                << "wait"_L1,
                                  "Waits the given number of seconds before startup"_L1,
                                  "waitSeconds"_L1);
    parser.addOption(waitOption);

    QCommandLineOption verboseOption(
            QStringList() << "v"_L1
                          << "verbose"_L1,
            "Outputs extra information on the operations being performed"_L1);
    parser.addOption(verboseOption);

    QCommandLineOption logFileOption(QStringList() << "l"_L1
                                                   << "log-file"_L1,
                                     "Writes logging to the given file"_L1, "logFile"_L1);
    parser.addOption(logFileOption);

    QString buildDir = QStringLiteral(u"buildDir");
    QCommandLineOption buildDirOption(QStringList() << "b"_L1
                                                    << "build-dir"_L1,
                                      "Adds a build dir to look up for qml information"_L1,
                                      buildDir);
    parser.addOption(buildDirOption);
    settings.addOption(buildDir);

    QString qmlImportPath = QStringLiteral(u"importPaths");
    QCommandLineOption qmlImportPathOption(QStringList() << "I"_L1,
                                           "Look for QML modules in the specified directory"_L1,
                                           qmlImportPath);
    parser.addOption(qmlImportPathOption);
    settings.addOption(qmlImportPath);

    QCommandLineOption environmentOption(
            QStringList() << "E"_L1,
            "Use the QML_IMPORT_PATH environment variable to look for QML Modules"_L1);
    parser.addOption(environmentOption);

    QCommandLineOption writeDefaultsOption(
            QStringList() << "write-defaults"_L1,
            "Writes defaults settings to .qmlls.ini and exits (Warning: This "
            "will overwrite any existing settings and comments!)"_L1);
    parser.addOption(writeDefaultsOption);

    QCommandLineOption ignoreSettings(QStringList() << "ignore-settings"_L1,
                                      "Ignores all settings files and only takes "
                                      "command line options into consideration"_L1);
    parser.addOption(ignoreSettings);

    QCommandLineOption noCMakeCallsOption(
            QStringList() << "no-cmake-calls"_L1,
            "Disables automatic CMake rebuilds and C++ file watching."_L1);
    parser.addOption(noCMakeCallsOption);
    settings.addOption("no-cmake-calls"_L1, "false"_L1);

    QCommandLineOption cmakeJobsOption(QStringList() << "cmake-jobs"_L1 << "j"_L1,
                                       "Number of CMake jobs for automatic CMake rebuilds. "_L1,
                                       "jobs"_L1, "1"_L1);
    parser.addOption(cmakeJobsOption);
    settings.addOption("CMakeJobs"_L1, "1"_L1);

    QCommandLineOption docDir({ { "d"_L1, "p"_L1, "doc-dir"_L1 },
                                "Documentation path to use for the documentation hints feature"_L1,
                                "path"_L1,
                                QString() });
    parser.addOption(docDir);
    settings.addOption("docDir"_L1);

    QCommandLineOption qmlImportNoDefault(QStringList() << "bare"_L1,
                                          "Do not include default import directories. "
                                          "This may be used to run qmlls on a Boot2Qt project."_L1);
    parser.addOption(qmlImportNoDefault);
    const QString qmlImportNoDefaultSetting = "DisableDefaultImports"_L1;
    settings.addOption(qmlImportNoDefaultSetting, false);

    QCommandLineOption inputFile(QStringList() << "inputFile"_L1,
                                 "Read from file instead of stdin"_L1, "fileName"_L1);
    inputFile.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(inputFile);

    // we can't use parser.addVersionOption() because we already have one '-v' option for verbose...
    QCommandLineOption versionOption("version"_L1, "Displays version information."_L1);
    parser.addOption(versionOption);

    parser.process(app);

    if (parser.isSet(versionOption)) {
        parser.showVersion();
        return EXIT_SUCCESS;
    }

    if (parser.isSet(writeDefaultsOption)) {
        return settings.writeDefaults() ? 0 : 1;
    }

    if (parser.isSet(logFileOption)) {
        QString fileName = parser.value(logFileOption);
        qInfo() << "will log to" << fileName;
        logFile = new QFile(fileName);
        logFileLock = new QMutex;
        if (!logFile->open(QFile::WriteOnly | QFile::Truncate | QFile::Text)) {
            qWarning("Failed to open file %s: %s", qPrintable(logFile->fileName()),
                     qPrintable(logFile->errorString()));
        }
        qInstallMessageHandler([](QtMsgType t, const QMessageLogContext &, const QString &msg) {
            QMutexLocker l(logFileLock);
            logFile->write(QString::number(int(t)).toUtf8());
            logFile->write(" ");
            logFile->write(msg.toUtf8());
            logFile->write("\n");
            logFile->flush();
        });
    }
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

    if (parser.isSet(verboseOption)) {
        QLoggingCategory::setFilterRules("qt.languageserver*.debug=true\n"_L1);
        qmlServer.codeModelManager()->setVerbose(true);
    }
    if (parser.isSet(docDir))
        qmlServer.codeModelManager()->setDocumentationRootPath(
                QString::fromUtf8(parser.value(docDir).toUtf8()));

    if (parser.isSet(buildDirOption)) {
        const QStringList dirs =
                QQmlToolingUtils::getAndWarnForInvalidDirsFromOption(parser, buildDirOption);

        qInfo().nospace().noquote()
                << "Using build directories passed by -b: \"" << dirs.join(u"\", \""_s) << "\".";

        qmlServer.codeModelManager()->setBuildPathsForRootUrl(QByteArray(), dirs);
    } else if (QStringList dirsFromEnv =
                       QQmlToolingUtils::getAndWarnForInvalidDirsFromEnv(u"QMLLS_BUILD_DIRS"_s);
               !dirsFromEnv.isEmpty()) {

        // warn now at qmlls startup that those directories will be used later in qqmlcodemodel when
        // searching for build folders.
        qInfo().nospace().noquote() << "Using build directories passed from environment variable "
                                       "\"QMLLS_BUILD_DIRS\": \""
                                    << dirsFromEnv.join(u"\", \""_s) << "\".";
        qmlServer.codeModelManager()->setBuildPathsForRootUrl(QByteArray(), dirsFromEnv);
    } else {
        qInfo() << "Using the build directories found in the .qmlls.ini file. Your build folder "
                   "might not be found if no .qmlls.ini files are present in the root source "
                   "folder.";
    }

    qmlServer.codeModelManager()->setImportPaths(
            collectImportPaths(parser, qmlImportPathOption, environmentOption, qmlImportNoDefault));

    if (prepareCMakeFeature(parser, noCMakeCallsOption)) {
        qmlServer.codeModelManager()->setCMakeJobs(cmakeJobs(parser, cmakeJobsOption));
        qmlServer.codeModelManager()->tryEnableCMakeCalls();
    } else {
        qmlServer.codeModelManager()->disableCMakeCalls();
    }

    auto r = parser.isSet(inputFile) && parser.value(inputFile) != "-"_L1
            ? std::unique_ptr<AbstractReader>(std::make_unique<FileReader>(parser.value(inputFile)))
            : std::unique_ptr<AbstractReader>(std::make_unique<StdinReader>());
    QThread workerThread;
    r->moveToThread(&workerThread);
    QObject::connect(r.get(), &AbstractReader::receivedData, qmlServer.server(),
                     &QLanguageServer::receiveData);
    QObject::connect(qmlServer.server(), &QLanguageServer::readNextMessage, r.get(),
                     &AbstractReader::readNextMessage);
    auto exit = [&app, &workerThread]() {
        workerThread.quit();
        workerThread.wait();
        QTimer::singleShot(100, &app, []() {
            QCoreApplication::processEvents();
            QCoreApplication::exit();
        });
    };
    QObject::connect(r.get(), &StdinReader::eof, &app, exit);
    QObject::connect(qmlServer.server(), &QLanguageServer::exit, &workerThread, exit);

    emit r->readNextMessage();
    workerThread.start();
    app.exec();
    workerThread.quit();
    workerThread.wait();
    return qmlServer.returnValue();
}

} // namespace QmlLsp

QT_END_NAMESPACE

#include <qmllsmain.moc>
