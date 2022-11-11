// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debugutil_p.h"
#include "qqmldebugprocess_p.h"
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <private/qqmlenginedebugclient_p.h>
#include <private/qv4debugclient_p.h>
#include <private/qqmldebugconnection_p.h>
#include <private/qpacket_p.h>

#include <QtTest/qtest.h>
#include <QtTest/qtestsystem.h>
#include <QtCore/qprocess.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qmutex.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>

const char *BLOCKMODE = "-qmljsdebugger=port:3771,3800,block";
const char *NORMALMODE = "-qmljsdebugger=port:3771,3800";
const char *BLOCKRESTRICTEDMODE = "-qmljsdebugger=port:3771,3800,block,services:V8Debugger";
const char *NORMALRESTRICTEDMODE = "-qmljsdebugger=port:3771,3800,services:V8Debugger";
const char *TEST_QMLFILE = "test.qml";
const char *TEST_JSFILE = "test.js";
const char *TIMER_QMLFILE = "timer.qml";
const char *LOADJSFILE_QMLFILE = "loadjsfile.qml";
const char *EXCEPTION_QMLFILE = "exception.qml";
const char *ONCOMPLETED_QMLFILE = "oncompleted.qml";
const char *CREATECOMPONENT_QMLFILE = "createComponent.qml";
const char *CONDITION_QMLFILE = "condition.qml";
const char *QUIT_QMLFILE = "quit.qml";
const char *QUITINJS_QMLFILE = "quitInJS.qml";
const char *QUIT_JSFILE = "quit.js";
const char *CHANGEBREAKPOINT_QMLFILE = "changeBreakpoint.qml";
const char *STEPACTION_QMLFILE = "stepAction.qml";
const char *BREAKPOINTRELOCATION_QMLFILE = "breakpointRelocation.qml";
const char *ENCODEQMLSCOPE_QMLFILE = "encodeQmlScope.qml";
const char *BREAKONANCHOR_QMLFILE = "breakOnAnchor.qml";
const char *BREAKPOINTIDS_QMLFILE = "breakPointIds.qml";
const char *LETCONSTLOCALS_QMLFILE = "letConstLocals.qml";

#undef QVERIFY
#define QVERIFY(statement) \
do {\
    if (!QTest::qVerify((statement), #statement, "", __FILE__, __LINE__)) {\
        if (QTest::currentTestFailed()) \
          qDebug().nospace() << "\nDEBUGGEE OUTPUT:\n" << m_process->output();\
        return;\
    }\
} while (0)

class tst_QQmlDebugJS : public QQmlDebugTest
{
    Q_OBJECT

public:
    tst_QQmlDebugJS();

private slots:
    void initTestCase() override;

    void connect_data();
    void connect();
    void interrupt_data() { targetData(); }
    void interrupt();
    void getVersion_data() { targetData(); }
    void getVersion();
    void getVersionWhenAttaching_data() { targetData(); }
    void getVersionWhenAttaching();

    void disconnect_data() { targetData(); }
    void disconnect();

    void setBreakpointInScriptOnCompleted_data() { targetData(); }
    void setBreakpointInScriptOnCompleted();
    void setBreakpointInScriptOnComponentCreated_data() { targetData(); }
    void setBreakpointInScriptOnComponentCreated();
    void setBreakpointInScriptOnTimerCallback_data() { targetData(); }
    void setBreakpointInScriptOnTimerCallback();
    void setBreakpointInScriptInDifferentFile_data() { targetData(); }
    void setBreakpointInScriptInDifferentFile();
    void setBreakpointInScriptOnComment_data() { targetData(); }
    void setBreakpointInScriptOnComment();
    void setBreakpointInScriptOnEmptyLine_data() { targetData(); }
    void setBreakpointInScriptOnEmptyLine();
    void setBreakpointInScriptOnOptimizedBinding_data() { targetData(); }
    void setBreakpointInScriptOnOptimizedBinding();
    void setBreakpointInScriptWithCondition_data() { targetData(); }
    void setBreakpointInScriptWithCondition();
    void setBreakpointInScriptThatQuits_data() { targetData(); };
    void setBreakpointInScriptThatQuits();
    void setBreakpointInJavaScript_data();
    void setBreakpointInJavaScript();
    void setBreakpointWhenAttaching();

    void clearBreakpoint_data() { targetData(); }
    void clearBreakpoint();

    void changeBreakpoint_data() { targetData(); }
    void changeBreakpoint();

    void setExceptionBreak_data() { targetData(); }
    void setExceptionBreak();

    void stepNext_data() { targetData(); }
    void stepNext();
    void stepIn_data() { targetData(); }
    void stepIn();
    void stepOut_data() { targetData(); }
    void stepOut();
    void continueDebugging_data() { targetData(); }
    void continueDebugging();

    void backtrace_data() { targetData(); }
    void backtrace();

    void getFrameDetails_data() { targetData(); }
    void getFrameDetails();

    void getScopeDetails_data() { targetData(); }
    void getScopeDetails();

    void evaluateInGlobalScope();
    void evaluateInLocalScope_data() { targetData(); }
    void evaluateInLocalScope();

    void evaluateInContext();

    void getScripts_data() { targetData(); }
    void getScripts();

    void encodeQmlScope();
    void breakOnAnchor();

    void breakPointIds();
    void letConstLocals();

private:
    ConnectResult init(bool qmlscene, const QString &qmlFile = QString(TEST_QMLFILE),
                    bool blockMode = true, bool restrictServices = false);
    QList<QQmlDebugClient *> createClients() override;
    QPointer<QV4DebugClient> m_client;

    void targetData();
    bool waitForClientSignal(const char *signal, int timeout = 30000);
    void checkVersionParameters();
    int setBreakPoint(const QString &file, int sourceLine, bool enabled);
    void clearBreakPoint(int id);
};


tst_QQmlDebugJS::tst_QQmlDebugJS()
    : QQmlDebugTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQmlDebugJS::initTestCase()
{
    QQmlDebugTest::initTestCase();
}
#include <iostream>
QQmlDebugTest::ConnectResult tst_QQmlDebugJS::init(bool qmlscene, const QString &qmlFile,
                                                   bool blockMode, bool restrictServices)
{
    const QString executable = qmlscene
            ? QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/qmlscene"
            : debugJsServerPath("qqmldebugjs");
    return QQmlDebugTest::connectTo(
                executable, restrictServices ? QStringLiteral("V8Debugger") : QString(),
                testFile(qmlFile), blockMode);
}

void tst_QQmlDebugJS::connect_data()
{
    QTest::addColumn<bool>("blockMode");
    QTest::addColumn<bool>("restrictMode");
    QTest::addColumn<bool>("qmlscene");
    QTest::newRow("normal / unrestricted / custom")   << false << false << false;
    QTest::newRow("block  / unrestricted / custom")   << true  << false << false;
    QTest::newRow("normal / restricted   / custom")   << false << true  << false;
    QTest::newRow("block  / restricted   / custom")   << true  << true  << false;
    QTest::newRow("normal / unrestricted / qmlscene") << false << false << true;
    QTest::newRow("block  / unrestricted / qmlscene") << true  << false << true;
    QTest::newRow("normal / restricted   / qmlscene") << false << true  << true;
    QTest::newRow("block  / restricted   / qmlscene") << true  << true  << true;
}

void tst_QQmlDebugJS::connect()
{
    QFETCH(bool, blockMode);
    QFETCH(bool, restrictMode);
    QFETCH(bool, qmlscene);

    QCOMPARE(init(qmlscene, QString(TEST_QMLFILE), blockMode, restrictMode), ConnectSuccess);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(connected())));
}

