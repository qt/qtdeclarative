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

#include "qv4debugging_p.h"
#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include "qv4function_p.h"
#include "qv4instr_moth_p.h"
#include "qv4runtime_p.h"
#include "qv4script_p.h"
#include "qv4identifier_p.h"
#include "qv4string_p.h"
#include "qv4objectiterator_p.h"

#include <iostream>
#include <algorithm>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonValue>

QT_BEGIN_NAMESPACE

using namespace QV4;
using namespace QV4::Debugging;

V4Debugger::JavaScriptJob::JavaScriptJob(QV4::ExecutionEngine *engine, int frameNr,
                                       const QString &script)
    : engine(engine)
    , frameNr(frameNr)
    , script(script)
    , resultIsException(false)
{}

void V4Debugger::JavaScriptJob::run()
{
    Scope scope(engine);

    ExecutionContextSaver saver(scope);

    ExecutionContext *ctx = engine->currentContext;
    if (frameNr > 0) {
        for (int i = 0; i < frameNr; ++i) {
            ctx = engine->parentContext(ctx);
        }
        engine->pushContext(ctx);
    }

    QV4::Script script(ctx, this->script);
    script.strictMode = ctx->d()->strictMode;
    // In order for property lookups in QML to work, we need to disable fast v4 lookups. That
    // is a side-effect of inheritContext.
    script.inheritContext = true;
    script.parse();
    QV4::ScopedValue result(scope);
    if (!scope.engine->hasException)
        result = script.run();
    if (scope.engine->hasException) {
        result = scope.engine->catchException();
        resultIsException = true;
    }
    handleResult(result);
}

bool V4Debugger::JavaScriptJob::hasExeption() const
{
    return resultIsException;
}

class EvalJob: public V4Debugger::JavaScriptJob
{
    bool result;

public:
    EvalJob(QV4::ExecutionEngine *engine, const QString &script)
        : V4Debugger::JavaScriptJob(engine, /*frameNr*/-1, script)
        , result(false)
    {}

    virtual void handleResult(QV4::ScopedValue &result)
    {
        this->result = result->toBoolean();
    }

    bool resultAsBoolean() const
    {
        return result;
    }
};

V4Debugger::V4Debugger(QV4::ExecutionEngine *engine)
    : m_engine(engine)
    , m_state(Running)
    , m_stepping(NotStepping)
    , m_pauseRequested(false)
    , m_haveBreakPoints(false)
    , m_breakOnThrow(false)
    , m_returnedValue(engine, Primitive::undefinedValue())
    , m_gatherSources(0)
    , m_runningJob(0)
{
    qMetaTypeId<V4Debugger*>();
    qMetaTypeId<PauseReason>();
}

void V4Debugger::pause()
{
    QMutexLocker locker(&m_lock);
    if (m_state == Paused)
        return;
    m_pauseRequested = true;
}

void V4Debugger::resume(Speed speed)
{
    QMutexLocker locker(&m_lock);
    if (m_state != Paused)
        return;

    if (!m_returnedValue.isUndefined())
        m_returnedValue.set(m_engine, Encode::undefined());

    m_currentContext.set(m_engine, *m_engine->currentContext);
    m_stepping = speed;
    m_runningCondition.wakeAll();
}

void V4Debugger::addBreakPoint(const QString &fileName, int lineNumber, const QString &condition)
{
    QMutexLocker locker(&m_lock);
    m_breakPoints.insert(DebuggerBreakPoint(fileName.mid(fileName.lastIndexOf('/') + 1), lineNumber), condition);
    m_haveBreakPoints = true;
}

void V4Debugger::removeBreakPoint(const QString &fileName, int lineNumber)
{
    QMutexLocker locker(&m_lock);
    m_breakPoints.remove(DebuggerBreakPoint(fileName.mid(fileName.lastIndexOf('/') + 1), lineNumber));
    m_haveBreakPoints = !m_breakPoints.isEmpty();
}

void V4Debugger::setBreakOnThrow(bool onoff)
{
    QMutexLocker locker(&m_lock);

    m_breakOnThrow = onoff;
}

