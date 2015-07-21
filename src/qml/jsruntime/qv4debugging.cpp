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

namespace {
class JavaScriptJob: public Debugger::Job
{
    QV4::ExecutionEngine *engine;
    int frameNr;
    const QString &script;

public:
    JavaScriptJob(QV4::ExecutionEngine *engine, int frameNr, const QString &script)
        : engine(engine)
        , frameNr(frameNr)
        , script(script)
    {}

    void run()
    {
        Scope scope(engine);

        ExecutionContextSaver saver(scope, engine->currentContext());

        if (frameNr > 0) {
            Value *savedContexts = scope.alloc(frameNr);
            for (int i = 0; i < frameNr; ++i) {
                savedContexts[i] = engine->currentContext();
                engine->popContext();
            }
        }

        ScopedContext ctx(scope, engine->currentContext());
        QV4::Script script(ctx, this->script);
        script.strictMode = ctx->d()->strictMode;
        // In order for property lookups in QML to work, we need to disable fast v4 lookups. That
        // is a side-effect of inheritContext.
        script.inheritContext = true;
        script.parse();
        QV4::ScopedValue result(scope);
        if (!scope.engine->hasException)
            result = script.run();
        if (scope.engine->hasException)
            result = scope.engine->catchException();
        handleResult(result);
    }

protected:
    virtual void handleResult(QV4::ScopedValue &result) = 0;
};

class EvalJob: public JavaScriptJob
{
    bool result;

public:
    EvalJob(QV4::ExecutionEngine *engine, const QString &script)
        : JavaScriptJob(engine, /*frameNr*/-1, script)
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

class ExpressionEvalJob: public JavaScriptJob
{
    QV4::Debugging::DataCollector *collector;

public:
    ExpressionEvalJob(ExecutionEngine *engine, int frameNr, const QString &expression,
                      QV4::Debugging::DataCollector *collector)
        : JavaScriptJob(engine, frameNr, expression)
        , collector(collector)
    {
    }

    virtual void handleResult(QV4::ScopedValue &result)
    {
        collector->collect(result);
    }
};

class GatherSourcesJob: public Debugger::Job
{
    QV4::ExecutionEngine *engine;
    const int seq;

public:
    GatherSourcesJob(QV4::ExecutionEngine *engine, int seq)
        : engine(engine)
        , seq(seq)
    {}

    ~GatherSourcesJob() {}

