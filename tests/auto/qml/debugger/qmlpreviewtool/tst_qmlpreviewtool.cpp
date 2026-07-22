// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qprocess.h>
#include <QtCore/qtemporarydir.h>

#include <memory>

class tst_QmlPreviewTool : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void helpOption();
    void missingExecutable();
    void basicLaunch();
    void verboseOutput();
    void fileUpdate();
    void resourceMapping();
    void resourceFileUpdate();
    void resourceVerboseOutput();
    void loadFromModuleFileUpdate();
    void settingConfiguration();
    void settingConfirmation();
    void hotReloadStaysInPlace();
    void inputNotReplayedOnHotReload();
    void hotReloadFailure();
    void interactiveCommands();
    void saveAndReplayRoundTrip();
    void classicModeReplaysRecordedInput();

private:
    QString m_qmlPreviewPath;
    QString m_qmlRuntimePath;
    QString m_testHelperPath;
    QString m_testHelperModulePath;

    QString m_output;
    QProcess m_process;
    std::unique_ptr<QTemporaryDir> m_tempDir;

    void startPreview(const QStringList &args);
    void sendCommand(const QString &command);
    void readProcessOutput();
    bool waitForOutput(const QString &needle, int timeout = 30000);
};


static QString findTestHelper()
{
    const QLatin1String self("qmlpreviewtool");
    const QLatin1String helper("qmlpreviewtesthelper");
    QString appPath = QCoreApplication::applicationDirPath();
    const qsizetype pos = appPath.lastIndexOf(self);
    if (pos != -1)
        appPath.replace(pos, self.size(), helper);
    return appPath + QLatin1Char('/') + helper;
}

static QString findTestHelperModule()
{
    const QLatin1String self("qmlpreviewtool");
    const QLatin1String helper("qmlpreviewtesthelpermodule");
    QString appPath = QCoreApplication::applicationDirPath();
    const qsizetype pos = appPath.lastIndexOf(self);
    if (pos != -1)
        appPath.replace(pos, self.size(), helper);
    return appPath + QLatin1Char('/') + helper;
}

static bool writeFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    file.write(content);
    file.close();
    return true;
}

static QByteArray makeQmlContent(const QString &marker)
{
    return QLatin1String(R"(
        import QtQml
        Timer {
           interval: 10
           running: true
           repeat: true
           onTriggered: console.log("%1")
        })").arg(marker).toUtf8();
}

// A scene with a click counter and a built-in synthetic input source: once the
// window is active it injects exactly one mouse click (recorded by the preview
// tool's input recorder). didClick is preserved across in-place updates, so the
// synthetic click never fires again on hot reload - any further increment can
// only come from the tool replaying the recorded event.
static QByteArray makeCounterQml(const QString &marker)
{
    return QLatin1String(R"(
        import QtQuick
        import QtTest
        Window {
            id: window
            visible: true
            width: 100
            height: 100
            property int counter: 0
            property bool didClick: false
            property string marker: "%1"
            Component.onCompleted: window.requestActivate()
            Timer {
                interval: 50
                repeat: false
                running: window.active && !window.didClick
                onTriggered: {
                    window.didClick = true;
                    event.mouseClick(area, 12, 13, Qt.LeftButton, Qt.NoModifier, -1);
                }
            }
            Timer {
                interval: 100
                repeat: true
                running: true
                onTriggered: console.log(window.marker, "count=" + window.counter)
            }
            MouseArea {
                id: area
                anchors.fill: parent
                onClicked: window.counter = window.counter + 1
            }
            TestEvent { id: event }
        })").arg(marker).toUtf8();
}

// A scene that does not inject any input of its own. Its click counter can only
// be incremented by input the preview tool replays into it, so reaching count=1
// proves that a recorded click was replayed to restore the UI state.
static QByteArray makeReplayTargetQml(const QString &marker)
{
    return QLatin1String(R"(
        import QtQuick
        Window {
            id: window
            visible: true
            width: 100
            height: 100
            property int counter: 0
            property string marker: "%1"
            Component.onCompleted: window.requestActivate()
            Timer {
                interval: 100
                repeat: true
                running: true
                onTriggered: console.log(window.marker, "count=" + window.counter)
            }
            MouseArea {
                anchors.fill: parent
                onClicked: window.counter = window.counter + 1
            }
        })")
            .arg(marker)
            .toUtf8();
}

