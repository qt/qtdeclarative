/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4vme_moth_p.h"
#include "qv4instr_moth_p.h"

#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>

#include <private/qv4value_p.h>
#include <private/qv4debugging_p.h>
#include <private/qv4function_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4math_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4lookup_p.h>
#include <private/qv4string_p.h>
#include <iostream>

#include "qv4alloca_p.h"


extern "C" {

// This is the interface to Qt Creator's (new) QML debugger.

/*! \internal
    \since 5.5

    This function is called uncondionally from VME::run().

    An attached debugger can set a breakpoint here to
    intercept calls to VME::run().
 */

Q_QML_EXPORT void qt_v4ResolvePendingBreakpointsHook()
{
}

/*! \internal
    \since 5.5

    This function is called when a QML interpreter breakpoint
    is hit.

    An attached debugger can set a breakpoint here.
*/
Q_QML_EXPORT void qt_v4TriggeredBreakpointHook()
{
}

/*! \internal
    \since 5.5

    The main entry point into "Native Mixed" Debugging.

    Commands are passed as UTF-8 encoded JSON data.
    The data has two compulsory fields:
    \list
    \li \c version: Version of the protocol (currently 1)
    \li \c command: Name of the command
    \endlist

    Depending on \c command, more fields can be present.

    Error is indicated by negative return values,
    success by non-negative return values.

    \c protocolVersion:
    Returns version of implemented protocol.

    \c insertBreakpoint:
    Sets a breakpoint on a given file and line.
    \list
    \li \c fullName: Name of the QML/JS file
    \li \c lineNumber: Line number in the file
    \li \c condition: Breakpoint condition
    \endlist
    Returns a unique positive number as handle.

    \c removeBreakpoint:
    Removes a breakpoint from a given file and line.
    \list
    \li \c fullName: Name of the QML/JS file
    \li \c lineNumber: Line number in the file
    \li \c condition: Breakpoint condition
    \endlist
    Returns zero on success, a negative number on failure.

    \c prepareStep:
    Puts the interpreter in stepping mode.
    Returns zero.

*/
Q_QML_EXPORT int qt_v4DebuggerHook(const char *json);


} // extern "C"

#ifndef QT_NO_QML_DEBUGGER
static int qt_v4BreakpointCount = 0;
static bool qt_v4IsDebugging = true;
static bool qt_v4IsStepping = false;

class Breakpoint
{
public:
    Breakpoint() : bpNumber(0), lineNumber(-1) {}

    bool matches(const QString &file, int line) const
    {
        return fullName == file && lineNumber == line;
    }

    int bpNumber;
    int lineNumber;
    QString fullName;      // e.g. /opt/project/main.qml
    QString engineName;    // e.g. qrc:/main.qml
    QString condition;     // optional
};

static QVector<Breakpoint> qt_v4Breakpoints;
static Breakpoint qt_v4LastStop;

static QV4::Function *qt_v4ExtractFunction(QV4::ExecutionContext *context)
{
    if (QV4::Function *function = context->getFunction())
        return function;
    else
        return context->engine()->globalCode;
}

static void qt_v4TriggerBreakpoint(const Breakpoint &bp, QV4::Function *function)
{
    qt_v4LastStop = bp;

    // Set up some auxiliary data for informational purpose.
    // This is not part of the protocol.
    QV4::Heap::String *functionName = function->name();
    QByteArray functionNameUtf8;
    if (functionName)
        functionNameUtf8 = functionName->toQString().toUtf8();

    qt_v4TriggeredBreakpointHook(); // Trigger Breakpoint.
}

int qt_v4DebuggerHook(const char *json)
{
    const int ProtocolVersion = 1;

    enum {
        Success = 0,
        WrongProtocol,
        NoSuchCommand,
        NoSuchBreakpoint
    };

    QJsonDocument doc = QJsonDocument::fromJson(json);
    QJsonObject ob = doc.object();
    QByteArray command = ob.value(QLatin1String("command")).toString().toUtf8();

    if (command == "protocolVersion") {
        return ProtocolVersion; // Version number.
    }

    int version = ob.value(QLatin1Literal("version")).toString().toInt();
    if (version != ProtocolVersion) {
        return -WrongProtocol;
    }

    if (command == "insertBreakpoint") {
        Breakpoint bp;
        bp.bpNumber = ++qt_v4BreakpointCount;
        bp.lineNumber = ob.value(QLatin1String("lineNumber")).toString().toInt();
        bp.engineName = ob.value(QLatin1String("engineName")).toString();
        bp.fullName = ob.value(QLatin1String("fullName")).toString();
        bp.condition = ob.value(QLatin1String("condition")).toString();
        qt_v4Breakpoints.append(bp);
        return bp.bpNumber;
    }

    if (command == "removeBreakpoint") {
        int lineNumber = ob.value(QLatin1String("lineNumber")).toString().toInt();
        QString fullName = ob.value(QLatin1String("fullName")).toString();
        if (qt_v4Breakpoints.last().matches(fullName, lineNumber)) {
            qt_v4Breakpoints.removeLast();
            return Success;
        }
        for (int i = 0; i + 1 < qt_v4Breakpoints.size(); ++i) {
            if (qt_v4Breakpoints.at(i).matches(fullName, lineNumber)) {
                qt_v4Breakpoints[i] = qt_v4Breakpoints.takeLast();
                return Success; // Ok.
            }
        }
        return -NoSuchBreakpoint; // Failure
    }

    if (command == "prepareStep") {
        qt_v4IsStepping = true;
        return Success; // Ok.
    }


    return -NoSuchCommand; // Failure.
}