    void run()
    {
        QStringList sources;

        foreach (QV4::CompiledData::CompilationUnit *unit, engine->compilationUnits) {
            QString fileName = unit->fileName();
            if (!fileName.isEmpty())
                sources.append(fileName);
        }

        Debugger *debugger = engine->debugger;
        emit debugger->sourcesCollected(debugger, sources, seq);
    }
};
}


DataCollector::DataCollector(QV4::ExecutionEngine *engine)
    : m_engine(engine), m_collectedRefs(Q_NULLPTR)
{
    values.set(engine, engine->newArrayObject());
}

DataCollector::~DataCollector()
{
}

void DataCollector::collect(const ScopedValue &value)
{
    if (m_collectedRefs)
        m_collectedRefs->append(addRef(value));
}

QJsonObject DataCollector::lookupRef(Ref ref)
{
    QJsonObject dict;
    if (lookupSpecialRef(ref, &dict))
        return dict;

    dict.insert(QStringLiteral("handle"), qint64(ref));

    QV4::Scope scope(engine());
    QV4::ScopedValue value(scope, getValue(ref));
    switch (value->type()) {
    case QV4::Value::Empty_Type:
        Q_ASSERT(!"empty Value encountered");
        break;
    case QV4::Value::Undefined_Type:
        dict.insert(QStringLiteral("type"), QStringLiteral("undefined"));
        break;
    case QV4::Value::Null_Type:
        dict.insert(QStringLiteral("type"), QStringLiteral("null"));
        break;
    case QV4::Value::Boolean_Type:
        dict.insert(QStringLiteral("type"), QStringLiteral("boolean"));
        dict.insert(QStringLiteral("value"), value->booleanValue() ? QStringLiteral("true")
                                                                   : QStringLiteral("false"));
        break;
    case QV4::Value::Managed_Type:
        if (QV4::String *s = value->as<QV4::String>()) {
            dict.insert(QStringLiteral("type"), QStringLiteral("string"));
            dict.insert(QStringLiteral("value"), s->toQString());
        } else if (QV4::Object *o = value->as<QV4::Object>()) {
            dict.insert(QStringLiteral("type"), QStringLiteral("object"));
            dict.insert(QStringLiteral("properties"), collectProperties(o));
        } else {
            Q_UNREACHABLE();
        }
        break;
    case QV4::Value::Integer_Type:
        dict.insert(QStringLiteral("type"), QStringLiteral("number"));
        dict.insert(QStringLiteral("value"), value->integerValue());
        break;
    default: // double
        dict.insert(QStringLiteral("type"), QStringLiteral("number"));
        dict.insert(QStringLiteral("value"), value->doubleValue());
        break;
    }

    return dict;
}

DataCollector::Ref DataCollector::addFunctionRef(const QString &functionName)
{
    Ref ref = addRef(QV4::Primitive::emptyValue(), false);

    QJsonObject dict;
    dict.insert(QStringLiteral("handle"), qint64(ref));
    dict.insert(QStringLiteral("type"), QStringLiteral("function"));
    dict.insert(QStringLiteral("className"), QStringLiteral("Function"));
    dict.insert(QStringLiteral("name"), functionName);
    specialRefs.insert(ref, dict);

    return ref;
}

DataCollector::Ref DataCollector::addScriptRef(const QString &scriptName)
{
    Ref ref = addRef(QV4::Primitive::emptyValue(), false);

    QJsonObject dict;
    dict.insert(QStringLiteral("handle"), qint64(ref));
    dict.insert(QStringLiteral("type"), QStringLiteral("script"));
    dict.insert(QStringLiteral("name"), scriptName);
    specialRefs.insert(ref, dict);

    return ref;
}

void DataCollector::collectScope(QJsonObject *dict, Debugger *debugger, int frameNr, int scopeNr)
{
    QStringList names;

    Refs refs;
    {
        RefHolder holder(this, &refs);
        debugger->collectArgumentsInContext(this, &names, frameNr, scopeNr);
        debugger->collectLocalsInContext(this, &names, frameNr, scopeNr);
    }

    QV4::Scope scope(engine());
    QV4::ScopedObject scopeObject(scope, engine()->newObject());

    Q_ASSERT(names.size() == refs.size());
    for (int i = 0, ei = refs.size(); i != ei; ++i)
        scopeObject->put(engine(), names.at(i),
                         QV4::Value::fromReturnedValue(getValue(refs.at(i))));

    Ref scopeObjectRef = addRef(scopeObject);
    dict->insert(QStringLiteral("ref"), qint64(scopeObjectRef));
    if (m_collectedRefs)
        m_collectedRefs->append(scopeObjectRef);
}

DataCollector::Ref DataCollector::addRef(Value value, bool deduplicate)
{
    class ExceptionStateSaver
    {
        quint32 *hasExceptionLoc;
        quint32 hadException;

    public:
        ExceptionStateSaver(QV4::ExecutionEngine *engine)
            : hasExceptionLoc(&engine->hasException)
            , hadException(false)
        { std::swap(*hasExceptionLoc, hadException); }

        ~ExceptionStateSaver()
        { std::swap(*hasExceptionLoc, hadException); }
    };

    // if we wouldn't do this, the putIndexed won't work.
    ExceptionStateSaver resetExceptionState(engine());
    QV4::Scope scope(engine());
    QV4::ScopedObject array(scope, values.value());
    if (deduplicate) {
        for (Ref i = 0; i < array->getLength(); ++i) {
            if (array->getIndexed(i) == value.rawValue())
                return i;
        }
    }
    Ref ref = array->getLength();
    array->putIndexed(ref, value);
    Q_ASSERT(array->getLength() - 1 == ref);
    return ref;
}

ReturnedValue DataCollector::getValue(Ref ref)
{
    QV4::Scope scope(engine());
    QV4::ScopedObject array(scope, values.value());
    Q_ASSERT(ref < array->getLength());
    return array->getIndexed(ref, Q_NULLPTR);
}

bool DataCollector::lookupSpecialRef(Ref ref, QJsonObject *dict)
{
    SpecialRefs::const_iterator it = specialRefs.find(ref);
    if (it == specialRefs.end())
        return false;

    *dict = it.value();
    return true;
}

QJsonArray DataCollector::collectProperties(Object *object)
{
    QJsonArray res;

    QV4::Scope scope(engine());
    QV4::ObjectIterator it(scope, object, QV4::ObjectIterator::EnumerableOnly);
    QV4::ScopedValue name(scope);
    QV4::ScopedValue value(scope);
    while (true) {
        QV4::Value v;
        name = it.nextPropertyNameAsString(&v);
        if (name->isNull())
            break;
        QString key = name->toQStringNoThrow();
        value = v;
        res.append(collectAsJson(key, value));
    }

    return res;
}

QJsonObject DataCollector::collectAsJson(const QString &name, const ScopedValue &value)
{
    QJsonObject dict;
    if (!name.isNull())
        dict.insert(QStringLiteral("name"), name);
    Ref ref = addRef(value);
    dict.insert(QStringLiteral("ref"), qint64(ref));
    if (m_collectedRefs)
        m_collectedRefs->append(ref);

    // TODO: enable this when creator can handle it.
    if (false) {
        if (value->isManaged() && !value->isString()) {
            QV4::Scope scope(engine());
            QV4::ScopedObject obj(scope, value->as<QV4::Object>());
            dict.insert(QStringLiteral("propertycount"), qint64(obj->getLength()));
        }
    }

    return dict;
}

Debugger::Debugger(QV4::ExecutionEngine *engine)
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
    qMetaTypeId<Debugger*>();
    qMetaTypeId<PauseReason>();
}

