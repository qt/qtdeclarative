/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qv4debugging_p.h"
#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include "qv4function_p.h"
#include "qv4instr_moth_p.h"
#include "qv4runtime_p.h"
#include <iostream>

#include <algorithm>

using namespace QV4;
using namespace QV4::Debugging;

Debugger::Debugger(QV4::ExecutionEngine *engine)
    : m_engine(engine)
    , m_agent(0)
    , m_state(Running)
    , m_pauseRequested(false)
    , m_havePendingBreakPoints(false)
    , m_currentInstructionPointer(0)
{
    qMetaTypeId<Debugger*>();
}

Debugger::~Debugger()
{
    detachFromAgent();
}

void Debugger::attachToAgent(DebuggerAgent *agent)
{
    Q_ASSERT(!m_agent);
    m_agent = agent;
}

void Debugger::detachFromAgent()
{
    DebuggerAgent *agent = 0;
    {
        QMutexLocker locker(&m_lock);
        agent = m_agent;
        m_agent = 0;
    }
    if (agent)
        agent->removeDebugger(this);
}

void Debugger::pause()
{
    QMutexLocker locker(&m_lock);
    if (m_state == Paused)
        return;
    m_pauseRequested = true;
}

void Debugger::resume()
{
    QMutexLocker locker(&m_lock);
    Q_ASSERT(m_state == Paused);
    m_runningCondition.wakeAll();
}

void Debugger::addBreakPoint(const QString &fileName, int lineNumber)
{
    QMutexLocker locker(&m_lock);
    if (!m_pendingBreakPointsToRemove.remove(fileName, lineNumber))
        m_pendingBreakPointsToAdd.add(fileName, lineNumber);
    m_havePendingBreakPoints = !m_pendingBreakPointsToAdd.isEmpty() || !m_pendingBreakPointsToRemove.isEmpty();
}

void Debugger::removeBreakPoint(const QString &fileName, int lineNumber)
{
    QMutexLocker locker(&m_lock);
    if (!m_pendingBreakPointsToAdd.remove(fileName, lineNumber))
        m_pendingBreakPointsToRemove.add(fileName, lineNumber);
    m_havePendingBreakPoints = !m_pendingBreakPointsToAdd.isEmpty() || !m_pendingBreakPointsToRemove.isEmpty();
}

Debugger::ExecutionState Debugger::currentExecutionState(const uchar *code) const
{
    if (!code)
        code = m_currentInstructionPointer;
    // ### Locking
    ExecutionState state;

    QV4::ExecutionContext *context = m_engine->current;
    QV4::Function *function = 0;
    CallContext *callCtx = context->asCallContext();
    if (callCtx && callCtx->function)
        function = callCtx->function->function;
    else {
        Q_ASSERT(context->type == QV4::ExecutionContext::Type_GlobalContext);
        function = context->engine->globalCode;
    }

    state.function = function;
    state.fileName = function->sourceFile();

    qptrdiff relativeProgramCounter = code - function->codeData;
    state.lineNumber = function->lineNumberForProgramCounter(relativeProgramCounter);

    return state;
}

void Debugger::setPendingBreakpoints(Function *function)
{
    m_pendingBreakPointsToAddToFutureCode.applyToFunction(function, /*removeBreakPoints*/ false);
}

QVector<StackFrame> Debugger::stackTrace(int frameLimit) const
{
    return m_engine->stackTrace(frameLimit);
}

QList<Debugger::VarInfo> Debugger::retrieveFromValue(const ObjectRef o, const QStringList &path) const
{
    QList<Debugger::VarInfo> props;
    if (!o)
        return props;

    Scope scope(m_engine);
    ObjectIterator it(scope, o, ObjectIterator::EnumerableOnly);
    ScopedValue name(scope);
    ScopedValue val(scope);
    while (true) {
        Value v;
        name = it.nextPropertyNameAsString(&v);
        if (name->isNull())
            break;
        QString key = name->toQStringNoThrow();
        if (path.isEmpty()) {
            val = v;
            QVariant varValue;
            VarInfo::Type type;
            convert(val, &varValue, &type);
            props.append(VarInfo(key, varValue, type));
        } else if (path.first() == key) {
            QStringList pathTail = path;
            pathTail.pop_front();
            return retrieveFromValue(ScopedObject(scope, v), pathTail);
        }
    }

    return props;
}