Q_NEVER_INLINE static void qt_v4CheckForBreak(QV4::ExecutionContext *context)
{
    if (!qt_v4IsStepping && !qt_v4Breakpoints.size())
        return;

    const int lineNumber = context->d()->lineNumber;
    QV4::Function *function = qt_v4ExtractFunction(context);
    QString engineName = function->sourceFile();

    if (engineName.isEmpty())
        return;

    if (qt_v4IsStepping) {
        if (qt_v4LastStop.lineNumber != lineNumber
                || qt_v4LastStop.engineName != engineName) {
            qt_v4IsStepping = false;
            Breakpoint bp;
            bp.bpNumber = 0;
            bp.lineNumber = lineNumber;
            bp.engineName = engineName;
            qt_v4TriggerBreakpoint(bp, function);
            return;
        }
    }

    for (int i = qt_v4Breakpoints.size(); --i >= 0; ) {
        const Breakpoint &bp = qt_v4Breakpoints.at(i);
        if (bp.lineNumber != lineNumber)
            continue;
        if (bp.engineName != engineName)
            continue;

        qt_v4TriggerBreakpoint(bp, function);
    }
}

Q_NEVER_INLINE static void debug_slowPath(const QV4::Moth::Instr::instr_debug &instr,
                                          QV4::ExecutionEngine *engine)
{
    engine->current->lineNumber = instr.lineNumber;
    QV4::Debugging::Debugger *debugger = engine->debugger();
    if (debugger && debugger->pauseAtNextOpportunity())
        debugger->maybeBreakAtInstruction();
    if (qt_v4IsDebugging)
        qt_v4CheckForBreak(engine->currentContext);
}

#endif // QT_NO_QML_DEBUGGER
// End of debugger interface

using namespace QV4;
using namespace QV4::Moth;

#define MOTH_BEGIN_INSTR_COMMON(I) { \
    const InstrMeta<int(Instr::I)>::DataType &instr = InstrMeta<int(Instr::I)>::data(*genericInstr); \
    code += InstrMeta<int(Instr::I)>::Size; \
    Q_UNUSED(instr);

#ifdef MOTH_THREADED_INTERPRETER

#  define MOTH_BEGIN_INSTR(I) op_##I: \
    MOTH_BEGIN_INSTR_COMMON(I)

#  define MOTH_END_INSTR(I) } \
    genericInstr = reinterpret_cast<const Instr *>(code); \
    goto *jumpTable[genericInstr->common.instructionType]; \

#else

#  define MOTH_BEGIN_INSTR(I) \
    case Instr::I: \
    MOTH_BEGIN_INSTR_COMMON(I)

#  define MOTH_END_INSTR(I) } \
    continue;

#endif

// ### add write barrier here
#define STOREVALUE(param, value) { \
    QV4::ReturnedValue tmp = (value); \
    if (engine->hasException) \
        goto catchException; \
    if (Q_LIKELY(!engine->writeBarrierActive || !scopes[param.scope].base)) { \
        VALUE(param) = tmp; \
    } else { \
        QV4::WriteBarrier::write(engine, scopes[param.scope].base, VALUEPTR(param), QV4::Value::fromReturnedValue(tmp)); \
    } \
}

#define TEMP_VALUE(temp) stack[temp.index]
#define STORE_TEMP_VALUE(temp, value) { \
    QV4::ReturnedValue tmp = (value); \
    if (engine->hasException) \
        goto catchException; \
    stack[temp.index] = tmp; \
}

#define STORE_ACCUMULATOR(value) { \
    accumulator = (value); \
    if (engine->hasException) \
        goto catchException; \
}

// qv4scopedvalue_p.h also defines a CHECK_EXCEPTION macro
#ifdef CHECK_EXCEPTION
#undef CHECK_EXCEPTION
#endif
#define CHECK_EXCEPTION \
    if (engine->hasException) \
        goto catchException

static inline QV4::Heap::ExecutionContext *getScope(QV4::Heap::ExecutionContext *functionScope,
                                                    int level)
{
    QV4::Heap::ExecutionContext *scope = functionScope;
    while (level > 0) {
        --level;
        scope = scope->outer;
    }
    Q_ASSERT(scope);
    return scope;
}