static bool writeQrcFile(const QString &path, const QString &prefix, const QStringList &files)
{
    QByteArray content = "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n";
    content += "    <qresource prefix=\"" + prefix.toUtf8() + "\">\n";
    for (const QString &file : files)
        content += "        <file>" + file.toUtf8() + "</file>\n";
    content += "    </qresource>\n</RCC>\n";
    return writeFile(path, content);
}

void tst_QmlPreviewTool::initTestCase()
{
    const QString binDir = QLibraryInfo::path(QLibraryInfo::BinariesPath);
    m_qmlPreviewPath = binDir + QLatin1String("/qmlpreview");
    m_qmlRuntimePath = binDir + QLatin1String("/qml");
    m_testHelperPath = findTestHelper();
    m_testHelperModulePath = findTestHelperModule();
}

void tst_QmlPreviewTool::cleanup()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.closeWriteChannel();
        if (!m_process.waitForFinished()) {
            m_process.terminate();
            if (!m_process.waitForFinished()) {
                m_process.kill();
                QVERIFY(m_process.waitForFinished());
            }
        }
    }

    if (QTest::currentTestFailed())
        qDebug().noquote() << "Process output:" << m_output;

    m_tempDir.reset();
    m_output.clear();
}

void tst_QmlPreviewTool::startPreview(const QStringList &args)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &tst_QmlPreviewTool::readProcessOutput);
    m_process.setEnvironment(QProcess::systemEnvironment());
    m_process.start(m_qmlPreviewPath, args);
    QVERIFY2(m_process.waitForStarted(5000),
             qPrintable(QLatin1String("Failed to start qmlpreview: ") + m_process.errorString()));
}

void tst_QmlPreviewTool::sendCommand(const QString &command)
{
    m_process.write(command.toUtf8() + '\n');
    m_process.waitForBytesWritten(2000);
}

void tst_QmlPreviewTool::readProcessOutput()
{
    m_output += QString::fromUtf8(m_process.readAll());
}

bool tst_QmlPreviewTool::waitForOutput(const QString &needle, int timeout)
{
    return QTest::qWaitFor([this, &needle]() {
        readProcessOutput();
        return m_output.contains(needle);
    }, timeout);
}

void tst_QmlPreviewTool::helpOption()
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(m_qmlPreviewPath, {QLatin1String("--help")});
    QVERIFY(proc.waitForFinished(5000));
    const QString output = QString::fromUtf8(proc.readAll());
    QVERIFY2(output.contains(QLatin1String("Preview")), qPrintable(output));
    QCOMPARE(proc.exitCode(), 0);
}

void tst_QmlPreviewTool::missingExecutable()
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(m_qmlPreviewPath,
               {QLatin1String("--verbose"),
                QLatin1String("/nonexistent/path/to/executable")});
    QVERIFY(proc.waitForFinished(15000));
    const QString output = QString::fromUtf8(proc.readAll());
    QVERIFY2(output.contains(QLatin1String("Could not run")), qPrintable(output));
    QVERIFY(proc.exitCode() != 0);
}

void tst_QmlPreviewTool::basicLaunch()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    const QString qmlFile = m_tempDir->filePath(QLatin1String("test.qml"));
    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("BASIC_LAUNCH_OK"))));

    startPreview({QLatin1String("--verbose"), m_qmlRuntimePath, qmlFile});
    QVERIFY2(waitForOutput(QLatin1String("BASIC_LAUNCH_OK")),
             qPrintable(QLatin1String("Timed out waiting for app output. Got:\n")
                        + m_output));
}

void tst_QmlPreviewTool::verboseOutput()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    const QString qmlFile = m_tempDir->filePath(QLatin1String("test.qml"));
    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("VERBOSE_TEST_OK"))));

    startPreview({QLatin1String("--verbose"), m_qmlRuntimePath, qmlFile});
    QVERIFY2(waitForOutput(QLatin1String("VERBOSE_TEST_OK")),
             qPrintable(QLatin1String("Timed out. Output:\n") + m_output));

    QVERIFY2(m_output.contains(QLatin1String("Listening on")),
             qPrintable(QLatin1String("Missing 'Listening on'. Output:\n") + m_output));
    QVERIFY2(m_output.contains(QLatin1String("Starting '")),
             qPrintable(QLatin1String("Missing 'Starting'. Output:\n") + m_output));
}

