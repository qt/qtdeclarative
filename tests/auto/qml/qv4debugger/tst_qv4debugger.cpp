/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtTest/QtTest>

#include <QJSEngine>
#include <private/qv4engine_p.h>
#include <private/qv4debugging_p.h>
#include <private/qv8engine_p.h>

static bool waitForSignal(QObject* obj, const char* signal, int timeout = 10000)
{
    QEventLoop loop;
    QObject::connect(obj, signal, &loop, SLOT(quit()));
    QTimer timer;
    QSignalSpy timeoutSpy(&timer, SIGNAL(timeout()));
    if (timeout > 0) {
        QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        timer.setSingleShot(true);
        timer.start(timeout);
    }
    loop.exec();
    return timeoutSpy.isEmpty();
}

class TestEngine : public QJSEngine
{
    Q_OBJECT
public:
    TestEngine()
    {
        qMetaTypeId<InjectedFunction>();
    }

    Q_INVOKABLE void evaluate(const QString &script, const QString &fileName, int lineNumber = 1)
    {
        QJSEngine::evaluate(script, fileName, lineNumber);
        emit evaluateFinished();
    }

    QV4::ExecutionEngine *v4Engine() { return QV8Engine::getV4(this); }

    typedef QV4::ReturnedValue (*InjectedFunction)(QV4::SimpleCallContext*);

    Q_INVOKABLE void injectFunction(const QString &functionName, TestEngine::InjectedFunction injectedFunction)
    {
        QV4::ExecutionEngine *v4 = v4Engine();

        QV4::String *name = v4->newString(functionName);
        QV4::Value function = QV4::Value::fromObject(v4->newBuiltinFunction(v4->rootContext, name, injectedFunction));
        v4->globalObject->put(name, function);
    }

signals:
    void evaluateFinished();
};

Q_DECLARE_METATYPE(TestEngine::InjectedFunction)

class TestAgent : public QV4::Debugging::DebuggerAgent
{
    Q_OBJECT
public:
    TestAgent()
        : m_wasPaused(false)
    {
    }

    virtual void debuggerPaused(QV4::Debugging::Debugger *debugger)
    {
        Q_ASSERT(m_debuggers.count() == 1 && m_debuggers.first() == debugger);
        m_wasPaused = true;
        m_statesWhenPaused << debugger->currentExecutionState();

        foreach (const TestBreakPoint &bp, m_breakPointsToAddWhenPaused)
            debugger->addBreakPoint(bp.fileName, bp.lineNumber);
        m_breakPointsToAddWhenPaused.clear();

        debugger->resume();
    }

    int debuggerCount() const { return m_debuggers.count(); }

    struct TestBreakPoint
    {
        TestBreakPoint() : lineNumber(-1) {}
        TestBreakPoint(const QString &fileName, int lineNumber)
            : fileName(fileName), lineNumber(lineNumber) {}
        QString fileName;
        int lineNumber;
    };

    bool m_wasPaused;
    QList<QV4::Debugging::Debugger::ExecutionState> m_statesWhenPaused;
    QList<TestBreakPoint> m_breakPointsToAddWhenPaused;
};

class tst_qv4debugger : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    void breakAnywhere();
    void pendingBreakpoint();
    void liveBreakPoint();
    void removePendingBreakPoint();
    void addBreakPointWhilePaused();
    void removeBreakPointForNextInstruction();

private:
    void evaluateJavaScript(const QString &script, const QString &fileName, int lineNumber = 1)
    {
        QMetaObject::invokeMethod(m_engine, "evaluate", Qt::QueuedConnection,
                                  Q_ARG(QString, script), Q_ARG(QString, fileName),
                                  Q_ARG(int, lineNumber));
        waitForSignal(m_engine, SIGNAL(evaluateFinished()), /*timeout*/0);
    }

    TestEngine *m_engine;
    QV4::ExecutionEngine *m_v4;
    TestAgent *m_debuggerAgent;
    QThread *m_javaScriptThread;
};

void tst_qv4debugger::init()
{
    m_javaScriptThread = new QThread;
    m_engine = new TestEngine;
    m_v4 = m_engine->v4Engine();
    m_v4->enableDebugger();
    m_engine->moveToThread(m_javaScriptThread);
    m_javaScriptThread->start();
    m_debuggerAgent = new TestAgent;
    m_debuggerAgent->addDebugger(m_v4->debugger);
}