void Debugger::gatherSources(int requestSequenceNr)
{
    QMutexLocker locker(&m_lock);

    m_gatherSources = new GatherSourcesJob(m_engine, requestSequenceNr);
    if (m_state == Paused) {
        runInEngine_havingLock(m_gatherSources);
        delete m_gatherSources;
        m_gatherSources = 0;
    }
}

void Debugger::pause()
{
    QMutexLocker locker(&m_lock);
    if (m_state == Paused)
        return;
    m_pauseRequested = true;
}

void Debugger::resume(Speed speed)
{
    QMutexLocker locker(&m_lock);
    if (m_state != Paused)
        return;

    if (!m_returnedValue.isUndefined())
        m_returnedValue.set(m_engine, Encode::undefined());

    m_currentContext.set(m_engine, m_engine->currentContext());
    m_stepping = speed;
    m_runningCondition.wakeAll();
}

void Debugger::addBreakPoint(const QString &fileName, int lineNumber, const QString &condition)
{
    QMutexLocker locker(&m_lock);
    m_breakPoints.insert(DebuggerBreakPoint(fileName.mid(fileName.lastIndexOf('/') + 1), lineNumber), condition);
    m_haveBreakPoints = true;
}

void Debugger::removeBreakPoint(const QString &fileName, int lineNumber)
{
    QMutexLocker locker(&m_lock);
    m_breakPoints.remove(DebuggerBreakPoint(fileName.mid(fileName.lastIndexOf('/') + 1), lineNumber));
    m_haveBreakPoints = !m_breakPoints.isEmpty();
}

void Debugger::setBreakOnThrow(bool onoff)
{
    QMutexLocker locker(&m_lock);

    m_breakOnThrow = onoff;
}

Debugger::ExecutionState Debugger::currentExecutionState() const
{
    ExecutionState state;
    state.fileName = getFunction()->sourceFile();
    state.lineNumber = engine()->currentContext()->lineNumber;

    return state;
}

QVector<StackFrame> Debugger::stackTrace(int frameLimit) const
{
    return m_engine->stackTrace(frameLimit);
}

static inline Heap::CallContext *findContext(Heap::ExecutionContext *ctxt, int frame)
{
    if (!ctxt)
        return 0;

    Scope scope(ctxt->engine);
    ScopedContext ctx(scope, ctxt);
    while (ctx) {
        CallContext *cCtxt = ctx->asCallContext();
        if (cCtxt && cCtxt->d()->function) {
            if (frame < 1)
                return cCtxt->d();
            --frame;
        }
        ctx = ctx->d()->parent;
    }

    return 0;
}

