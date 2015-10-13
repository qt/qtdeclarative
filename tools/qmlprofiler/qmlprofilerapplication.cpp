/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmlprofilerapplication.h"
#include "constants.h"
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QCommandLineParser>

static const char commandTextC[] =
        "The following commands are available:\n"
        "'r', 'record'\n"
        "    Switch recording on or off.\n"
        "'o [file]', 'output [file]'\n"
        "    Output profiling data to <file>. If no <file>\n"
        "    parameter is given, output to whatever was given\n"
        "    with --output, or standard output.\n"
        "'c', 'clear'\n"
        "    Clear profiling data recorded so far from memory.\n"
        "'f [file]', 'flush [file]'\n"
        "    Stop recording if it is running, then output the\n"
        "    data, and finally clear it from memory.\n"
        "'q', 'quit'\n"
        "    Terminate the program if started from qmlprofiler,\n"
        "    and qmlprofiler itself.";

static const char *features[] = {
    "javascript",
    "memory",
    "pixmapcache",
    "scenegraph",
    "animations",
    "painting",
    "compiling",
    "creating",
    "binding",
    "handlingsignal",
    "inputevents"
};

QmlProfilerApplication::QmlProfilerApplication(int &argc, char **argv) :
    QCoreApplication(argc, argv),
    m_runMode(LaunchMode),
    m_process(0),
    m_hostName(QLatin1String("127.0.0.1")),
    m_port(3768),
    m_pendingRequest(REQUEST_NONE),
    m_verbose(false),
    m_recording(true),
    m_interactive(false),
    m_qmlProfilerClient(&m_connection),
    m_v8profilerClient(&m_connection),
    m_connectionAttempts(0),
    m_qmlDataReady(false),
    m_v8DataReady(false)
{
    m_connectTimer.setInterval(1000);
    connect(&m_connectTimer, SIGNAL(timeout()), this, SLOT(tryToConnect()));

    connect(&m_connection, SIGNAL(connected()), this, SLOT(connected()));
    connect(&m_connection, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(connectionStateChanged(QAbstractSocket::SocketState)));
    connect(&m_connection, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectionError(QAbstractSocket::SocketError)));

    connect(&m_qmlProfilerClient, SIGNAL(enabledChanged()), this, SLOT(traceClientEnabled()));
    connect(&m_qmlProfilerClient, SIGNAL(range(QQmlProfilerDefinitions::RangeType,QQmlProfilerDefinitions::BindingType,qint64,qint64,QStringList,QmlEventLocation)),
            &m_profilerData, SLOT(addQmlEvent(QQmlProfilerDefinitions::RangeType,QQmlProfilerDefinitions::BindingType,qint64,qint64,QStringList,QmlEventLocation)));
    connect(&m_qmlProfilerClient, SIGNAL(traceFinished(qint64)), &m_profilerData, SLOT(setTraceEndTime(qint64)));
    connect(&m_qmlProfilerClient, SIGNAL(traceStarted(qint64)), &m_profilerData, SLOT(setTraceStartTime(qint64)));
    connect(&m_qmlProfilerClient, SIGNAL(traceStarted(qint64)), this, SLOT(notifyTraceStarted()));
    connect(&m_qmlProfilerClient, SIGNAL(frame(qint64,int,int,int)), &m_profilerData, SLOT(addFrameEvent(qint64,int,int,int)));
    connect(&m_qmlProfilerClient, SIGNAL(sceneGraphFrame(QQmlProfilerDefinitions::SceneGraphFrameType,
                                         qint64,qint64,qint64,qint64,qint64,qint64)),
            &m_profilerData, SLOT(addSceneGraphFrameEvent(QQmlProfilerDefinitions::SceneGraphFrameType,
                                  qint64,qint64,qint64,qint64,qint64,qint64)));
    connect(&m_qmlProfilerClient, SIGNAL(pixmapCache(QQmlProfilerDefinitions::PixmapEventType,qint64,
                                                     QmlEventLocation,int,int,int)),
            &m_profilerData, SLOT(addPixmapCacheEvent(QQmlProfilerDefinitions::PixmapEventType,qint64,
                                                      QmlEventLocation,int,int,int)));
    connect(&m_qmlProfilerClient, SIGNAL(memoryAllocation(QQmlProfilerDefinitions::MemoryType,qint64,
                                                          qint64)),
            &m_profilerData, SLOT(addMemoryEvent(QQmlProfilerDefinitions::MemoryType,qint64,
                                                 qint64)));
    connect(&m_qmlProfilerClient, SIGNAL(inputEvent(QQmlProfilerDefinitions::EventType,qint64)),
            &m_profilerData, SLOT(addInputEvent(QQmlProfilerDefinitions::EventType,qint64)));

    connect(&m_qmlProfilerClient, SIGNAL(complete()), this, SLOT(qmlComplete()));

    connect(&m_v8profilerClient, SIGNAL(enabledChanged()), this, SLOT(profilerClientEnabled()));
    connect(&m_v8profilerClient, SIGNAL(range(int,QString,QString,int,double,double)),
            &m_profilerData, SLOT(addV8Event(int,QString,QString,int,double,double)));
    connect(&m_v8profilerClient, SIGNAL(complete()), this, SLOT(v8Complete()));

    connect(&m_profilerData, SIGNAL(error(QString)), this, SLOT(logError(QString)));
    connect(&m_profilerData, SIGNAL(dataReady()), this, SLOT(traceFinished()));

}

