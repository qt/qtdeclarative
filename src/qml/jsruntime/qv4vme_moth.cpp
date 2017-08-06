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

#undef COUNT_INSTRUCTIONS

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
static bool qt_v4IsDebugging = false;
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
        qt_v4IsDebugging = true;
        return bp.bpNumber;
    }

    if (command == "removeBreakpoint") {
        int lineNumber = ob.value(QLatin1String("lineNumber")).toString().toInt();
        QString fullName = ob.value(QLatin1String("fullName")).toString();
        if (qt_v4Breakpoints.last().matches(fullName, lineNumber)) {
            qt_v4Breakpoints.removeLast();
            qt_v4IsDebugging = !qt_v4Breakpoints.isEmpty();
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

Q_NEVER_INLINE static void qt_v4CheckForBreak(QV4::EngineBase::StackFrame *frame)
{
    if (!qt_v4IsStepping && !qt_v4Breakpoints.size())
        return;

    const int lineNumber = frame->line;
    QV4::Function *function = frame->v4Function;
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
    engine->currentStackFrame->line = instr.lineNumber;
    QV4::Debugging::Debugger *debugger = engine->debugger();
    if (debugger && debugger->pauseAtNextOpportunity())
        debugger->maybeBreakAtInstruction();
    if (qt_v4IsDebugging)
        qt_v4CheckForBreak(engine->currentStackFrame);
}

#endif // QT_NO_QML_DEBUGGER
// End of debugger interface

using namespace QV4;
using namespace QV4::Moth;

#ifdef COUNT_INSTRUCTIONS
static struct InstrCount {
    InstrCount() {
        fprintf(stderr, "Counting instructions...\n");
        for (int i = 0; i < Instr::LastInstruction; ++i)
            hits[i] = 0;
    }
    ~InstrCount() {
        fprintf(stderr, "Instruction count:\n");
#define BLAH(I, FMT) \
        fprintf(stderr, "%llu : %s\n", hits[Instr::I], #I);
        FOR_EACH_MOTH_INSTR(BLAH)
        #undef BLAH
    }
    quint64 hits[Instr::LastInstruction];
    void hit(Instr::Type i) { hits[i]++; }
} instrCount;
#endif // COUNT_INSTRUCTIONS

#define MOTH_BEGIN_INSTR_COMMON(I) { \
    const InstrMeta<int(Instr::I)>::DataType &instr = InstrMeta<int(Instr::I)>::data(*genericInstr); \
    code += InstrMeta<int(Instr::I)>::Size; \
    Q_UNUSED(instr);

#ifdef MOTH_THREADED_INTERPRETER


#ifdef COUNT_INSTRUCTIONS
#  define MOTH_BEGIN_INSTR(I) op_##I: \
    instrCount.hit(Instr::I); \
    MOTH_BEGIN_INSTR_COMMON(I)
#else // !COUNT_INSTRUCTIONS
#  define MOTH_BEGIN_INSTR(I) op_##I: \
    MOTH_BEGIN_INSTR_COMMON(I)
#endif // COUNT_INSTRUCTIONS

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

#define STACK_VALUE(temp) stack[temp.stackSlot()]
#define STORE_STACK_VALUE(temp, value) { \
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

static inline ReturnedValue loadScopedLocal(ExecutionEngine *engine, int index, int scope)
{
    auto ctxt = getScope(engine->current, scope);
    Q_ASSERT(ctxt->type == QV4::Heap::ExecutionContext::Type_CallContext);
    auto cc = static_cast<Heap::CallContext *>(ctxt);
    return cc->locals[index].asReturnedValue();
}

static inline void storeScopedLocal(ExecutionEngine *engine, int index, int scope,
                                    const QV4::Value &value)
{
    auto ctxt = getScope(engine->current, scope);
    Q_ASSERT(ctxt->type == QV4::Heap::ExecutionContext::Type_CallContext);
    auto cc = static_cast<Heap::CallContext *>(ctxt);

    if (Q_UNLIKELY(engine->writeBarrierActive))
        QV4::WriteBarrier::write(engine, cc, cc->locals.values + index, value);
    else
        *(cc->locals.values + index) = value;
}

static inline ReturnedValue loadScopedArg(ExecutionEngine *engine, int index, int scope)
{
    auto ctxt = getScope(engine->current, scope);
    Q_ASSERT(ctxt->type == QV4::Heap::ExecutionContext::Type_CallContext);
    auto cc = static_cast<Heap::CallContext *>(ctxt);
    return cc->callData->args[index].asReturnedValue();
}

static inline void storeScopedArg(ExecutionEngine *engine, int index, int scope,
                                  const QV4::Value &value)
{
    auto ctxt = getScope(engine->current, scope);
    Q_ASSERT(ctxt->type == QV4::Heap::ExecutionContext::Type_CallContext);
    auto cc = static_cast<Heap::CallContext *>(ctxt);

    if (Q_UNLIKELY(engine->writeBarrierActive))
        QV4::WriteBarrier::write(engine, cc, cc->callData->args + index, value);
    else
        *(cc->callData->args + index) = value;
}

static inline const QV4::Value &constant(Function *function, int index)
{
    return function->compilationUnit->constants[index];
}

QV4::ReturnedValue VME::exec(Function *function)
{
    qt_v4ResolvePendingBreakpointsHook();

#ifdef MOTH_THREADED_INTERPRETER
#define MOTH_INSTR_ADDR(I, FMT) &&op_##I,
    static void *jumpTable[] = {
        FOR_EACH_MOTH_INSTR(MOTH_INSTR_ADDR)
    };
#undef MOTH_INSTR_ADDR
#endif

    ExecutionEngine *engine = function->internalClass->engine;

    EngineBase::StackFrame frame;
    frame.parent = engine->currentStackFrame;
    frame.v4Function = function;
    engine->currentStackFrame = &frame;

    QV4::Value *stack = nullptr;
    const uchar *exceptionHandler = 0;

    QV4::Scope scope(engine);
    int nFormals = function->nFormals;
    stack = scope.alloc(function->compiledFunction->nRegisters + nFormals + 2);
    QV4::Value &accumulator = *stack;
    ++stack;
    memcpy(stack, &engine->current->callData->thisObject, (nFormals + 1)*sizeof(Value));
    stack += nFormals + 1;

    if (QV4::Debugging::Debugger *debugger = engine->debugger())
        debugger->enteringFunction();

    const uchar *code = function->codeData;

    for (;;) {
        const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
#ifdef MOTH_THREADED_INTERPRETER
        goto *jumpTable[genericInstr->common.instructionType];
#else
        switch (genericInstr->common.instructionType) {
#endif

    MOTH_BEGIN_INSTR(LoadConst)
        accumulator = constant(function, instr.index);
    MOTH_END_INSTR(LoadConst)

    MOTH_BEGIN_INSTR(MoveConst)
        STACK_VALUE(instr.destTemp) = constant(function, instr.constIndex);
    MOTH_END_INSTR(MoveConst)

    MOTH_BEGIN_INSTR(LoadReg)
        accumulator = STACK_VALUE(instr.reg);
    MOTH_END_INSTR(LoadReg)

    MOTH_BEGIN_INSTR(StoreReg)
        STACK_VALUE(instr.reg) = accumulator;
    MOTH_END_INSTR(StoreReg)

    MOTH_BEGIN_INSTR(MoveReg)
        STACK_VALUE(instr.destReg) = STACK_VALUE(instr.srcReg);
    MOTH_END_INSTR(MoveReg)

    MOTH_BEGIN_INSTR(LoadScopedLocal)
        accumulator = loadScopedLocal(engine, instr.index, instr.scope);
    MOTH_END_INSTR(LoadScopedLocal)

    MOTH_BEGIN_INSTR(StoreScopedLocal)
        CHECK_EXCEPTION;
        storeScopedLocal(engine, instr.index, instr.scope, accumulator);
    MOTH_END_INSTR(StoreScopedLocal)

    MOTH_BEGIN_INSTR(LoadScopedArgument)
        accumulator = loadScopedArg(engine, instr.index, instr.scope);
    MOTH_END_INSTR(LoadScopedArgument)

    MOTH_BEGIN_INSTR(StoreScopedArgument)
        CHECK_EXCEPTION;
        storeScopedArg(engine, instr.index, instr.scope, accumulator);
    MOTH_END_INSTR(StoreScopedArgument)

    MOTH_BEGIN_INSTR(LoadRuntimeString)
        accumulator = function->compilationUnit->runtimeStrings[instr.stringId];
    MOTH_END_INSTR(LoadRuntimeString)

    MOTH_BEGIN_INSTR(LoadRegExp)
        accumulator = function->compilationUnit->runtimeRegularExpressions[instr.regExpId];
    MOTH_END_INSTR(LoadRegExp)

    MOTH_BEGIN_INSTR(LoadClosure)
        STORE_ACCUMULATOR(Runtime::method_closure(engine, instr.value));
    MOTH_END_INSTR(LoadClosure)

    MOTH_BEGIN_INSTR(LoadName)
        STORE_ACCUMULATOR(Runtime::method_getActivationProperty(engine, instr.name));
    MOTH_END_INSTR(LoadName)

    MOTH_BEGIN_INSTR(GetGlobalLookup)
        QV4::Lookup *l = function->compilationUnit->runtimeLookups + instr.index;
        STORE_ACCUMULATOR(l->globalGetter(l, engine));
    MOTH_END_INSTR(GetGlobalLookup)

    MOTH_BEGIN_INSTR(StoreName)
        Runtime::method_setActivationProperty(engine, instr.name, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreName)

    MOTH_BEGIN_INSTR(LoadElement)
        STORE_ACCUMULATOR(Runtime::method_getElement(engine, STACK_VALUE(instr.base), STACK_VALUE(instr.index)));
    MOTH_END_INSTR(LoadElement)

    MOTH_BEGIN_INSTR(LoadElementA)
        STORE_ACCUMULATOR(Runtime::method_getElement(engine, STACK_VALUE(instr.base), accumulator));
    MOTH_END_INSTR(LoadElementA)

    MOTH_BEGIN_INSTR(LoadElementLookup)
        QV4::Lookup *l = function->compilationUnit->runtimeLookups + instr.lookup;
        STORE_ACCUMULATOR(l->indexedGetter(l, engine, STACK_VALUE(instr.base), STACK_VALUE(instr.index)));
    MOTH_END_INSTR(LoadElementLookup)

    MOTH_BEGIN_INSTR(StoreElement)
        Runtime::method_setElement(engine, STACK_VALUE(instr.base), STACK_VALUE(instr.index), accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreElement)

    MOTH_BEGIN_INSTR(StoreElementLookup)
        QV4::Lookup *l = function->compilationUnit->runtimeLookups + instr.lookup;
        l->indexedSetter(l, engine, STACK_VALUE(instr.base), STACK_VALUE(instr.index), accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreElementLookup)

    MOTH_BEGIN_INSTR(LoadProperty)
        STORE_ACCUMULATOR(Runtime::method_getProperty(engine, STACK_VALUE(instr.base), instr.name));
    MOTH_END_INSTR(LoadProperty)

    MOTH_BEGIN_INSTR(LoadPropertyA)
        STORE_ACCUMULATOR(Runtime::method_getProperty(engine, accumulator, instr.name));
    MOTH_END_INSTR(LoadPropertyA)

    MOTH_BEGIN_INSTR(GetLookup)
        QV4::Lookup *l = function->compilationUnit->runtimeLookups + instr.index;
        STORE_ACCUMULATOR(l->getter(l, engine, STACK_VALUE(instr.base)));
    MOTH_END_INSTR(GetLookup)

    MOTH_BEGIN_INSTR(GetLookupA)
        QV4::Lookup *l = function->compilationUnit->runtimeLookups + instr.index;
        STORE_ACCUMULATOR(l->getter(l, engine, accumulator));
    MOTH_END_INSTR(GetLookupA)

    MOTH_BEGIN_INSTR(StoreProperty)
        Runtime::method_setProperty(engine, STACK_VALUE(instr.base), instr.name, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreProperty)

    MOTH_BEGIN_INSTR(SetLookup)
        QV4::Lookup *l = function->compilationUnit->runtimeLookups + instr.index;
        l->setter(l, engine, STACK_VALUE(instr.base), accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(SetLookup)

    MOTH_BEGIN_INSTR(StoreScopeObjectProperty)
        Runtime::method_setQmlScopeObjectProperty(engine, STACK_VALUE(instr.base), instr.propertyIndex, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreScopeObjectProperty)

    MOTH_BEGIN_INSTR(LoadScopeObjectProperty)
        STORE_ACCUMULATOR(Runtime::method_getQmlScopeObjectProperty(engine, STACK_VALUE(instr.base), instr.propertyIndex, instr.captureRequired));
    MOTH_END_INSTR(LoadScopeObjectProperty)

    MOTH_BEGIN_INSTR(StoreContextObjectProperty)
        Runtime::method_setQmlContextObjectProperty(engine, STACK_VALUE(instr.base), instr.propertyIndex, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreContextObjectProperty)

    MOTH_BEGIN_INSTR(LoadContextObjectProperty)
        STORE_ACCUMULATOR(Runtime::method_getQmlContextObjectProperty(engine, STACK_VALUE(instr.base), instr.propertyIndex, instr.captureRequired));
    MOTH_END_INSTR(LoadContextObjectProperty)

    MOTH_BEGIN_INSTR(LoadIdObject)
        STORE_ACCUMULATOR(Runtime::method_getQmlIdObject(engine, STACK_VALUE(instr.base), instr.index));
    MOTH_END_INSTR(LoadIdObject)

    MOTH_BEGIN_INSTR(CallValue)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        STORE_ACCUMULATOR(Runtime::method_callValue(engine, accumulator, callData));
    MOTH_END_INSTR(CallValue)

    MOTH_BEGIN_INSTR(CallProperty)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        callData->thisObject = STACK_VALUE(instr.base);
        STORE_ACCUMULATOR(Runtime::method_callProperty(engine, instr.name, callData));
    MOTH_END_INSTR(CallProperty)

    MOTH_BEGIN_INSTR(CallPropertyLookup)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        callData->thisObject = STACK_VALUE(instr.base);
        STORE_ACCUMULATOR(Runtime::method_callPropertyLookup(engine, instr.lookupIndex, callData));
    MOTH_END_INSTR(CallPropertyLookup)

    MOTH_BEGIN_INSTR(CallElement)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        callData->thisObject = STACK_VALUE(instr.base);
        STORE_ACCUMULATOR(Runtime::method_callElement(engine, STACK_VALUE(instr.index), callData));
    MOTH_END_INSTR(CallElement)

    MOTH_BEGIN_INSTR(CallActivationProperty)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        STORE_ACCUMULATOR(Runtime::method_callActivationProperty(engine, instr.name, callData));
    MOTH_END_INSTR(CallActivationProperty)

    MOTH_BEGIN_INSTR(CallGlobalLookup)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
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
        STORE_ACCUMULATOR(Runtime::method_deleteMember(engine, STACK_VALUE(instr.base), instr.member));
    MOTH_END_INSTR(CallBuiltinDeleteMember)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteSubscript)
        STORE_ACCUMULATOR(Runtime::method_deleteElement(engine, STACK_VALUE(instr.base), STACK_VALUE(instr.index)));
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
        QV4::Value *args = stack + instr.args.stackSlot();
        STORE_ACCUMULATOR(Runtime::method_arrayLiteral(engine, args, instr.argc));
    MOTH_END_INSTR(CallBuiltinDefineArray)

    MOTH_BEGIN_INSTR(CallBuiltinDefineObjectLiteral)
        QV4::Value *args = stack + instr.args.stackSlot();
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
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        STORE_ACCUMULATOR(Runtime::method_constructValue(engine, STACK_VALUE(instr.func), callData));
        //### write barrier?
    MOTH_END_INSTR(CreateValue)

    MOTH_BEGIN_INSTR(CreateProperty)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        callData->tag = quint32(Value::ValueTypeInternal::Integer);
        callData->argc = instr.argc;
        callData->thisObject = STACK_VALUE(instr.base);
        STORE_ACCUMULATOR(Runtime::method_constructProperty(engine, instr.name, callData));
    MOTH_END_INSTR(CreateProperty)

    MOTH_BEGIN_INSTR(ConstructPropertyLookup)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        callData->tag = quint32(Value::ValueTypeInternal::Integer);
        callData->argc = instr.argc;
        callData->thisObject = STACK_VALUE(instr.base);
        STORE_ACCUMULATOR(Runtime::method_constructPropertyLookup(engine, instr.index, callData));
    MOTH_END_INSTR(ConstructPropertyLookup)

    MOTH_BEGIN_INSTR(CreateActivationProperty)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        callData->tag = quint32(Value::ValueTypeInternal::Integer);
        callData->argc = instr.argc;
        callData->thisObject = QV4::Primitive::undefinedValue();
        STORE_ACCUMULATOR(Runtime::method_constructActivationProperty(engine, instr.name, callData));
    MOTH_END_INSTR(CreateActivationProperty)

    MOTH_BEGIN_INSTR(ConstructGlobalLookup)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        callData->tag = quint32(Value::ValueTypeInternal::Integer);
        callData->argc = instr.argc;
        callData->thisObject = QV4::Primitive::undefinedValue();
        STORE_ACCUMULATOR(Runtime::method_constructGlobalLookup(engine, instr.index, callData));
    MOTH_END_INSTR(ConstructGlobalLookup)

    MOTH_BEGIN_INSTR(Jump)
        code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(Jump)

    MOTH_BEGIN_INSTR(JumpEq)
        if ((accumulator.integerCompatible() && accumulator.int_32()) ||
            accumulator.toBoolean())
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(JumpEq)

    MOTH_BEGIN_INSTR(JumpNe)
        if ((accumulator.integerCompatible() && !accumulator.int_32()) ||
            !accumulator.toBoolean())
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(JumpNe)

    MOTH_BEGIN_INSTR(CmpJmpEq)
        const Value lhs = STACK_VALUE(instr.lhs);
        if (Q_LIKELY(lhs.asReturnedValue() == accumulator.asReturnedValue())) {
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } else if (Q_LIKELY(lhs.isInteger() && accumulator.isInteger())) {
            if (lhs.int_32() == accumulator.int_32())
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } else {
            if (Runtime::method_compareEqual(lhs, accumulator))
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        }
    MOTH_END_INSTR(CmpJmpEq)

    MOTH_BEGIN_INSTR(CmpJmpNe)
        const Value lhs = STACK_VALUE(instr.lhs);
        if (Q_LIKELY(lhs.isInteger() && accumulator.isInteger())) {
            if (lhs.int_32() != accumulator.int_32())
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } else {
            if (Runtime::method_compareNotEqual(lhs, accumulator))
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        }
    MOTH_END_INSTR(CmpJmpNe)

    MOTH_BEGIN_INSTR(CmpJmpGt)
        const Value lhs = STACK_VALUE(instr.lhs);
        if (Q_LIKELY(lhs.isInteger() && accumulator.isInteger())) {
            if (lhs.int_32() > accumulator.int_32())
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } if (lhs.isNumber() && accumulator.isNumber()) {
            if (lhs.asDouble() > accumulator.asDouble())
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } else {
            if (Runtime::method_compareGreaterThan(lhs, accumulator))
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        }
    MOTH_END_INSTR(CmpJmpGt)

    MOTH_BEGIN_INSTR(CmpJmpGe)
        const Value lhs = STACK_VALUE(instr.lhs);
        if (Q_LIKELY(lhs.isInteger() && accumulator.isInteger())) {
            if (lhs.int_32() >= accumulator.int_32())
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } if (lhs.isNumber() && accumulator.isNumber()) {
            if (lhs.asDouble() >= accumulator.asDouble())
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } else {
            if (Runtime::method_compareGreaterEqual(lhs, accumulator))
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        }
    MOTH_END_INSTR(CmpJmpGe)

    MOTH_BEGIN_INSTR(CmpJmpLt)
        const Value lhs = STACK_VALUE(instr.lhs);
        if (Q_LIKELY(lhs.isInteger() && accumulator.isInteger())) {
            if (lhs.int_32() < accumulator.int_32())
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } if (lhs.isNumber() && accumulator.isNumber()) {
            if (lhs.asDouble() < accumulator.asDouble())
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } else {
            if (Runtime::method_compareLessThan(lhs, accumulator))
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        }
    MOTH_END_INSTR(CmpJmpLt)

    MOTH_BEGIN_INSTR(CmpJmpLe)
        const Value lhs = STACK_VALUE(instr.lhs);
        if (Q_LIKELY(lhs.isInteger() && accumulator.isInteger())) {
            if (lhs.int_32() <= accumulator.int_32())
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } if (lhs.isNumber() && accumulator.isNumber()) {
            if (lhs.asDouble() <= accumulator.asDouble())
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } else {
            if (Runtime::method_compareLessEqual(lhs, accumulator))
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        }
    MOTH_END_INSTR(CmpJmpLe)

    MOTH_BEGIN_INSTR(JumpStrictEqual)
        if (RuntimeHelpers::strictEqual(STACK_VALUE(instr.lhs), accumulator))
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(JumpStrictEqual)

    MOTH_BEGIN_INSTR(JumpStrictNotEqual)
        if (!RuntimeHelpers::strictEqual(STACK_VALUE(instr.lhs), accumulator))
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(JumpStrictNotEqual)

    MOTH_BEGIN_INSTR(UNot)
        STORE_ACCUMULATOR(Runtime::method_uNot(accumulator));
    MOTH_END_INSTR(UNot)

    MOTH_BEGIN_INSTR(UPlus)
        if (!accumulator.isNumber()) {
            STORE_ACCUMULATOR(Runtime::method_uPlus(accumulator));
        }
    MOTH_END_INSTR(UPlus)

    MOTH_BEGIN_INSTR(UMinus)
        if (Q_LIKELY(accumulator.isInteger() && accumulator.int_32() != 0 &&
                accumulator.int_32() != std::numeric_limits<int>::min())) {
            accumulator = sub_int32(0, accumulator.int_32());
        } else {
            STORE_ACCUMULATOR(Runtime::method_uMinus(accumulator));
        }
    MOTH_END_INSTR(UMinus)

    MOTH_BEGIN_INSTR(UCompl)
        if (Q_LIKELY(accumulator.isInteger())) {
            accumulator.setInt_32(~accumulator.int_32());
        } else {
            STORE_ACCUMULATOR(Runtime::method_complement(accumulator));
        }
    MOTH_END_INSTR(UCompl)

    MOTH_BEGIN_INSTR(Increment)
        if (Q_LIKELY(accumulator.isInteger())) {
            accumulator = add_int32(accumulator.int_32(), 1);
        } else if (accumulator.isDouble()) {
            accumulator = QV4::Encode(accumulator.doubleValue() + 1.);
        } else {
            STORE_ACCUMULATOR(Runtime::method_increment(accumulator));
        }
    MOTH_END_INSTR(Increment)

    MOTH_BEGIN_INSTR(Decrement)
        if (Q_LIKELY(accumulator.isInteger())) {
            accumulator = sub_int32(accumulator.int_32(), 1);
        } else if (accumulator.isDouble()) {
            accumulator = QV4::Encode(accumulator.doubleValue() - 1.);
        } else {
            STORE_ACCUMULATOR(Runtime::method_decrement(accumulator));
        }
    MOTH_END_INSTR(Decrement)

    MOTH_BEGIN_INSTR(Binop)
        QV4::Runtime::BinaryOperation op = *reinterpret_cast<QV4::Runtime::BinaryOperation *>(reinterpret_cast<char *>(&engine->runtime.runtimeMethods[instr.alu]));
        STORE_ACCUMULATOR(op(STACK_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(Binop)

    MOTH_BEGIN_INSTR(Add)
        QV4::Value lhs = STACK_VALUE(instr.lhs);
        if (Q_LIKELY(Value::integerCompatible(lhs, accumulator))) {
            accumulator = add_int32(lhs.int_32(), accumulator.int_32());
        } else {
            STORE_ACCUMULATOR(Runtime::method_add(engine, lhs, accumulator));
        }
    MOTH_END_INSTR(Add)

    MOTH_BEGIN_INSTR(Sub)
        QV4::Value lhs = STACK_VALUE(instr.lhs);
        if (Q_LIKELY(Value::integerCompatible(lhs, accumulator))) {
            accumulator = sub_int32(lhs.int_32(), accumulator.int_32());
        } else {
            STORE_ACCUMULATOR(Runtime::method_sub(lhs, accumulator));
        }
    MOTH_END_INSTR(Sub)

    MOTH_BEGIN_INSTR(Mul)
        QV4::Value lhs = STACK_VALUE(instr.lhs);
        if (Q_LIKELY(Value::integerCompatible(lhs, accumulator))) {
            accumulator = mul_int32(lhs.int_32(), accumulator.int_32());
        } else {
            STORE_ACCUMULATOR(Runtime::method_mul(lhs, accumulator));
        }
    MOTH_END_INSTR(Mul)

    MOTH_BEGIN_INSTR(BitAnd)
        STORE_ACCUMULATOR(Runtime::method_bitAnd(STACK_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(BitAnd)

    MOTH_BEGIN_INSTR(BitOr)
        STORE_ACCUMULATOR(Runtime::method_bitOr(STACK_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(BitOr)

    MOTH_BEGIN_INSTR(BitXor)
        STORE_ACCUMULATOR(Runtime::method_bitXor(STACK_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(BitXor)

    MOTH_BEGIN_INSTR(Shr)
        STORE_ACCUMULATOR(Runtime::method_shr(STACK_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(Shr)

    MOTH_BEGIN_INSTR(Shl)
        STORE_ACCUMULATOR(Runtime::method_shl(STACK_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(Shl)

    MOTH_BEGIN_INSTR(BitAndConst)
        if (Q_LIKELY(accumulator.isInteger())) {
            accumulator.setInt_32(accumulator.int_32() & instr.rhs);
        } else {
            STORE_ACCUMULATOR(QV4::Primitive::fromInt32(accumulator.toInt32() & instr.rhs));
        }
    MOTH_END_INSTR(BitAndConst)

    MOTH_BEGIN_INSTR(BitOrConst)
        if (Q_LIKELY(accumulator.isInteger())) {
            accumulator.setInt_32(accumulator.int_32() | instr.rhs);
        } else {
            STORE_ACCUMULATOR(QV4::Primitive::fromInt32(accumulator.toInt32() | instr.rhs));
        }
    MOTH_END_INSTR(BitOrConst)

    MOTH_BEGIN_INSTR(BitXorConst)
        if (Q_LIKELY(accumulator.isInteger())) {
            accumulator.setInt_32(accumulator.int_32() ^ instr.rhs);
        } else {
            STORE_ACCUMULATOR(QV4::Primitive::fromInt32(accumulator.toInt32() ^ instr.rhs));
        }
    MOTH_END_INSTR(BitXorConst)

    MOTH_BEGIN_INSTR(ShrConst)
        if (Q_LIKELY(accumulator.isInteger())) {
            accumulator.setInt_32(accumulator.int_32() >> instr.rhs);
        } else {
            STORE_ACCUMULATOR(QV4::Primitive::fromInt32(accumulator.toInt32() >> instr.rhs));
        }
    MOTH_END_INSTR(ShrConst)

    MOTH_BEGIN_INSTR(ShlConst)
        if (Q_LIKELY(accumulator.isInteger())) {
            accumulator.setInt_32(accumulator.int_32() << instr.rhs);
        } else {
            STORE_ACCUMULATOR(QV4::Primitive::fromInt32(accumulator.toInt32() << instr.rhs));
        }
    MOTH_END_INSTR(ShlConst)

    MOTH_BEGIN_INSTR(BinopContext)
        QV4::Runtime::BinaryOperationContext op = *reinterpret_cast<QV4::Runtime::BinaryOperationContext *>(reinterpret_cast<char *>(&engine->runtime.runtimeMethods[instr.alu]));
        STORE_ACCUMULATOR(op(engine, STACK_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(BinopContext)

    MOTH_BEGIN_INSTR(Ret)
        accumulator = STACK_VALUE(instr.result).asReturnedValue();
        goto functionExit;
    MOTH_END_INSTR(Ret)

#ifndef QT_NO_QML_DEBUGGER
    MOTH_BEGIN_INSTR(Debug)
        debug_slowPath(instr, engine);
    MOTH_END_INSTR(Debug)

    MOTH_BEGIN_INSTR(Line)
        frame.line = instr.lineNumber;
        if (Q_UNLIKELY(qt_v4IsDebugging))
            qt_v4CheckForBreak(&frame);
    MOTH_END_INSTR(Line)
#endif // QT_NO_QML_DEBUGGER

    MOTH_BEGIN_INSTR(LoadThis)
        STORE_ACCUMULATOR(engine->currentContext->thisObject());
    MOTH_END_INSTR(LoadThis)

    MOTH_BEGIN_INSTR(LoadQmlContext)
        STACK_VALUE(instr.result) = Runtime::method_getQmlContext(static_cast<QV4::NoThrowEngine*>(engine));
    MOTH_END_INSTR(LoadQmlContext)

    MOTH_BEGIN_INSTR(LoadQmlImportedScripts)
        STACK_VALUE(instr.result) = Runtime::method_getQmlImportedScripts(static_cast<QV4::NoThrowEngine*>(engine));
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
    engine->currentStackFrame = frame.parent;
    return accumulator.asReturnedValue();
}
