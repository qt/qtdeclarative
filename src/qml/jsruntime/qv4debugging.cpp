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
#include <iostream>

#include <algorithm>

using namespace QV4;
using namespace QV4::Debugging;

Debugger::Debugger(QV4::ExecutionEngine *engine)
    : _engine(engine)
    , m_agent(0)
    , m_state(Running)
    , m_pauseRequested(false)
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

    QV4::ExecutionContext *context = _engine->current;
    QV4::Function *function = 0;
    if (CallContext *callCtx = context->asCallContext())
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

void Debugger::aboutToThrow(const QV4::Value &value)
{
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
    foreach (QV4::CompiledData::CompilationUnit *unit, _engine->compilationUnits) {
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

static void realDumpValue(QV4::Value v, QV4::ExecutionContext *ctx, std::string prefix)
{
    using namespace QV4;
    using namespace std;
    cout << prefix << "tag: " << hex << v.tag << dec << endl << prefix << "\t-> ";
    switch (v.type()) {
    case Value::Undefined_Type: cout << "Undefined" << endl; return;
    case Value::Null_Type: cout << "Null" << endl; return;
    case Value::Boolean_Type: cout << "Boolean"; break;
    case Value::Integer_Type: cout << "Integer"; break;
    case Value::Object_Type: cout << "Object"; break;
    case Value::String_Type: cout << "String"; break;
    default: cout << "UNKNOWN" << endl; return;
    }
    cout << endl;

    if (v.isBoolean()) {
        cout << prefix << "\t-> " << (v.booleanValue() ? "TRUE" : "FALSE") << endl;
        return;
    }

    if (v.isInteger()) {
        cout << prefix << "\t-> " << v.integerValue() << endl;
        return;
    }

    if (v.isDouble()) {
        cout << prefix << "\t-> " << v.doubleValue() << endl;
        return;
    }

    if (v.isString()) {
        // maybe check something on the Managed object?
        cout << prefix << "\t-> @" << hex << v.stringValue() << endl;
        cout << prefix << "\t-> \"" << qPrintable(v.stringValue()->toQString()) << "\"" << endl;
        return;
    }

    Object *o = v.objectValue();
    if (!o)
        return;

    cout << prefix << "\t-> @" << hex << o << endl;
    cout << prefix << "object type: " << o->internalType() << endl << prefix << "\t-> ";
    switch (o->internalType()) {
    case QV4::Managed::Type_Invalid: cout << "Invalid"; break;
    case QV4::Managed::Type_String: cout << "String"; break;
    case QV4::Managed::Type_Object: cout << "Object"; break;
    case QV4::Managed::Type_ArrayObject: cout << "ArrayObject"; break;
    case QV4::Managed::Type_FunctionObject: cout << "FunctionObject"; break;
    case QV4::Managed::Type_BooleanObject: cout << "BooleanObject"; break;
    case QV4::Managed::Type_NumberObject: cout << "NumberObject"; break;
    case QV4::Managed::Type_StringObject: cout << "StringObject"; break;
    case QV4::Managed::Type_DateObject: cout << "DateObject"; break;
    case QV4::Managed::Type_RegExpObject: cout << "RegExpObject"; break;
    case QV4::Managed::Type_ErrorObject: cout << "ErrorObject"; break;
    case QV4::Managed::Type_ArgumentsObject: cout << "ArgumentsObject"; break;
    case QV4::Managed::Type_JSONObject: cout << "JSONObject"; break;
    case QV4::Managed::Type_MathObject: cout << "MathObject"; break;
    case QV4::Managed::Type_ForeachIteratorObject: cout << "ForeachIteratorObject"; break;
    default: cout << "UNKNOWN" << endl; return;
    }
    cout << endl;

    cout << prefix << "properties:" << endl;
    ForEachIteratorObject it(ctx, o);
    for (Value name = it.nextPropertyName(); !name.isNull(); name = it.nextPropertyName()) {
        cout << prefix << "\t\"" << qPrintable(name.stringValue()->toQString()) << "\"" << endl;
        PropertyAttributes attrs;
        Property *d = o->__getOwnProperty__(name.stringValue(), &attrs);
        Value pval = o->getValue(d, attrs);
        cout << prefix << "\tvalue:" << endl;
        realDumpValue(pval, ctx, prefix + "\t");
    }
}

void dumpValue(QV4::Value v, QV4::ExecutionContext *ctx)
{
    realDumpValue(v, ctx, std::string(""));
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
        for (int i = 0; i < function->compiledFunction->nLineNumberMappingEntries; ++i) {
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