void tst_QmlPreviewTool::fileUpdate()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    const QString qmlFile = m_tempDir->filePath(QLatin1String("test.qml"));
    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("FILE_UPDATE_INITIAL"))));

    startPreview({QLatin1String("--verbose"), m_qmlRuntimePath, qmlFile});
    QVERIFY2(waitForOutput(QLatin1String("FILE_UPDATE_INITIAL")),
             qPrintable(QLatin1String("Initial load failed. Output:\n") + m_output));

    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("FILE_UPDATE_MODIFIED"))));

    QVERIFY2(waitForOutput(QLatin1String("FILE_UPDATE_MODIFIED")),
             qPrintable(QLatin1String("File update not detected. Output:\n") + m_output));
}

void tst_QmlPreviewTool::resourceMapping()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    const QString qmlFile = m_tempDir->filePath(QLatin1String("Main.qml"));
    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("RESOURCE_MAP_OK"))));

    const QString qrcFile = m_tempDir->filePath(QLatin1String("test.qrc"));
    QVERIFY(writeQrcFile(qrcFile, QLatin1String("/test"),
                         {QLatin1String("Main.qml")}));

    startPreview({QLatin1String("--verbose"),
                  QLatin1String("--resource"), qrcFile,
                  m_testHelperPath});

    QVERIFY2(waitForOutput(QLatin1String("RESOURCE_MAP_OK")),
             qPrintable(QLatin1String("Resource mapping failed. Output:\n") + m_output));
}

void tst_QmlPreviewTool::resourceFileUpdate()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    const QString qmlFile = m_tempDir->filePath(QLatin1String("Main.qml"));
    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("RES_UPDATE_INITIAL"))));

    const QString qrcFile = m_tempDir->filePath(QLatin1String("test.qrc"));
    QVERIFY(writeQrcFile(qrcFile, QLatin1String("/test"),
                         {QLatin1String("Main.qml")}));

    startPreview({QLatin1String("--verbose"),
                  QLatin1String("--resource"), qrcFile,
                  m_testHelperPath});

    QVERIFY2(waitForOutput(QLatin1String("RES_UPDATE_INITIAL")),
             qPrintable(QLatin1String("Initial load failed. Output:\n") + m_output));

    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("RES_UPDATE_MODIFIED"))));

    QVERIFY2(waitForOutput(QLatin1String("RES_UPDATE_MODIFIED")),
             qPrintable(QLatin1String("Resource file update not detected. Output:\n")
                        + m_output));
}

void tst_QmlPreviewTool::resourceVerboseOutput()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    const QString qmlFile = m_tempDir->filePath(QLatin1String("Main.qml"));
    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("RES_VERBOSE_OK"))));

    const QString qrcFile = m_tempDir->filePath(QLatin1String("test.qrc"));
    QVERIFY(writeQrcFile(qrcFile, QLatin1String("/test"),
                         {QLatin1String("Main.qml")}));

    startPreview({QLatin1String("--verbose"),
                  QLatin1String("--resource"), qrcFile,
                  m_testHelperPath});

    QVERIFY2(waitForOutput(QLatin1String("Resolved resource path")),
             qPrintable(QLatin1String("Resource resolution log missing. Output:\n")
                        + m_output));

    QVERIFY2(m_output.contains(QLatin1String(":/test/Main.qml")),
             qPrintable(QLatin1String("Resource path not in output. Output:\n")
                        + m_output));
}

void tst_QmlPreviewTool::loadFromModuleFileUpdate()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    const QString qmlFile = m_tempDir->filePath(QLatin1String("Main.qml"));
    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("MODULE_UPDATE_INITIAL"))));

    const QString qrcFile = m_tempDir->filePath(QLatin1String("test.qrc"));
    QVERIFY(writeQrcFile(qrcFile, QLatin1String("/qt/qml/QmlPreviewTestHelpers"),
                         {QLatin1String("Main.qml")}));

    startPreview({QLatin1String("--verbose"),
                  QLatin1String("--resource"), qrcFile,
                  m_testHelperModulePath});

    QVERIFY2(waitForOutput(QLatin1String("MODULE_UPDATE_INITIAL")),
             qPrintable(QLatin1String("Initial module load failed. Output:\n") + m_output));

    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("MODULE_UPDATE_MODIFIED"))));

    QVERIFY2(waitForOutput(QLatin1String("MODULE_UPDATE_MODIFIED")),
             qPrintable(QLatin1String("Module file update not detected. Output:\n")
                        + m_output));

    // The test application has never seen its own file.
    QVERIFY(!m_output.contains(QLatin1String("HELPER_MODULE_RESOURCE")));
}