QmlProfilerApplication::~QmlProfilerApplication()
{
    if (!m_process)
        return;
    logStatus("Terminating process ...");
    m_process->disconnect();
    m_process->terminate();
    if (!m_process->waitForFinished(1000)) {
        logStatus("Killing process ...");
        m_process->kill();
    }
    delete m_process;
}

void QmlProfilerApplication::parseArguments()
{
    setApplicationName(QLatin1String("qmlprofiler"));
    setApplicationVersion(QLatin1String(qVersion()));

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);

    parser.setApplicationDescription(QChar::LineFeed + tr(
        "The QML Profiler retrieves QML tracing data from an application. The data\n"
        "collected can then be visualized in Qt Creator. The application to be profiled\n"
        "has to enable QML debugging. See the Qt Creator documentation on how to do\n"
        "this for different Qt versions."));

    QCommandLineOption attach(QStringList() << QLatin1String("a") << QLatin1String("attach"),
                              tr("Attach to an application already running on <hostname>, "
                                 "instead of starting it locally."),
                              QLatin1String("hostname"));
    parser.addOption(attach);

    QCommandLineOption port(QStringList() << QLatin1String("p") << QLatin1String("port"),
                            tr("Connect to the TCP port <port>. The default is 3768."),
                            QLatin1String("port"), QLatin1String("3768"));
    parser.addOption(port);

    QCommandLineOption output(QStringList() << QLatin1String("o") << QLatin1String("output"),
                              tr("Save tracing data in <file>. By default the data is sent to the "
                                 "standard output."), QLatin1String("file"), QString());
    parser.addOption(output);

    QCommandLineOption record(QLatin1String("record"),
                              tr("If set to 'off', don't immediately start recording data when the "
                                 "QML engine starts, but instead either start the recording "
                                 "interactively or with the JavaScript console.profile() function. "
                                 "By default the recording starts immediately."),
                              QLatin1String("on|off"), QLatin1String("on"));
    parser.addOption(record);

    QStringList featureList;
    for (int i = 0; i < QQmlProfilerDefinitions::MaximumProfileFeature; ++i)
        featureList << QLatin1String(features[i]);

    QCommandLineOption include(QLatin1String("include"),
                               tr("Comma-separated list of features to record. By default all "
                                  "features supported by the QML engine are recorded. If --include "
                                  "is specified, only the given features will be recorded. "
                                  "The following features are unserstood by qmlprofiler: %1").arg(
                                   featureList.join(", ")),
                               QLatin1String("feature,..."));
    parser.addOption(include);

    QCommandLineOption exclude(QLatin1String("exclude"),
                            tr("Comma-separated list of features to exclude when recording. By "
                               "default all features supported by the QML engine are recorded. "
                               "See --include for the features understood by qmlprofiler."),
                            QLatin1String("feature,..."));
    parser.addOption(exclude);

    QCommandLineOption interactive(QLatin1String("interactive"),
                                   tr("Manually control the recording from the command line. The "
                                      "profiler will not terminate itself when the application "
                                      "does so in this case.") + QChar::Space + tr(commandTextC));
    parser.addOption(interactive);

    QCommandLineOption verbose(QStringList() << QLatin1String("verbose"),
                               tr("Print debugging output."));
    parser.addOption(verbose);

    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument(QLatin1String("program"),
                                 tr("The program to be started and profiled."),
                                 QLatin1String("[program]"));
    parser.addPositionalArgument(QLatin1String("parameters"),
                                 tr("Parameters for the program to be started."),
                                 QLatin1String("[parameters...]"));

    parser.process(*this);

    if (parser.isSet(attach)) {
        m_hostName = parser.value(attach);
        m_runMode = AttachMode;
    }

    if (parser.isSet(port)) {
        bool isNumber;
        m_port = parser.value(port).toUShort(&isNumber);
        if (!isNumber) {
            logError(tr("'%1' is not a valid port.").arg(parser.value(port)));
            parser.showHelp(1);
        }
    }

    m_outputFile = parser.value(output);

    m_recording = (parser.value(record) == QLatin1String("on"));
    m_interactive = parser.isSet(interactive);

    quint64 features = std::numeric_limits<quint64>::max();
    if (parser.isSet(include)) {
        if (parser.isSet(exclude)) {
            logError(tr("qmlprofiler can only process either --include or --exclude, not both."));
            parser.showHelp(4);
        }
        features = parseFeatures(featureList, parser.value(include), false);
    }

    if (parser.isSet(exclude))
        features = parseFeatures(featureList, parser.value(exclude), true);

    if (features == 0)
        parser.showHelp(4);

    m_qmlProfilerClient.setFeatures(features);

    if (parser.isSet(verbose))
        m_verbose = true;

    m_programArguments = parser.positionalArguments();
    if (!m_programArguments.isEmpty())
        m_programPath = m_programArguments.takeFirst();

    if (m_runMode == LaunchMode && m_programPath.isEmpty()) {
        logError(tr("You have to specify either --attach or a program to start."));
        parser.showHelp(2);
    }

    if (m_runMode == AttachMode && !m_programPath.isEmpty()) {
        logError(tr("--attach cannot be used when starting a program."));
        parser.showHelp(3);
    }
}