void Debugger::convert(ValueRef v, QVariant *varValue, VarInfo::Type *type) const
{
    Q_ASSERT(varValue);
    Q_ASSERT(type);

    switch (v->type()) {
    case Value::Empty_Type:
        Q_ASSERT(!"empty Value encountered");
        break;
    case Value::Undefined_Type:
        *type = VarInfo::Undefined;
        varValue->setValue<int>(0);
        break;
    case Value::Null_Type:
        *type = VarInfo::Null;
        varValue->setValue<int>(0);
        break;
    case Value::Boolean_Type:
        *type = VarInfo::Bool;
        varValue->setValue<bool>(v->booleanValue());
        break;
    case Value::Managed_Type:
        if (v->isString()) {
            *type = VarInfo::String;
            varValue->setValue<QString>(v->stringValue()->toQString());
        } else {
            *type = VarInfo::Object;
            ExecutionContext *ctx = v->objectValue()->internalClass->engine->current;
            Scope scope(ctx);
            ScopedValue prim(scope, __qmljs_to_primitive(v, STRING_HINT));
            varValue->setValue<QString>(prim->toQString());
        }
        break;
    case Value::Integer_Type:
        *type = VarInfo::Number;
        varValue->setValue<double>((double)v->int_32);
        break;
    default: // double
        *type = VarInfo::Number;
        varValue->setValue<double>(v->doubleValue());
        break;
    }
}

static CallContext *findContext(ExecutionContext *ctxt, int frame)
{
    while (ctxt) {
        CallContext *cCtxt = ctxt->asCallContext();
        if (cCtxt && cCtxt->function) {
            if (frame < 1)
                return cCtxt;
            --frame;
        }
        ctxt = ctxt->parent;
    }

    return 0;
}

/// Retrieves all arguments from a context, or all properties in an object passed in an argument.
///
/// \arg frame specifies the frame number: 0 is top of stack, 1 is the parent of the current frame, etc.
/// \arg path when empty, retrieve all arguments in the specified frame. When not empty, find the
///      argument with the same name as the first element in the path (in the specified frame, of
///      course), and then use the rest of the path to walk nested objects. When the path is empty,
///      retrieve all properties in that object. If an intermediate non-object is specified by the
///      path, or non of the property names match, an empty list is returned.
QList<Debugger::VarInfo> Debugger::retrieveArgumentsFromContext(const QStringList &path, int frame)
{
    QList<VarInfo> args;

    if (state() != Paused)
        return args;

    if (frame < 0)
        return args;

    CallContext *ctxt = findContext(m_engine->current, frame);
    if (!ctxt)
        return args;

    Scope scope(m_engine);
    ScopedValue v(scope);
    for (unsigned i = 0, ei = ctxt->formalCount(); i != ei; ++i) {
        // value = ctxt->argument(i);
        String *name = ctxt->formals()[i];
        QString qName;
        if (name)
            qName = name->toQString();
        if (path.isEmpty()) {
            v = ctxt->argument(i);
            QVariant value;
            VarInfo::Type type;
            convert(v, &value, &type);
            args.append(VarInfo(qName, value, type));
        } else if (path.first() == qName) {
            ScopedObject o(scope, ctxt->argument(i));
            QStringList pathTail = path;
            pathTail.pop_front();
            return retrieveFromValue(o, pathTail);
        }
    }

    return args;
}

/// Same as \c retrieveArgumentsFromContext, but now for locals.
QList<Debugger::VarInfo> Debugger::retrieveLocalsFromContext(const QStringList &path, int frame)
{
    QList<VarInfo> args;

    if (state() != Paused)
        return args;

    if (frame < 0)
        return args;

    CallContext *sctxt = findContext(m_engine->current, frame);
    if (!sctxt || sctxt->type < ExecutionContext::Type_SimpleCallContext)
        return args;
    CallContext *ctxt = static_cast<CallContext *>(sctxt);

    Scope scope(m_engine);
    ScopedValue v(scope);
    for (unsigned i = 0, ei = ctxt->variableCount(); i != ei; ++i) {
        String *name = ctxt->variables()[i];
        QString qName;
        if (name)
            qName = name->toQString();
        if (path.isEmpty()) {
            v = ctxt->locals[i];
            QVariant value;
            VarInfo::Type type;
            convert(v, &value, &type);
            args.append(VarInfo(qName, value, type));
        } else if (path.first() == qName) {
            ScopedObject o(scope, ctxt->locals[i]);
            QStringList pathTail = path;
            pathTail.pop_front();
            return retrieveFromValue(o, pathTail);
        }
    }

    return args;
}

void Debugger::maybeBreakAtInstruction(const uchar *code, bool breakPointHit)
{
    QMutexLocker locker(&m_lock);
    m_currentInstructionPointer = code;

    // Do debugger internal work
    if (m_havePendingBreakPoints) {

        if (breakPointHit) {
            ExecutionState state = currentExecutionState();
            breakPointHit = !m_pendingBreakPointsToRemove.contains(state.fileName, state.lineNumber);
        }

        applyPendingBreakPoints();
    }

    // Serve debugging requests from the agent
    if (m_pauseRequested) {
        m_pauseRequested = false;
        pauseAndWait();
    } else if (breakPointHit)
        pauseAndWait();

    if (!m_pendingBreakPointsToAdd.isEmpty() || !m_pendingBreakPointsToRemove.isEmpty())
        applyPendingBreakPoints();
}