void tst_QmlPreviewTool::settingConfiguration()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    const QString qmlFile = m_tempDir->filePath(QLatin1String("test.qml"));
    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("CONFIGURATION"))));

    startPreview({ QLatin1String("--verbose"), m_qmlRuntimePath, qmlFile });
    QVERIFY2(waitForOutput(QLatin1String("Inplace updates configuration: enabled")),
             qPrintable(QLatin1String("Did not receive configuration log. Output:\n") + m_output));
}

void tst_QmlPreviewTool::settingConfirmation()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    const QString qmlFile = m_tempDir->filePath(QLatin1String("test.qml"));
    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("CONFIRMATION"))));

    startPreview({ QLatin1String("--verbose"), m_qmlRuntimePath, qmlFile });
    QVERIFY2(waitForOutput(QLatin1String("Inplace updates setting confirmed as: enabled")),
             qPrintable(QLatin1String("Did not receive confirmation log. Output:\n") + m_output));
}

// A compatible edit (only the logged marker changes) hot reloads in place.
// The tool confirmed in-place updates on connect, so the loadTimer must NOT
// take the "replay recorded events" branch (that branch is reserved for the
// non-hot-reload workflow), and a successful hot reload must NOT restart the
// process: the UI is updated in place. This exercises "we must _not_ send the
// recorded events if hot reload succeeded".
void tst_QmlPreviewTool::hotReloadStaysInPlace()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    const QString qmlFile = m_tempDir->filePath(QLatin1String("test.qml"));
    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("INPLACE_INITIAL"))));

    startPreview({QLatin1String("--verbose"), m_qmlRuntimePath, qmlFile});
    QVERIFY2(waitForOutput(QLatin1String("Inplace updates setting confirmed as: enabled")),
             qPrintable(QLatin1String("Did not receive confirmation. Output:\n") + m_output));
    QVERIFY2(waitForOutput(QLatin1String("INPLACE_INITIAL")),
             qPrintable(QLatin1String("Initial load failed. Output:\n") + m_output));

    // Change only the marker string: a compatible in-place update.
    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("INPLACE_MODIFIED"))));
    QVERIFY2(waitForOutput(QLatin1String("INPLACE_MODIFIED")),
             qPrintable(QLatin1String("In-place update not applied. Output:\n") + m_output));

    // The update happened in place: no failure and no restart.
    QVERIFY2(!m_output.contains(QLatin1String("Hot reload failure")),
             qPrintable(QLatin1String("Unexpected hot reload failure. Output:\n") + m_output));
    QVERIFY2(!m_output.contains(QLatin1String("Restarting process")),
             qPrintable(QLatin1String("Process was restarted instead of updated in place. "
                                      "Output:\n")
                        + m_output));
}

// Regression test for input events destroying UI state on a successful hot
// reload. The scene injects one click, bumping a click counter to 1. When the
// file is hot reloaded in place, the tool must NOT replay the recorded click:
// the counter must stay 1. Before the fix the tool replayed the events even
// when the hot reload succeeded, so the counter kept incrementing on every
// reload.
void tst_QmlPreviewTool::inputNotReplayedOnHotReload()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    const QString qmlFile = m_tempDir->filePath(QLatin1String("test.qml"));
    QVERIFY(writeFile(qmlFile, makeCounterQml(QLatin1String("MARKER_INITIAL"))));

    startPreview({QLatin1String("--verbose"), m_qmlRuntimePath, qmlFile});

    // The synthetic click has been injected, recorded and applied: counter == 1.
    QVERIFY2(waitForOutput(QLatin1String("MARKER_INITIAL count=1")),
             qPrintable(QLatin1String("Synthetic click was not applied. Output:\n") + m_output));

    // Hot reload in place (marker-only, compatible change).
    QVERIFY(writeFile(qmlFile, makeCounterQml(QLatin1String("MARKER_MODIFIED"))));

    // Wait for a few post-reload heartbeats so a wrongly replayed click would
    // have had time to bump the counter.
    QVERIFY2(waitForOutput(QLatin1String("MARKER_MODIFIED count=")),
             qPrintable(QLatin1String("Hot reload was not applied. Output:\n") + m_output));
    const bool sawEnoughTicks = QTest::qWaitFor([this]() {
        readProcessOutput();
        return m_output.count(QLatin1String("MARKER_MODIFIED count=")) >= 3;
    }, 5000);
    QVERIFY2(sawEnoughTicks,
             qPrintable(QLatin1String("Not enough output after reload. Output:\n") + m_output));

    // The recorded click must not have been replayed into the in-place scene.
    QVERIFY2(!m_output.contains(QLatin1String("count=2")),
             qPrintable(QLatin1String("Recorded input was replayed after a successful hot "
                                      "reload, destroying UI state. Output:\n")
                        + m_output));

    // And the update happened in place, not via a restart.
    QVERIFY2(!m_output.contains(QLatin1String("Restarting process")),
             qPrintable(QLatin1String("Process was restarted instead of updated in place. "
                                      "Output:\n")
                        + m_output));
}