void tst_qv4debugger::cleanup()
{
    m_javaScriptThread->exit();
    m_javaScriptThread->wait();
    delete m_engine;
    delete m_javaScriptThread;
    m_engine = 0;
    m_v4 = 0;
    QCOMPARE(m_debuggerAgent->debuggerCount(), 0);
    delete m_debuggerAgent;
    m_debuggerAgent = 0;
}

void tst_qv4debugger::breakAnywhere()
{
    QString script =
            "var i = 42;\n"
            "var j = i + 1\n"
            "var k = i\n";
    m_debuggerAgent->pauseAll();
    evaluateJavaScript(script, "testFile");
    QVERIFY(m_debuggerAgent->m_wasPaused);
}

void tst_qv4debugger::pendingBreakpoint()
{
    QString script =
            "var i = 42;\n"
            "var j = i + 1\n"
            "var k = i\n";
    m_debuggerAgent->addBreakPoint("testfile", 2);
    evaluateJavaScript(script, "testfile");
    QVERIFY(m_debuggerAgent->m_wasPaused);
    QCOMPARE(m_debuggerAgent->m_statesWhenPaused.count(), 1);
    QV4::Debugging::Debugger::ExecutionState state = m_debuggerAgent->m_statesWhenPaused.first();
    QCOMPARE(state.fileName, QString("testfile"));
    QCOMPARE(state.lineNumber, 2);
}

void tst_qv4debugger::liveBreakPoint()
{
    QString script =
            "var i = 42;\n"
            "var j = i + 1\n"
            "var k = i\n";
    m_debuggerAgent->m_breakPointsToAddWhenPaused << TestAgent::TestBreakPoint("liveBreakPoint", 3);
    m_debuggerAgent->pauseAll();
    evaluateJavaScript(script, "liveBreakPoint");
    QVERIFY(m_debuggerAgent->m_wasPaused);
    QCOMPARE(m_debuggerAgent->m_statesWhenPaused.count(), 2);
    QV4::Debugging::Debugger::ExecutionState state = m_debuggerAgent->m_statesWhenPaused.at(1);
    QCOMPARE(state.fileName, QString("liveBreakPoint"));
    QCOMPARE(state.lineNumber, 3);
}

void tst_qv4debugger::removePendingBreakPoint()
{
    QString script =
            "var i = 42;\n"
            "var j = i + 1\n"
            "var k = i\n";
    m_debuggerAgent->addBreakPoint("removePendingBreakPoint", 2);
    m_debuggerAgent->removeBreakPoint("removePendingBreakPoint", 2);
    evaluateJavaScript(script, "removePendingBreakPoint");
    QVERIFY(!m_debuggerAgent->m_wasPaused);
}

void tst_qv4debugger::addBreakPointWhilePaused()
{
    QString script =
            "var i = 42;\n"
            "var j = i + 1\n"
            "var k = i\n";
    m_debuggerAgent->addBreakPoint("addBreakPointWhilePaused", 1);
    m_debuggerAgent->m_breakPointsToAddWhenPaused << TestAgent::TestBreakPoint("addBreakPointWhilePaused", 2);
    evaluateJavaScript(script, "addBreakPointWhilePaused");
    QVERIFY(m_debuggerAgent->m_wasPaused);
    QCOMPARE(m_debuggerAgent->m_statesWhenPaused.count(), 2);

    QV4::Debugging::Debugger::ExecutionState state = m_debuggerAgent->m_statesWhenPaused.at(0);
    QCOMPARE(state.fileName, QString("addBreakPointWhilePaused"));
    QCOMPARE(state.lineNumber, 1);

    state = m_debuggerAgent->m_statesWhenPaused.at(1);
    QCOMPARE(state.fileName, QString("addBreakPointWhilePaused"));
    QCOMPARE(state.lineNumber, 2);
}

static QV4::ReturnedValue someCall(QV4::SimpleCallContext *ctx)
{
    ctx->engine->debugger->removeBreakPoint("removeBreakPointForNextInstruction", 2);
    return QV4::Encode::undefined();
}

void tst_qv4debugger::removeBreakPointForNextInstruction()
{
    QString script =
            "someCall();\n"
            "var i = 42;";

    QMetaObject::invokeMethod(m_engine, "injectFunction", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, "someCall"), Q_ARG(TestEngine::InjectedFunction, someCall));

    m_debuggerAgent->addBreakPoint("removeBreakPointForNextInstruction", 2);

    evaluateJavaScript(script, "removeBreakPointForNextInstruction");
    QVERIFY(!m_debuggerAgent->m_wasPaused);
}

QTEST_MAIN(tst_qv4debugger)

#include "tst_qv4debugger.moc"