int QmlProfilerApplication::exec()
{
    QTimer::singleShot(0, this, SLOT(run()));
    return QCoreApplication::exec();
}

bool QmlProfilerApplication::isInteractive() const
{
    return m_interactive;
}

quint64 QmlProfilerApplication::parseFeatures(const QStringList &featureList, const QString &values,
                                              bool exclude)
{
    quint64 features = exclude ? std::numeric_limits<quint64>::max() : 0;
    QStringList givenFeatures = values.split(QLatin1Char(','));
    foreach (const QString &f, givenFeatures) {
        int index =  featureList.indexOf(f);
        if (index < 0) {
            logError(tr("Unknown feature '%1'").arg(f));
            return 0;
        }
        quint64 flag = static_cast<quint64>(1) << index;
        features = (exclude ? (features ^ flag) : (features | flag));
    }
    if (features == 0) {
        logError(exclude ? tr("No features remaining to record after processing --exclude.") :
                           tr("No features specified for --include."));
    }
    return features;
}

void QmlProfilerApplication::flush()
{
    if (m_recording) {
        m_pendingRequest = REQUEST_FLUSH;
        m_qmlProfilerClient.sendRecordingStatus(false);
        m_v8profilerClient.sendRecordingStatus(false);
    } else {
        if (m_profilerData.save(m_interactiveOutputFile)) {
            m_profilerData.clear();
            if (!m_interactiveOutputFile.isEmpty())
                prompt(tr("Data written to %1.").arg(m_interactiveOutputFile));
            else
                prompt();
        } else {
            prompt(tr("Saving failed."));
        }
        m_interactiveOutputFile.clear();
        m_pendingRequest = REQUEST_NONE;
    }
}

void QmlProfilerApplication::output()
{
    if (m_profilerData.save(m_interactiveOutputFile)) {
        if (!m_interactiveOutputFile.isEmpty())
            prompt(tr("Data written to %1.").arg(m_interactiveOutputFile));
        else
            prompt();
    } else {
        prompt(tr("Saving failed"));
    }

    m_interactiveOutputFile.clear();
    m_pendingRequest = REQUEST_NONE;
}

bool QmlProfilerApplication::checkOutputFile(PendingRequest pending)
{
    if (m_interactiveOutputFile.isEmpty())
        return true;
    QFileInfo file(m_interactiveOutputFile);
    if (file.exists()) {
        if (!file.isFile()) {
            prompt(tr("Cannot overwrite %1.").arg(m_interactiveOutputFile));
            m_interactiveOutputFile.clear();
        } else {
            prompt(tr("%1 exists. Overwrite (y/n)?").arg(m_interactiveOutputFile));
            m_pendingRequest = pending;
        }
        return false;
    } else {
        return true;
    }
}