static inline Heap::CallContext *findScope(Heap::ExecutionContext *ctxt, int scope)
{
    if (!ctxt)
        return 0;

    Scope s(ctxt->engine);
    ScopedContext ctx(s, ctxt);
    for (; scope > 0 && ctx; --scope)
        ctx = ctx->d()->outer;

    return (ctx && ctx->d()) ? ctx->asCallContext()->d() : 0;
}

void Debugger::collectArgumentsInContext(DataCollector *collector, QStringList *names, int frameNr,
                                         int scopeNr)
{
    if (state() != Paused)
        return;

    class ArgumentCollectJob: public Job
    {
        QV4::ExecutionEngine *engine;
        QV4::Debugging::DataCollector *collector;
        QStringList *names;
        int frameNr;
        int scopeNr;

    public:
        ArgumentCollectJob(QV4::ExecutionEngine *engine, DataCollector *collector,
                           QStringList *names, int frameNr, int scopeNr)
            : engine(engine)
            , collector(collector)
            , names(names)
            , frameNr(frameNr)
            , scopeNr(scopeNr)
        {}

        ~ArgumentCollectJob() {}

        void run()
        {
            if (frameNr < 0)
                return;

            Scope scope(engine);
            Scoped<CallContext> ctxt(scope, findScope(findContext(engine->currentContext(), frameNr), scopeNr));
            if (!ctxt)
                return;

            ScopedValue v(scope);
            int nFormals = ctxt->formalCount();
            for (unsigned i = 0, ei = nFormals; i != ei; ++i) {
                QString qName;
                if (Identifier *name = ctxt->formals()[nFormals - i - 1])
                    qName = name->string;
                names->append(qName);
                v = ctxt->argument(i);
                collector->collect(v);
            }
        }
    };

    ArgumentCollectJob job(m_engine, collector, names, frameNr, scopeNr);
    runInEngine(&job);
}

/// Same as \c retrieveArgumentsFromContext, but now for locals.
void Debugger::collectLocalsInContext(DataCollector *collector, QStringList *names, int frameNr,
                                      int scopeNr)
{
    Q_ASSERT(collector);
    Q_ASSERT(names);

    if (state() != Paused)
        return;

    class LocalCollectJob: public Job
    {
        QV4::ExecutionEngine *engine;
        DataCollector *collector;
        QStringList *names;
        int frameNr;
        int scopeNr;

    public:
        LocalCollectJob(QV4::ExecutionEngine *engine, DataCollector *collector, QStringList *names, int frameNr, int scopeNr)
            : engine(engine)
            , collector(collector)
            , names(names)
            , frameNr(frameNr)
            , scopeNr(scopeNr)
        {}

        void run()
        {
            if (frameNr < 0)
                return;

            Scope scope(engine);
            Scoped<CallContext> ctxt(scope, findScope(findContext(engine->currentContext(), frameNr), scopeNr));
            if (!ctxt)
                return;

            ScopedValue v(scope);
            for (unsigned i = 0, ei = ctxt->variableCount(); i != ei; ++i) {
                QString qName;
                if (Identifier *name = ctxt->variables()[i])
                    qName = name->string;
                names->append(qName);
                v = ctxt->d()->locals[i];
                collector->collect(v);
            }
        }
    };

    LocalCollectJob job(m_engine, collector, names, frameNr, scopeNr);
    runInEngine(&job);
}

bool Debugger::collectThisInContext(DataCollector *collector, int frame)
{
    if (state() != Paused)
        return false;

    class ThisCollectJob: public Job
    {
        QV4::ExecutionEngine *engine;
        DataCollector *collector;
        int frameNr;
        bool *foundThis;

    public:
        ThisCollectJob(QV4::ExecutionEngine *engine, DataCollector *collector, int frameNr,
                       bool *foundThis)
            : engine(engine)
            , collector(collector)
            , frameNr(frameNr)
            , foundThis(foundThis)
        {}

        void run()
        {
            *foundThis = myRun();
        }

        bool myRun()
        {
            Scope scope(engine);
            ScopedContext ctxt(scope, findContext(engine->currentContext(), frameNr));
            while (ctxt) {
                if (CallContext *cCtxt = ctxt->asCallContext())
                    if (cCtxt->d()->activation)
                        break;
                ctxt = ctxt->d()->outer;
            }

            if (!ctxt)
                return false;

            ScopedValue o(scope, ctxt->asCallContext()->d()->activation);
            collector->collect(o);
            return true;
        }
    };

    bool foundThis = false;
    ThisCollectJob job(m_engine, collector, frame, &foundThis);
    runInEngine(&job);
    return foundThis;
}

