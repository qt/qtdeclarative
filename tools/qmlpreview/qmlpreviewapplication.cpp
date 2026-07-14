// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewapplication.h"

#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QCommandLineParser>
#include <QtCore/QTemporaryFile>
#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QLibraryInfo>

using namespace Qt::StringLiterals;

static QString qmlPreviewServices()
{
    return QStringLiteral("QmlPreview,CanvasFrameRate,EventReplay");
}

static QString makeQmlPreviewArgument(const QString &socketFile)
{
    return QString("-qmljsdebugger=file:%1,block,services:%2")
            .arg(socketFile)
            .arg(qmlPreviewServices());
}

bool QmlPreviewApplication::argumentsFromCommandLineAndFile(
        QStringList &allArguments, const QStringList &arguments)
{
    allArguments.reserve(arguments.size());
    for (const QString &argument : arguments) {
        // "@file" doesn't start with a '-' so we can't use QCommandLineParser for it
        if (argument.startsWith(u'@')) {
            QString optionsFile = argument;
            optionsFile.remove(0, 1);
            if (optionsFile.isEmpty()) {
                logError("The @ option requires an input file");
                return false;
            }
            QFile f(optionsFile);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                logError("Cannot open options file specified with @");
                return false;
            }
            while (!f.atEnd()) {
                QString line = QString::fromLocal8Bit(f.readLine().trimmed());
                if (!line.isEmpty())
                    allArguments << line;
            }
        } else {
            allArguments << argument;
        }
    }
    return true;
}

QmlPreviewApplication::QmlPreviewApplication(int &argc, char **argv) : QCoreApplication(argc, argv)
{
    m_connection.reset(new QQmlDebugConnection);
    m_qmlPreviewClient.reset(new QQmlPreviewClient(m_connection.data()));

    m_loadTimer.setInterval(100);
    m_loadTimer.setSingleShot(true);
    connect(&m_loadTimer, &QTimer::timeout, this,
            [this]() {

                // Only replay events if we're not in hot reload mode.
                // In hot reload mode the replay is done when receiving the
                // settings confirmation.
                if (!m_confirmedSettings.enableInPlaceUpdates)
                    replayEvents();

                m_qmlPreviewClient->triggerLoad(QUrl());
            });

    m_connectTimer.setInterval(1000);
    connect(&m_connectTimer, &QTimer::timeout, this, &QmlPreviewApplication::tryToConnect);

    connectConnectionSignals();
    connectQmlPreviewClientSignals();
    connectWatcherSignals();
    registerCommands();

    // Closing standard input ends the interactive session, saving any recorded
    // events to the --output file first.
    connect(&m_console, &QQmlDebugConsole::endOfInput, this, [this]() {
        saveEventsToOutput();
        quit();
    });
}

QmlPreviewApplication::~QmlPreviewApplication()
{
    killProcess();
}

void QmlPreviewApplication::connectQmlPreviewClientSignals()
{
    connect(m_qmlPreviewClient.data(), &QQmlPreviewClient::error, this,
            [this](const QString &message) {
                // If it rejects the configuration message, replay initial events.
                // Otherwise replay initial events from the Confirmation handler.
                static_assert(QQmlPreviewClient::Configuration == 10);
                if (message == QLatin1String("Invalid command: 10"))
                    replayEvents();
                else
                    logError(message);
            });
    connect(m_qmlPreviewClient.data(), &QQmlPreviewClient::request, this,
            &QmlPreviewApplication::serveRequest);
    connect(m_qmlPreviewClient.data(), &QQmlPreviewClient::confirmation, this,
            [this](const QQmlPreviewClient::Settings &settings) {

                // Before we switch to hot reload mode, replay the events gathered
                // in the last run, or, on a fresh start, the stream from the
                // --replay file.
                if (settings.enableInPlaceUpdates)
                    replayEvents();

                m_confirmedSettings = settings;
                const QString status = QString::fromUtf8("Inplace updates setting confirmed as: %1")
                                       .arg(settings.enableInPlaceUpdates ? "enabled" : "disabled");
                logStatus(status);
            });
    connect(m_qmlPreviewClient.data(), &QQmlPreviewClient::hotReloadFailure, this,
            [this](const QString &reason) {
                logError(QString::fromUtf8("Hot reload failure: %1").arg(reason));
                restartProcess();
            });
}

