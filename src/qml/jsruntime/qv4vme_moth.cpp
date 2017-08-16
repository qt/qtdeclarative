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
#include <private/qv4profiling_p.h>
#include <private/qqmljavascriptexpression_p.h>
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

Q_NEVER_INLINE static void qt_v4CheckForBreak(QV4::CppStackFrame *frame)
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

static inline QV4::Heap::ExecutionContext *getScope(JSStackFrame *frame, int level)
{
    QV4::Heap::ExecutionContext *scope = static_cast<ExecutionContext &>(frame->context).d();
    while (level > 0) {
        --level;
        scope = scope->outer;
    }
    Q_ASSERT(scope);
    return scope;
}

static inline ReturnedValue loadScopedLocal(CppStackFrame &frame, int index, int scope)
{
    auto ctxt = getScope(frame.jsFrame, scope);
    Q_ASSERT(ctxt->type == QV4::Heap::ExecutionContext::Type_CallContext);
    auto cc = static_cast<Heap::CallContext *>(ctxt);
    return cc->locals[index].asReturnedValue();
}

static inline void storeScopedLocal(ExecutionEngine *engine, CppStackFrame &frame, int index, int scope,
                                    const QV4::Value &value)
{
    auto ctxt = getScope(frame.jsFrame, scope);
    Q_ASSERT(ctxt->type == QV4::Heap::ExecutionContext::Type_CallContext);
    auto cc = static_cast<Heap::CallContext *>(ctxt);

    QV4::WriteBarrier::write(engine, cc, cc->locals.values + index, value);
}

static inline const QV4::Value &constant(Function *function, int index)
{
    return function->compilationUnit->constants[index];
}


static bool compareEqual(Value lhs, Value rhs)
{
    if (lhs.asReturnedValue() == rhs.asReturnedValue())
        return !lhs.isNaN();

    int lt = lhs.quickType();
    int rt = rhs.quickType();
    if (rt < lt) {
        qSwap(lhs, rhs);
        qSwap(lt, rt);
    }

    switch (lt) {
    case Value::QT_ManagedOrUndefined:
        if (lhs.isUndefined())
            return rhs.isNullOrUndefined();
        Q_FALLTHROUGH();
    case Value::QT_ManagedOrUndefined1:
    case Value::QT_ManagedOrUndefined2:
    case Value::QT_ManagedOrUndefined3:
        // LHS: Managed
        switch (rt) {
        case Value::QT_ManagedOrUndefined:
            if (rhs.isUndefined())
                return false;
            Q_FALLTHROUGH();
        case Value::QT_ManagedOrUndefined1:
        case Value::QT_ManagedOrUndefined2:
        case Value::QT_ManagedOrUndefined3: {
            // RHS: Managed
            Heap::Base *l = lhs.m();
            Heap::Base *r = rhs.m();
            Q_ASSERT(l);
            Q_ASSERT(r);
            if (l->vtable()->isString == r->vtable()->isString)
                return static_cast<QV4::Managed &>(lhs).isEqualTo(&static_cast<QV4::Managed &>(rhs));
            if (l->vtable()->isString) {
                rhs = Primitive::fromReturnedValue(RuntimeHelpers::objectDefaultValue(&static_cast<QV4::Object &>(rhs), PREFERREDTYPE_HINT));
                break;
            } else {
                Q_ASSERT(r->vtable()->isString);
                lhs = Primitive::fromReturnedValue(RuntimeHelpers::objectDefaultValue(&static_cast<QV4::Object &>(lhs), PREFERREDTYPE_HINT));
                break;
            }
            return false;
        }
        case Value::QT_Empty:
            Q_UNREACHABLE();
        case Value::QT_Null:
            return false;
        case Value::QT_Bool:
        case Value::QT_Int:
            rhs = Primitive::fromDouble(rhs.int_32());
            // fall through
        default: // double
            if (lhs.m()->vtable()->isString)
                return RuntimeHelpers::toNumber(lhs) == rhs.doubleValue();
            else
                lhs = Primitive::fromReturnedValue(RuntimeHelpers::objectDefaultValue(&static_cast<QV4::Object &>(lhs), PREFERREDTYPE_HINT));
        }
        return compareEqual(lhs, rhs);
    case Value::QT_Empty:
        Q_UNREACHABLE();
    case Value::QT_Null:
        return rhs.isNull();
    case Value::QT_Bool:
    case Value::QT_Int:
        switch (rt) {
        case Value::QT_ManagedOrUndefined:
        case Value::QT_ManagedOrUndefined1:
        case Value::QT_ManagedOrUndefined2:
        case Value::QT_ManagedOrUndefined3:
        case Value::QT_Empty:
        case Value::QT_Null:
            Q_UNREACHABLE();
        case Value::QT_Bool:
        case Value::QT_Int:
            return lhs.int_32() == rhs.int_32();
        default: // double
            return lhs.int_32() == rhs.doubleValue();
        }
    default: // double
        Q_ASSERT(rhs.isDouble());
        return lhs.doubleValue() == rhs.doubleValue();
    }
}

