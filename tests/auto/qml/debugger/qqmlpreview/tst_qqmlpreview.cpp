// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "debugutil_p.h"
#include "qqmldebugprocess_p.h"
#include "qqmlpreviewblacklist.h"

#include <private/qqmldebugconnection_p.h>
#include <private/qqmlpreviewclient_p.h>
#include <private/qqmlprofilerclient_p.h>
#include <private/qqmlprofilerqtdwriter_p.h>
#include <private/qquickeventreplayclient_p.h>

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qthread.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qregularexpression.h>
#include <QtNetwork/qhostaddress.h>

class tst_QQmlPreview : public QQmlDebugTest
{
    Q_OBJECT

public:
    tst_QQmlPreview();

private:
    ConnectResult startQmlProcess(const QString &qmlFile, QStringList environmentVariables = QStringList());
    void serveRequest(const QString &path);
    void serveFile(const QString &path, const QByteArray &contents);
    void enableInPlaceUpdates();
    struct Replacement {
        QByteArray from;
        QByteArray to;
    };
    QByteArray readAndModify(const QString &file, const QList<Replacement> &replacements);

    QList<QQmlDebugClient *> createClients() override;
    void verifyProcessOutputContains(const QString &string) const;

    QPointer<QQmlPreviewClient> m_client;
    QPointer<QQmlProfilerQtdWriter> m_qtdWriter;
    QPointer<QQmlProfilerClient> m_profiler;
    QPointer<QQuickEventReplayClient> m_replay;

    QStringList m_files;
    QStringList m_filesNotFound;
    QStringList m_directories;
    QStringList m_serviceErrors;
    QQmlPreviewClient::FpsInfo m_frameStats;
    QQmlPreviewClient::Settings m_confirmedSettings;
    QStringList m_hotReloadFailureReasons;

private slots:
    void cleanup() final;

    void connect();
    void load();
    void loadFromQrc();
    void rerun();
    void blacklist();
    void error();
    void zoom();
    void fps();
    void unhandledFiles_data();
    void unhandledFiles();
    void updateFile();
    void qqcStyleSelection();
    void singleton();
    void handleInput();
    void setAnimationSpeed();
    void createDirectory();

    // In-place update tests (Configuration + LoadResponse + loadPatch)
    void configurationMessage();
    void disableInPlaceUpdatesFails();
    void hotReloadFailureMessage();
    void firstLoadWithInPlaceEnabled();
    void inPlaceUpdateConstant();
    void inPlaceUpdateColor();
    void inPlaceUpdateBindingChange();
    void inPlaceUpdatePropertyAdd();
    void inPlaceUpdateBrokenFile();
    void rerunAfterInPlaceUpdate();
    void inPlaceUpdateMultipleSequential();
    void inPlacePropertyIntAccChange();
    void inPlaceAnchorsTargetChange();
    void inPlaceUpdatePropertyRemove();

    // QTBUG-142436: Window handling via in-place updates
    void inPlaceWindowPositionPreserved();
    void inPlaceWindowPositionNotOverriddenByBindings();

    // QTBUG-145905/145907/145908/145922: In-place update crash tests
    void inPlaceAnchorsTopTargetChange();
    void inPlaceSingletonBindingEdit();
    void inPlaceSingletonSelfBindingEdit();
    void inPlaceJsFunctionBodyEdit();
    void inPlaceObjectTreeRemoveChild();
    void inPlaceObjectTreeAddChild();
    void inPlaceChildComponentMultipleEdits();
    void inPlaceLazyComponentUpdateBeforeInstantiation();
    void inPlaceRequiredPropertyDelegateCrash();

    // samegame hot-reload trace regressions.
    // A child's parent.width/parent.height bindings must not become null after
    // an in-place rebuild of the root ("Cannot read property 'width' of null").
    void inPlaceChildParentBindingNotNull();
    // Same flood, but parent reached through `property Item parentBlock: parent`
    // (content/BlockEmitter.qml).
    void inPlaceParentIndirectionNotNull();
    // Reading a composite's VME var/QObject property while it is rebuilt must
    // not hit null member-data storage ("Cannot find member data").
    void inPlaceCompositeVmePropertyMemberData();
    // A dropped non-QML resource (image) must not be fed to the QML compiler
    // (".png:1:1: Unexpected token" / "Syntax error").
    void inPlaceImageNotParsedAsQml();
};

tst_QQmlPreview::tst_QQmlPreview()
    : QQmlDebugTest(QT_QMLTEST_DATADIR)
{
}

QQmlDebugTest::ConnectResult tst_QQmlPreview::startQmlProcess(const QString &qmlFile, QStringList environmentVariables)
{
    return QQmlDebugTest::connectTo(
            QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/qml",
            QStringLiteral("QmlPreview,CanvasFrameRate,EventReplay,EngineControl"),
            testFile(qmlFile), true, environmentVariables);
}

void tst_QQmlPreview::serveRequest(const QString &path)
{
    QFileInfo info(path);

    if (info.isDir()) {
        m_directories.append(path);
        m_client->sendDirectory(path, QDir(path).entryList());
    } else {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            serveFile(path, file.readAll());
        } else {
            m_filesNotFound.append(path);
            m_client->sendError(path);
        }
    }
}

void tst_QQmlPreview::serveFile(const QString &path, const QByteArray &contents)
{
    m_files.append(path);
    m_client->sendFile(path, contents);
}

QList<QQmlDebugClient *> tst_QQmlPreview::createClients()
{
    m_client = new QQmlPreviewClient(m_connection);
    m_qtdWriter = new QQmlProfilerQtdWriter(m_connection);
    m_profiler = new QQmlProfilerClient(m_connection, m_qtdWriter, 1 << ProfileInputEvents);
    m_replay = new QQuickEventReplayClient(m_connection);

    QObject::connect(m_client.data(), &QQmlPreviewClient::request, this, &tst_QQmlPreview::serveRequest);
    QObject::connect(m_client.data(), &QQmlPreviewClient::error, this, [this](const QString &error) {
        m_serviceErrors.append(error);
    });
    QObject::connect(m_client.data(), &QQmlPreviewClient::fps,
                     this, [this](const QQmlPreviewClient::FpsInfo &info) {
        m_frameStats = info;
    });
    QObject::connect(m_client.data(), &QQmlPreviewClient::confirmation,
                     this, [this](const QQmlPreviewClient::Settings &settings) {
        m_confirmedSettings = settings;
    });
    QObject::connect(m_client.data(), &QQmlPreviewClient::hotReloadFailure,
                     this, [this](const QString &reason) {
        m_hotReloadFailureReasons.append(reason);
    });

    return QList<QQmlDebugClient *>({m_client, m_profiler, m_replay});
}