V4Debugger::ExecutionState V4Debugger::currentExecutionState() const
{
    ExecutionState state;
    state.fileName = getFunction()->sourceFile();
    state.lineNumber = engine()->current->lineNumber;

    return state;
}

QVector<StackFrame> V4Debugger::stackTrace(int frameLimit) const
{
    return m_engine->stackTrace(frameLimit);
}

void V4Debugger::maybeBreakAtInstruction()
{
    if (m_runningJob) // do not re-enter when we're doing a job for the debugger.
        return;

    QMutexLocker locker(&m_lock);

    if (m_gatherSources) {
        m_gatherSources->run();
        delete m_gatherSources;
        m_gatherSources = 0;
    }

    switch (m_stepping) {
    case StepOver:
        if (m_currentContext.asManaged()->d() != m_engine->current)
            break;
        // fall through
    case StepIn:
        pauseAndWait(Step);
        return;
    case StepOut:
    case NotStepping:
        break;
    }

    if (m_pauseRequested) { // Serve debugging requests from the agent
        m_pauseRequested = false;
        pauseAndWait(PauseRequest);
    } else if (m_haveBreakPoints) {
        if (Function *f = getFunction()) {
            const int lineNumber = engine()->current->lineNumber;
            if (reallyHitTheBreakPoint(f->sourceFile(), lineNumber))
                pauseAndWait(BreakPoint);
        }
    }
}

void V4Debugger::enteringFunction()
{
    if (m_runningJob)
        return;
    QMutexLocker locker(&m_lock);

    if (m_stepping == StepIn) {
        m_currentContext.set(m_engine, *m_engine->currentContext);
    }
}

void V4Debugger::leavingFunction(const ReturnedValue &retVal)
{
    if (m_runningJob)
        return;
    Q_UNUSED(retVal); // TODO

    QMutexLocker locker(&m_lock);

    if (m_stepping != NotStepping && m_currentContext.asManaged()->d() == m_engine->current) {
        m_currentContext.set(m_engine, *m_engine->parentContext(m_engine->currentContext));
        m_stepping = StepOver;
        m_returnedValue.set(m_engine, retVal);
    }
}

void V4Debugger::aboutToThrow()
{
    if (!m_breakOnThrow)
        return;

    if (m_runningJob) // do not re-enter when we're doing a job for the debugger.
        return;

    QMutexLocker locker(&m_lock);
    pauseAndWait(Throwing);
}

Function *V4Debugger::getFunction() const
{
    Scope scope(m_engine);
    ExecutionContext *context = m_engine->currentContext;
    ScopedFunctionObject function(scope, context->getFunctionObject());
    if (function)
        return function->function();
    else
        return context->d()->engine->globalCode;
}

void V4Debugger::pauseAndWait(PauseReason reason)
{
    if (m_runningJob)
        return;

    m_state = Paused;
    emit debuggerPaused(this, reason);

    while (true) {
        m_runningCondition.wait(&m_lock);
        if (m_runningJob) {
            m_runningJob->run();
            m_jobIsRunning.wakeAll();
        } else {
            break;
        }
    }

    m_state = Running;
}

bool V4Debugger::reallyHitTheBreakPoint(const QString &filename, int linenr)
{
    BreakPoints::iterator it = m_breakPoints.find(DebuggerBreakPoint(filename.mid(filename.lastIndexOf('/') + 1), linenr));
    if (it == m_breakPoints.end())
        return false;
    QString condition = it.value();
    if (condition.isEmpty())
        return true;

    Q_ASSERT(m_runningJob == 0);
    EvalJob evilJob(m_engine, condition);
    m_runningJob = &evilJob;
    m_runningJob->run();
    m_runningJob = 0;

    return evilJob.resultAsBoolean();
}

void V4Debugger::runInEngine(V4Debugger::Job *job)
{
    QMutexLocker locker(&m_lock);
    runInEngine_havingLock(job);
}

void V4Debugger::runInEngine_havingLock(V4Debugger::Job *job)
{
    Q_ASSERT(job);
    Q_ASSERT(m_runningJob == 0);

    m_runningJob = job;
    m_runningCondition.wakeAll();
    m_jobIsRunning.wait(&m_lock);
    m_runningJob = 0;
}

V4Debugger::Job::~Job()
{
}

QT_END_NAMESPACE