QV4::ReturnedValue VME::exec(const FunctionObject *jsFunction, CallData *callData, Heap::ExecutionContext *context, QV4::Function *function)
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
    Profiling::FunctionCallProfiler(engine, function);

    Value *jsStackTop = engine->jsStackTop;
    engine->jsStackTop = reinterpret_cast<QV4::Value *>(callData) + 2 + (int)function->nFormals;
    for (int i = callData->argc; i < (int)function->nFormals; ++i)
        callData->args[i] = Encode::undefined();

    CppStackFrame frame;
    frame.parent = engine->currentStackFrame;
    frame.v4Function = function;
    engine->currentStackFrame = &frame;

    QV4::Value *stack = nullptr;
    const uchar *exceptionHandler = 0;

    stack = engine->jsAlloca(function->compiledFunction->nRegisters + sizeof(JSStackFrame)/sizeof(QV4::Value));
    frame.jsFrame = reinterpret_cast<JSStackFrame *>(stack);
    frame.jsFrame->context = context;
    if (jsFunction)
        frame.jsFrame->jsFunction = *jsFunction;

    QV4::Value &accumulator = frame.jsFrame->accumulator;

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

    MOTH_BEGIN_INSTR(LoadNull)
        accumulator = Encode::null();
    MOTH_END_INSTR(LoadNull)

    MOTH_BEGIN_INSTR(LoadZero)
        accumulator = Encode(static_cast<int>(0));
    MOTH_END_INSTR(LoadZero)

    MOTH_BEGIN_INSTR(LoadTrue)
        accumulator = Encode(true);
    MOTH_END_INSTR(LoadTrue)

    MOTH_BEGIN_INSTR(LoadFalse)
        accumulator = Encode(false);
    MOTH_END_INSTR(LoadFalse)

    MOTH_BEGIN_INSTR(LoadUndefined)
        accumulator = Encode::undefined();
    MOTH_END_INSTR(LoadUndefined)

    MOTH_BEGIN_INSTR(LoadInt)
        accumulator = Encode(instr.value);
    MOTH_END_INSTR(LoadInt)

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
        accumulator = loadScopedLocal(frame, instr.index, instr.scope);
    MOTH_END_INSTR(LoadScopedLocal)

    MOTH_BEGIN_INSTR(StoreScopedLocal)
        CHECK_EXCEPTION;
        storeScopedLocal(engine, frame, instr.index, instr.scope, accumulator);
    MOTH_END_INSTR(StoreScopedLocal)

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
        STORE_ACCUMULATOR(Runtime::method_loadName(engine, instr.name));
    MOTH_END_INSTR(LoadName)

    MOTH_BEGIN_INSTR(LoadGlobalLookup)
        QV4::Lookup *l = function->compilationUnit->runtimeLookups + instr.index;
        STORE_ACCUMULATOR(l->globalGetter(l, engine));
    MOTH_END_INSTR(LoadGlobalLookup)

    MOTH_BEGIN_INSTR(StoreNameStrict)
        Runtime::method_storeNameStrict(engine, instr.name, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreNameSloppy)

    MOTH_BEGIN_INSTR(StoreNameSloppy)
        Runtime::method_storeNameSloppy(engine, instr.name, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreNameSloppy)

    MOTH_BEGIN_INSTR(LoadElement)
        STORE_ACCUMULATOR(Runtime::method_loadElement(engine, STACK_VALUE(instr.base), STACK_VALUE(instr.index)));
    MOTH_END_INSTR(LoadElement)

    MOTH_BEGIN_INSTR(LoadElementA)
        STORE_ACCUMULATOR(Runtime::method_loadElement(engine, STACK_VALUE(instr.base), accumulator));
    MOTH_END_INSTR(LoadElementA)

    MOTH_BEGIN_INSTR(StoreElement)
        if (!Runtime::method_storeElement(engine, STACK_VALUE(instr.base), STACK_VALUE(instr.index), accumulator) && function->isStrict())
            engine->throwTypeError();
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreElement)

    MOTH_BEGIN_INSTR(LoadProperty)
        STORE_ACCUMULATOR(Runtime::method_loadProperty(engine, STACK_VALUE(instr.base), instr.name));
    MOTH_END_INSTR(LoadProperty)

    MOTH_BEGIN_INSTR(LoadPropertyA)
        STORE_ACCUMULATOR(Runtime::method_loadProperty(engine, accumulator, instr.name));
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
        if (!Runtime::method_storeProperty(engine, STACK_VALUE(instr.base), instr.name, accumulator) && function->isStrict())
            engine->throwTypeError();
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreProperty)

    MOTH_BEGIN_INSTR(SetLookup)
        QV4::Lookup *l = function->compilationUnit->runtimeLookups + instr.index;
        if (!l->setter(l, engine, STACK_VALUE(instr.base), accumulator) && function->isStrict())
            engine->throwTypeError();
        CHECK_EXCEPTION;
    MOTH_END_INSTR(SetLookup)

    MOTH_BEGIN_INSTR(StoreScopeObjectProperty)
        Runtime::method_storeQmlScopeObjectProperty(engine, STACK_VALUE(instr.base), instr.propertyIndex, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreScopeObjectProperty)

    MOTH_BEGIN_INSTR(LoadScopeObjectProperty)
        STORE_ACCUMULATOR(Runtime::method_loadQmlScopeObjectProperty(engine, STACK_VALUE(instr.base), instr.propertyIndex, instr.captureRequired));
    MOTH_END_INSTR(LoadScopeObjectProperty)

    MOTH_BEGIN_INSTR(StoreContextObjectProperty)
        Runtime::method_storeQmlContextObjectProperty(engine, STACK_VALUE(instr.base), instr.propertyIndex, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreContextObjectProperty)

    MOTH_BEGIN_INSTR(LoadContextObjectProperty)
        STORE_ACCUMULATOR(Runtime::method_loadQmlContextObjectProperty(engine, STACK_VALUE(instr.base), instr.propertyIndex, instr.captureRequired));
    MOTH_END_INSTR(LoadContextObjectProperty)

    MOTH_BEGIN_INSTR(LoadIdObject)
        STORE_ACCUMULATOR(Runtime::method_loadQmlIdObject(engine, STACK_VALUE(instr.base), instr.index));
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

    MOTH_BEGIN_INSTR(CallName)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        STORE_ACCUMULATOR(Runtime::method_callName(engine, instr.name, callData));
    MOTH_END_INSTR(CallName)

    MOTH_BEGIN_INSTR(CallGlobalLookup)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        STORE_ACCUMULATOR(Runtime::method_callGlobalLookup(engine, instr.index, callData));
    MOTH_END_INSTR(CallGlobalLookup)

    MOTH_BEGIN_INSTR(SetExceptionHandler)
        exceptionHandler = instr.offset ? reinterpret_cast<const uchar *>(&instr.offset) + instr.offset
                                        : nullptr;
    MOTH_END_INSTR(SetExceptionHandler)

    MOTH_BEGIN_INSTR(ThrowException)
        Runtime::method_throwException(engine, accumulator);
        goto catchException;
    MOTH_END_INSTR(ThrowException)

    MOTH_BEGIN_INSTR(GetException)
        accumulator = engine->hasException ? *engine->exceptionValue : Primitive::emptyValue();
        engine->hasException = false;
    MOTH_END_INSTR(HasException)

    MOTH_BEGIN_INSTR(SetException)
        *engine->exceptionValue = accumulator;
        engine->hasException = true;
    MOTH_END_INSTR(SetException)

    MOTH_BEGIN_INSTR(UnwindException)
        STORE_ACCUMULATOR(Runtime::method_unwindException(engine));
    MOTH_END_INSTR(UnwindException)

    MOTH_BEGIN_INSTR(PushCatchContext)
        STACK_VALUE(instr.reg) = Runtime::method_pushCatchContext(static_cast<QV4::NoThrowEngine*>(engine), instr.name);
    MOTH_END_INSTR(PushCatchContext)

    MOTH_BEGIN_INSTR(PushWithContext)
        accumulator = accumulator.toObject(engine);
        CHECK_EXCEPTION;
        STACK_VALUE(instr.reg) = Runtime::method_pushWithContext(accumulator, static_cast<QV4::NoThrowEngine*>(engine));
    MOTH_END_INSTR(PushWithContext)

    MOTH_BEGIN_INSTR(PopContext)
        Runtime::method_popContext(static_cast<QV4::NoThrowEngine*>(engine), STACK_VALUE(instr.reg));
    MOTH_END_INSTR(PopContext)

    MOTH_BEGIN_INSTR(ForeachIteratorObject)
        STORE_ACCUMULATOR(Runtime::method_foreachIterator(engine, accumulator));
    MOTH_END_INSTR(ForeachIteratorObject)

    MOTH_BEGIN_INSTR(ForeachNextPropertyName)
        STORE_ACCUMULATOR(Runtime::method_foreachNextPropertyName(accumulator));
    MOTH_END_INSTR(ForeachNextPropertyName)

    MOTH_BEGIN_INSTR(DeleteMember)
        if (!Runtime::method_deleteMember(engine, STACK_VALUE(instr.base), instr.member)) {
            if (function->isStrict()) {
                engine->throwTypeError();
                goto catchException;
            }
            accumulator = Encode(false);
        } else {
            accumulator = Encode(true);
        }
    MOTH_END_INSTR(DeleteMember)

    MOTH_BEGIN_INSTR(DeleteSubscript)
        if (!Runtime::method_deleteElement(engine, STACK_VALUE(instr.base), STACK_VALUE(instr.index))) {
            if (function->isStrict()) {
                engine->throwTypeError();
                goto catchException;
            }
            accumulator = Encode(false);
        } else {
            accumulator = Encode(true);
        }
    MOTH_END_INSTR(DeleteSubscript)

    MOTH_BEGIN_INSTR(DeleteName)
        if (!Runtime::method_deleteName(engine, instr.name)) {
            if (function->isStrict()) {
                QString name = function->compilationUnit->runtimeStrings[instr.name]->toQString();
                engine->throwSyntaxError(QStringLiteral("Can't delete property %1").arg(name));
                goto catchException;
            }
            accumulator = Encode(false);
        } else {
            accumulator = Encode(true);
        }
    MOTH_END_INSTR(DeleteName)

    MOTH_BEGIN_INSTR(TypeofName)
        STORE_ACCUMULATOR(Runtime::method_typeofName(engine, instr.name));
    MOTH_END_INSTR(TypeofName)

    MOTH_BEGIN_INSTR(TypeofValue)
        STORE_ACCUMULATOR(Runtime::method_typeofValue(engine, accumulator));
    MOTH_END_INSTR(TypeofValue)

    MOTH_BEGIN_INSTR(DeclareVar)
        Runtime::method_declareVar(engine, instr.isDeletable, instr.varName);
    MOTH_END_INSTR(DeclareVar)

    MOTH_BEGIN_INSTR(DefineArray)
        QV4::Value *args = stack + instr.args.stackSlot();
        STORE_ACCUMULATOR(Runtime::method_arrayLiteral(engine, args, instr.argc));
    MOTH_END_INSTR(DefineArray)

    MOTH_BEGIN_INSTR(DefineObjectLiteral)
        QV4::Value *args = stack + instr.args.stackSlot();
        STORE_ACCUMULATOR(Runtime::method_objectLiteral(engine, args, instr.internalClassId, instr.arrayValueCount, instr.arrayGetterSetterCountAndFlags));
    MOTH_END_INSTR(DefineObjectLiteral)

    MOTH_BEGIN_INSTR(CreateMappedArgumentsObject)
        STORE_ACCUMULATOR(Runtime::method_createMappedArgumentsObject(engine));
    MOTH_END_INSTR(CreateMappedArgumentsObject)

    MOTH_BEGIN_INSTR(CreateUnmappedArgumentsObject)
        STORE_ACCUMULATOR(Runtime::method_createUnmappedArgumentsObject(engine));
    MOTH_END_INSTR(CreateUnmappedArgumentsObject)

    MOTH_BEGIN_INSTR(ConvertThisToObject)
        Value *t = &stack[-(int)function->nFormals - 1];
        if (!t->isObject()) {
            if (t->isNullOrUndefined()) {
                *t = engine->globalObject->asReturnedValue();
            } else {
                *t = t->toObject(engine)->asReturnedValue();
            }
        }
        CHECK_EXCEPTION;
    MOTH_END_INSTR(ConvertThisToObject)

    MOTH_BEGIN_INSTR(CreateValue)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        STORE_ACCUMULATOR(Runtime::method_constructValue(engine, STACK_VALUE(instr.func), callData));
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

    MOTH_BEGIN_INSTR(CreateName)
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData.stackSlot());
        callData->tag = quint32(Value::ValueTypeInternal::Integer);
        callData->argc = instr.argc;
        callData->thisObject = QV4::Primitive::undefinedValue();
        STORE_ACCUMULATOR(Runtime::method_constructName(engine, instr.name, callData));
    MOTH_END_INSTR(CreateName)

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

    MOTH_BEGIN_INSTR(CmpJmpEqNull)
        if (accumulator.isNullOrUndefined())
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(CmpJmpEqNull)

    MOTH_BEGIN_INSTR(CmpJmpNeNull)
        if (!accumulator.isNullOrUndefined())
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(CmpJmpNeNull)

    MOTH_BEGIN_INSTR(CmpJmpEq)
        const Value lhs = STACK_VALUE(instr.lhs);
        if (Q_LIKELY(lhs.asReturnedValue() == accumulator.asReturnedValue())) {
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } else if (Q_LIKELY(lhs.isInteger() && accumulator.isInteger())) {
            if (lhs.int_32() == accumulator.int_32())
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } else {
            if (compareEqual(lhs, accumulator))
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        }
    MOTH_END_INSTR(CmpJmpEq)

    MOTH_BEGIN_INSTR(CmpJmpNe)
        const Value lhs = STACK_VALUE(instr.lhs);
        if (Q_LIKELY(lhs.isInteger() && accumulator.isInteger())) {
            if (lhs.int_32() != accumulator.int_32())
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        } else {
            if (!compareEqual(lhs, accumulator))
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
        if (STACK_VALUE(instr.lhs).rawValue() == accumulator.rawValue() && !accumulator.isNaN())
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        else if (RuntimeHelpers::strictEqual(STACK_VALUE(instr.lhs), accumulator))
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(JumpStrictEqual)

    MOTH_BEGIN_INSTR(JumpStrictNotEqual)
        if (STACK_VALUE(instr.lhs).rawValue() != accumulator.rawValue() || accumulator.isNaN()) {
            if (!RuntimeHelpers::strictEqual(STACK_VALUE(instr.lhs), accumulator))
                code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
        }
    MOTH_END_INSTR(JumpStrictNotEqual)

    MOTH_BEGIN_INSTR(JumpStrictNotEqualStackSlotInt)
        if (STACK_VALUE(instr.lhs).int_32() != instr.rhs || STACK_VALUE(instr.lhs).isUndefined())
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(JumpStrictNotEqualStackSlotInt)

    MOTH_BEGIN_INSTR(JumpStrictEqualStackSlotInt)
        if (STACK_VALUE(instr.lhs).int_32() == instr.rhs && !STACK_VALUE(instr.lhs).isUndefined())
            code = reinterpret_cast<const uchar *>(&instr.offset) + instr.offset;
    MOTH_END_INSTR(JumpStrictNotEqualStackSlotInt)

    MOTH_BEGIN_INSTR(UNot)
        if (accumulator.integerCompatible()) {
            STORE_ACCUMULATOR(Encode(!static_cast<bool>(accumulator.int_32())))
        } else {
            STORE_ACCUMULATOR(Encode(!accumulator.toBoolean()));
        }
    MOTH_END_INSTR(UNot)

    MOTH_BEGIN_INSTR(UPlus)
        if (!accumulator.isNumber())
            STORE_ACCUMULATOR(Encode(accumulator.toNumberImpl()));
    MOTH_END_INSTR(UPlus)

    MOTH_BEGIN_INSTR(UMinus)
        if (Q_LIKELY(accumulator.integerCompatible() && accumulator.int_32() != 0 &&
                accumulator.int_32() != std::numeric_limits<int>::min())) {
            accumulator = sub_int32(0, accumulator.int_32());
        } else {
            STORE_ACCUMULATOR(Encode(-accumulator.toNumber()));
        }
    MOTH_END_INSTR(UMinus)

    MOTH_BEGIN_INSTR(UCompl)
        if (Q_LIKELY(accumulator.integerCompatible())) {
            accumulator.setInt_32(~accumulator.int_32());
        } else {
            STORE_ACCUMULATOR(Encode((int)~accumulator.toInt32()));
        }
    MOTH_END_INSTR(UCompl)

    MOTH_BEGIN_INSTR(Increment)
        if (Q_LIKELY(accumulator.integerCompatible())) {
            accumulator = add_int32(accumulator.int_32(), 1);
        } else if (accumulator.isDouble()) {
            accumulator = QV4::Encode(accumulator.doubleValue() + 1.);
        } else {
            STORE_ACCUMULATOR(Encode(accumulator.toNumberImpl() + 1.));
        }
    MOTH_END_INSTR(Increment)

    MOTH_BEGIN_INSTR(Decrement)
        if (Q_LIKELY(accumulator.integerCompatible())) {
            accumulator = sub_int32(accumulator.int_32(), 1);
        } else if (accumulator.isDouble()) {
            accumulator = QV4::Encode(accumulator.doubleValue() - 1.);
        } else {
            STORE_ACCUMULATOR(Encode(accumulator.toNumberImpl() - 1.));
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
        } else if (lhs.isNumber() && accumulator.isNumber()) {
            accumulator = Encode(lhs.asDouble() + accumulator.asDouble());
        } else {
            STORE_ACCUMULATOR(Runtime::method_add(engine, lhs, accumulator));
        }
    MOTH_END_INSTR(Add)

    MOTH_BEGIN_INSTR(Sub)
        QV4::Value lhs = STACK_VALUE(instr.lhs);
        if (Q_LIKELY(Value::integerCompatible(lhs, accumulator))) {
            accumulator = sub_int32(lhs.int_32(), accumulator.int_32());
        } else if (lhs.isNumber() && accumulator.isNumber()) {
            accumulator = Encode(lhs.asDouble() - accumulator.asDouble());
        } else {
            STORE_ACCUMULATOR(Runtime::method_sub(lhs, accumulator));
        }
    MOTH_END_INSTR(Sub)

    MOTH_BEGIN_INSTR(Mul)
        QV4::Value lhs = STACK_VALUE(instr.lhs);
        if (Q_LIKELY(Value::integerCompatible(lhs, accumulator))) {
            accumulator = mul_int32(lhs.int_32(), accumulator.int_32());
        } else if (lhs.isNumber() && accumulator.isNumber()) {
            accumulator = Encode(lhs.asDouble() * accumulator.asDouble());
        } else {
            STORE_ACCUMULATOR(Runtime::method_mul(lhs, accumulator));
        }
    MOTH_END_INSTR(Mul)

    MOTH_BEGIN_INSTR(BitAnd)
        STORE_ACCUMULATOR(Encode((int)(STACK_VALUE(instr.lhs).toInt32() & accumulator.toInt32())));
    MOTH_END_INSTR(BitAnd)

    MOTH_BEGIN_INSTR(BitOr)
        STORE_ACCUMULATOR(Encode((int)(STACK_VALUE(instr.lhs).toInt32() | accumulator.toInt32())));
    MOTH_END_INSTR(BitOr)

    MOTH_BEGIN_INSTR(BitXor)
        STORE_ACCUMULATOR(Encode((int)(STACK_VALUE(instr.lhs).toInt32() ^ accumulator.toInt32())));
    MOTH_END_INSTR(BitXor)

    MOTH_BEGIN_INSTR(Shr)
        STORE_ACCUMULATOR(Encode((int)(STACK_VALUE(instr.lhs).toInt32() >> (accumulator.toInt32() & 0x1f))));
    MOTH_END_INSTR(Shr)

    MOTH_BEGIN_INSTR(Shl)
        STORE_ACCUMULATOR(Encode((int)(STACK_VALUE(instr.lhs).toInt32() << (accumulator.toInt32() & 0x1f))));
    MOTH_END_INSTR(Shl)

    MOTH_BEGIN_INSTR(BitAndConst)
        STORE_ACCUMULATOR(QV4::Primitive::fromInt32(accumulator.toInt32() & instr.rhs));
    MOTH_END_INSTR(BitAndConst)

    MOTH_BEGIN_INSTR(BitOrConst)
        STORE_ACCUMULATOR(QV4::Primitive::fromInt32(accumulator.toInt32() | instr.rhs));
    MOTH_END_INSTR(BitOrConst)

    MOTH_BEGIN_INSTR(BitXorConst)
        STORE_ACCUMULATOR(QV4::Primitive::fromInt32(accumulator.toInt32() ^ instr.rhs));
    MOTH_END_INSTR(BitXorConst)

    MOTH_BEGIN_INSTR(ShrConst)
        STORE_ACCUMULATOR(QV4::Primitive::fromInt32(accumulator.toInt32() >> instr.rhs));
    MOTH_END_INSTR(ShrConst)

    MOTH_BEGIN_INSTR(ShlConst)
        STORE_ACCUMULATOR(QV4::Primitive::fromInt32(accumulator.toInt32() << instr.rhs));
    MOTH_END_INSTR(ShlConst)

    MOTH_BEGIN_INSTR(BinopContext)
        QV4::Runtime::BinaryOperationContext op = *reinterpret_cast<QV4::Runtime::BinaryOperationContext *>(reinterpret_cast<char *>(&engine->runtime.runtimeMethods[instr.alu]));
        STORE_ACCUMULATOR(op(engine, STACK_VALUE(instr.lhs), accumulator));
    MOTH_END_INSTR(BinopContext)

    MOTH_BEGIN_INSTR(Ret)
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

    MOTH_BEGIN_INSTR(LoadQmlContext)
        STACK_VALUE(instr.result) = Runtime::method_loadQmlContext(static_cast<QV4::NoThrowEngine*>(engine));
    MOTH_END_INSTR(LoadQmlContext)

    MOTH_BEGIN_INSTR(LoadQmlImportedScripts)
        STACK_VALUE(instr.result) = Runtime::method_loadQmlImportedScripts(static_cast<QV4::NoThrowEngine*>(engine));
    MOTH_END_INSTR(LoadQmlImportedScripts)

    MOTH_BEGIN_INSTR(LoadQmlSingleton)
        accumulator = Runtime::method_loadQmlSingleton(static_cast<QV4::NoThrowEngine*>(engine), instr.name);
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
    engine->jsStackTop = jsStackTop;

    if (function->hasQmlDependencies) {
        Q_ASSERT(context->type == Heap::ExecutionContext::Type_QmlContext);
        QQmlPropertyCapture::registerQmlDependencies(static_cast<Heap::QmlContext *>(context), engine, function->compiledFunction);
    }

    return accumulator.asReturnedValue();
}