void tst_QmlPreviewTool::hotReloadFailure()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    QString qmlFile = m_tempDir->filePath(QLatin1String("test.qml"));
    // Start with valid QML
    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("HOTRELOAD_START"))));

    startPreview({QLatin1String("--verbose"), m_qmlRuntimePath, qmlFile});
    QVERIFY2(waitForOutput(QLatin1String("HOTRELOAD_START")),
             qPrintable(QLatin1String("Initial load failed. Output:\n") + m_output));

    // Change non-composite basetype to trigger hotReloadFailure event
    QVERIFY(writeFile(qmlFile, QLatin1String(R"(
        import QtQuick
        Item {
        }
    )").toUtf8()));

    QVERIFY2(waitForOutput(QLatin1String("Hot reload failure"), 10000),
             qPrintable(QLatin1String("Did not detect hot reload failure. Output:\n") + m_output));

    // After a hot reload failure, the process should be restarted
    QVERIFY2(waitForOutput(QLatin1String("Restarting process"), 10000),
             qPrintable(QLatin1String("Did not receive restart signal. Output:\n") + m_output));

    // Check for process lifecycle messages
    QVERIFY2(m_output.contains(QLatin1String("Terminating process")),
             qPrintable(QLatin1String("Missing 'Terminating process' message. Output:\n") + m_output));

    QVERIFY2(m_output.contains(QLatin1String("Starting '")),
             qPrintable(QLatin1String("Missing 'Starting' message. Output:\n") + m_output));
}

// In interactive mode the tool reads commands from standard input. Verify that
// the command loop starts, 'help' lists the registered commands, a command is
// dispatched to its handler, and 'quit' terminates the tool.
void tst_QmlPreviewTool::interactiveCommands()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    const QString qmlFile = m_tempDir->filePath(QLatin1String("test.qml"));
    QVERIFY(writeFile(qmlFile, makeQmlContent(QLatin1String("INTERACTIVE_OK"))));

    startPreview({ QLatin1String("--interactive"), QLatin1String("--verbose"), m_qmlRuntimePath,
                   qmlFile });

    QVERIFY2(waitForOutput(QLatin1String("Connected. Type a command")),
             qPrintable(QLatin1String("Interactive prompt did not appear. Output:\n") + m_output));

    sendCommand(QLatin1String("help"));
    QVERIFY2(waitForOutput(QLatin1String("'restart', 'r'")),
             qPrintable(QLatin1String("help did not list the commands. Output:\n") + m_output));

    sendCommand(QLatin1String("clear"));
    QVERIFY2(waitForOutput(QLatin1String("Recorded events cleared.")),
             qPrintable(QLatin1String("clear command was not handled. Output:\n") + m_output));

    sendCommand(QLatin1String("quit"));
    QVERIFY2(m_process.waitForFinished(10000),
             qPrintable(QLatin1String("quit did not terminate the tool. Output:\n") + m_output));
    QCOMPARE(m_process.exitCode(), 0);
}