void QmlProfilerApplication::userCommand(const QString &command)
{
    QStringList args = command.split(QChar::Space, QString::SkipEmptyParts);
    if (args.isEmpty()) {
        prompt();
        return;
    }

    QByteArray cmd = args.takeFirst().trimmed().toLatin1();

    if (m_pendingRequest == REQUEST_QUIT) {
        if (cmd == Constants::CMD_YES || cmd == Constants::CMD_YES2) {
            quit();
        } else if (cmd == Constants::CMD_NO || cmd == Constants::CMD_NO2) {
            m_pendingRequest = REQUEST_NONE;
            prompt();
        } else {
            prompt(tr("The application is still generating data. Really quit (y/n)?"));
        }
        return;
    }

    if (m_pendingRequest == REQUEST_OUTPUT_FILE || m_pendingRequest == REQUEST_FLUSH_FILE) {
        if (cmd == Constants::CMD_YES || cmd == Constants::CMD_YES2) {
            if (m_pendingRequest == REQUEST_OUTPUT_FILE)
                output();
            else
                flush();
        } else if (cmd == Constants::CMD_NO || cmd == Constants::CMD_NO2) {
            m_pendingRequest = REQUEST_NONE;
            m_interactiveOutputFile.clear();
            prompt();
        } else {
            prompt(tr("%1 exists. Overwrite (y/n)?"));
        }
        return;
    }

    if (cmd == Constants::CMD_RECORD || cmd == Constants::CMD_RECORD2) {
        m_pendingRequest = REQUEST_TOGGLE_RECORDING;
        m_qmlProfilerClient.sendRecordingStatus(!m_recording);
        m_v8profilerClient.sendRecordingStatus(!m_recording);
    } else if (cmd == Constants::CMD_QUIT || cmd == Constants::CMD_QUIT2) {
        m_pendingRequest = REQUEST_QUIT;
        if (m_recording) {
            prompt(tr("The application is still generating data. Really quit (y/n)?"));
        } else if (!m_profilerData.isEmpty()) {
            prompt(tr("There is still trace data in memory. Really quit (y/n)?"));
        } else {
            quit();
        }
    } else if (cmd == Constants::CMD_OUTPUT || cmd == Constants::CMD_OUTPUT2) {
        if (m_recording) {
            prompt(tr("Cannot output while recording data."));
        } else if (m_profilerData.isEmpty()) {
            prompt(tr("No data was recorded so far."));
        } else {
            m_interactiveOutputFile = args.length() > 0 ? args[0] : m_outputFile;
            if (checkOutputFile(REQUEST_OUTPUT_FILE))
                output();
        }
    } else if (cmd == Constants::CMD_CLEAR || cmd == Constants::CMD_CLEAR2) {
        if (m_recording) {
            prompt(tr("Cannot clear data while recording."));
        } else if (m_profilerData.isEmpty()) {
            prompt(tr("No data was recorded so far."));
        } else {
            m_profilerData.clear();
            prompt(tr("Trace data cleared."));
        }
    } else if (cmd == Constants::CMD_FLUSH || cmd == Constants::CMD_FLUSH2) {
        if (!m_recording && m_profilerData.isEmpty()) {
            prompt(tr("No data was recorded so far."));
        } else {
            m_interactiveOutputFile = args.length() > 0 ? args[0] : m_outputFile;
            if (checkOutputFile(REQUEST_FLUSH_FILE))
                flush();
        }
    } else {
        prompt(tr(commandTextC));
    }
}

void QmlProfilerApplication::notifyTraceStarted()
{
    // Synchronize to server state. It doesn't hurt to do this multiple times in a row for
    // different traces. There is no symmetric event to "Complete" after all.
    m_recording = true;

    if (m_pendingRequest == REQUEST_TOGGLE_RECORDING) {
        m_pendingRequest = REQUEST_NONE;
        prompt(tr("Recording started"));
    } else {
        prompt(tr("Application started recording"), false);
    }
}

void QmlProfilerApplication::outputData()
{
    if (!m_profilerData.isEmpty()) {
        m_profilerData.save(m_outputFile);
        m_profilerData.clear();
    }
}

void QmlProfilerApplication::run()
{
    if (m_runMode == LaunchMode) {
        m_process = new QProcess(this);
        QStringList arguments;
        arguments << QString::fromLatin1("-qmljsdebugger=port:%1,block,services:CanvasFrameRate")
                     .arg(m_port);
        arguments << m_programArguments;

        m_process->setProcessChannelMode(QProcess::MergedChannels);
        connect(m_process, SIGNAL(readyRead()), this, SLOT(processHasOutput()));
        connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this,
                SLOT(processFinished()));
        logStatus(QString("Starting '%1 %2' ...").arg(m_programPath,
                                                      arguments.join(QLatin1Char(' '))));
        m_process->start(m_programPath, arguments);
        if (!m_process->waitForStarted()) {
            logError(QString("Could not run '%1': %2").arg(m_programPath,
                                                           m_process->errorString()));
            exit(1);
        }

    }
    m_connectTimer.start();
}