void tst_QQmlPreview::verifyProcessOutputContains(const QString &string) const
{
    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().contains(string), 30000);
}

void checkFiles(const QStringList &files)
{
    QVERIFY(!files.contains("/etc/localtime"));
    QVERIFY(!files.contains("/etc/timezome"));
}

void tst_QQmlPreview::cleanup()
{
    // Use a separate function so that we don't return early from cleanup() on failure.
    checkFiles(m_files);

    QQmlDebugTest::cleanup();
    if (QTest::currentTestFailed()) {
        qDebug() << "Files loaded:" << m_files;
        qDebug() << "Files not loaded:" << m_filesNotFound;
        qDebug() << "Directories loaded:" << m_directories;
        qDebug() << "Errors reported:" << m_serviceErrors;
    }

    m_directories.clear();
    m_files.clear();
    m_filesNotFound.clear();
    m_serviceErrors.clear();
    m_frameStats = QQmlPreviewClient::FpsInfo();
    m_confirmedSettings = {};
    m_hotReloadFailureReasons.clear();
}

void tst_QQmlPreview::enableInPlaceUpdates()
{
    QQmlPreviewClient::Settings settings;
    settings.enableInPlaceUpdates = true;
    m_client->sendConfiguration(settings);
    QTRY_VERIFY_WITH_TIMEOUT(m_confirmedSettings.enableInPlaceUpdates, 15000);
}