// End-to-end round trip: record the input event stream of one session to a .qtd
// file, then load it in the next session and replay it to restore the UI state.
void tst_QmlPreviewTool::saveAndReplayRoundTrip()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    const QString recordQml = m_tempDir->filePath(QLatin1String("record.qml"));
    const QString replayQml = m_tempDir->filePath(QLatin1String("replay.qml"));
    const QString eventFile = m_tempDir->filePath(QLatin1String("events.qtd"));
    QVERIFY(writeFile(recordQml, makeCounterQml(QLatin1String("RECORD"))));
    QVERIFY(writeFile(replayQml, makeReplayTargetQml(QLatin1String("REPLAY"))));

    // Session 1: the scene injects one click; the tool records it. Save the
    // recorded stream to a .qtd file, then quit.
    startPreview({ QLatin1String("--interactive"), QLatin1String("--verbose"), m_qmlRuntimePath,
                   recordQml });
    QVERIFY2(waitForOutput(QLatin1String("RECORD count=1")),
             qPrintable(QLatin1String("Synthetic click was not recorded. Output:\n") + m_output));

    sendCommand(QLatin1String("output ") + eventFile);
    QVERIFY2(waitForOutput(QLatin1String("Events written to")),
             qPrintable(QLatin1String("output command failed. Output:\n") + m_output));

    sendCommand(QLatin1String("quit"));
    QVERIFY(m_process.waitForFinished(10000));

    QVERIFY2(QFile::exists(eventFile), "The .qtd event file was not created.");
    QVERIFY(QFileInfo(eventFile).size() > 0);

    // Session 2: a scene that never injects input of its own. Loading and
    // replaying the recorded stream must bump its counter to 1.
    m_output.clear();
    m_process.start(m_qmlPreviewPath,
                    { QLatin1String("--verbose"), QLatin1String("--replay"), eventFile,
                      m_qmlRuntimePath, replayQml });
    QVERIFY2(m_process.waitForStarted(5000),
             qPrintable(QLatin1String("Failed to start qmlpreview: ") + m_process.errorString()));

    QVERIFY2(waitForOutput(QLatin1String("REPLAY count=1")),
             qPrintable(QLatin1String("Replayed events did not restore the UI state. Output:\n")
                        + m_output));
}

// Same round trip as above, but with hot reload disabled (QMLPREVIEW_HOTRELOAD=0),
// so the target answers the configuration with an error instead of a confirmation.
// The replay then happens from the load timer, which only fires once the preview
// has issued its initial Load - so this also exercises that the client sends that
// Load without hot reload.
void tst_QmlPreviewTool::classicModeReplaysRecordedInput()
{
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    const QString recordQml = m_tempDir->filePath(QLatin1String("record.qml"));
    const QString replayQml = m_tempDir->filePath(QLatin1String("replay.qml"));
    const QString eventFile = m_tempDir->filePath(QLatin1String("events.qtd"));
    QVERIFY(writeFile(recordQml, makeCounterQml(QLatin1String("RECORD"))));
    QVERIFY(writeFile(replayQml, makeReplayTargetQml(QLatin1String("REPLAY"))));

    // Session 1: record the synthetic click and save it to a .qtd file.
    startPreview({ QLatin1String("--interactive"), QLatin1String("--verbose"), m_qmlRuntimePath,
                   recordQml });
    QVERIFY2(waitForOutput(QLatin1String("RECORD count=1")),
             qPrintable(QLatin1String("Synthetic click was not recorded. Output:\n") + m_output));

    sendCommand(QLatin1String("output ") + eventFile);
    QVERIFY2(waitForOutput(QLatin1String("Events written to")),
             qPrintable(QLatin1String("output command failed. Output:\n") + m_output));

    sendCommand(QLatin1String("quit"));
    QVERIFY(m_process.waitForFinished(10000));
    QVERIFY(QFileInfo(eventFile).size() > 0);

    // Session 2: replay with hot reload disabled. Restoring the counter to 1
    // requires the preview to have loaded the scene and then replayed the click.
    m_output.clear();
    QStringList env = QProcess::systemEnvironment();
    env.removeIf([](const QString &entry) {
        return entry.startsWith(QLatin1String("QMLPREVIEW_HOTRELOAD="));
    });
    env << QLatin1String("QMLPREVIEW_HOTRELOAD=0");
    m_process.setEnvironment(env);
    m_process.start(m_qmlPreviewPath,
                    { QLatin1String("--verbose"), QLatin1String("--replay"), eventFile,
                      m_qmlRuntimePath, replayQml });
    QVERIFY2(m_process.waitForStarted(5000),
             qPrintable(QLatin1String("Failed to start qmlpreview: ") + m_process.errorString()));

    QVERIFY2(waitForOutput(QLatin1String("REPLAY count=1")),
             qPrintable(QLatin1String("Replayed events did not restore the UI state without hot "
                                      "reload. Output:\n")
                        + m_output));
}

QTEST_MAIN(tst_QmlPreviewTool)

#include "tst_qmlpreviewtool.moc"