void tst_QQmlDebugJS::interrupt()
{
    //void connect()
    QFETCH(bool, qmlscene);

    QCOMPARE(init(qmlscene), ConnectSuccess);
    m_client->connect();

    m_client->interrupt();
    QVERIFY(waitForClientSignal(SIGNAL(interrupted())));
}

void tst_QQmlDebugJS::getVersion()
{
    //void version()
    QFETCH(bool, qmlscene);

    QCOMPARE(init(qmlscene), ConnectSuccess);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(connected())));

    m_client->version();
    QVERIFY(waitForClientSignal(SIGNAL(result())));
    checkVersionParameters();
}

void tst_QQmlDebugJS::getVersionWhenAttaching()
{
    //void version()
    QFETCH(bool, qmlscene);

    QCOMPARE(init(qmlscene, QLatin1String(TIMER_QMLFILE), false), ConnectSuccess);
    m_client->connect();

    m_client->version();
    QVERIFY(waitForClientSignal(SIGNAL(result())));
    checkVersionParameters();
}

void tst_QQmlDebugJS::disconnect()
{
    //void disconnect()
    QFETCH(bool, qmlscene);

    QCOMPARE(init(qmlscene), ConnectSuccess);
    m_client->connect();

    m_client->disconnect();
    QVERIFY(waitForClientSignal(SIGNAL(result())));
}