bool Debugger::collectThrownValue(DataCollector *collector)
{
    if (state() != Paused || !m_engine->hasException)
        return false;

    class ExceptionCollectJob: public Job
    {
        QV4::ExecutionEngine *engine;
        DataCollector *collector;

    public:
        ExceptionCollectJob(QV4::ExecutionEngine *engine, DataCollector *collector)
            : engine(engine)
            , collector(collector)
        {}

        void run()
        {
            Scope scope(engine);
            ScopedValue v(scope, *engine->exceptionValue);
            collector->collect(v);
        }
    };

    ExceptionCollectJob job(m_engine, collector);
    runInEngine(&job);
    return true;
}

QVector<Heap::ExecutionContext::ContextType> Debugger::getScopeTypes(int frame) const
{
    QVector<Heap::ExecutionContext::ContextType> types;

    if (state() != Paused)
        return types;

    Scope scope(m_engine);
    Scoped<CallContext> sctxt(scope, findContext(m_engine->currentContext(), frame));
    if (!sctxt || sctxt->d()->type < Heap::ExecutionContext::Type_QmlContext)
        return types;

    ScopedContext it(scope, sctxt->d());
    for (; it; it = it->d()->outer)
        types.append(it->d()->type);

    return types;
}


void Debugger::evaluateExpression(int frameNr, const QString &expression,
                                  DataCollector *resultsCollector)
{
    Q_ASSERT(state() == Paused);

    Q_ASSERT(m_runningJob == 0);
    ExpressionEvalJob job(m_engine, frameNr, expression, resultsCollector);
    runInEngine(&job);
}

void Debugger::maybeBreakAtInstruction()
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
        if (m_currentContext.asManaged()->d() != m_engine->currentContext())
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
            const int lineNumber = engine()->currentContext()->lineNumber;
            if (reallyHitTheBreakPoint(f->sourceFile(), lineNumber))
                pauseAndWait(BreakPoint);
        }
    }
}

void Debugger::enteringFunction()
{
    if (m_runningJob)
        return;
    QMutexLocker locker(&m_lock);

    if (m_stepping == StepIn) {
        m_currentContext.set(m_engine, m_engine->currentContext());
    }
}

void Debugger::leavingFunction(const ReturnedValue &retVal)
{
    if (m_runningJob)
        return;
    Q_UNUSED(retVal); // TODO

    QMutexLocker locker(&m_lock);

    if (m_stepping != NotStepping && m_currentContext.asManaged()->d() == m_engine->currentContext()) {
        m_currentContext.set(m_engine, m_engine->currentContext()->parent);
        m_stepping = StepOver;
        m_returnedValue.set(m_engine, retVal);
    }
}

void Debugger::aboutToThrow()
{
    if (!m_breakOnThrow)
        return;

    if (m_runningJob) // do not re-enter when we're doing a job for the debugger.
        return;

    QMutexLocker locker(&m_lock);
    pauseAndWait(Throwing);
}

Function *Debugger::getFunction() const
{
    Scope scope(m_engine);
    ScopedContext context(scope, m_engine->currentContext());
    ScopedFunctionObject function(scope, context->getFunctionObject());
    if (function)
        return function->function();
    else
        return context->d()->engine->globalCode;
}

void Debugger::pauseAndWait(PauseReason reason)
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

bool Debugger::reallyHitTheBreakPoint(const QString &filename, int linenr)
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

void Debugger::runInEngine(Debugger::Job *job)
{
    QMutexLocker locker(&m_lock);
    runInEngine_havingLock(job);
}

void Debugger::runInEngine_havingLock(Debugger::Job *job)
{
    Q_ASSERT(job);
    Q_ASSERT(m_runningJob == 0);

    m_runningJob = job;
    m_runningCondition.wakeAll();
    m_jobIsRunning.wait(&m_lock);
    m_runningJob = 0;
}

Debugger::Job::~Job()
{
}

QT_END_NAMESPACE