void Debugger::aboutToThrow(const QV4::ValueRef value)
{
    Q_UNUSED(value);

    qDebug() << "*** We are about to throw...";
}

void Debugger::pauseAndWait()
{
    m_state = Paused;
    QMetaObject::invokeMethod(m_agent, "debuggerPaused", Qt::QueuedConnection, Q_ARG(QV4::Debugging::Debugger*, this));
    m_runningCondition.wait(&m_lock);
    m_state = Running;
}

void Debugger::applyPendingBreakPoints()
{
    foreach (QV4::CompiledData::CompilationUnit *unit, m_engine->compilationUnits) {
        foreach (Function *function, unit->runtimeFunctions) {
            m_pendingBreakPointsToAdd.applyToFunction(function, /*removeBreakPoints*/false);
            m_pendingBreakPointsToRemove.applyToFunction(function, /*removeBreakPoints*/true);
        }
    }

    for (BreakPoints::ConstIterator it = m_pendingBreakPointsToAdd.constBegin(),
         end = m_pendingBreakPointsToAdd.constEnd(); it != end; ++it) {
        foreach (int lineNumber, it.value())
            m_pendingBreakPointsToAddToFutureCode.add(it.key(), lineNumber);
    }

    m_pendingBreakPointsToAdd.clear();
    m_pendingBreakPointsToRemove.clear();
    m_havePendingBreakPoints = false;
}

void DebuggerAgent::addDebugger(Debugger *debugger)
{
    Q_ASSERT(!m_debuggers.contains(debugger));
    m_debuggers << debugger;
    debugger->attachToAgent(this);
}

void DebuggerAgent::removeDebugger(Debugger *debugger)
{
    m_debuggers.removeAll(debugger);
    debugger->detachFromAgent();
}

void DebuggerAgent::pause(Debugger *debugger) const
{
    debugger->pause();
}

void DebuggerAgent::pauseAll() const
{
    foreach (Debugger *debugger, m_debuggers)
        pause(debugger);
}

void DebuggerAgent::resumeAll() const
{
    foreach (Debugger *debugger, m_debuggers)
        if (debugger->state() == Debugger::Paused)
            debugger->resume();
}

void DebuggerAgent::addBreakPoint(const QString &fileName, int lineNumber) const
{
    foreach (Debugger *debugger, m_debuggers)
        debugger->addBreakPoint(fileName, lineNumber);
}

void DebuggerAgent::removeBreakPoint(const QString &fileName, int lineNumber) const
{
    foreach (Debugger *debugger, m_debuggers)
        debugger->removeBreakPoint(fileName, lineNumber);
}

DebuggerAgent::~DebuggerAgent()
{
    Q_ASSERT(m_debuggers.isEmpty());
}

void Debugger::BreakPoints::add(const QString &fileName, int lineNumber)
{
    QList<int> &lines = (*this)[fileName];
    if (!lines.contains(lineNumber)) {
        lines.append(lineNumber);
        std::sort(lines.begin(), lines.end());
    }
}

bool Debugger::BreakPoints::remove(const QString &fileName, int lineNumber)
{
    Iterator breakPoints = find(fileName);
    if (breakPoints == constEnd())
        return false;
    return breakPoints->removeAll(lineNumber) > 0;
}

bool Debugger::BreakPoints::contains(const QString &fileName, int lineNumber) const
{
    ConstIterator breakPoints = find(fileName);
    if (breakPoints == constEnd())
        return false;
    return breakPoints->contains(lineNumber);
}

void Debugger::BreakPoints::applyToFunction(Function *function, bool removeBreakPoints)
{
    Iterator breakPointsForFile = find(function->sourceFile());
    if (breakPointsForFile == end())
        return;

    QList<int>::Iterator breakPoint = breakPointsForFile->begin();
    while (breakPoint != breakPointsForFile->end()) {
        bool breakPointFound = false;
        const quint32 *lineNumberMappings = function->compiledFunction->lineNumberMapping();
        for (quint32 i = 0; i < function->compiledFunction->nLineNumberMappingEntries; ++i) {
            const int codeOffset = lineNumberMappings[i * 2];
            const int lineNumber = lineNumberMappings[i * 2 + 1];
            if (lineNumber == *breakPoint) {
                uchar *codePtr = const_cast<uchar *>(function->codeData) + codeOffset;
                QQmlJS::Moth::Instr *instruction = reinterpret_cast<QQmlJS::Moth::Instr*>(codePtr);
                instruction->common.breakPoint = !removeBreakPoints;
                // Continue setting the next break point.
                breakPointFound = true;
                break;
            }
        }
        if (breakPointFound)
            breakPoint = breakPointsForFile->erase(breakPoint);
        else
            ++breakPoint;
    }

    if (breakPointsForFile->isEmpty())
        erase(breakPointsForFile);
}