void QmlPreviewApplication::connectConnectionSignals()
{
    connect(m_connection.data(), &QQmlDebugConnection::connected, this, [this]() {
        QQmlPreviewClient::Settings settings;
        settings.enableInPlaceUpdates = true;
        m_qmlPreviewClient->sendConfiguration(settings);
        const QString status = QString::fromUtf8("Inplace updates configuration: %1")
                                       .arg(settings.enableInPlaceUpdates ? "enabled" : "disabled");
        logStatus(status);
        m_connectTimer.stop();

        // Kick off the interactive command loop once, on the first connection.
        // Reconnections (e.g. after 'restart') are re-prompted by the command
        // handler that triggered them.
        if (m_interactive && !m_promptShown) {
            m_promptShown = true;
            prompt(tr("Connected. Type a command ('help' shows the list)."));
        }
    });
}

void QmlPreviewApplication::connectWatcherSignals()
{
    connect(&m_watcher, &QmlPreviewFileSystemWatcher::fileChanged,
            this, &QmlPreviewApplication::sendFile);
    connect(&m_watcher, &QmlPreviewFileSystemWatcher::directoryChanged,
            this, &QmlPreviewApplication::sendDirectory);
}

void QmlPreviewApplication::killProcess()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        logStatus("Terminating process ...");
        m_process->disconnect();
        m_process->terminate();
        if (!m_process->waitForFinished(1000)) {
            logStatus("Killing process ...");
            m_process->kill();
        }
    }
}

void QmlPreviewApplication::restartProcess()
{
    logStatus(QString::fromUtf8("Restarting process ..."));

    if (m_process) {
        disconnect(m_process.data(), nullptr, this, nullptr);
    }

    killProcess();

    // reset the connection
    m_connection->close();
    m_connectionAttempts = 0;
    m_connection->startLocalServer(m_socketFile);
    startProcess();
}

void QmlPreviewApplication::parseArguments()
{
    setApplicationName(QLatin1String("qmlpreview"));
    setApplicationVersion(QLatin1String(qVersion()));

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);

    parser.setApplicationDescription(QChar::LineFeed + tr(
        "The QML Preview tool watches QML and JavaScript files on disk and updates\n"
        "the application live with any changes. The application to be previewed\n"
        "has to enable QML debugging. See the Qt Creator documentation on how to do\n"
        "this for different Qt versions."));

    QCommandLineOption verbose(QStringList() << QLatin1String("verbose"),
                               tr("Print debugging output."));
    parser.addOption(verbose);

    QCommandLineOption interactive(QStringList() <<  QLatin1String("interactive"),
                                   tr("Manually control the preview from the command line. Type "
                                      "'help' at the prompt to see the list of available "
                                      "commands."));
    parser.addOption(interactive);

    QCommandLineOption output(QStringList() << QLatin1String("o") << QLatin1String("output"),
                              tr("Save the recorded input event stream to <file> (a .qtd file) "
                                 "when quitting."),
                              QLatin1String("file"));
    parser.addOption(output);

    QCommandLineOption replay(QStringList() << QLatin1String("r") << QLatin1String("replay"),
                              tr("Load the input event stream from <file> (a .qtd file) on startup "
                                 "and replay it to restore the UI state from a previous session."),
                              QLatin1String("file"));
    parser.addOption(replay);

    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption resources(QLatin1String("resource"),
                                 tr("Qt resource file to map qrc: paths to file system paths"),
                                 QLatin1String("resource-file-name"));
    parser.addOption(resources);

    parser.addPositionalArgument(QLatin1String("executable"),
                                 tr("The executable to be started and previewed."),
                                 QLatin1String("[executable]"));
    parser.addPositionalArgument(QLatin1String("parameters"),
                                 tr("Parameters for the executable to be started."),
                                 QLatin1String("[parameters...]"));

    QStringList expandedArguments;
    if (!argumentsFromCommandLineAndFile(expandedArguments, arguments()))
        ::exit(1);
    parser.process(expandedArguments);

    QTemporaryFile file;
    if (file.open())
        m_socketFile = file.fileName();

    if (parser.isSet(verbose))
        m_verbose = true;

    m_interactive = parser.isSet(interactive);
    m_outputFile = parser.value(output);
    m_replayFile = parser.value(replay);

    m_arguments = parser.positionalArguments();
    if (!m_arguments.isEmpty())
        m_executablePath = m_arguments.takeFirst();

    if (m_executablePath.isEmpty()) {
        logError(tr("You have to specify an executable to start."));
        parser.showHelp(2);
    }

    m_resourceFileMapper.reset(new QQmlJSResourceFileMapper(parser.values(resources)));
}

int QmlPreviewApplication::exec()
{
    QTimer::singleShot(0, this, &QmlPreviewApplication::run);
    return QCoreApplication::exec();
}

bool QmlPreviewApplication::isInteractive() const
{
    return m_interactive;
}

void QmlPreviewApplication::startConsole()
{
    m_console.start();
}

void QmlPreviewApplication::prompt(const QString &line, bool ready)
{
    if (m_interactive)
        m_console.prompt(line, ready);
}