void tst_QQmlPreview::connect()
{
    const QString file("window.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);
    m_client->triggerLoad(testFileUrl(file));
    QTRY_VERIFY(m_files.contains(testFile(file)));
    verifyProcessOutputContains(file);
    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

QByteArray tst_QQmlPreview::readAndModify(const QString &file,
                                          const QList<Replacement> &replacements)
{
    QFile input(testFile(file));
    if (!input.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QByteArray contents = input.readAll();
    for (const auto &r : replacements) {
        if (!contents.contains(r.from))
            return {};
        contents.replace(r.from, r.to);
    }
    return contents;
}

void tst_QQmlPreview::load()
{
    const QString file("qtquick2.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);
    m_client->triggerLoad(testFileUrl(file));
    QTRY_VERIFY(m_files.contains(testFile(file)));
    verifyProcessOutputContains("ms/degrees");

    const QStringList files({"window2.qml", "window1.qml", "window.qml"});
    for (const QString &newFile : files) {
        m_client->triggerLoad(testFileUrl(newFile));
        QTRY_VERIFY(m_files.contains(testFile(newFile)));
        verifyProcessOutputContains(newFile);
    }

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

void tst_QQmlPreview::loadFromQrc()
{
    // One of the configuration files built into the "qml" executable.
    const QString fromQrc(":/qt-project.org/imports/QmlRuntime/Config/default.qml");

    QCOMPARE(QQmlDebugTest::connectTo(
                     QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/qml",
                     QStringLiteral("QmlPreview,CanvasFrameRate,EventReplay,EngineControl"),
                     fromQrc, true),
             ConnectSuccess);

    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    serveFile(fromQrc, R"(
        import QtQuick
        Item {
            Component.onCompleted: console.log("default.qml replaced")
        }
    )");

    m_client->triggerLoad(QUrl("qrc" + fromQrc));
    verifyProcessOutputContains("default.qml replaced");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

void tst_QQmlPreview::rerun()
{
    const QString file("window.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    m_client->triggerLoad(testFileUrl(file));
    const QLatin1String message("window.qml");
    verifyProcessOutputContains(message);
    const int pos = m_process->output().lastIndexOf(message) + message.size();
    QVERIFY(pos >= 0);

    m_client->triggerRerun();
    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().indexOf(message, pos) >= pos, 30000);

    m_process->stop();
    QVERIFY(m_serviceErrors.isEmpty());
}

void tst_QQmlPreview::blacklist()
{
    QQmlPreviewBlacklist blacklist;

    QStringList strings({
        "lalala", "lulul", "trakdkd", "suppe", "zack"
    });

    for (const QString &string : strings)
        QVERIFY(!blacklist.isBlacklisted(string));

    for (const QString &string : strings)
        blacklist.blacklist(string);

    for (const QString &string : strings) {
        QVERIFY(blacklist.isBlacklisted(string));
        QVERIFY(!blacklist.isBlacklisted(string.left(string.size() / 2)));
        QVERIFY(!blacklist.isBlacklisted(string + "45"));
        QVERIFY(!blacklist.isBlacklisted(" " + string));
        QVERIFY(blacklist.isBlacklisted(string + "/45"));
    }

    for (auto begin = strings.begin(), it = begin, end = strings.end(); it != end; ++it) {
        std::rotate(begin, it, end);
        QString path = "/" + strings.join('/');
        blacklist.blacklist(path);
        QVERIFY(blacklist.isBlacklisted(path));
        QVERIFY(blacklist.isBlacklisted(path + "/file"));
        QVERIFY(!blacklist.isBlacklisted(path + "more"));
        path.chop(1);
        QVERIFY(!blacklist.isBlacklisted(path));
        std::reverse(begin, end);
    }

    blacklist.clear();
    for (const QString &string : strings)
        QVERIFY(!blacklist.isBlacklisted(string));

    blacklist.blacklist(":/qt-project.org");
    QVERIFY(blacklist.isBlacklisted(":/qt-project.org/QmlRuntime/conf/configuration.qml"));
    QVERIFY(!blacklist.isBlacklisted(":/qt-project.orgQmlRuntime/conf/configuration.qml"));

    QQmlPreviewBlacklist blacklist2;

    blacklist2.blacklist(":/qt-project.org");
    blacklist2.blacklist(":/QtQuick/Controls/Styles");
    blacklist2.blacklist(":/ExtrasImports/QtQuick/Controls/Styles");
    blacklist2.blacklist(QLibraryInfo::path(QLibraryInfo::QmlImportsPath));
    blacklist2.blacklist("/home/ulf/.local/share/QtProject/Qml Runtime/configuration.qml");
    blacklist2.blacklist("/usr/share");
    blacklist2.blacklist("/usr/share/QtProject/Qml Runtime/configuration.qml");
    QVERIFY(blacklist2.isBlacklisted(QLibraryInfo::path(QLibraryInfo::QmlImportsPath)));
    blacklist2.blacklist("/usr/local/share/QtProject/Qml Runtime/configuration.qml");
    blacklist2.blacklist("qml");
    blacklist2.blacklist(""); // This should not remove all other paths.

    QVERIFY(blacklist2.isBlacklisted(QLibraryInfo::path(QLibraryInfo::QmlImportsPath) +
                                     "/QtQuick/Window.2.0"));
    QVERIFY(blacklist2.isBlacklisted(QLibraryInfo::path(QLibraryInfo::QmlImportsPath)));
    QVERIFY(blacklist2.isBlacklisted("/usr/share/QtProject/Qml Runtime/configuration.qml"));
    QVERIFY(blacklist2.isBlacklisted("/usr/share/stuff"));
    QVERIFY(blacklist2.isBlacklisted(""));

    QQmlPreviewBlacklist blacklist3;
    blacklist3.blacklist("/usr/share");
    blacklist3.blacklist("/usr");
    blacklist3.blacklist("/usrdings");
    QVERIFY(blacklist3.isBlacklisted("/usrdings"));
    QVERIFY(blacklist3.isBlacklisted("/usr/src"));
    QVERIFY(!blacklist3.isBlacklisted("/opt/share"));
    QVERIFY(!blacklist3.isBlacklisted("/opt"));

    blacklist3.whitelist("/usr/share");
    QVERIFY(blacklist3.isBlacklisted("/usrdings"));
    QVERIFY(!blacklist3.isBlacklisted("/usr"));
    QVERIFY(!blacklist3.isBlacklisted("/usr/share"));
    QVERIFY(!blacklist3.isBlacklisted("/usr/src"));
    QVERIFY(!blacklist3.isBlacklisted("/opt/share"));
    QVERIFY(!blacklist3.isBlacklisted("/opt"));

    QQmlPreviewBlacklist blacklist4;
    blacklist4.whitelist(":/some/directory/with/file.qml");
    blacklist4.blacklist(":/some/directory/with");
    QVERIFY(blacklist4.isBlacklisted(":/some/directory/with"));
    QVERIFY(!blacklist4.isBlacklisted(":/some/directory/with/file.qml"));
}

void tst_QQmlPreview::error()
{
    QCOMPARE(startQmlProcess("window.qml"), ConnectSuccess);
    QVERIFY(m_client);
    m_client->triggerLoad(testFileUrl("broken.qml"));
    QTRY_COMPARE_WITH_TIMEOUT(m_serviceErrors.size(), 1, 10000);
    QVERIFY(m_serviceErrors.first().contains("broken.qml:7:1: Expected token `}'"));
}

static float parseZoomFactor(const QString &output)
{
    const QString prefix("zoom ");
    const int start = output.lastIndexOf(prefix) + prefix.size();
    if (start < 0)
        return -1;
    const int end = output.indexOf('\n', start);
    if (end < 0)
        return -1;
    bool ok = false;
    const float zoomFactor = output.mid(start, end - start).toFloat(&ok);
    if (!ok)
        return -1;
    return zoomFactor;
}

static void verifyZoomFactor(const QQmlDebugProcess *process, float factor)
{
    QTRY_VERIFY_WITH_TIMEOUT(qFuzzyCompare(parseZoomFactor(process->output()), factor), 30000);
}

void tst_QQmlPreview::zoom()
{
    const QString file("zoom.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    m_client->triggerLoad(testFileUrl(file));
    QTRY_VERIFY(m_files.contains(testFile(file)));
    float baseZoomFactor = -1;
    QTRY_VERIFY_WITH_TIMEOUT((baseZoomFactor = parseZoomFactor(m_process->output())) > 0, 30000);

    for (auto testZoomFactor : {2.0f, 1.5f, 0.5f}) {
        m_client->triggerZoom(testZoomFactor);
        verifyZoomFactor(m_process, testZoomFactor * baseZoomFactor);
    }

    m_client->triggerZoom(-1.0f);
    verifyZoomFactor(m_process, baseZoomFactor);
    m_process->stop();
    QVERIFY(m_serviceErrors.isEmpty());
}

void tst_QQmlPreview::fps()
{
    const QString file("qtquick2.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    m_client->triggerLoad(testFileUrl(file));
    if (QGuiApplication::platformName() != "offscreen") {
        QTRY_VERIFY_WITH_TIMEOUT(m_frameStats.numSyncs > 10, 30000);
        QVERIFY(m_frameStats.minSync <= m_frameStats.maxSync);
        QVERIFY(m_frameStats.totalSync / m_frameStats.numSyncs >= m_frameStats.minSync - 1);
        QVERIFY(m_frameStats.totalSync / m_frameStats.numSyncs <= m_frameStats.maxSync);

        QVERIFY(m_frameStats.numRenders > 0);
        QVERIFY(m_frameStats.minRender <= m_frameStats.maxRender);
        QVERIFY(m_frameStats.totalRender / m_frameStats.numRenders >= m_frameStats.minRender - 1);
        QVERIFY(m_frameStats.totalRender / m_frameStats.numRenders <= m_frameStats.maxRender);
    } else {
        QSKIP("offscreen rendering doesn't produce any frames");
    }
}

void tst_QQmlPreview::unhandledFiles_data()
{
    QTest::addColumn<QUrl>("file");
    QTest::addRow("dll")   << testFileUrl("a.dll");
    QTest::addRow("dylib") << testFileUrl("a.dylib");
    QTest::addRow("jsc")   << testFileUrl("a.jsc");
    QTest::addRow("mjsc")  << testFileUrl("a.mjsc");
    QTest::addRow("qmlc")  << testFileUrl("a.qmlc");
    QTest::addRow("so")    << testFileUrl("a.so");
}

void tst_QQmlPreview::unhandledFiles()
{
    QFETCH(QUrl, file);
    QCOMPARE(startQmlProcess("qtquick2.qml"), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);
    m_client->triggerLoad(file);
    verifyProcessOutputContains("fooh");
    QVERIFY(!m_files.contains(file.toLocalFile()));
}

void tst_QQmlPreview::updateFile()
{
    const QString file("qtquick2.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);
    m_client->triggerLoad(testFileUrl(file));
    QTRY_VERIFY(m_files.contains(testFile(file)));
    verifyProcessOutputContains("ms/degrees");

    QFile input(testFile(file));
    QVERIFY(input.open(QIODevice::ReadOnly));
    QByteArray contents = input.readAll();
    contents.replace("ms/degrees", "foozle/barzle");
    contents.replace("blue", "red");

    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));
    verifyProcessOutputContains("foozle/barzle");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

void tst_QQmlPreview::qqcStyleSelection()
{
    const QString file("withQQC.qml");
    const QString config = testFile("qqc2.conf");
    QCOMPARE(startQmlProcess(file, {"QT_QUICK_CONTROLS_CONF=" + config}), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);
    m_client->triggerLoad(testFileUrl(file));
    QTRY_VERIFY_WITH_TIMEOUT(m_files.contains(testFile(file)), 2000);
    QTRY_VERIFY_WITH_TIMEOUT(m_files.contains(testFile("qqc2.conf")), 2000);
    verifyProcessOutputContains("loaded");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

void tst_QQmlPreview::singleton()
{
    const QString file("singletonUser.qml");
    QCOMPARE(startQmlProcess(file, {"QML_IMPORT_PATH=" + dataDirectory()}), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);
    m_client->triggerLoad(testFileUrl(file));
    QTRY_VERIFY(m_files.contains(testFile(file)));
    verifyProcessOutputContains("col 0");

    QFile input(testFile("M/S.qml"));
    QVERIFY(input.open(QIODevice::ReadOnly));
    QByteArray contents = input.readAll();
    contents.replace("0", "5");

    serveFile(testFile("M/S.qml"), contents);
    m_client->triggerLoad(testFileUrl(file));
    verifyProcessOutputContains("col 5");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

void tst_QQmlPreview::handleInput()
{
    const QString file("input.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);
    m_client->triggerLoad(testFileUrl(file));
    QTRY_VERIFY(m_files.contains(testFile(file)));
    verifyProcessOutputContains("aaa #0000ff");

    const QQmlProfilerEventType mouseType {Event, MaximumRangeType, Mouse};
    const QList<QQmlProfilerEvent> clickEvents {{
        0ll, 0, QList<int>({InputMouseMove, 12, 13})
    }, {
        1ll, 0, QList<int>({InputMousePress, Qt::LeftButton, Qt::LeftButton})
    }, {
        2ll, 0, QList<int>({InputMouseRelease, Qt::LeftButton, Qt::NoButton})
    }};

    for (const QQmlProfilerEvent &event : clickEvents)
        m_replay->sendEvent(mouseType, event);

    verifyProcessOutputContains("aaa #ff0000");

    QFile input(testFile("input.qml"));
    QVERIFY(input.open(QIODevice::ReadOnly));
    QByteArray contents = input.readAll();
    contents.replace("aaa", "bbb");
    serveFile(testFile("input.qml"), contents);
    m_client->triggerLoad(testFileUrl(file));
    verifyProcessOutputContains("bbb #0000ff");

    for (const QQmlProfilerEvent &event : clickEvents)
        m_replay->sendEvent(mouseType, event);

    verifyProcessOutputContains("bbb #ff0000");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

void tst_QQmlPreview::setAnimationSpeed()
{
    const QString file("qtquick2.qml");

    // Basic render loop because that results in more exact animation timing.
    // With the other render loops it can take a long time until the animation timer
    // settles when you change it.
    QCOMPARE(startQmlProcess(file, {QLatin1String("QSG_RENDER_LOOP=basic")}), ConnectSuccess);

    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);
    m_client->triggerLoad(testFileUrl(file));
    QTRY_VERIFY(m_files.contains(testFile(file)));
    checkAnimationSpeed(m_process, 10);

    m_client->triggerAnimationSpeed(2);
    checkAnimationSpeed(m_process, 5);

    m_client->triggerAnimationSpeed(0.5);
    checkAnimationSpeed(m_process, 20);

    m_client->triggerAnimationSpeed(1);
    checkAnimationSpeed(m_process, 10);
}

void tst_QQmlPreview::createDirectory()
{
    const QString file("mkdir.qml");

    QQmlDebugTest::connectTo(
            debugJsServerPath("qqmlpreview"),
            QStringLiteral("QmlPreview,CanvasFrameRate,EventReplay,EngineControl"),
            testFile(file), true, {"TEST_MKDIR_RMDIR=1"});

    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);
    m_client->triggerLoad(testFileUrl(file));
    QTRY_VERIFY(m_files.contains(testFile(file)));

    verifyProcessOutputContains("mkdir rmdir ok");
}

void tst_QQmlPreview::configurationMessage()
{
    const QString file("window.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    // Send configuration before any load — should not cause errors.
    enableInPlaceUpdates();

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

void tst_QQmlPreview::disableInPlaceUpdatesFails()
{
    const QString file("window.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();

    // Attempt to disable — should produce an error, not a confirmation.
    QQmlPreviewClient::Settings settings;
    settings.enableInPlaceUpdates = false;
    m_client->sendConfiguration(settings);
    QTRY_VERIFY(!m_serviceErrors.isEmpty());
    QVERIFY(m_serviceErrors.first().contains("Cannot disable"));

    // The setting should still be enabled (unchanged).
    QVERIFY(m_confirmedSettings.enableInPlaceUpdates);

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

void tst_QQmlPreview::hotReloadFailureMessage()
{
    const QString file("window.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    m_client->triggerLoad(testFileUrl(file));

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);

    QVERIFY(m_hotReloadFailureReasons.isEmpty());
}

void tst_QQmlPreview::firstLoadWithInPlaceEnabled()
{
    // Start with window.qml, then try to debug-load inplace.qml.
    const QString startFile("window.qml");
    const QString loadFile("inplace.qml");
    QCOMPARE(startQmlProcess(startFile), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    // Enable in-place updates BEFORE the first debug-protocol load.
    enableInPlaceUpdates();

    // Loading an unrelated file makes no sense when doing in-place updates.
    // Nothing happens here.
    m_client->triggerLoad(testFileUrl(loadFile));
    verifyProcessOutputContains("window.qml");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());

    // The file is _not_ instantiated
    QVERIFY(!m_process->output().contains("inplace"));
}

// Core in-place update: change a constant integer value via the patch path.
void tst_QQmlPreview::inPlaceUpdateConstant()
{
    const QString file("inplace.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("inplace count=10 color=#0000ff");

    // Serve modified file: change count from 10 to 42.
    QByteArray contents = readAndModify(file, {{"count: 10", "count: 42"}});
    serveFile(testFile(file), contents);

    // Trigger re-load — should go through loadPatch.
    m_client->triggerLoad(testFileUrl(file));

    // The constant should be updated in-place; the Timer should output the new value.
    verifyProcessOutputContains("inplace count=42");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

// In-place update of a string constant (color property).
void tst_QQmlPreview::inPlaceUpdateColor()
{
    const QString file("inplace.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("color=#0000ff");

    // Serve modified file: change color from "blue" to "red".
    QByteArray contents = readAndModify(file, {{"\"blue\"", "\"red\""}});
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    // The color should be updated in-place.
    verifyProcessOutputContains("color=#ff0000");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

// In-place update: change a property binding (constant → script binding).
void tst_QQmlPreview::inPlaceUpdateBindingChange()
{
    const QString file("inplace_binding.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("binding computed=6");

    // Change the binding: base + 1 → base * 10 (i.e., computed becomes 50).
    QByteArray contents = readAndModify(file, {{"base + 1", "base * 10"}});
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    // After the in-place binding change, the computed value should update.
    verifyProcessOutputContains("binding computed=50");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

// In-place update: add a new property.  This is a structural change that
// requires reattach.
void tst_QQmlPreview::inPlaceUpdatePropertyAdd()
{
    const QString file("inplace.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("inplace count=10 color=#0000ff");

    // Add a new property and change the log to include it.
    QFile input(testFile(file));
    QVERIFY(input.open(QIODevice::ReadOnly));
    QByteArray contents = input.readAll();
    contents.replace("property int count: 10",
                     "property int count: 10\n    property string label: \"new\"");
    contents.replace("\"inplace count=\" + parent.count",
                     "\"inplace count=\" + parent.count + \" label=\" + parent.label");
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    // After the property-add reattach, both properties should be available.
    verifyProcessOutputContains("label=new");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

// Send a broken QML file during an in-place update.  The engine should
// report a compile error via the error signal rather than crashing.
void tst_QQmlPreview::inPlaceUpdateBrokenFile()
{
    const QString file("inplace.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("inplace count=10");

    // Serve a syntactically broken file.
    QByteArray broken("import QtQuick 2.0\nRectangle {\n  BROKEN SYNTAX\n");
    serveFile(testFile(file), broken);
    m_client->triggerLoad(testFileUrl(file));

    // An error should be reported (compile error), not a crash.
    QTRY_COMPARE_WITH_TIMEOUT(m_serviceErrors.size(), 1, 10000);
    QVERIFY(m_serviceErrors.first().contains("inplace.qml"));

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

void tst_QQmlPreview::rerunAfterInPlaceUpdate()
{
    const QString file("inplace.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();

    verifyProcessOutputContains("inplace count=10 color=#0000ff");

    QByteArray contents = readAndModify(file, {{"count: 10", "count: 55"}});
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));
    verifyProcessOutputContains("count=55");

    // Now trigger rerun. It should fail because we're in in-place mode.
    m_client->triggerRerun();
    QTRY_COMPARE(m_serviceErrors.size(), 1);

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QCOMPARE(m_serviceErrors.size(), 1);

    // Verify the in-place update actually took effect (count=55 appeared).
    QVERIFY(m_process->output().contains("inplace count=55"));
}

// Switch from in-place mode back to regular load mode.  The regular load
// should work as before (full scene recreation).
// Perform three sequential in-place updates to verify that the
// m_inplaceUpdates vector and finalize scope guard are cleaned up properly.
void tst_QQmlPreview::inPlaceUpdateMultipleSequential()
{
    const QString file("inplace.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("inplace count=10 color=#0000ff");

    // Update 1: count 10 → 20.
    QByteArray contents = readAndModify(file, {{"count: 10", "count: 20"}});
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));
    verifyProcessOutputContains("count=20");

    // Update 2: count 20 → 30 (modify the already-modified content).
    contents.replace("count: 20", "count: 30");
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));
    verifyProcessOutputContains("count=30");

    // Update 3: count 30 → 40, color blue → green.
    contents.replace("count: 30", "count: 40");
    contents.replace("\"blue\"", "\"green\"");
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));
    verifyProcessOutputContains("count=40");
    verifyProcessOutputContains("color=#008000");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

// Reproduce QTBUG-145906: editing a `property int acc: 0` binding value
// crashes qmlpreview.  This test creates a file with `property int acc: 0`
// and a binding `display: acc + 1`, then does an in-place update changing
// `acc: 0` to `acc: 5`.
void tst_QQmlPreview::inPlacePropertyIntAccChange()
{
    const QString file("inplace_acc.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("acc=0");

    QByteArray contents = readAndModify(file, {{"acc: 0", "acc: 5"}});
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().contains("acc=5") || !m_serviceErrors.isEmpty(),
                             15000);

    // Verify the process is still alive.
    QVERIFY2(m_process->state() != QProcess::NotRunning,
             "Process crashed during in-place update of property int binding");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

// Test in-place update that changes an anchors target from parent to sibling.
void tst_QQmlPreview::inPlaceAnchorsTargetChange()
{
    const QString file("inplace_anchors.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("anchors target.w=200");

    // Change anchors.fill from parent to sibling (width 100).
    QByteArray contents = readAndModify(file, {{"anchors.fill: parent", "anchors.fill: sibling"}});
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    // After the anchors change, target width should now be 100 (sibling size).
    verifyProcessOutputContains("anchors target.w=100");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

// In-place update that removes a property from the QML file.
// Structural removals may not be supported — the system should either
// handle it gracefully or fall back to a full reload, not crash.
void tst_QQmlPreview::inPlaceUpdatePropertyRemove()
{
    const QString file("inplace.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("inplace count=10 color=#0000ff");

    // Remove the count property and simplify the Timer log.
    QFile input(testFile(file));
    QVERIFY(input.open(QIODevice::ReadOnly));
    QByteArray contents = input.readAll();
    contents.replace("property int count: 10\n", "");
    contents.replace("\"inplace count=\" + parent.count + \" color=\" + parent.color",
                     "\"inplace removed color=\" + parent.color");
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    // Give it time to process; the main thing is no crash.
    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().contains("inplace removed"), 15000);

    // Verify the process is still alive.
    QVERIFY2(m_process->state() != QProcess::NotRunning,
             "Process crashed during in-place property removal");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

// QTBUG-142436 #1/#2: Window position should be preserved across in-place
// updates.  The window is never torn down, so its position should remain
// unchanged after patching a property.
void tst_QQmlPreview::inPlaceWindowPositionPreserved()
{
    const QString file("window_position.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    // Enable in-place updates before any file requests are served, so that
    // the service does not auto-load the first .qml file (which would create
    // duplicate objects).
    enableInPlaceUpdates();

    // The qml binary already loaded the file. Wait for initial output.
    verifyProcessOutputContains("pos x=");

    // Capture the position before the update.
    const QString output1 = m_process->output();
    const auto extractPos = [](const QString &output) -> std::pair<int, int> {
        int idx = output.lastIndexOf("pos x=");
        if (idx < 0)
            return { -1, -1 };
        const auto sub = QStringView(output).mid(idx);
        static const QRegularExpression re("pos x=(\\d+) y=(\\d+)");
        const auto match = re.matchView(sub);
        if (!match.hasMatch())
            return { -1, -1 };
        return { match.captured(1).toInt(), match.captured(2).toInt() };
    };

    const auto [x1, y1] = extractPos(output1);
    QVERIFY(x1 >= 0);
    QVERIFY(y1 >= 0);

    // Change width from 200 to 180 via in-place update (a harmless change).
    QByteArray contents = readAndModify(file, {{"width: 200", "width: 180"}});
    QVERIFY(!contents.isEmpty());
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    // Wait for output after the update.
    const int prevLen = output1.size();
    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().size() > prevLen
                                     && m_process->output().mid(prevLen).contains("pos x="),
                             15000);

    const auto [x2, y2] = extractPos(m_process->output().mid(prevLen));
    QVERIFY(x2 >= 0);
    QVERIFY(y2 >= 0);

    // Position should be unchanged after in-place update.
    QCOMPARE(x2, x1);
    QCOMPARE(y2, y1);

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

// QTBUG-142436 #1: When x/y bindings change in an in-place update, the
// window should NOT jump to the new binding values — the user's position
// should be preserved.
void tst_QQmlPreview::inPlaceWindowPositionNotOverriddenByBindings()
{
    const QString file("window_position.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();

    verifyProcessOutputContains("pos x=");

    // Change x/y binding values to something far away.
    QByteArray contents = readAndModify(file, {{"x: 50", "x: 300"}, {"y: 50", "y: 300"}});
    QVERIFY(!contents.isEmpty());

    const int prevLen = m_process->output().size();

    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().size() > prevLen
                                     && m_process->output().mid(prevLen).contains("pos x="),
                             15000);

    const QString newOutput = m_process->output().mid(prevLen);
    static const QRegularExpression re("pos x=(\\d+) y=(\\d+)");
    const auto match = re.match(newOutput);
    QVERIFY(match.hasMatch());
    const int xAfter = match.captured(1).toInt();
    const int yAfter = match.captured(2).toInt();

    // The window should NOT have jumped to x=300 y=300.
    QVERIFY2(xAfter != 300 || yAfter != 300,
             "Window position was overridden by QML bindings after in-place update");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

// QTBUG-145905: Changing an individual anchor target (e.g. anchors.top from
// parent.top to parent.verticalCenter) is not updated via in-place patching.
void tst_QQmlPreview::inPlaceAnchorsTopTargetChange()
{
    const QString file("inplace_anchors_top.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();

    verifyProcessOutputContains("anchors_top target.y=0");

    // Change anchors.top from parent.top to parent.verticalCenter.
    // QTBUG-145905: individual anchor target changes via in-place updates.
    QByteArray contents =
            readAndModify(file, {{"anchors.top: parent.top",
                                  "anchors.top: parent.verticalCenter"}});
    QVERIFY(!contents.isEmpty());
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    // After the anchor change, target.y should be non-zero (parent.verticalCenter).
    // The exact value depends on the window geometry (ResizeItemToWindow may
    // override the root's dimensions).
    const QRegularExpression nonZeroY("anchors_top target\\.y=([1-9]\\d*)");
    QTRY_VERIFY_WITH_TIMEOUT(nonZeroY.match(m_process->output()).hasMatch(), 15000);

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

// QTBUG-145907: Editing a file that has width: Settings.screenWidth
// (a singleton binding) crashes qmlpreview.
void tst_QQmlPreview::inPlaceSingletonBindingEdit()
{
    const QString file("singleton_test/Main.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();

    verifyProcessOutputContains("singleton_width root.w=320");

    QByteArray contents = readAndModify(file, {{"Settings.screenWidth",
                                                "Settings.screenWidth / 2"}});
    QVERIFY(!contents.isEmpty());
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().contains("singleton_width root.w=160"), 15000);

    QVERIFY2(m_process->state() != QProcess::NotRunning,
             "Process crashed during in-place edit of singleton binding");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

// Editing a singleton that has internal bindings referencing itself through its
// own type name (like Colors.qml in the coffee example: "derivedSize: Colors.baseSize * 2")
// crashed because removeCompilationUnitForUrl() destroyed the CU while
// lookupSingletonProperty still needed it during binding re-evaluation.
void tst_QQmlPreview::inPlaceSingletonSelfBindingEdit()
{
    const QString file("singleton_selfbind_test/Main.qml");
    const QString singletonFile("singleton_selfbind_test/Colors.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();

    verifyProcessOutputContains("selfbind_color color=#4682b4");

    // Edit the singleton itself: change currentColor. The singleton has a
    // self-referencing binding (derivedSize: Colors.baseSize * 2) that goes
    // through lookupSingletonProperty. Previously, removeCompilationUnitForUrl()
    // destroyed the CU, causing lookupSingletonProperty to crash when the binding
    // re-evaluated during the singleton object rebuild.
    QByteArray contents = readAndModify(singletonFile,
                                        {{"\"steelblue\"", "\"tomato\""}});
    QVERIFY(!contents.isEmpty());
    serveFile(testFile(singletonFile), contents);
    m_client->triggerLoad(testFileUrl(singletonFile));

    QTRY_VERIFY_WITH_TIMEOUT(
            m_process->output().contains("selfbind_color color=#ff6347"),
            15000);

    QVERIFY2(m_process->state() != QProcess::NotRunning,
             "Process crashed during in-place edit of singleton with self-bindings");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

// QTBUG-145908: Commenting out the nextPuzzle() implementation in
// samegame hits an assert in QQmlPropertyPrivate::setBinding.
void tst_QQmlPreview::inPlaceJsFunctionBodyEdit()
{
    const QString file("inplace_js_func.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();

    verifyProcessOutputContains("js_func loading level ");

    // Comment out the nextPuzzle() body, mirroring the samegame scenario
    // where acc is a property modified inside a function that also calls
    // another function (loadPuzzle).
    QByteArray contents = readAndModify(file,
                                        {{"acc = (acc + 1) % 10;", "// acc = (acc + 1) % 10;"},
                                         {"loadPuzzle();", "// loadPuzzle();"}});
    QVERIFY(!contents.isEmpty());
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().contains("js_func loading level "), 15000);

    QVERIFY2(m_process->state() != QProcess::NotRunning,
             "Process crashed during in-place JS function body edit");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

// QTBUG-145922 Test 1: Removing a child object from the QML tree via
// in-place update crashes.
void tst_QQmlPreview::inPlaceObjectTreeRemoveChild()
{
    const QString file("inplace_objtree.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();

    verifyProcessOutputContains("objtree children=");

    // Remove the background Image entirely (mirrors samegame's
    // Image { source: "content/gfx/background.png"; anchors.fill: parent }).
    QFile input(testFile(file));
    QVERIFY(input.open(QIODevice::ReadOnly | QIODevice::Text));
    QByteArray contents = input.readAll();
    contents.replace("    Image {\n"
                     "        source: \"content/gfx/background.png\"\n"
                     "        anchors.fill: parent\n"
                     "    }\n",
                     "");
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().contains("objtree children="), 15000);

    QVERIFY2(m_process->state() != QProcess::NotRunning,
             "Process crashed during in-place child object removal");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

// QTBUG-145922 Test 2: Adding a child object to the QML tree via
// in-place update crashes.
void tst_QQmlPreview::inPlaceObjectTreeAddChild()
{
    const QString file("inplace_objtree.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();

    verifyProcessOutputContains("objtree children=");

    // Add a new child Image after the existing background.
    QByteArray contents = readAndModify(file,
                                        {{"anchors.fill: parent",
                                          "anchors.fill: parent\n    }\n\n"
                                          "    Image {\n"
                                          "        source: \"content/gfx/bar.png\"\n"
                                          "        anchors.bottom: parent.bottom"}});
    QVERIFY(!contents.isEmpty());
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().contains("objtree children="), 15000);

    QVERIFY2(m_process->state() != QProcess::NotRunning,
             "Process crashed during in-place child object addition");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

// Test that editing a child component three consecutive times applies all
// changes.  Without the fix for stale resolved type references (commit
// 1b014d5ac6), the third edit would silently fail because the parent's
// type reference still pointed to the original compilation unit, preventing
// object discovery on the third reload round.
void tst_QQmlPreview::inPlaceChildComponentMultipleEdits()
{
    const QString file("inplace_typeref_test/Main.qml");
    const QString childFile("inplace_typeref_test/ChildItem.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("typeref_test label=initial");

    // Edit 1: label "initial" → "first"
    QByteArray contents = readAndModify(childFile, {{"\"initial\"", "\"first\""}});
    QVERIFY(!contents.isEmpty());
    serveFile(testFile(childFile), contents);
    m_client->triggerLoad(testFileUrl(childFile));
    verifyProcessOutputContains("typeref_test label=first");

    // Edit 2: label "first" → "second"
    contents.replace("\"first\"", "\"second\"");
    serveFile(testFile(childFile), contents);
    m_client->triggerLoad(testFileUrl(childFile));
    verifyProcessOutputContains("typeref_test label=second");

    // Edit 3: label "second" → "third"
    // This is the edit that fails without the type-reference update fix.
    contents.replace("\"second\"", "\"third\"");
    serveFile(testFile(childFile), contents);
    m_client->triggerLoad(testFileUrl(childFile));
    verifyProcessOutputContains("typeref_test label=third");

    QVERIFY2(m_process->state() != QProcess::NotRunning,
             "Process crashed during repeated child component edits");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

void tst_QQmlPreview::inPlaceLazyComponentUpdateBeforeInstantiation()
{
    const QString file("inplace_lazy_component_test/Main.qml");
    const QString childFile("inplace_lazy_component_test/LazyChild.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();

    // Wait until the app signals it's ready (child component is NOT yet instantiated).
    verifyProcessOutputContains("lazy_component_test ready");

    // Modify the child component BEFORE it has been instantiated.
    // Change label from "original" to "modified".
    QByteArray contents = readAndModify(childFile, {{"\"original\"", "\"modified\""}});
    QVERIFY(!contents.isEmpty());
    serveFile(testFile(childFile), contents);
    m_client->triggerLoad(testFileUrl(childFile));

    // The component will be instantiated after 0.5 seconds via the Loader timer.
    // When it is instantiated, it should use the MODIFIED version ("modified"),
    // not the old version ("original").
    // This verifies that in-place updates to not-yet-instantiated components
    // take effect upon future instantiation.
    QTRY_VERIFY_WITH_TIMEOUT(
            m_process->output().contains("lazy_component_test label=modified"), 15000);

    QVERIFY2(m_process->state() != QProcess::NotRunning,
             "Process crashed during lazy component in-place update");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
    QVERIFY(m_serviceErrors.isEmpty());
}

// Reproduces a use-after-free crash: when rebuilding a ListView that has an
// inline delegate with required properties, stashExternalState() stashes the
// QQmlPropertyToPropertyBinding that QQmlDelegateModel installed for each
// required property. The binding's sourceObject points to a
// QQmlDMListAccessorData that is freed (ref-count → 0, not via deleteLater)
// when reset() clears the model. restoreExternalState() then calls
// readSourceValue() with the dangling sourceObject → crash.
void tst_QQmlPreview::inPlaceRequiredPropertyDelegateCrash()
{
    const QString file("inplace_required_prop_delegate.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();

    // Wait for the process to be fully up and for the ListView to have
    // created its delegate items (currentItem must be non-null for
    // stashExternalState to create a child context for it).
    verifyProcessOutputContains("required_prop_delegate ready");

    const int prevLen = m_process->output().size();

    // Change clip: true → false in the ListView. This triggers rebuildObject()
    // on the ListView, stashing the QQmlPropertyToPropertyBindings for the
    // delegate's required properties, then freeing the source objects during
    // reset(), and finally crashing in restoreExternalState().
    QByteArray contents = readAndModify(file, {{"clip: true", "clip: false"}});
    QVERIFY(!contents.isEmpty());
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    // The Timer fires every 200 ms. After a successful in-place update the
    // process stays alive and produces new output shortly. If the process
    // crashed the condition stays false and the test fails at QTRY_VERIFY.
    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().size() > prevLen
                                    && m_process->output().mid(prevLen).contains(
                                            "required_prop_delegate ready"),
                             5000);

    QVERIFY2(m_process->state() != QProcess::NotRunning,
             "Process crashed when changing clip property on a ListView with "
             "required-property delegates (use-after-free in restoreExternalState)");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

// After an in-place rebuild of the root, a child whose geometry binds to
// parent.width / parent.height must keep a valid parent. In the samegame trace
// this manifested as a flood of "Cannot read property 'width' of null".
void tst_QQmlPreview::inPlaceChildParentBindingNotNull()
{
    const QString file("inplace_parent.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("parent_test hasParent=true marker=1");

    // Structural edit (add a child) forces a full rebuild of the root, plus a
    // marker bump so we can detect the new version took effect.
    QByteArray contents = readAndModify(
            file,
            {{"property int marker: 1", "property int marker: 2"},
             {"Rectangle {\n        id: child",
              "Rectangle { objectName: \"added\"; width: 10 }\n\n    Rectangle {\n        id: child"}});
    QVERIFY(!contents.isEmpty());

    const int prevLen = m_process->output().size();
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));

    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().size() > prevLen
                                     && m_process->output().mid(prevLen).contains("marker=2"),
                             15000);

    const QString after = m_process->output().mid(prevLen);

    // The child keeps a valid parent after the rebuild settles.
    QVERIFY(after.contains("parent_test hasParent=true marker=2"));

    // During the rebuild the child must keep a valid parent: clearBindingsRecursive
    // now runs before resetBindings clears the default-property list, preventing
    // QProperty bindings from firing with parent == null.
    const bool hadNullParentError =
            after.contains("Cannot read property 'width' of null")
            || after.contains("Cannot read property 'height' of null");
    QVERIFY2(!hadNullParentError,
             "child lost its parent during the in-place rebuild");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

// parent reached through `property Item parentBlock: parent` (BlockEmitter).
void tst_QQmlPreview::inPlaceParentIndirectionNotNull()
{
    const QString file("inplace_parentindirect.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("indirect hasParent=true marker=1");

    QByteArray contents = readAndModify(
            file,
            {{"property int marker: 1", "property int marker: 2"},
             {"Item {\n        id: child",
              "Item { objectName: \"added\"; width: 10 }\n\n    Item {\n        id: child"}});
    QVERIFY(!contents.isEmpty());

    const int prevLen = m_process->output().size();
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));
    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().size() > prevLen
                                     && m_process->output().mid(prevLen).contains("marker=2"),
                             15000);

    const QString after = m_process->output().mid(prevLen);
    const bool hadNull = after.contains("hasParent=false")
            || after.contains("Cannot read property 'width' of null")
            || after.contains("Cannot read property 'height' of null");
    QVERIFY2(!hadNull, "parentBlock indirection became null during the in-place rebuild");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

// A composite type with a VME QObject property (like LogoAnimation's
// `property ParticleSystem particleSystem`) read by inner Repeater-delegated
// children. When the composite's VME meta-object is rebuilt in place, that
// property is transiently unavailable: the delegate bindings read it as null,
// the same root cause as the samegame "Cannot find member data" /
// "Cannot read property ... of null" warnings around LogoAnimation.
void tst_QQmlPreview::inPlaceCompositeVmePropertyMemberData()
{
    const QString file("inplace_defaultprop.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("vmeprop size=8 marker=1");

    // Reload the composite type itself: this rebuilds its VME meta-object
    // (stash/restore of member data) while the Repeater-delegated children and
    // the root Timer keep reading container.payload through it.
    const QString composite("HotComposite.qml");
    QByteArray contents = readAndModify(composite,
                                        {{"property int size: 8", "property int size: 16"}});
    QVERIFY(!contents.isEmpty());

    const int prevLen = m_process->output().size();
    serveFile(testFile(composite), contents);
    m_client->triggerLoad(testFileUrl(composite));
    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().size() > prevLen
                                     && m_process->output().mid(prevLen).contains("size=16"),
                             15000);

    const QString after = m_process->output().mid(prevLen);

    // The new value does propagate once the rebuild settles.
    QVERIFY(after.contains("vmeprop size=16"));

    const bool vmePropertyNull = after.contains("Cannot find member data")
            || after.contains("Cannot read property 'size' of null");
    QVERIFY2(!vmePropertyNull,
             "composite VME property read as null during rebuild");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

// A dropped non-QML resource (image) must not be fed to the QML compiler.
// updateEngine() in the in-place handler constructs a QQmlComponent for every
// dropped URL; a dropped .png is therefore compiled as QML and reports
// ".png:1:1: Unexpected token" / "Syntax error" (samegame trace lines 9-16).
void tst_QQmlPreview::inPlaceImageNotParsedAsQml()
{
    const QString file("inplace_image.qml");
    QCOMPARE(startQmlProcess(file), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    enableInPlaceUpdates();
    verifyProcessOutputContains("image_test marker=1");

    // Simulate the IDE pushing a changed image and the .qml that references it,
    // exactly as qmlpreview does when watched project files change. Both are
    // "dropped" and then a Load is triggered.
    const QString pngPath = testFile("inplace_pixel.png");
    QFile png(pngPath);
    QVERIFY(png.open(QIODevice::ReadOnly));
    const QByteArray pngBytes = png.readAll();

    QByteArray contents = readAndModify(file, {{"property int marker: 1",
                                                "property int marker: 2"}});
    QVERIFY(!contents.isEmpty());

    serveFile(pngPath, pngBytes);
    serveFile(testFile(file), contents);
    m_client->triggerLoad(testFileUrl(file));
    verifyProcessOutputContains("image_test marker=2");

    const auto mentionsPngParseError = [](const QString &s) {
        return s.contains(".png") && (s.contains("Unexpected token") || s.contains("Syntax error"));
    };
    bool pngParsedAsQml = mentionsPngParseError(m_process->output());
    for (const QString &error : std::as_const(m_serviceErrors))
        pngParsedAsQml = pngParsedAsQml || mentionsPngParseError(error);

    QVERIFY2(!pngParsedAsQml, "image resource was parsed as QML during hot reload");

    m_process->stop();
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::NotConnected);
}

QTEST_MAIN(tst_QQmlPreview)

#include "tst_qqmlpreview.moc"