void tst_QQmlDebugJS::setBreakpointInScriptOnCompleted()
{
    //void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = false, QString condition = QString(), int ignoreCount = -1)
    QFETCH(bool, qmlscene);

    int sourceLine = 9;
    QCOMPARE(init(qmlscene, ONCOMPLETED_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(ONCOMPLETED_QMLFILE), sourceLine, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    const QJsonObject body = m_client->response().body.toObject();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine);
    QCOMPARE(QFileInfo(body.value("script").toObject().value("name").toString()).fileName(),
             QLatin1String(ONCOMPLETED_QMLFILE));
}

void tst_QQmlDebugJS::setBreakpointInScriptOnComponentCreated()
{
    //void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = false, QString condition = QString(), int ignoreCount = -1)
    QFETCH(bool, qmlscene);

    int sourceLine = 9;
    QCOMPARE(init(qmlscene, CREATECOMPONENT_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(ONCOMPLETED_QMLFILE), sourceLine, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    const QJsonObject body = m_client->response().body.toObject();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine);
    QCOMPARE(QFileInfo(body.value("script").toObject().value("name").toString()).fileName(),
             QLatin1String(ONCOMPLETED_QMLFILE));
}

void tst_QQmlDebugJS::setBreakpointInScriptOnTimerCallback()
{
    QFETCH(bool, qmlscene);

    int sourceLine = 10;
    QCOMPARE(init(qmlscene, TIMER_QMLFILE), ConnectSuccess);

    m_client->connect();
    //We can set the breakpoint after connect() here because the timer is repeating and if we miss
    //its first iteration we can still catch the second one.
    m_client->setBreakpoint(QLatin1String(TIMER_QMLFILE), sourceLine, -1, true);
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    const QJsonObject body = m_client->response().body.toObject();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine);
    QCOMPARE(QFileInfo(body.value("script").toObject().value("name").toString()).fileName(),
             QLatin1String(TIMER_QMLFILE));
}

void tst_QQmlDebugJS::setBreakpointInScriptInDifferentFile()
{
    //void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = false, QString condition = QString(), int ignoreCount = -1)
    QFETCH(bool, qmlscene);

    int sourceLine = 6;
    QCOMPARE(init(qmlscene, LOADJSFILE_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(TEST_JSFILE), sourceLine, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    const QJsonObject body = m_client->response().body.toObject();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine);
    QCOMPARE(QFileInfo(body.value("script").toObject().value("name").toString()).fileName(),
             QLatin1String(TEST_JSFILE));
}

void tst_QQmlDebugJS::setBreakpointInScriptOnComment()
{
    //void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = false, QString condition = QString(), int ignoreCount = -1)
    QFETCH(bool, qmlscene);

    int sourceLine = 9;
    int actualLine = 11;
    QCOMPARE(init(qmlscene, BREAKPOINTRELOCATION_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(BREAKPOINTRELOCATION_QMLFILE), sourceLine, -1, true);
    m_client->connect();
    QEXPECT_FAIL("", "Relocation of breakpoints is disabled right now", Abort);
    QVERIFY(waitForClientSignal(SIGNAL(stopped()), 1));

    const QJsonObject body = m_client->response().body.toObject();

    QCOMPARE(body.value("sourceLine").toInt(), actualLine);
    QCOMPARE(QFileInfo(body.value("script").toObject().value("name").toString()).fileName(),
             QLatin1String(BREAKPOINTRELOCATION_QMLFILE));
}

void tst_QQmlDebugJS::setBreakpointInScriptOnEmptyLine()
{
    //void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = false, QString condition = QString(), int ignoreCount = -1)
    QFETCH(bool, qmlscene);

    int sourceLine = 10;
    int actualLine = 11;
    QCOMPARE(init(qmlscene, BREAKPOINTRELOCATION_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(BREAKPOINTRELOCATION_QMLFILE), sourceLine, -1, true);
    m_client->connect();
    QEXPECT_FAIL("", "Relocation of breakpoints is disabled right now", Abort);
    QVERIFY(waitForClientSignal(SIGNAL(stopped()), 1));

    const QJsonObject body = m_client->response().body.toObject();

    QCOMPARE(body.value("sourceLine").toInt(), actualLine);
    QCOMPARE(QFileInfo(body.value("script").toObject().value("name").toString()).fileName(),
             QLatin1String(BREAKPOINTRELOCATION_QMLFILE));
}

void tst_QQmlDebugJS::setBreakpointInScriptOnOptimizedBinding()
{
    //void setBreakpoint(QString type, QString target, int line = -1, int column = -1, bool enabled = false, QString condition = QString(), int ignoreCount = -1)
    QFETCH(bool, qmlscene);

    int sourceLine = 14;
    QCOMPARE(init(qmlscene, BREAKPOINTRELOCATION_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(BREAKPOINTRELOCATION_QMLFILE), sourceLine, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    const QJsonObject body = m_client->response().body.toObject();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine);
    QCOMPARE(QFileInfo(body.value("script").toObject().value("name").toString()).fileName(),
             QLatin1String(BREAKPOINTRELOCATION_QMLFILE));
}

void tst_QQmlDebugJS::setBreakpointInScriptWithCondition()
{
    QFETCH(bool, qmlscene);

    int out = 10;
    int sourceLine = 12;
    QCOMPARE(init(qmlscene, CONDITION_QMLFILE), ConnectSuccess);

    m_client->connect();
    //The breakpoint is in a timer loop so we can set it after connect().
    m_client->setBreakpoint(QLatin1String(CONDITION_QMLFILE), sourceLine, 1, true, QLatin1String("a > 10"));
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    //Get the frame index
    {
        const QJsonObject body = m_client->response().body.toObject();
        int frameIndex = body.value("index").toInt();

        //Verify the value of 'result'
        m_client->evaluate(QLatin1String("a"),frameIndex);
        QVERIFY(waitForClientSignal(SIGNAL(result())));
    }

    const QJsonObject body = m_client->response().body.toObject();
    QVERIFY(!body.isEmpty());
    QJsonValue val = body.value("value");
    QVERIFY(val.isDouble());

    const int a = val.toInt();
    QVERIFY(a > out);
}

void tst_QQmlDebugJS::setBreakpointInScriptThatQuits()
{
    QFETCH(bool, qmlscene);

    QCOMPARE(init(qmlscene, QUIT_QMLFILE), ConnectSuccess);

    int sourceLine = 11;

    m_client->setBreakpoint(QLatin1String(QUIT_QMLFILE), sourceLine, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    const QJsonObject body = m_client->response().body.toObject();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine);
    QCOMPARE(QFileInfo(body.value("script").toObject().value("name").toString()).fileName(), QLatin1String(QUIT_QMLFILE));

    m_client->continueDebugging(QV4DebugClient::Continue);
    QVERIFY(m_process->waitForFinished());
    QCOMPARE(m_process->exitStatus(), QProcess::NormalExit);
}

void tst_QQmlDebugJS::setBreakpointInJavaScript_data()
{
    QTest::addColumn<bool>("qmlscene");
    QTest::addColumn<bool>("seedCache");
    QTest::newRow("custom / immediate") << false << false;
    QTest::newRow("qmlscene / immediate") << true << false;
    QTest::newRow("custom / seeded") << false << true;
    QTest::newRow("qmlscene / seeded") << true << true;
}

void tst_QQmlDebugJS::setBreakpointInJavaScript()
{
    QFETCH(bool, qmlscene);
    QFETCH(bool, seedCache);

    if (seedCache) { // Make sure there is a qmlc file that the engine should _not_ laod.
        QProcess process;
        process.start(QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/qmlscene",
                      { testFile(QUITINJS_QMLFILE) });
        QTRY_COMPARE(process.state(), QProcess::NotRunning);
    }

    QCOMPARE(init(qmlscene, QUITINJS_QMLFILE), ConnectSuccess);

    const int sourceLine = 2;

    m_client->setBreakpoint(QLatin1String(QUIT_JSFILE), sourceLine, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    const QJsonObject body = m_client->response().body.toObject();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine);
    QCOMPARE(QFileInfo(body.value("script").toObject().value("name").toString()).fileName(),
             QLatin1String(QUIT_JSFILE));

    m_client->continueDebugging(QV4DebugClient::Continue);

    QVERIFY(m_process->waitForFinished());
    QCOMPARE(m_process->exitStatus(), QProcess::NormalExit);
}

void tst_QQmlDebugJS::setBreakpointWhenAttaching()
{
    int sourceLine = 10;
    QCOMPARE(init(true, QLatin1String(TIMER_QMLFILE), false), ConnectSuccess);

    m_client->connect();

    QSKIP("\nThe breakpoint may not hit because the engine may run in JIT mode or not have debug\n"
          "instructions, as we've connected in non-blocking mode above. That means we may have\n"
          "connected after the engine was already running, with all the QML already compiled.");

    //The breakpoint is in a timer loop so we can set it after connect().
    m_client->setBreakpoint(QLatin1String(TIMER_QMLFILE), sourceLine);

    QVERIFY(waitForClientSignal(SIGNAL(stopped())));
}

void tst_QQmlDebugJS::clearBreakpoint()
{
    //void clearBreakpoint(int breakpoint);
    QFETCH(bool, qmlscene);

    int sourceLine1 = 12;
    int sourceLine2 = 13;
    QCOMPARE(init(qmlscene, CHANGEBREAKPOINT_QMLFILE), ConnectSuccess);

    m_client->connect();
    //The breakpoints are in a timer loop so we can set them after connect().
    //Furthermore the breakpoints should be hit in the right order because setting of breakpoints
    //can only occur in the QML event loop. (see QCOMPARE for sourceLine2 below)
    m_client->setBreakpoint(QLatin1String(CHANGEBREAKPOINT_QMLFILE), sourceLine1, -1, true);
    m_client->setBreakpoint(QLatin1String(CHANGEBREAKPOINT_QMLFILE), sourceLine2, -1, true);

    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    {
        //Will hit 1st brakpoint, change this breakpoint enable = false
        const QJsonObject body = m_client->response().body.toObject();
        const QJsonArray breakpointsHit = body.value("breakpoints").toArray();

        int breakpoint = breakpointsHit.at(0).toInt();
        m_client->clearBreakpoint(breakpoint);

        QVERIFY(waitForClientSignal(SIGNAL(result())));

        //Continue with debugging
        m_client->continueDebugging(QV4DebugClient::Continue);
        //Hit 2nd breakpoint
        QVERIFY(waitForClientSignal(SIGNAL(stopped())));

        //Continue with debugging
        m_client->continueDebugging(QV4DebugClient::Continue);
    }

    //Should stop at 2nd breakpoint
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    {
        const QJsonObject body = m_client->response().body.toObject();
        QCOMPARE(body.value("sourceLine").toInt(), sourceLine2);
    }
}

void tst_QQmlDebugJS::changeBreakpoint()
{
    //void clearBreakpoint(int breakpoint);
    QFETCH(bool, qmlscene);

    int sourceLine2 = 12;
    int sourceLine1 = 13;
    const QString file = QLatin1String(CHANGEBREAKPOINT_QMLFILE);
    QCOMPARE(init(qmlscene, file), ConnectSuccess);

    bool isStopped = false;
    QObject::connect(m_client.data(), &QV4DebugClient::stopped, this, [&]() { isStopped = true; });

    auto continueDebugging = [&]() {
        m_client->continueDebugging(QV4DebugClient::Continue);
        isStopped = false;
    };

    m_client->connect();

    auto extractBody = [&]() {
        return m_client->response().body.toObject();
    };

    auto extractBreakPointId = [&](const QJsonObject &body) {
        const QJsonArray breakpointsHit = body.value("breakpoints").toArray();
        if (breakpointsHit.size() != 1)
            return -1;
        return breakpointsHit[0].toInt();
    };

    //The breakpoints are in a timer loop so we can set them after connect().
    //Furthermore the breakpoints should be hit in the right order because setting of breakpoints
    //can only occur in the QML event loop. (see QCOMPARE for sourceLine2 below)
    const int breakpoint1 = setBreakPoint(file, sourceLine1, false);
    QVERIFY(breakpoint1 >= 0);

    const int breakpoint2 = setBreakPoint(file, sourceLine2, true);
    QVERIFY(breakpoint2 >= 0);

    auto verifyBreakpoint = [&](int sourceLine, int breakpointId) {
        QTRY_VERIFY_WITH_TIMEOUT(isStopped, 30000);
        const QJsonObject body = extractBody();
        QCOMPARE(body.value("sourceLine").toInt(), sourceLine);
        QCOMPARE(extractBreakPointId(body), breakpointId);
    };

    verifyBreakpoint(sourceLine2, breakpoint2);

    continueDebugging();
    verifyBreakpoint(sourceLine2, breakpoint2);

    m_client->changeBreakpoint(breakpoint2, false);
    QVERIFY(waitForClientSignal(SIGNAL(result())));

    m_client->changeBreakpoint(breakpoint1, true);
    QVERIFY(waitForClientSignal(SIGNAL(result())));

    continueDebugging();
    verifyBreakpoint(sourceLine1, breakpoint1);

    continueDebugging();
    verifyBreakpoint(sourceLine1, breakpoint1);

    m_client->changeBreakpoint(breakpoint2, true);
    QVERIFY(waitForClientSignal(SIGNAL(result())));

    m_client->changeBreakpoint(breakpoint1, false);
    QVERIFY(waitForClientSignal(SIGNAL(result())));

    for (int i = 0; i < 3; ++i) {
        continueDebugging();
        verifyBreakpoint(sourceLine2, breakpoint2);
    }
}

void tst_QQmlDebugJS::setExceptionBreak()
{
    //void setExceptionBreak(QString type, bool enabled = false);
    QFETCH(bool, qmlscene);

    QCOMPARE(init(qmlscene, EXCEPTION_QMLFILE), ConnectSuccess);
    m_client->setExceptionBreak(QV4DebugClient::All,true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));
}

void tst_QQmlDebugJS::stepNext()
{
    //void continueDebugging(StepAction stepAction, int stepCount = 1);
    QFETCH(bool, qmlscene);

    int sourceLine = 12;
    QCOMPARE(init(qmlscene, STEPACTION_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(STEPACTION_QMLFILE), sourceLine, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    m_client->continueDebugging(QV4DebugClient::Next);
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    const QJsonObject body = m_client->response().body.toObject();

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine + 1);
    QCOMPARE(QFileInfo(body.value("script").toObject().value("name").toString()).fileName(),
             QLatin1String(STEPACTION_QMLFILE));
}

static QJsonObject responseBody(QV4DebugClient *client)
{
    return client->response().body.toObject();
}

void tst_QQmlDebugJS::stepIn()
{
    //void continueDebugging(StepAction stepAction, int stepCount = 1);
    QFETCH(bool, qmlscene);

    int sourceLine = 16;
    int actualLine = 11;
    QCOMPARE(init(qmlscene, STEPACTION_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(STEPACTION_QMLFILE), sourceLine, 1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));
    QCOMPARE(responseBody(m_client).value("sourceLine").toInt(), sourceLine);

    m_client->continueDebugging(QV4DebugClient::In);
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    const QJsonObject body = responseBody(m_client);
    QCOMPARE(body.value("sourceLine").toInt(), actualLine);
    QCOMPARE(QFileInfo(body.value("script").toObject().value("name").toString()).fileName(), QLatin1String(STEPACTION_QMLFILE));
}

void tst_QQmlDebugJS::stepOut()
{
    //void continueDebugging(StepAction stepAction, int stepCount = 1);
    QFETCH(bool, qmlscene);

    int sourceLine = 12;
    int actualLine = 16;
    QCOMPARE(init(qmlscene, STEPACTION_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(STEPACTION_QMLFILE), sourceLine, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));
    QCOMPARE(responseBody(m_client).value("sourceLine").toInt(), sourceLine);

    m_client->continueDebugging(QV4DebugClient::Out);
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    const QJsonObject body = responseBody(m_client);
    QCOMPARE(body.value("sourceLine").toInt(), actualLine);
    QCOMPARE(QFileInfo(body.value("script").toObject().value("name").toString()).fileName(), QLatin1String(STEPACTION_QMLFILE));
}

void tst_QQmlDebugJS::continueDebugging()
{
    //void continueDebugging(StepAction stepAction, int stepCount = 1);
    QFETCH(bool, qmlscene);

    int sourceLine1 = 16;
    int sourceLine2 = 13;
    QCOMPARE(init(qmlscene, STEPACTION_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(STEPACTION_QMLFILE), sourceLine1, -1, true);
    m_client->setBreakpoint(QLatin1String(STEPACTION_QMLFILE), sourceLine2, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    m_client->continueDebugging(QV4DebugClient::Continue);
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    const QJsonObject body = responseBody(m_client);

    QCOMPARE(body.value("sourceLine").toInt(), sourceLine2);
    QCOMPARE(QFileInfo(body.value("script").toObject().value("name").toString()).fileName(),
             QLatin1String(STEPACTION_QMLFILE));
}

void tst_QQmlDebugJS::backtrace()
{
    //void backtrace(int fromFrame = -1, int toFrame = -1, bool bottom = false);
    QFETCH(bool, qmlscene);

    int sourceLine = 9;
    QCOMPARE(init(qmlscene, ONCOMPLETED_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(ONCOMPLETED_QMLFILE), sourceLine, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    m_client->backtrace();
    QVERIFY(waitForClientSignal(SIGNAL(result())));
}

void tst_QQmlDebugJS::getFrameDetails()
{
    //void frame(int number = -1);
    QFETCH(bool, qmlscene);

    int sourceLine = 9;
    QCOMPARE(init(qmlscene, ONCOMPLETED_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(ONCOMPLETED_QMLFILE), sourceLine, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    m_client->frame();
    QVERIFY(waitForClientSignal(SIGNAL(result())));
}

void tst_QQmlDebugJS::getScopeDetails()
{
    //void scope(int number = -1, int frameNumber = -1);
    QFETCH(bool, qmlscene);

    int sourceLine = 9;
    QCOMPARE(init(qmlscene, ONCOMPLETED_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(ONCOMPLETED_QMLFILE), sourceLine, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    m_client->scope();
    QVERIFY(waitForClientSignal(SIGNAL(result())));
}

void tst_QQmlDebugJS::evaluateInGlobalScope()
{
    //void evaluate(QString expr, int frame = -1);
    QCOMPARE(init(true), ConnectSuccess);

    m_client->connect();

    for (int i = 0; i < 10; ++i) {
        // The engine might not be initialized, yet. We just try until it shows up.
        m_client->evaluate(QLatin1String("console.log('Hello World')"));
        if (waitForClientSignal(SIGNAL(result()), 500))
            break;
    }

    //Verify the return value of 'console.log()', which is "undefined"
    QCOMPARE(responseBody(m_client).value("type").toString(), QLatin1String("undefined"));
}

void tst_QQmlDebugJS::evaluateInLocalScope()
{
    //void evaluate(QString expr, bool global = false, bool disableBreak = false, int frame = -1, const QVariantMap &addContext = QVariantMap());
    QFETCH(bool, qmlscene);

    int sourceLine = 9;
    QCOMPARE(init(qmlscene, ONCOMPLETED_QMLFILE), ConnectSuccess);

    m_client->setBreakpoint(QLatin1String(ONCOMPLETED_QMLFILE), sourceLine, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    m_client->frame();
    QVERIFY(waitForClientSignal(SIGNAL(result())));

    {
        //Get the frame index
        const QJsonObject body = responseBody(m_client);
        int frameIndex = body.value("index").toInt();
        m_client->evaluate(QLatin1String("root.a"), frameIndex);
        QVERIFY(waitForClientSignal(SIGNAL(result())));
    }

    {
        //Verify the value of 'timer.interval'
        const QJsonObject body = responseBody(m_client);
        QCOMPARE(body.value("value").toInt(),10);
    }
}

void tst_QQmlDebugJS::evaluateInContext()
{
    m_connection = new QQmlDebugConnection();
    m_process = new QQmlDebugProcess(
                QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/qmlscene", this);
    m_client = new QV4DebugClient(m_connection);
    QScopedPointer<QQmlEngineDebugClient> engineClient(new QQmlEngineDebugClient(m_connection));
    m_process->start(QStringList() << QLatin1String(BLOCKMODE) << testFile(ONCOMPLETED_QMLFILE));

    QVERIFY(m_process->waitForSessionStart());

    m_connection->connectToHost("127.0.0.1", m_process->debugPort());
    QVERIFY(m_connection->waitForConnected());

    QTRY_COMPARE(m_client->state(), QQmlEngineDebugClient::Enabled);
    QTRY_COMPARE(engineClient->state(), QQmlEngineDebugClient::Enabled);
    m_client->connect();

    // "a" not accessible without extra context
    m_client->evaluate(QLatin1String("a + 10"), -1, -1);
    QVERIFY(waitForClientSignal(SIGNAL(failure())));

    bool success = false;
    engineClient->queryAvailableEngines(&success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(engineClient.data(), SIGNAL(result())));

    QVERIFY(engineClient->engines().size());
    engineClient->queryRootContexts(engineClient->engines()[0], &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(engineClient.data(), SIGNAL(result())));

    auto contexts = engineClient->rootContext().contexts;
    QCOMPARE(contexts.size(), 1);
    auto objects = contexts[0].objects;
    QCOMPARE(objects.size(), 1);
    engineClient->queryObjectRecursive(objects[0], &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(engineClient.data(), SIGNAL(result())));
    auto object = engineClient->object();

    // "a" accessible in context of surrounding object
    m_client->evaluate(QLatin1String("a + 10"), -1, object.debugId);
    QVERIFY(waitForClientSignal(SIGNAL(result())));

    QTRY_COMPARE(responseBody(m_client).value("value").toInt(), 20);

    auto childObjects = object.children;
    QVERIFY(childObjects.size() > 0); // QQmlComponentAttached is also in there
    QCOMPARE(childObjects[0].className, QString::fromLatin1("Item"));

    // "b" accessible in context of surrounding (child) object
    m_client->evaluate(QLatin1String("b"), -1, childObjects[0].debugId);
    QVERIFY(waitForClientSignal(SIGNAL(result())));

    QTRY_COMPARE(responseBody(m_client).value("value").toInt(), 11);
}

void tst_QQmlDebugJS::getScripts()
{
    //void scripts(int types = -1, QList<int> ids = QList<int>(), bool includeSource = false, QVariant filter = QVariant());
    QFETCH(bool, qmlscene);

    QCOMPARE(init(qmlscene), ConnectSuccess);

    m_client->setBreakpoint(QString(TEST_QMLFILE), 10, -1, true);
    m_client->connect();
    QVERIFY(waitForClientSignal(SIGNAL(stopped())));

    m_client->scripts();
    QVERIFY(waitForClientSignal(SIGNAL(result())));

    const QJsonArray scripts = m_client->response().body.toArray();

    QCOMPARE(scripts.count(), 1);
    QVERIFY(scripts.first().toObject()[QStringLiteral("name")].toString()
            .endsWith(QStringLiteral("data/test.qml")));
}

void tst_QQmlDebugJS::encodeQmlScope()
{
    QString file(ENCODEQMLSCOPE_QMLFILE);
    QCOMPARE(init(true, file), ConnectSuccess);

    int numFrames = 0;
    int numExpectedScopes = 0;
    int numReceivedScopes = 0;
    bool isStopped = false;
    bool scopesFailed = false;

    QObject::connect(m_client.data(), &QV4DebugClient::failure, this, [&]() {
        qWarning() << "received failure" << m_client->response().body;
        scopesFailed = true;
        m_process->stop();
        numFrames = 2;
        isStopped = false;
    });

    QObject::connect(m_client.data(), &QV4DebugClient::stopped, this, [&]() {
        m_client->frame();
        isStopped = true;
    });

    QObject::connect(m_client.data(), &QV4DebugClient::result, this, [&]() {
        const QV4DebugClient::Response value = m_client->response();

        if (value.command == QString("scope")) {
            // If the scope commands fail we get a failure() signal above.
            if (++numReceivedScopes == numExpectedScopes) {
                m_client->continueDebugging(QV4DebugClient::Continue);
                isStopped = false;
            }
        } else if (value.command == QString("frame")) {

            // We want at least a global scope and some kind of local scope here.
            const QJsonArray scopes = value.body.toObject().value("scopes").toArray();
            if (scopes.count() < 2)
                scopesFailed = true;

            for (const QJsonValue scope : scopes) {
                ++numExpectedScopes;
                m_client->scope(scope.toObject().value("index").toInt());
            }

            ++numFrames;
        }
    });

    m_client->setBreakpoint(file, 6);
    m_client->setBreakpoint(file, 8);
    m_client->connect();

    QTRY_COMPARE(numFrames, 2);
    QVERIFY(numExpectedScopes > 3);
    QVERIFY(!scopesFailed);
    QTRY_VERIFY(!isStopped);
    QCOMPARE(numReceivedScopes, numExpectedScopes);
}

void tst_QQmlDebugJS::breakOnAnchor()
{
    QString file(BREAKONANCHOR_QMLFILE);
    QCOMPARE(init(true, file), ConnectSuccess);

    int breaks = 0;
    bool stopped = false;
    QObject::connect(m_client.data(), &QV4DebugClient::stopped, this, [&]() {
        stopped = true;
        ++breaks;
        m_client->evaluate("this", 0, -1);
    });

    QObject::connect(m_client.data(), &QV4DebugClient::result, this, [&]() {
        if (stopped) {
            m_client->continueDebugging(QV4DebugClient::Continue);
            stopped = false;
        }
    });

    QObject::connect(m_client.data(), &QV4DebugClient::failure, this, [&]() {
        qWarning() << "received failure" << m_client->response().body;
    });

    m_client->setBreakpoint(file, 9);
    m_client->setBreakpoint(file, 12);

    QTRY_COMPARE(m_process->state(), QProcess::Running);

    m_client->connect();

    QTRY_COMPARE(m_process->state(), QProcess::NotRunning);
    QCOMPARE(m_process->exitStatus(), QProcess::NormalExit);

    QCOMPARE(breaks, 2);
}

void tst_QQmlDebugJS::breakPointIds()
{
    QString file(BREAKPOINTIDS_QMLFILE);
    QCOMPARE(init(true, file), ConnectSuccess);

    int breaks = 0;
    int breakPointIds[] = { -1, -1, -1, -1, -1, -1};

    QObject::connect(m_client.data(), &QV4DebugClient::stopped, this, [&]() {
        const QJsonObject body = m_client->response().body.toObject();
        QCOMPARE(body.value("sourceLine").toInt(), breaks + 4);
        const QJsonArray breakpointsHit = body.value("breakpoints").toArray();
        QVERIFY(breakpointsHit.size() > 0);
        QCOMPARE(breakpointsHit[0].toInt(), breakPointIds[breaks]);
        ++breaks;
        m_client->continueDebugging(QV4DebugClient::Continue);
    });

    for (int i = 0; i < 6; ++i)
        breakPointIds[i] = setBreakPoint(file, i + 4, true);

    clearBreakPoint(breakPointIds[2]);
    breakPointIds[2] = setBreakPoint(file, 6, true);

    QTRY_COMPARE(m_process->state(), QProcess::Running);
    m_client->connect();

    QTRY_COMPARE(m_process->state(), QProcess::NotRunning);
    QCOMPARE(m_process->exitStatus(), QProcess::NormalExit);

    QCOMPARE(breaks, 6);
}

void tst_QQmlDebugJS::letConstLocals()
{
    QString file(LETCONSTLOCALS_QMLFILE);
    QCOMPARE(init(true, file), ConnectSuccess);

    QObject::connect(m_client.data(), &QV4DebugClient::stopped, this, [&]() {
        m_client->frame();
    });

    int numScopes = 0;
    QString expectedMembers = QStringLiteral("abcde");
    QObject::connect(m_client.data(), &QV4DebugClient::result, this, [&]() {
        const auto value = m_client->response();
        if (value.command == QStringLiteral("frame")) {
            const auto scopes = value.body.toObject().value(QStringLiteral("scopes")).toArray();
            for (const auto scope : scopes) {
                const auto scopeObject = scope.toObject();
                const int type = scopeObject.value("type").toInt();
                if (type == 1 || type == 4) {
                    m_client->scope(scopeObject.value("index").toInt());
                    ++numScopes;
                }
            }
            QVERIFY(numScopes > 0);
        } else if (value.command == QStringLiteral("scope")) {
            const auto props = value.body.toObject().value(QStringLiteral("object")).toObject()
                    .value(QStringLiteral("properties")).toArray();
            for (const auto prop : props) {
                const auto propObj = prop.toObject();
                QString name = propObj.value(QStringLiteral("name")).toString();
                QVERIFY(name.size() == 1);
                auto i = expectedMembers.indexOf(name.at(0));
                QVERIFY(i != -1);
                expectedMembers.remove(i, 1);
                QCOMPARE(propObj.value(QStringLiteral("type")).toString(),
                         QStringLiteral("number"));
                QCOMPARE(propObj.value(QStringLiteral("value")).toInt(),
                         int(name.at(0).toLatin1()));
            }
            if (--numScopes == 0) {
                QVERIFY(expectedMembers.isEmpty());
                m_client->continueDebugging(QV4DebugClient::Continue);
            }
        }
    });

    setBreakPoint(file, 10, true);

    QTRY_COMPARE(m_process->state(), QProcess::Running);
    m_client->connect();

    QTRY_COMPARE(m_process->state(), QProcess::NotRunning);
    QCOMPARE(m_process->exitStatus(), QProcess::NormalExit);
}

QList<QQmlDebugClient *> tst_QQmlDebugJS::createClients()
{
    m_client = new QV4DebugClient(m_connection);
    return QList<QQmlDebugClient *>({m_client});
}

void tst_QQmlDebugJS::targetData()
{
    QTest::addColumn<bool>("qmlscene");
    QTest::newRow("custom")   << false;
    QTest::newRow("qmlscene") << true;
}

bool tst_QQmlDebugJS::waitForClientSignal(const char *signal, int timeout)
{
    return QQmlDebugTest::waitForSignal(m_client.data(), signal, timeout);
}

void tst_QQmlDebugJS::checkVersionParameters()
{
    const QV4DebugClient::Response value = m_client->response();
    QCOMPARE(value.command, QString("version"));
    const QJsonObject body = value.body.toObject();
    QCOMPARE(body.value("UnpausedEvaluate").toBool(), true);
    QCOMPARE(body.value("ContextEvaluate").toBool(), true);
    QCOMPARE(body.value("ChangeBreakpoint").toBool(), true);
}

int tst_QQmlDebugJS::setBreakPoint(const QString &file, int sourceLine, bool enabled)
{
    int id = -1;
    auto connection = QObject::connect(m_client.data(), &QV4DebugClient::result, [&]() {
        id = m_client->response().body.toObject().value("breakpoint").toInt();
    });

    m_client->setBreakpoint(file, sourceLine, -1, enabled);
    bool success = QTest::qWaitFor([&]() { return id >= 0; });
    Q_UNUSED(success);

    QObject::disconnect(connection);
    return id;
}

void tst_QQmlDebugJS::clearBreakPoint(int id)
{
    bool ok = false;
    auto connection = QObject::connect(m_client.data(), &QV4DebugClient::result, [&]() {
        ok = true;
    });

    m_client->clearBreakpoint(id);
    bool success = QTest::qWaitFor([&]() { return ok; });
    Q_UNUSED(success);

    QObject::disconnect(connection);
}

QTEST_MAIN(tst_QQmlDebugJS)

#include "tst_qqmldebugjs.moc"