void QmlPreviewApplication::replayEvents()
{
    // If we have been running before and recorded input events, replay those to
    // restore the UI state. On a fresh start there is nothing recorded yet, so
    // replay the stream from the file given with --replay instead. Once the file's
    // events have been replayed they are recorded too, so any later replay (e.g.
    // after a reload or 'restart') naturally comes from memory again.
    if (m_qmlPreviewClient->hasRecordedEvents()) {
        m_qmlPreviewClient->replayEvents();
    } else if (!m_replayFile.isEmpty()) {
        if (m_qmlPreviewClient->replayEventsFromFile(m_replayFile))
            logStatus(QString::fromUtf8("Replaying events from %1").arg(m_replayFile));
        else
            logError(QString::fromUtf8("Could not replay events from %1").arg(m_replayFile));
    }
}

void QmlPreviewApplication::saveEventsToOutput()
{
    if (m_outputFile.isEmpty())
        return;
    if (m_qmlPreviewClient->saveEvents(m_outputFile))
        logStatus(QString::fromUtf8("Events written to %1").arg(m_outputFile));
    else
        logError(QString::fromUtf8("Could not write events to %1").arg(m_outputFile));
}

void QmlPreviewApplication::registerCommands()
{
    m_console.registerCommand(
            { "load"_L1, "l"_L1 }, "[file]"_L1,
            tr("Load a .qtd input event file and replay it. Uses the file given with "
               "--replay if no argument is provided."),
            [this](const QStringList &args) {
                const QString file = args.isEmpty() ? m_replayFile : args.first();
                if (file.isEmpty())
                    prompt(tr("No file given and no --replay file configured."));
                else if (m_qmlPreviewClient->replayEventsFromFile(file))
                    prompt(tr("Replaying events from %1.").arg(file));
                else
                    prompt(tr("Could not replay events from %1.").arg(file));
            });

    m_console.registerCommand(
            { "output"_L1, "o"_L1 }, "[file]"_L1,
            tr("Output the recorded input event stream to a .qtd file. Uses the file "
               "given with --output if no argument is provided."),
            [this](const QStringList &args) {
                const QString file = args.isEmpty() ? m_outputFile : args.first();
                if (file.isEmpty())
                    prompt(tr("No file given and no --output file configured."));
                else if (m_qmlPreviewClient->saveEvents(file))
                    prompt(tr("Events written to %1.").arg(file));
                else
                    prompt(tr("Saving to %1 failed.").arg(file));
            });

    m_console.registerCommand(
            { "replay"_L1 }, {},
            tr("Replay the input events recorded so far into the running target."),
            [this](const QStringList &) {
                m_qmlPreviewClient->replayEvents();
                prompt(tr("Replaying recorded events."));
            });

    m_console.registerCommand({ "clear"_L1, "c"_L1 }, {},
                              tr("Discard the input events recorded so far."),
                              [this](const QStringList &) {
                                  m_qmlPreviewClient->clearRecordedEvents();
                                  prompt(tr("Recorded events cleared."));
                              });

    m_console.registerCommand(
            { "kill"_L1, "k"_L1 }, {}, tr("Forcefully terminate the target application."),
            [this](const QStringList &) {
                killProcess();
                prompt(tr("Target process terminated. Use 'restart' to relaunch."));
            });

    m_console.registerCommand({ "restart"_L1, "r"_L1 }, {},
                              tr("Restart the target application and replay the recorded events to "
                                 "restore the UI state."),
                              [this](const QStringList &) {
                                  restartProcess();
                                  prompt(tr("Target process restarting ..."));
                              });

    m_console.registerCommand(
            { "quit"_L1, "q"_L1 }, {},
            tr("Save the recorded events to the --output file if configured, then quit."),
            [this](const QStringList &) {
                saveEventsToOutput();
                quit();
            });
}

void QmlPreviewApplication::startProcess()
{
    m_process.reset(new QProcess(this));
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process.data(), &QIODevice::readyRead,
            this, &QmlPreviewApplication::processHasOutput);
    connect(m_process.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int){ processFinished(); });
    m_arguments.append(makeQmlPreviewArgument(m_socketFile));
    logStatus(QString("Starting '%1 %2' ...")
                      .arg(m_executablePath, m_arguments.join(QLatin1Char(' '))));
    m_process->start(m_executablePath, m_arguments);
    if (!m_process->waitForStarted()) {
        logError(QString("Could not run '%1': %2").arg(m_executablePath, m_process->errorString()));
        exit(1);
    }
    m_connectTimer.start();
}

void QmlPreviewApplication::run()
{
    logStatus(QString("Listening on %1 ...").arg(m_socketFile));
    m_connection->startLocalServer(m_socketFile);
    startProcess();
}

