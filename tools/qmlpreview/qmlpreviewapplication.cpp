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

QmlPreviewApplication::QmlPreviewApplication(int &argc, char **argv) :
    QCoreApplication(argc, argv),
    m_verbose(false),
    m_connectionAttempts(0)
{
    m_connection.reset(new QQmlDebugConnection);
    m_qmlPreviewClient.reset(new QQmlPreviewClient(m_connection.data()));
    m_connectTimer.setInterval(1000);

    m_loadTimer.setInterval(100);
    m_loadTimer.setSingleShot(true);
    connect(&m_loadTimer, &QTimer::timeout, this, [this]() {
        m_qmlPreviewClient->triggerLoad(QUrl());
    });

    connect(&m_connectTimer, &QTimer::timeout, this, &QmlPreviewApplication::tryToConnect);
    connect(m_connection.data(), &QQmlDebugConnection::connected, &m_connectTimer, &QTimer::stop);

    connect(m_qmlPreviewClient.data(), &QQmlPreviewClient::error,
            this, &QmlPreviewApplication::logError);
    connect(m_qmlPreviewClient.data(), &QQmlPreviewClient::request,
            this, &QmlPreviewApplication::serveRequest);

    connect(&m_watcher, &QmlPreviewFileSystemWatcher::fileChanged,
            this, &QmlPreviewApplication::sendFile);
    connect(&m_watcher, &QmlPreviewFileSystemWatcher::directoryChanged,
            this, &QmlPreviewApplication::sendDirectory);
}

QmlPreviewApplication::~QmlPreviewApplication()
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

void QmlPreviewApplication::run()
{
    logStatus(QString("Listening on %1 ...").arg(m_socketFile));
    m_connection->startLocalServer(m_socketFile);
    m_process.reset(new QProcess(this));
    QStringList arguments;
    arguments << QString("-qmljsdebugger=file:%1,block,services:QmlPreview").arg(m_socketFile);
    arguments << m_arguments;

    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process.data(), &QIODevice::readyRead,
            this, &QmlPreviewApplication::processHasOutput);
    connect(m_process.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int){ processFinished(); });
    logStatus(QString("Starting '%1 %2' ...").arg(m_executablePath, arguments.join(QLatin1Char(' '))));
    m_process->start(m_executablePath, arguments);
    if (!m_process->waitForStarted()) {
        logError(QString("Could not run '%1': %2").arg(m_executablePath, m_process->errorString()));
        exit(1);
    }
    m_connectTimer.start();
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