void QmlProfilerApplication::tryToConnect()
{
    Q_ASSERT(!m_connection.isConnected());
    ++ m_connectionAttempts;

    if (!m_verbose && !(m_connectionAttempts % 5)) {// print every 5 seconds
        if (!m_verbose)
            logError(QString("Could not connect to %1:%2 for %3 seconds ...").arg(
                         m_hostName, QString::number(m_port),
                         QString::number(m_connectionAttempts)));
    }

    if (m_connection.state() == QAbstractSocket::UnconnectedState) {
        logStatus(QString("Connecting to %1:%2 ...").arg(m_hostName,
                                                         QString::number(m_port)));
        m_connection.connectToHost(m_hostName, m_port);
    }
}

void QmlProfilerApplication::connected()
{
    m_connectTimer.stop();
    prompt(tr("Connected to host:port %1:%2. Wait for profile data or type a command (type 'help' "
              "to show list of commands).\nRecording Status: %3")
           .arg(m_hostName).arg((m_port)).arg(m_recording ? tr("on") : tr("off")));
}

void QmlProfilerApplication::connectionStateChanged(
        QAbstractSocket::SocketState state)
{
    if (m_verbose)
        qDebug() << state;
}

void QmlProfilerApplication::connectionError(QAbstractSocket::SocketError error)
{
    if (m_verbose)
        qDebug() << error;
}

void QmlProfilerApplication::processHasOutput()
{
    Q_ASSERT(m_process);
    while (m_process->bytesAvailable()) {
        QTextStream out(stderr);
        out << m_process->readAll();
    }
}

void QmlProfilerApplication::processFinished()
{
    Q_ASSERT(m_process);
    int exitCode = 0;
    if (m_process->exitStatus() == QProcess::NormalExit) {
        logStatus(QString("Process exited (%1).").arg(m_process->exitCode()));
        if (m_recording) {
            logError("Process exited while recording, last trace is damaged!");
            exitCode = 2;
        }
    } else {
        logError("Process crashed!");
        exitCode = 3;
    }
    if (!m_interactive)
        exit(exitCode);
}

void QmlProfilerApplication::traceClientEnabled()
{
    logStatus("Trace client is attached.");
    // blocked server is waiting for recording message from both clients
    // once the last one is connected, both messages should be sent
    m_qmlProfilerClient.sendRecordingStatus(m_recording);
    m_v8profilerClient.sendRecordingStatus(m_recording);
}

void QmlProfilerApplication::profilerClientEnabled()
{
    logStatus("Profiler client is attached.");

    // blocked server is waiting for recording message from both clients
    // once the last one is connected, both messages should be sent
    m_qmlProfilerClient.sendRecordingStatus(m_recording);
    m_v8profilerClient.sendRecordingStatus(m_recording);
}

void QmlProfilerApplication::traceFinished()
{
    m_recording = false; // only on "Complete" we know that the trace is really finished.

    // after receiving both notifications, reset the flags
    m_qmlDataReady = false;
    m_v8DataReady = false;

    if (m_pendingRequest == REQUEST_FLUSH) {
        flush();
    } else if (m_pendingRequest == REQUEST_TOGGLE_RECORDING) {
        m_pendingRequest = REQUEST_NONE;
        prompt(tr("Recording stopped."));
    } else {
        prompt(tr("Application stopped recording."), false);
    }
}

void QmlProfilerApplication::prompt(const QString &line, bool ready)
{
    if (m_interactive) {
        QTextStream err(stderr);
        if (!line.isEmpty())
            err << line << endl;
        err << QLatin1String("> ");
        if (ready)
            emit readyForCommand();
    }
}

void QmlProfilerApplication::logError(const QString &error)
{
    QTextStream err(stderr);
    err << "Error: " << error << endl;
}

void QmlProfilerApplication::logStatus(const QString &status)
{
    if (!m_verbose)
        return;
    QTextStream err(stderr);
    err << status << endl;
}

void QmlProfilerApplication::qmlComplete()
{
    m_qmlDataReady = true;
    if (m_v8profilerClient.state() != QQmlDebugClient::Enabled ||
            m_v8DataReady) {
        m_profilerData.complete();
    }
}

void QmlProfilerApplication::v8Complete()
{
    m_v8DataReady = true;
    if (m_qmlProfilerClient.state() != QQmlDebugClient::Enabled ||
            m_qmlDataReady) {
        m_profilerData.complete();
    }
}