void QmlPreviewApplication::tryToConnect()
{
    Q_ASSERT(!m_connection->isConnected());
    ++m_connectionAttempts;

    if (m_verbose && !(m_connectionAttempts % 5)) {// print every 5 seconds
        logError(QString("No connection received on %1 for %2 seconds ...")
                 .arg(m_socketFile).arg(m_connectionAttempts));
    }
}

void QmlPreviewApplication::processHasOutput()
{
    Q_ASSERT(m_process);
    while (m_process->bytesAvailable()) {
        QTextStream out(stderr);
        out << m_process->readAll();
    }
}

void QmlPreviewApplication::processFinished()
{
    Q_ASSERT(m_process);
    int exitCode = 0;
    if (m_process->exitStatus() == QProcess::NormalExit) {
        logStatus(QString("Process exited (%1).").arg(m_process->exitCode()));
    } else {
        logError("Process crashed!");
        exitCode = 3;
    }

    // In interactive mode keep running so the user can inspect the situation or
    // relaunch the target with 'restart'.
    if (m_interactive)
        return;

    saveEventsToOutput();
    exit(exitCode);
}

void QmlPreviewApplication::logError(const QString &error)
{
    QTextStream err(stderr);
    err << "Error: " << error << Qt::endl;
}

void QmlPreviewApplication::logStatus(const QString &status)
{
    if (!m_verbose)
        return;
    QTextStream err(stderr);
    err << status << Qt::endl;
}

void QmlPreviewApplication::serveFileRequest(const QString &remotePath, const QString &localPath)
{
    QFile file(localPath);
    if (file.open(QIODevice::ReadOnly)) {
        m_qmlPreviewClient->sendFile(remotePath, file.readAll());
        m_watcher.addFile(localPath);
        return;
    }

    logStatus(QString("Could not open file %1 for reading: %2")
                      .arg(localPath, file.errorString()));
    m_qmlPreviewClient->sendError(remotePath);
}

void QmlPreviewApplication::serveResourceRequest(const QString &path)
{
    const auto success = [&](const QString &localPath) {
        logStatus(QString("Resolved resource path %1 to %2").arg(path, localPath));
        m_localToResourcePath.insert(localPath, path);
        return localPath;
    };

    const auto failure = [&]() {
        logStatus(QString("Cannot resolve resource path %1").arg(path));
        m_qmlPreviewClient->sendError(path);
    };

    const QString resourcePath = path.mid(1);

    // Try to resolve as a resource file
    const QStringList filePaths = m_resourceFileMapper->filePaths(
            QQmlJSResourceFileMapper::resourceFileFilter(resourcePath));

    // Exactly one local file for the resource path: Return it.
    if (filePaths.length() == 1) {
        serveFileRequest(path, success(filePaths.first()));
        return;
    }

    // Multiple local files for the same resource path: We don't know what to do.
    if (!filePaths.isEmpty()) {
        failure();
        return;
    }

    // Resolve as a resource directory
    const QList<QQmlJSResourceFileMapper::Entry> entries = m_resourceFileMapper->filter({
        resourcePath,
        {},
        QQmlJSResourceFileMapper::Directory | QQmlJSResourceFileMapper::Resource
    });

    if (entries.isEmpty()) {
        failure();
        return;
    }

    // Send the resource directory contents
    // We can't watch this because there is no local directory to watch here.
    const qsizetype prefixLength = resourcePath.length() + (resourcePath.endsWith(u'/') ? 0 : 1);
    QStringList directory;
    for (const auto &entry : entries)
        directory.append(entry.resourcePath.mid(prefixLength));
    m_qmlPreviewClient->sendDirectory(path, directory);
}

void QmlPreviewApplication::serveRequest(const QString &path)
{
    if (path.startsWith(QLatin1String(":/"))) {
        serveResourceRequest(path);
        return;
    }

    const QFileInfo info(path);
    if (info.isDir()) {
        m_qmlPreviewClient->sendDirectory(path, QDir(path).entryList());
        m_watcher.addDirectory(path);
    } else {
        serveFileRequest(path, path);
    }
}

bool QmlPreviewApplication::sendFile(const QString &path)
{
    const QString effectivePath = m_localToResourcePath.value(path, path);

    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        m_qmlPreviewClient->sendFile(effectivePath, file.readAll());
        // Defer the Load, because files tend to change multiple times in a row.
        m_loadTimer.start();
        return true;
    }
    logStatus(QString("Could not open file %1 for reading: %2").arg(path, file.errorString()));
    return false;
}

void QmlPreviewApplication::sendDirectory(const QString &path)
{
    const QString effectivePath = m_localToResourcePath.value(path, path);
    m_qmlPreviewClient->sendDirectory(effectivePath, QDir(path).entryList());
    m_loadTimer.start();
}