static inline void storeLocal(ExecutionEngine *engine, QV4::Heap::ExecutionContext *scope,
                              QV4::Value *slot, QV4::Value value)
{
    Q_ASSERT(scope->type == QV4::Heap::ExecutionContext::Type_CallContext);
    if (Q_UNLIKELY(engine->writeBarrierActive))
        QV4::WriteBarrier::write(engine, scope, slot, value);
    else
        *slot = value;
}

static inline void storeArg(ExecutionEngine *engine, QV4::Heap::ExecutionContext *scope,
                            QV4::Value *slot, QV4::Value value)
{
    Q_ASSERT(scope->type == QV4::Heap::ExecutionContext::Type_SimpleCallContext
             || scope->type == QV4::Heap::ExecutionContext::Type_CallContext);
    if (Q_UNLIKELY(scope->type == QV4::Heap::ExecutionContext::Type_CallContext
                   && engine->writeBarrierActive))
        QV4::WriteBarrier::write(engine, scope, slot, value);
    else
        *slot = value;
}

QV4::ReturnedValue VME::exec(ExecutionEngine *engine, const uchar *code)
{
#ifdef DO_TRACE_INSTR
    qDebug("Starting VME with context=%p and code=%p", context, code);
#endif // DO_TRACE_INSTR

    qt_v4ResolvePendingBreakpointsHook();

#ifdef MOTH_THREADED_INTERPRETER
#define MOTH_INSTR_ADDR(I, FMT) &&op_##I,
    static void *jumpTable[] = {
        FOR_EACH_MOTH_INSTR(MOTH_INSTR_ADDR)
    };
#undef MOTH_INSTR_ADDR
#endif

    // Arguments/locals are used a *lot*, and pre-fetching them removes a whole bunch of triple
    // (or quadruple) indirect loads.
    QV4::Value *arguments = nullptr;
    QV4::Value *locals = nullptr;
    QV4::Value *argumentsScope1 = nullptr;
    QV4::Value *localsScope1 = nullptr;
    QV4::Heap::ExecutionContext *functionScope = nullptr;
    QV4::Heap::ExecutionContext *functionScope1 = nullptr;
    { // setup args/locals/etc
        functionScope = engine->current;
        arguments = functionScope->callData->args;
        if (functionScope->type == QV4::Heap::ExecutionContext::Type_CallContext)
            locals = static_cast<QV4::Heap::CallContext *>(functionScope)->locals.values;

        functionScope1 = functionScope->outer;
        if (functionScope1) {
            argumentsScope1 = functionScope1->callData->args;
            if (functionScope1->type == QV4::Heap::ExecutionContext::Type_CallContext)
                localsScope1 = static_cast<QV4::Heap::CallContext *>(functionScope1)->locals.values;
        }
    }

    QV4::Value accumulator = Primitive::undefinedValue();
    QV4::Value *stack = 0;
    unsigned stackSize = 0;

    const uchar *exceptionHandler = 0;

    QV4::Scope scope(engine);
    engine->current->lineNumber = -1;

    if (QV4::Debugging::Debugger *debugger = engine->debugger())
        debugger->enteringFunction();

    for (;;) {
        const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
#ifdef MOTH_THREADED_INTERPRETER
        goto *jumpTable[genericInstr->common.instructionType];
#else
        switch (genericInstr->common.instructionType) {
#endif

    MOTH_BEGIN_INSTR(LoadConst)
        accumulator = static_cast<CompiledData::CompilationUnit*>(engine->current->compilationUnit)->constants[instr.index];
    MOTH_END_INSTR(LoadConst)

    MOTH_BEGIN_INSTR(MoveConst)
        TEMP_VALUE(instr.destTemp) = static_cast<CompiledData::CompilationUnit*>(engine->current->compilationUnit)->constants[instr.constIndex];
    MOTH_END_INSTR(MoveConst)

    MOTH_BEGIN_INSTR(LoadReg)
        accumulator = TEMP_VALUE(instr.reg);
    MOTH_END_INSTR(LoadReg)

    MOTH_BEGIN_INSTR(StoreReg)
        TEMP_VALUE(instr.reg) = accumulator;
    MOTH_END_INSTR(StoreReg)

    MOTH_BEGIN_INSTR(MoveReg)
        TEMP_VALUE(instr.destReg) = TEMP_VALUE(instr.srcReg);
    MOTH_END_INSTR(MoveReg)

    MOTH_BEGIN_INSTR(LoadLocal)
        accumulator = locals[instr.index];
    MOTH_END_INSTR(LoadLocal)

    MOTH_BEGIN_INSTR(StoreLocal)
        CHECK_EXCEPTION;
        storeLocal(engine, functionScope, locals + instr.index, accumulator);
    MOTH_END_INSTR(StoreLocal)

    MOTH_BEGIN_INSTR(LoadArg)
        accumulator = arguments[instr.index];
    MOTH_END_INSTR(LoadArg)

    MOTH_BEGIN_INSTR(StoreArg)
        CHECK_EXCEPTION;
        storeArg(engine, functionScope, arguments + instr.index, accumulator);
    MOTH_END_INSTR(StoreArg)

    MOTH_BEGIN_INSTR(LoadScopedLocal)
        if (Q_LIKELY(instr.scope == 1))
            accumulator = localsScope1[instr.index];
        else
            accumulator = static_cast<QV4::Heap::CallContext *>(getScope(functionScope, instr.scope))->locals[instr.index];
    MOTH_END_INSTR(LoadScopedLocal)

    MOTH_BEGIN_INSTR(StoreScopedLocal)
        CHECK_EXCEPTION;
        if (Q_LIKELY(instr.scope == 1)) {
            storeLocal(engine, functionScope1, localsScope1 + instr.index, accumulator);
        } else {
            QV4::Heap::ExecutionContext *scope = getScope(functionScope, instr.scope);
            QV4::Heap::CallContext *cc = static_cast<QV4::Heap::CallContext *>(scope);
            storeLocal(engine, cc, cc->locals.values + instr.index, accumulator);
        }
    MOTH_END_INSTR(StoreScopedLocal)

    MOTH_BEGIN_INSTR(LoadScopedArg)
        if (Q_LIKELY(instr.scope == 1))
            accumulator = argumentsScope1[instr.index];
        else
            accumulator = getScope(functionScope, instr.scope)->callData->args[instr.index];
    MOTH_END_INSTR(LoadScopedArg);

    MOTH_BEGIN_INSTR(StoreScopedArg)
        CHECK_EXCEPTION;
        if (Q_LIKELY(instr.scope == 1)) {
            storeArg(engine, functionScope1, argumentsScope1 + instr.index, accumulator);
        } else {
            QV4::Heap::ExecutionContext *scope = getScope(functionScope, instr.scope);
            storeLocal(engine, scope, scope->callData->args + instr.index, accumulator);
        }
    MOTH_END_INSTR(StoreScopedArg)

    MOTH_BEGIN_INSTR(LoadRuntimeString)
        accumulator = engine->current->compilationUnit->runtimeStrings[instr.stringId];
    MOTH_END_INSTR(LoadRuntimeString)

    MOTH_BEGIN_INSTR(LoadRegExp)
        accumulator = static_cast<CompiledData::CompilationUnit*>(engine->current->compilationUnit)->runtimeRegularExpressions[instr.regExpId];
    MOTH_END_INSTR(LoadRegExp)

    MOTH_BEGIN_INSTR(LoadClosure)
        STORE_ACCUMULATOR(Runtime::method_closure(engine, instr.value));
    MOTH_END_INSTR(LoadClosure)

    MOTH_BEGIN_INSTR(LoadName)
        STORE_ACCUMULATOR(Runtime::method_getActivationProperty(engine, instr.name));
    MOTH_END_INSTR(LoadName)

    MOTH_BEGIN_INSTR(GetGlobalLookup)
        QV4::Lookup *l = engine->current->lookups + instr.index;
        STORE_ACCUMULATOR(l->globalGetter(l, engine));
    MOTH_END_INSTR(GetGlobalLookup)

    MOTH_BEGIN_INSTR(StoreName)
        Runtime::method_setActivationProperty(engine, instr.name, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreName)

    MOTH_BEGIN_INSTR(LoadElement)
        STORE_ACCUMULATOR(Runtime::method_getElement(engine, TEMP_VALUE(instr.base), TEMP_VALUE(instr.index)));
    MOTH_END_INSTR(LoadElement)

    MOTH_BEGIN_INSTR(LoadElementLookup)
        QV4::Lookup *l = engine->current->lookups + instr.lookup;
        STORE_ACCUMULATOR(l->indexedGetter(l, engine, TEMP_VALUE(instr.base), TEMP_VALUE(instr.index)));
    MOTH_END_INSTR(LoadElementLookup)

    MOTH_BEGIN_INSTR(StoreElement)
        Runtime::method_setElement(engine, TEMP_VALUE(instr.base), TEMP_VALUE(instr.index), accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreElement)

    MOTH_BEGIN_INSTR(StoreElementLookup)
        QV4::Lookup *l = engine->current->lookups + instr.lookup;
        l->indexedSetter(l, engine, TEMP_VALUE(instr.base), TEMP_VALUE(instr.index), accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreElementLookup)

    MOTH_BEGIN_INSTR(LoadProperty)
        STORE_ACCUMULATOR(Runtime::method_getProperty(engine, TEMP_VALUE(instr.base), instr.name));
    MOTH_END_INSTR(LoadProperty)

    MOTH_BEGIN_INSTR(GetLookup)
        QV4::Lookup *l = engine->current->lookups + instr.index;
        STORE_ACCUMULATOR(l->getter(l, engine, TEMP_VALUE(instr.base)));
    MOTH_END_INSTR(GetLookup)

    MOTH_BEGIN_INSTR(StoreProperty)
        Runtime::method_setProperty(engine, TEMP_VALUE(instr.base), instr.name, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreProperty)

    MOTH_BEGIN_INSTR(SetLookup)
        QV4::Lookup *l = engine->current->lookups + instr.index;
        l->setter(l, engine, TEMP_VALUE(instr.base), accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(SetLookup)

    MOTH_BEGIN_INSTR(StoreScopeObjectProperty)
        Runtime::method_setQmlScopeObjectProperty(engine, TEMP_VALUE(instr.base), instr.propertyIndex, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreScopeObjectProperty)

    MOTH_BEGIN_INSTR(LoadScopeObjectProperty)
        STORE_ACCUMULATOR(Runtime::method_getQmlScopeObjectProperty(engine, TEMP_VALUE(instr.base), instr.propertyIndex, instr.captureRequired));
    MOTH_END_INSTR(LoadScopeObjectProperty)

    MOTH_BEGIN_INSTR(StoreContextObjectProperty)
        Runtime::method_setQmlContextObjectProperty(engine, TEMP_VALUE(instr.base), instr.propertyIndex, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreContextObjectProperty)

    MOTH_BEGIN_INSTR(LoadContextObjectProperty)
        STORE_ACCUMULATOR(Runtime::method_getQmlContextObjectProperty(engine, TEMP_VALUE(instr.base), instr.propertyIndex, instr.captureRequired));
    MOTH_END_INSTR(LoadContextObjectProperty)

    MOTH_BEGIN_INSTR(LoadIdObject)
        STORE_ACCUMULATOR(Runtime::method_getQmlIdObject(engine, TEMP_VALUE(instr.base), instr.index));
    MOTH_END_INSTR(LoadIdObject)

    MOTH_BEGIN_INSTR(InitStackFrame)
        stackSize = unsigned(instr.value);
        stack = scope.alloc(instr.value);
    MOTH_END_INSTR(InitStackFrame)

    MOTH_BEGIN_INSTR(CallValue)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.index);
        STORE_ACCUMULATOR(Runtime::method_callValue(engine, TEMP_VALUE(instr.dest), callData));
    MOTH_END_INSTR(CallValue)

    MOTH_BEGIN_INSTR(CallProperty)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.index);
        callData->thisObject = TEMP_VALUE(instr.base);
        STORE_ACCUMULATOR(Runtime::method_callProperty(engine, instr.name, callData));
    MOTH_END_INSTR(CallProperty)

    MOTH_BEGIN_INSTR(CallPropertyLookup)
        Q_ASSERT(instr.callData.index + instr.argc + offsetof(QV4::CallData, args)/sizeof(QV4::Value) <= stackSize);
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.index);
        callData->tag = quint32(Value::ValueTypeInternal::Integer);
        callData->argc = instr.argc;
        callData->thisObject = TEMP_VALUE(instr.base);
        STORE_ACCUMULATOR(Runtime::method_callPropertyLookup(engine, instr.lookupIndex, callData));
    MOTH_END_INSTR(CallPropertyLookup)

    MOTH_BEGIN_INSTR(CallElement)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.index);
        callData->thisObject = TEMP_VALUE(instr.base);
        STORE_ACCUMULATOR(Runtime::method_callElement(engine, TEMP_VALUE(instr.index), callData));
    MOTH_END_INSTR(CallElement)

    MOTH_BEGIN_INSTR(CallActivationProperty)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.index);
        STORE_ACCUMULATOR(Runtime::method_callActivationProperty(engine, instr.name, callData));
    MOTH_END_INSTR(CallActivationProperty)

    MOTH_BEGIN_INSTR(CallGlobalLookup)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.index);
        STORE_ACCUMULATOR(Runtime::method_callGlobalLookup(engine, instr.index, callData));
    MOTH_END_INSTR(CallGlobalLookup)

    MOTH_BEGIN_INSTR(SetExceptionHandler)
        exceptionHandler = instr.offset ? reinterpret_cast<const uchar *>(&instr.offset) + instr.offset
                                        : nullptr;
    MOTH_END_INSTR(SetExceptionHandler)

    MOTH_BEGIN_INSTR(CallBuiltinThrow)
        Runtime::method_throwException(engine, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallBuiltinThrow)

    MOTH_BEGIN_INSTR(GetException)
        accumulator = engine->hasException ? *engine->exceptionValue : Primitive::emptyValue();
        engine->hasException = false;
    MOTH_END_INSTR(HasException)

    MOTH_BEGIN_INSTR(SetException)
        *engine->exceptionValue = accumulator;
        engine->hasException = true;
    MOTH_END_INSTR(SetException)

    MOTH_BEGIN_INSTR(CallBuiltinUnwindException)
        STORE_ACCUMULATOR(Runtime::method_unwindException(engine));
    MOTH_END_INSTR(CallBuiltinUnwindException)

    MOTH_BEGIN_INSTR(CallBuiltinPushCatchScope)
        Runtime::method_pushCatchScope(static_cast<QV4::NoThrowEngine*>(engine), instr.name);
    MOTH_END_INSTR(CallBuiltinPushCatchScope)

    MOTH_BEGIN_INSTR(CallBuiltinPushScope)
        Runtime::method_pushWithScope(accumulator, static_cast<QV4::NoThrowEngine*>(engine));
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallBuiltinPushScope)

    MOTH_BEGIN_INSTR(CallBuiltinPopScope)
        Runtime::method_popScope(static_cast<QV4::NoThrowEngine*>(engine));
    MOTH_END_INSTR(CallBuiltinPopScope)

    MOTH_BEGIN_INSTR(CallBuiltinForeachIteratorObject)
        STORE_ACCUMULATOR(Runtime::method_foreachIterator(engine, accumulator));
    MOTH_END_INSTR(CallBuiltinForeachIteratorObject)

    MOTH_BEGIN_INSTR(CallBuiltinForeachNextPropertyName)
        STORE_ACCUMULATOR(Runtime::method_foreachNextPropertyName(accumulator));
    MOTH_END_INSTR(CallBuiltinForeachNextPropertyName)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteMember)
        STORE_ACCUMULATOR(Runtime::method_deleteMember(engine, TEMP_VALUE(instr.base), instr.member));
    MOTH_END_INSTR(CallBuiltinDeleteMember)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteSubscript)
        STORE_ACCUMULATOR(Runtime::method_deleteElement(engine, TEMP_VALUE(instr.base), TEMP_VALUE(instr.index)));
    MOTH_END_INSTR(CallBuiltinDeleteSubscript)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteName)
        STORE_ACCUMULATOR(Runtime::method_deleteName(engine, instr.name));
    MOTH_END_INSTR(CallBuiltinDeleteName)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofName)
        STORE_ACCUMULATOR(Runtime::method_typeofName(engine, instr.name));
    MOTH_END_INSTR(CallBuiltinTypeofName)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofValue)
        STORE_ACCUMULATOR(Runtime::method_typeofValue(engine, accumulator));
    MOTH_END_INSTR(CallBuiltinTypeofValue)

    MOTH_BEGIN_INSTR(CallBuiltinDeclareVar)
        Runtime::method_declareVar(engine, instr.isDeletable, instr.varName);
    MOTH_END_INSTR(CallBuiltinDeclareVar)

    MOTH_BEGIN_INSTR(CallBuiltinDefineArray)
        Q_ASSERT(instr.args.index + instr.argc <= stackSize);
        QV4::Value *args = stack + instr.args.index;
        STORE_ACCUMULATOR(Runtime::method_arrayLiteral(engine, args, instr.argc));
    MOTH_END_INSTR(CallBuiltinDefineArray)

    MOTH_BEGIN_INSTR(CallBuiltinDefineObjectLiteral)
        QV4::Value *args = stack + instr.args.index;
        STORE_ACCUMULATOR(Runtime::method_objectLiteral(engine, args, instr.internalClassId, instr.arrayValueCount, instr.arrayGetterSetterCountAndFlags));
    MOTH_END_INSTR(CallBuiltinDefineObjectLiteral)

    MOTH_BEGIN_INSTR(CallBuiltinSetupArgumentsObject)
        STORE_ACCUMULATOR(Runtime::method_setupArgumentsObject(engine));
    MOTH_END_INSTR(CallBuiltinSetupArgumentsObject)

    MOTH_BEGIN_INSTR(CallBuiltinConvertThisToObject)
        Runtime::method_convertThisToObject(engine);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallBuiltinConvertThisToObject)

    MOTH_BEGIN_INSTR(CreateValue)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.index);
        STORE_ACCUMULATOR(Runtime::method_constructValue(engine, TEMP_VALUE(instr.func), callData));
        //### write barrier?
    MOTH_END_INSTR(CreateValue)

    MOTH_BEGIN_INSTR(CreateProperty)
        Q_ASSERT(instr.callData.index + instr.argc + offsetof(QV4::CallData, args)/sizeof(QV4::Value) <= stackSize);
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.index);
        callData->tag = quint32(Value::ValueTypeInternal::Integer);
        callData->argc = instr.argc;
        callData->thisObject = TEMP_VALUE(instr.base);
        STORE_ACCUMULATOR(Runtime::method_constructProperty(engine, instr.name, callData));
    MOTH_END_INSTR(CreateProperty)

    MOTH_BEGIN_INSTR(ConstructPropertyLookup)
        Q_ASSERT(instr.callData.index + instr.argc + offsetof(QV4::CallData, args)/sizeof(QV4::Value) <= stackSize);
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.index);
        callData->tag = quint32(Value::ValueTypeInternal::Integer);
        callData->argc = instr.argc;
        callData->thisObject = TEMP_VALUE(instr.base);
        STORE_ACCUMULATOR(Runtime::method_constructPropertyLookup(engine, instr.index, callData));
    MOTH_END_INSTR(ConstructPropertyLookup)

    MOTH_BEGIN_INSTR(CreateActivationProperty)
        Q_ASSERT(instr.callData.index + instr.argc + offsetof(QV4::CallData, args)/sizeof(QV4::Value) <= stackSize);
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.index);
        callData->tag = quint32(Value::ValueTypeInternal::Integer);
        callData->argc = instr.argc;
        callData->thisObject = QV4::Primitive::undefinedValue();
        STORE_ACCUMULATOR(Runtime::method_constructActivationProperty(engine, instr.name, callData));
    MOTH_END_INSTR(CreateActivationProperty)

    MOTH_BEGIN_INSTR(ConstructGlobalLookup)
        Q_ASSERT(instr.callData.index + instr.argc + offsetof(QV4::CallData, args)/sizeof(QV4::Value) <= stackSize);
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.index);
        callData->tag = quint32(Value::ValueTypeInternal::Integer);
        callData->argc = instr.argc;
        callData->thisObject = QV4::Primitive::undefinedValue();
        STORE_ACCUMULATOR(Runtime::method_constructGlobalLookup(engine, instr.index, callData));
    MOTH_END_INSTR(ConstructGlobalLookup)

    MOTH_BEGIN_INSTR(Jump)
        code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(Jump)

    MOTH_BEGIN_INSTR(JumpEq)
        if (accumulator.toBoolean())
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(JumpEq)

    MOTH_BEGIN_INSTR(JumpNe)
        if (!accumulator.toBoolean())
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(JumpNe)

    MOTH_BEGIN_INSTR(JumpStrictEqual)
        if (RuntimeHelpers::strictEqual(TEMP_VALUE(instr.lhs), accumulator))
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(JumpStrictEqual)

    MOTH_BEGIN_INSTR(JumpStrictNotEqual)
        if (!RuntimeHelpers::strictEqual(TEMP_VALUE(instr.lhs), accumulator))
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(JumpStrictNotEqual)

    MOTH_BEGIN_INSTR(UNot)
        STORE_ACCUMULATOR(Runtime::method_uNot(accumulator));
    MOTH_END_INSTR(UNot)

    MOTH_BEGIN_INSTR(UNotBool)
        bool b = accumulator.toBoolean();
        STORE_ACCUMULATOR(QV4::Encode(!b));
    MOTH_END_INSTR(UNotBool)

    MOTH_BEGIN_INSTR(UPlus)
        STORE_ACCUMULATOR(Runtime::method_uPlus(accumulator));
    MOTH_END_INSTR(UPlus)

    MOTH_BEGIN_INSTR(UMinus)
        STORE_ACCUMULATOR(Runtime::method_uMinus(accumulator));
    MOTH_END_INSTR(UMinus)

    MOTH_BEGIN_INSTR(UCompl)
        STORE_ACCUMULATOR(Runtime::method_complement(accumulator));
    MOTH_END_INSTR(UCompl)

    MOTH_BEGIN_INSTR(UComplInt)
        STORE_ACCUMULATOR(Runtime::method_complement(accumulator));
    MOTH_END_INSTR(UComplInt)

    MOTH_BEGIN_INSTR(Increment)
        STORE_ACCUMULATOR(Runtime::method_increment(accumulator));
    MOTH_END_INSTR(Increment)

    MOTH_BEGIN_INSTR(Decrement)
        STORE_ACCUMULATOR(Runtime::method_decrement(accumulator));
    MOTH_END_INSTR(Decrement)

    MOTH_BEGIN_INSTR(Binop)
        QV4::Runtime::BinaryOperation op = *reinterpret_cast<QV4::Runtime::BinaryOperation *>(reinterpret_cast<char *>(&engine->runtime.runtimeMethods[instr.alu]));
        STORE_ACCUMULATOR(op(TEMP_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(Binop)

    MOTH_BEGIN_INSTR(Add)
        STORE_ACCUMULATOR(Runtime::method_add(engine, TEMP_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(Add)

    MOTH_BEGIN_INSTR(BitAnd)
        STORE_ACCUMULATOR(Runtime::method_bitAnd(TEMP_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(BitAnd)

    MOTH_BEGIN_INSTR(BitOr)
        STORE_ACCUMULATOR(Runtime::method_bitOr(TEMP_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(BitOr)

    MOTH_BEGIN_INSTR(BitXor)
        STORE_ACCUMULATOR(Runtime::method_bitXor(TEMP_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(BitXor)

    MOTH_BEGIN_INSTR(Shr)
        STORE_ACCUMULATOR(Runtime::method_shr(TEMP_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(Shr)

    MOTH_BEGIN_INSTR(Shl)
        STORE_ACCUMULATOR(Runtime::method_shl(TEMP_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(Shl)

    MOTH_BEGIN_INSTR(BitAndConst)
        int lhs = accumulator.toInt32();
        STORE_ACCUMULATOR(QV4::Primitive::fromInt32(lhs & instr.rhs));
    MOTH_END_INSTR(BitAnd)

    MOTH_BEGIN_INSTR(BitOrConst)
        int lhs = accumulator.toInt32();
        STORE_ACCUMULATOR(QV4::Encode((int)(lhs | instr.rhs)));
    MOTH_END_INSTR(BitOr)

    MOTH_BEGIN_INSTR(BitXorConst)
        int lhs = accumulator.toInt32();
        STORE_ACCUMULATOR(QV4::Encode((int)(lhs ^ instr.rhs)));
    MOTH_END_INSTR(BitXor)

    MOTH_BEGIN_INSTR(ShrConst)
        STORE_ACCUMULATOR(QV4::Encode((int)(accumulator.toInt32() >> instr.rhs)));
    MOTH_END_INSTR(ShrConst)

    MOTH_BEGIN_INSTR(ShlConst)
        STORE_ACCUMULATOR(QV4::Encode((int)(accumulator.toInt32() << instr.rhs)));
    MOTH_END_INSTR(ShlConst)

    MOTH_BEGIN_INSTR(Mul)
        STORE_ACCUMULATOR(Runtime::method_mul(TEMP_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(Mul)

    MOTH_BEGIN_INSTR(Sub)
        STORE_ACCUMULATOR(Runtime::method_sub(TEMP_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(Sub)

    MOTH_BEGIN_INSTR(BinopContext)
        QV4::Runtime::BinaryOperationContext op = *reinterpret_cast<QV4::Runtime::BinaryOperationContext *>(reinterpret_cast<char *>(&engine->runtime.runtimeMethods[instr.alu]));
        STORE_ACCUMULATOR(op(engine, TEMP_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(BinopContext)

    MOTH_BEGIN_INSTR(Ret)
        accumulator = TEMP_VALUE(instr.result).asReturnedValue();
        goto functionExit;
    MOTH_END_INSTR(Ret)

#ifndef QT_NO_QML_DEBUGGER
    MOTH_BEGIN_INSTR(Debug)
        debug_slowPath(instr, engine);
    MOTH_END_INSTR(Debug)

    MOTH_BEGIN_INSTR(Line)
        engine->current->lineNumber = instr.lineNumber;
        if (Q_UNLIKELY(qt_v4IsDebugging))
            qt_v4CheckForBreak(engine->currentContext);
    MOTH_END_INSTR(Line)
#endif // QT_NO_QML_DEBUGGER

    MOTH_BEGIN_INSTR(LoadThis)
        STORE_ACCUMULATOR(engine->currentContext->thisObject());
    MOTH_END_INSTR(LoadThis)

    MOTH_BEGIN_INSTR(LoadQmlContext)
        TEMP_VALUE(instr.result) = Runtime::method_getQmlContext(static_cast<QV4::NoThrowEngine*>(engine));
    MOTH_END_INSTR(LoadQmlContext)

    MOTH_BEGIN_INSTR(LoadQmlImportedScripts)
        TEMP_VALUE(instr.result) = Runtime::method_getQmlImportedScripts(static_cast<QV4::NoThrowEngine*>(engine));
    MOTH_END_INSTR(LoadQmlImportedScripts)

    MOTH_BEGIN_INSTR(LoadQmlSingleton)
        accumulator = Runtime::method_getQmlSingleton(static_cast<QV4::NoThrowEngine*>(engine), instr.name);
    MOTH_END_INSTR(LoadQmlSingleton)

#ifdef MOTH_THREADED_INTERPRETER
    // nothing to do
#else
        default:
            qFatal("QQmlJS::Moth::VME: Internal error - unknown instruction %d", genericInstr->common.instructionType);
            break;
        }
#endif

        Q_ASSERT(false);
    catchException:
        Q_ASSERT(engine->hasException);
        if (!exceptionHandler) {
            accumulator = Primitive::undefinedValue();
            goto functionExit;
        }
        code = exceptionHandler;
    }

functionExit:
    if (QV4::Debugging::Debugger *debugger = engine->debugger())
        debugger->leavingFunction(accumulator.asReturnedValue());
    return accumulator.asReturnedValue();
}
