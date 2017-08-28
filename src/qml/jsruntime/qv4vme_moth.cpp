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

    const int lineNumber = frame->lineNumber();
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

Q_NEVER_INLINE static void debug_slowPath(QV4::ExecutionEngine *engine)
{
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

#define MOTH_BEGIN_INSTR_COMMON(instr) \
    { \
        INSTR_##instr(MOTH_DECODE)

#ifdef COUNT_INSTRUCTIONS
#  define MOTH_BEGIN_INSTR(instr) op_##I: \
    instrCount.hit(static_cast<int>(Instr::Type::instr)); \
    MOTH_BEGIN_INSTR_COMMON(instr)
#else // !COUNT_INSTRUCTIONS
#  define MOTH_BEGIN_INSTR(instr) \
    MOTH_BEGIN_INSTR_COMMON(instr)
#endif // COUNT_INSTRUCTIONS

#define MOTH_END_INSTR(instr) \
        MOTH_DISPATCH() \
    }

#define STACK_VALUE(temp) stack[temp]

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
                                    ReturnedValue value)
{
    auto ctxt = getScope(frame.jsFrame, scope);
    Q_ASSERT(ctxt->type == QV4::Heap::ExecutionContext::Type_CallContext);
    auto cc = static_cast<Heap::CallContext *>(ctxt);

    QV4::WriteBarrier::write(engine, cc, cc->locals.values + index, Primitive::fromReturnedValue(value));
}

static inline const QV4::Value &constant(Function *function, int index)
{
    return function->compilationUnit->constants[index];
}


static bool compareEqual(Value lhs, Value rhs)
{
  redo:
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
        goto redo;
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

static bool compareEqualInt(Value &accumulator, Value lhs, int rhs)
{
  redo:
    switch (lhs.quickType()) {
    case Value::QT_ManagedOrUndefined:
        if (lhs.isUndefined())
            return false;
        Q_FALLTHROUGH();
    case Value::QT_ManagedOrUndefined1:
    case Value::QT_ManagedOrUndefined2:
    case Value::QT_ManagedOrUndefined3:
        // LHS: Managed
        if (lhs.m()->vtable()->isString)
            return RuntimeHelpers::stringToNumber(static_cast<String &>(lhs).toQString()) == rhs;
        accumulator = lhs;
        lhs = Primitive::fromReturnedValue(RuntimeHelpers::objectDefaultValue(&static_cast<QV4::Object &>(accumulator), PREFERREDTYPE_HINT));
        goto redo;
    case Value::QT_Empty:
        Q_UNREACHABLE();
    case Value::QT_Null:
        return false;
    case Value::QT_Bool:
    case Value::QT_Int:
        return lhs.int_32() == rhs;
    default: // double
        return lhs.doubleValue() == rhs;
    }
}

#define STORE_IP() frame.instructionPointer = code;
#define STORE_ACC() accumulator = acc;
#define ACC Primitive::fromReturnedValue(acc)
#define VALUE_TO_INT(i, val) \
    int i; \
    do { \
        if (Q_LIKELY(val.integerCompatible())) { \
            i = val.int_32(); \
        } else { \
            double d; \
            if (val.isDouble()) \
                d = val.doubleValue(); \
            else { \
                d = val.toNumberImpl(); \
                CHECK_EXCEPTION; \
            } \
            i = Double::toInt32(d); \
        } \
    } while (false)


QV4::ReturnedValue VME::exec(const FunctionObject *jsFunction, CallData *callData, Heap::ExecutionContext *context, QV4::Function *function)
{
    qt_v4ResolvePendingBreakpointsHook();

    MOTH_JUMP_TABLE;

    ExecutionEngine *engine = function->internalClass->engine;
    Profiling::FunctionCallProfiler(engine, function);

    Value *jsStackTop = engine->jsStackTop;
    engine->jsStackTop = reinterpret_cast<QV4::Value *>(callData) + 2 + (int)function->nFormals;
    for (int i = callData->argc; i < (int)function->nFormals; ++i)
        callData->args[i] = Encode::undefined();

    CppStackFrame frame;
    frame.parent = engine->currentStackFrame;
    frame.v4Function = function;
    frame.instructionPointer = function->codeData;
    engine->currentStackFrame = &frame;

    QV4::Value *stack = nullptr;
    const uchar *exceptionHandler = 0;

    stack = engine->jsAlloca(function->compiledFunction->nRegisters + sizeof(JSStackFrame)/sizeof(QV4::Value));
    frame.jsFrame = reinterpret_cast<JSStackFrame *>(stack);
    frame.jsFrame->context = context;
    if (jsFunction)
        frame.jsFrame->jsFunction = *jsFunction;

    QV4::Value &accumulator = frame.jsFrame->accumulator;
    QV4::ReturnedValue acc = Encode::undefined();

    if (QV4::Debugging::Debugger *debugger = engine->debugger())
        debugger->enteringFunction();

    const uchar *code = function->codeData;

    for (;;) {
    MOTH_DISPATCH()

    MOTH_BEGIN_INSTR(LoadConst)
        acc = constant(function, index).asReturnedValue();
    MOTH_END_INSTR(LoadConst)

    MOTH_BEGIN_INSTR(LoadNull)
        acc = Encode::null();
    MOTH_END_INSTR(LoadNull)

    MOTH_BEGIN_INSTR(LoadZero)
        acc = Encode(static_cast<int>(0));
    MOTH_END_INSTR(LoadZero)

    MOTH_BEGIN_INSTR(LoadTrue)
        acc = Encode(true);
    MOTH_END_INSTR(LoadTrue)

    MOTH_BEGIN_INSTR(LoadFalse)
        acc = Encode(false);
    MOTH_END_INSTR(LoadFalse)

    MOTH_BEGIN_INSTR(LoadUndefined)
        acc = Encode::undefined();
    MOTH_END_INSTR(LoadUndefined)

    MOTH_BEGIN_INSTR(LoadInt)
        acc = Encode(value);
    MOTH_END_INSTR(LoadInt)

    MOTH_BEGIN_INSTR(MoveConst)
        STACK_VALUE(destTemp) = constant(function, constIndex);
    MOTH_END_INSTR(MoveConst)

    MOTH_BEGIN_INSTR(LoadReg)
        acc = STACK_VALUE(reg).asReturnedValue();
    MOTH_END_INSTR(LoadReg)

    MOTH_BEGIN_INSTR(StoreReg)
        STACK_VALUE(reg) = acc;
    MOTH_END_INSTR(StoreReg)

    MOTH_BEGIN_INSTR(MoveReg)
        STACK_VALUE(destReg) = STACK_VALUE(srcReg);
    MOTH_END_INSTR(MoveReg)

    MOTH_BEGIN_INSTR(LoadLocal)
        auto cc = static_cast<Heap::CallContext *>(frame.jsFrame->context.m());
        acc = cc->locals[index].asReturnedValue();
    MOTH_END_INSTR(LoadLocal)

    MOTH_BEGIN_INSTR(StoreLocal)
        CHECK_EXCEPTION;
        auto cc = static_cast<Heap::CallContext *>(frame.jsFrame->context.m());
        QV4::WriteBarrier::write(engine, cc, cc->locals.values + index, ACC);
    MOTH_END_INSTR(StoreLocal)

    MOTH_BEGIN_INSTR(LoadScopedLocal)
        acc = loadScopedLocal(frame, index, scope);
    MOTH_END_INSTR(LoadScopedLocal)

    MOTH_BEGIN_INSTR(StoreScopedLocal)
        CHECK_EXCEPTION;
        storeScopedLocal(engine, frame, index, scope, acc);
    MOTH_END_INSTR(StoreScopedLocal)

    MOTH_BEGIN_INSTR(LoadRuntimeString)
        acc = function->compilationUnit->runtimeStrings[stringId]->asReturnedValue();
    MOTH_END_INSTR(LoadRuntimeString)

    MOTH_BEGIN_INSTR(LoadRegExp)
        acc = function->compilationUnit->runtimeRegularExpressions[regExpId].asReturnedValue();
    MOTH_END_INSTR(LoadRegExp)

    MOTH_BEGIN_INSTR(LoadClosure)
        acc = Runtime::method_closure(engine, value);
    MOTH_END_INSTR(LoadClosure)

    MOTH_BEGIN_INSTR(LoadName)
        STORE_IP();
        acc = Runtime::method_loadName(engine, name);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(LoadName)

    MOTH_BEGIN_INSTR(LoadGlobalLookup)
        QV4::Lookup *l = function->compilationUnit->runtimeLookups + index;
        acc = l->globalGetter(l, engine);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(LoadGlobalLookup)

    MOTH_BEGIN_INSTR(StoreNameStrict)
        STORE_IP();
        STORE_ACC();
        Runtime::method_storeNameStrict(engine, name, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreNameSloppy)

    MOTH_BEGIN_INSTR(StoreNameSloppy)
        STORE_IP();
        STORE_ACC();
        Runtime::method_storeNameSloppy(engine, name, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreNameSloppy)

    MOTH_BEGIN_INSTR(LoadElement)
        STORE_IP();
        acc = Runtime::method_loadElement(engine, STACK_VALUE(base), STACK_VALUE(index));
        CHECK_EXCEPTION;
    MOTH_END_INSTR(LoadElement)

    MOTH_BEGIN_INSTR(LoadElementA)
        STORE_IP();
        STORE_ACC();
        acc = Runtime::method_loadElement(engine, STACK_VALUE(base), accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(LoadElementA)

    MOTH_BEGIN_INSTR(StoreElement)
        STORE_IP();
        STORE_ACC();
        if (!Runtime::method_storeElement(engine, STACK_VALUE(base), STACK_VALUE(index), accumulator) && function->isStrict())
            engine->throwTypeError();
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreElement)

    MOTH_BEGIN_INSTR(LoadProperty)
        STORE_IP();
        acc = Runtime::method_loadProperty(engine, STACK_VALUE(base), name);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(LoadProperty)

    MOTH_BEGIN_INSTR(LoadPropertyA)
        STORE_IP();
        STORE_ACC();
        acc = Runtime::method_loadProperty(engine, accumulator, name);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(LoadPropertyA)

    MOTH_BEGIN_INSTR(GetLookup)
        STORE_IP();
        QV4::Lookup *l = function->compilationUnit->runtimeLookups + index;
        acc = l->getter(l, engine, STACK_VALUE(base));
        CHECK_EXCEPTION;
    MOTH_END_INSTR(GetLookup)

    MOTH_BEGIN_INSTR(GetLookupA)
        STORE_IP();
        STORE_ACC();
        QV4::Lookup *l = function->compilationUnit->runtimeLookups + index;
        acc = l->getter(l, engine, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(GetLookupA)

    MOTH_BEGIN_INSTR(StoreProperty)
        STORE_IP();
        STORE_ACC();
        if (!Runtime::method_storeProperty(engine, STACK_VALUE(base), name, accumulator) && function->isStrict())
            engine->throwTypeError();
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreProperty)

    MOTH_BEGIN_INSTR(SetLookup)
        STORE_IP();
        STORE_ACC();
        QV4::Lookup *l = function->compilationUnit->runtimeLookups + index;
        if (!l->setter(l, engine, STACK_VALUE(base), accumulator) && function->isStrict())
            engine->throwTypeError();
        CHECK_EXCEPTION;
    MOTH_END_INSTR(SetLookup)

    MOTH_BEGIN_INSTR(StoreScopeObjectProperty)
        STORE_ACC();
        Runtime::method_storeQmlScopeObjectProperty(engine, STACK_VALUE(base), propertyIndex, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreScopeObjectProperty)

    MOTH_BEGIN_INSTR(LoadScopeObjectProperty)
        STORE_IP();
        acc = Runtime::method_loadQmlScopeObjectProperty(engine, STACK_VALUE(base), propertyIndex, captureRequired);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(LoadScopeObjectProperty)

    MOTH_BEGIN_INSTR(StoreContextObjectProperty)
        STORE_IP();
        STORE_ACC();
        Runtime::method_storeQmlContextObjectProperty(engine, STACK_VALUE(base), propertyIndex, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(StoreContextObjectProperty)

    MOTH_BEGIN_INSTR(LoadContextObjectProperty)
        STORE_IP();
        acc = Runtime::method_loadQmlContextObjectProperty(engine, STACK_VALUE(base), propertyIndex, captureRequired);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(LoadContextObjectProperty)

    MOTH_BEGIN_INSTR(LoadIdObject)
        STORE_IP();
        acc = Runtime::method_loadQmlIdObject(engine, STACK_VALUE(base), index);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(LoadIdObject)

    MOTH_BEGIN_INSTR(CallValue)
        STORE_IP();
        STORE_ACC();
        QV4::CallData *cData = reinterpret_cast<QV4::CallData *>(stack + callData);
        acc = Runtime::method_callValue(engine, accumulator, cData);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallValue)

    MOTH_BEGIN_INSTR(CallProperty)
        STORE_IP();
        QV4::CallData *cData = reinterpret_cast<QV4::CallData *>(stack + callData);
        cData->thisObject = STACK_VALUE(base);
        acc = Runtime::method_callProperty(engine, name, cData);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallProperty)

    MOTH_BEGIN_INSTR(CallPropertyLookup)
        STORE_IP();
        QV4::CallData *cData = reinterpret_cast<QV4::CallData *>(stack + callData);
        cData->thisObject = STACK_VALUE(base);
        acc = Runtime::method_callPropertyLookup(engine, lookupIndex, cData);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallPropertyLookup)

    MOTH_BEGIN_INSTR(CallElement)
        STORE_IP();
        QV4::CallData *cData = reinterpret_cast<QV4::CallData *>(stack + callData);
        cData->thisObject = STACK_VALUE(base);
        acc = Runtime::method_callElement(engine, STACK_VALUE(index), cData);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallElement)

    MOTH_BEGIN_INSTR(CallName)
        STORE_IP();
        QV4::CallData *cData = reinterpret_cast<QV4::CallData *>(stack + callData);
        acc = Runtime::method_callName(engine, name, cData);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallName)

    MOTH_BEGIN_INSTR(CallPossiblyDirectEval)
        STORE_IP();
        QV4::CallData *cData = reinterpret_cast<QV4::CallData *>(stack + callData);
        acc = Runtime::method_callPossiblyDirectEval(engine, cData);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallPossiblyDirectEval)

    MOTH_BEGIN_INSTR(CallGlobalLookup)
        STORE_IP();
        QV4::CallData *cData = reinterpret_cast<QV4::CallData *>(stack + callData);
        acc = Runtime::method_callGlobalLookup(engine, index, cData);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallGlobalLookup)

    MOTH_BEGIN_INSTR(SetExceptionHandler)
        exceptionHandler = offset ? code + offset : nullptr;
    MOTH_END_INSTR(SetExceptionHandler)

    MOTH_BEGIN_INSTR(ThrowException)
        STORE_IP();
        STORE_ACC();
        Runtime::method_throwException(engine, accumulator);
        goto catchException;
    MOTH_END_INSTR(ThrowException)

    MOTH_BEGIN_INSTR(GetException)
        acc = engine->hasException ? engine->exceptionValue->asReturnedValue() : Primitive::emptyValue().asReturnedValue();
        engine->hasException = false;
    MOTH_END_INSTR(HasException)

    MOTH_BEGIN_INSTR(SetException)
        *engine->exceptionValue = acc;
        engine->hasException = true;
    MOTH_END_INSTR(SetException)

    MOTH_BEGIN_INSTR(PushCatchContext)
        STACK_VALUE(reg) = STACK_VALUE(JSStackFrame::Context);
        ExecutionContext *c = static_cast<ExecutionContext *>(stack + JSStackFrame::Context);
        STACK_VALUE(JSStackFrame::Context) = Runtime::method_createCatchContext(c, name);
    MOTH_END_INSTR(PushCatchContext)

    MOTH_BEGIN_INSTR(PushWithContext)
        STORE_IP();
        STORE_ACC();
        accumulator = accumulator.toObject(engine);
        CHECK_EXCEPTION;
        STACK_VALUE(reg) = STACK_VALUE(JSStackFrame::Context);
        ExecutionContext *c = static_cast<ExecutionContext *>(stack + JSStackFrame::Context);
        STACK_VALUE(JSStackFrame::Context) = Runtime::method_createWithContext(c, accumulator);
    MOTH_END_INSTR(PushWithContext)

    MOTH_BEGIN_INSTR(PopContext)
        STACK_VALUE(JSStackFrame::Context) = STACK_VALUE(reg);
    MOTH_END_INSTR(PopContext)

    MOTH_BEGIN_INSTR(ForeachIteratorObject)
        STORE_ACC();
        acc = Runtime::method_foreachIterator(engine, accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(ForeachIteratorObject)

    MOTH_BEGIN_INSTR(ForeachNextPropertyName)
        STORE_ACC();
        acc = Runtime::method_foreachNextPropertyName(accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(ForeachNextPropertyName)

    MOTH_BEGIN_INSTR(DeleteMember)
        if (!Runtime::method_deleteMember(engine, STACK_VALUE(base), member)) {
            if (function->isStrict()) {
                STORE_IP();
                engine->throwTypeError();
                goto catchException;
            }
            acc = Encode(false);
        } else {
            acc = Encode(true);
        }
    MOTH_END_INSTR(DeleteMember)

    MOTH_BEGIN_INSTR(DeleteSubscript)
        if (!Runtime::method_deleteElement(engine, STACK_VALUE(base), STACK_VALUE(index))) {
            if (function->isStrict()) {
                STORE_IP();
                engine->throwTypeError();
                goto catchException;
            }
            acc = Encode(false);
        } else {
            acc = Encode(true);
        }
    MOTH_END_INSTR(DeleteSubscript)

    MOTH_BEGIN_INSTR(DeleteName)
        if (!Runtime::method_deleteName(engine, name)) {
            if (function->isStrict()) {
                STORE_IP();
                QString n = function->compilationUnit->runtimeStrings[name]->toQString();
                engine->throwSyntaxError(QStringLiteral("Can't delete property %1").arg(n));
                goto catchException;
            }
            acc = Encode(false);
        } else {
            acc = Encode(true);
        }
    MOTH_END_INSTR(DeleteName)

    MOTH_BEGIN_INSTR(TypeofName)
        acc = Runtime::method_typeofName(engine, name);
    MOTH_END_INSTR(TypeofName)

    MOTH_BEGIN_INSTR(TypeofValue)
        STORE_ACC();
        acc = Runtime::method_typeofValue(engine, accumulator);
    MOTH_END_INSTR(TypeofValue)

    MOTH_BEGIN_INSTR(DeclareVar)
        Runtime::method_declareVar(engine, isDeletable, varName);
    MOTH_END_INSTR(DeclareVar)

    MOTH_BEGIN_INSTR(DefineArray)
        QV4::Value *arguments = stack + args;
        acc = Runtime::method_arrayLiteral(engine, arguments, argc);
    MOTH_END_INSTR(DefineArray)

    MOTH_BEGIN_INSTR(DefineObjectLiteral)
        QV4::Value *arguments = stack + args;
        acc = Runtime::method_objectLiteral(engine, arguments, internalClassId, arrayValueCount, arrayGetterSetterCountAndFlags);
    MOTH_END_INSTR(DefineObjectLiteral)

    MOTH_BEGIN_INSTR(CreateMappedArgumentsObject)
        acc = Runtime::method_createMappedArgumentsObject(engine);
    MOTH_END_INSTR(CreateMappedArgumentsObject)

    MOTH_BEGIN_INSTR(CreateUnmappedArgumentsObject)
        acc = Runtime::method_createUnmappedArgumentsObject(engine);
    MOTH_END_INSTR(CreateUnmappedArgumentsObject)

    MOTH_BEGIN_INSTR(ConvertThisToObject)
        Value *t = &stack[-(int)function->nFormals - 1];
        if (!t->isObject()) {
            if (t->isNullOrUndefined()) {
                *t = engine->globalObject->asReturnedValue();
            } else {
                *t = t->toObject(engine)->asReturnedValue();
                CHECK_EXCEPTION;
            }
        }
    MOTH_END_INSTR(ConvertThisToObject)

    MOTH_BEGIN_INSTR(Construct)
        STORE_IP();
        QV4::CallData *cData = reinterpret_cast<QV4::CallData *>(stack + callData);
        acc = Runtime::method_construct(engine, STACK_VALUE(func), cData);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(Construct)

    MOTH_BEGIN_INSTR(Jump)
        code += offset;
    MOTH_END_INSTR(Jump)

    MOTH_BEGIN_INSTR(JumpEq)
        if ((ACC.integerCompatible() && ACC.int_32()) || ACC.toBoolean())
            code += offset;
    MOTH_END_INSTR(JumpEq)

    MOTH_BEGIN_INSTR(JumpNe)
        if ((ACC.integerCompatible() && !ACC.int_32()) || !ACC.toBoolean())
            code += offset;
    MOTH_END_INSTR(JumpNe)

    MOTH_BEGIN_INSTR(CmpJmpEqNull)
        if (ACC.isNullOrUndefined())
            code += offset;
    MOTH_END_INSTR(CmpJmpEqNull)

    MOTH_BEGIN_INSTR(CmpJmpNeNull)
        if (!ACC.isNullOrUndefined())
            code += offset;
    MOTH_END_INSTR(CmpJmpNeNull)

    MOTH_BEGIN_INSTR(CmpJmpEqInt)
        if (ACC.isIntOrBool()) {
            if (ACC.int_32() == lhs)
                code += offset;
        } else {
            if (compareEqualInt(accumulator, ACC, lhs))
                code += offset;
        }
    MOTH_END_INSTR(CmpJmpEqInt)

    MOTH_BEGIN_INSTR(CmpJmpNeInt)
        if (ACC.isIntOrBool()) {
            if (ACC.int_32() != lhs)
            code += offset;
        } else {
            if (!compareEqualInt(accumulator, ACC, lhs))
                code += offset;
        }
    MOTH_END_INSTR(CmpJmpNeInt)

    MOTH_BEGIN_INSTR(CmpJmpEq)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(left.asReturnedValue() == ACC.asReturnedValue())) {
            code += offset;
        } else if (Q_LIKELY(left.isInteger() && ACC.isInteger())) {
            if (left.int_32() == ACC.int_32())
                code += offset;
        } else {
            STORE_ACC();
            if (compareEqual(left, accumulator))
                code += offset;
        }
    MOTH_END_INSTR(CmpJmpEq)

    MOTH_BEGIN_INSTR(CmpJmpNe)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(left.isInteger() && ACC.isInteger())) {
            if (left.int_32() != ACC.int_32())
                code += offset;
        } else {
            STORE_ACC();
            if (!compareEqual(left, accumulator))
                code += offset;
        }
    MOTH_END_INSTR(CmpJmpNe)

    MOTH_BEGIN_INSTR(CmpJmpGt)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(left.isInteger() && ACC.isInteger())) {
            if (left.int_32() > ACC.int_32())
                code += offset;
        } else if (left.isNumber() && ACC.isNumber()) {
            if (left.asDouble() > ACC.asDouble())
                code += offset;
        } else {
            STORE_ACC();
            if (Runtime::method_compareGreaterThan(left, accumulator))
                code += offset;
        }
    MOTH_END_INSTR(CmpJmpGt)

    MOTH_BEGIN_INSTR(CmpJmpGe)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(left.isInteger() && ACC.isInteger())) {
            if (left.int_32() >= ACC.int_32())
                code += offset;
        } else if (left.isNumber() && ACC.isNumber()) {
            if (left.asDouble() >= ACC.asDouble())
                code += offset;
        } else {
            STORE_ACC();
            if (Runtime::method_compareGreaterEqual(left, accumulator))
                code += offset;
        }
    MOTH_END_INSTR(CmpJmpGe)

    MOTH_BEGIN_INSTR(CmpJmpLt)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(left.isInteger() && ACC.isInteger())) {
            if (left.int_32() < ACC.int_32())
                code += offset;
        } else if (left.isNumber() && ACC.isNumber()) {
            if (left.asDouble() < ACC.asDouble())
                code += offset;
        } else {
            STORE_ACC();
            if (Runtime::method_compareLessThan(left, accumulator))
                code += offset;
        }
    MOTH_END_INSTR(CmpJmpLt)

    MOTH_BEGIN_INSTR(CmpJmpLe)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(left.isInteger() && ACC.isInteger())) {
            if (left.int_32() <= ACC.int_32())
                code += offset;
        } else if (left.isNumber() && ACC.isNumber()) {
            if (left.asDouble() <= ACC.asDouble())
                code += offset;
        } else {
            STORE_ACC();
            if (Runtime::method_compareLessEqual(left, accumulator))
                code += offset;
        }
    MOTH_END_INSTR(CmpJmpLe)

    MOTH_BEGIN_INSTR(JumpStrictEqual)
        if (STACK_VALUE(lhs).rawValue() == ACC.rawValue() && !ACC.isNaN())
            code += offset;
        else {
            STORE_ACC();
            if (RuntimeHelpers::strictEqual(STACK_VALUE(lhs), accumulator))
                code += offset;
        }
    MOTH_END_INSTR(JumpStrictEqual)

    MOTH_BEGIN_INSTR(JumpStrictNotEqual)
        if (STACK_VALUE(lhs).rawValue() != ACC.rawValue() || ACC.isNaN()) {
            STORE_ACC();
            if (!RuntimeHelpers::strictEqual(STACK_VALUE(lhs), accumulator))
                code += offset;
        }
    MOTH_END_INSTR(JumpStrictNotEqual)

    MOTH_BEGIN_INSTR(JumpStrictNotEqualStackSlotInt)
        if (STACK_VALUE(lhs).int_32() != rhs || STACK_VALUE(lhs).isUndefined())
            code += offset;
    MOTH_END_INSTR(JumpStrictNotEqualStackSlotInt)

    MOTH_BEGIN_INSTR(JumpStrictEqualStackSlotInt)
        if (STACK_VALUE(lhs).int_32() == rhs && !STACK_VALUE(lhs).isUndefined())
            code += offset;
    MOTH_END_INSTR(JumpStrictNotEqualStackSlotInt)

    MOTH_BEGIN_INSTR(UNot)
        if (ACC.integerCompatible()) {
            acc = Encode(!static_cast<bool>(ACC.int_32()));
        } else {
            acc = Encode(!Value::toBooleanImpl(ACC));
        }
    MOTH_END_INSTR(UNot)

    MOTH_BEGIN_INSTR(UPlus)
        if (!ACC.isNumber()) {
            acc = Encode(ACC.toNumberImpl());
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(UPlus)

    MOTH_BEGIN_INSTR(UMinus)
        if (Q_LIKELY(ACC.integerCompatible())) {
            int a = ACC.int_32();
            if (a == 0 || a == std::numeric_limits<int>::min()) {
                acc = Encode(-static_cast<double>(a));
            } else {
                acc = sub_int32(0, ACC.int_32());
            }
        } else if (ACC.isDouble()) {
            acc ^= (1ull << 63); // simply flip sign bit
        } else {
            acc = Encode(-ACC.toNumberImpl());
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(UMinus)

    MOTH_BEGIN_INSTR(UCompl)
        VALUE_TO_INT(a, ACC);
        acc = Encode(~a);
    MOTH_END_INSTR(UCompl)

    MOTH_BEGIN_INSTR(Increment)
        if (Q_LIKELY(ACC.integerCompatible())) {
            acc = add_int32(ACC.int_32(), 1);
        } else if (ACC.isDouble()) {
            acc = QV4::Encode(ACC.doubleValue() + 1.);
        } else {
            acc = Encode(ACC.toNumberImpl() + 1.);
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(Increment)

    MOTH_BEGIN_INSTR(Decrement)
        if (Q_LIKELY(ACC.integerCompatible())) {
            acc = sub_int32(ACC.int_32(), 1);
        } else if (ACC.isDouble()) {
            acc = QV4::Encode(ACC.doubleValue() - 1.);
        } else {
            acc = Encode(ACC.toNumberImpl() - 1.);
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(Decrement)

    MOTH_BEGIN_INSTR(Binop)
        QV4::Runtime::BinaryOperation op = *reinterpret_cast<QV4::Runtime::BinaryOperation *>(reinterpret_cast<char *>(&engine->runtime.runtimeMethods[alu]));
        STORE_ACC();
        acc = op(STACK_VALUE(lhs), accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(Binop)

    MOTH_BEGIN_INSTR(Add)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(Value::integerCompatible(left, ACC))) {
            acc = add_int32(left.int_32(), ACC.int_32());
        } else if (left.isNumber() && ACC.isNumber()) {
            acc = Encode(left.asDouble() + ACC.asDouble());
        } else {
            STORE_ACC();
            acc = Runtime::method_add(engine, left, accumulator);
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(Add)

    MOTH_BEGIN_INSTR(Sub)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(Value::integerCompatible(left, ACC))) {
            acc = sub_int32(left.int_32(), ACC.int_32());
        } else if (left.isNumber() && ACC.isNumber()) {
            acc = Encode(left.asDouble() - ACC.asDouble());
        } else {
            STORE_ACC();
            acc = Runtime::method_sub(left, accumulator);
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(Sub)

    MOTH_BEGIN_INSTR(Mul)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(Value::integerCompatible(left, ACC))) {
            acc = mul_int32(left.int_32(), ACC.int_32());
        } else if (left.isNumber() && ACC.isNumber()) {
            acc = Encode(left.asDouble() * ACC.asDouble());
        } else {
            STORE_ACC();
            acc = Runtime::method_mul(left, accumulator);
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(Mul)

    MOTH_BEGIN_INSTR(BitAnd)
        VALUE_TO_INT(l, STACK_VALUE(lhs));
        VALUE_TO_INT(a, ACC);
        acc = Encode(l & a);
    MOTH_END_INSTR(BitAnd)

    MOTH_BEGIN_INSTR(BitOr)
        VALUE_TO_INT(l, STACK_VALUE(lhs));
        VALUE_TO_INT(a, ACC);
        acc = Encode(l | a);
    MOTH_END_INSTR(BitOr)

    MOTH_BEGIN_INSTR(BitXor)
        VALUE_TO_INT(l, STACK_VALUE(lhs));
        VALUE_TO_INT(a, ACC);
        acc = Encode(l ^ a);
    MOTH_END_INSTR(BitXor)

    MOTH_BEGIN_INSTR(Shr)
        VALUE_TO_INT(l, STACK_VALUE(lhs));
        VALUE_TO_INT(a, ACC);
        acc = Encode(l >> (a & 0x1f));
    MOTH_END_INSTR(Shr)

    MOTH_BEGIN_INSTR(Shl)
        VALUE_TO_INT(l, STACK_VALUE(lhs));
        VALUE_TO_INT(a, ACC);
        acc = Encode(l << (a & 0x1f));
    MOTH_END_INSTR(Shl)

    MOTH_BEGIN_INSTR(BitAndConst)
        VALUE_TO_INT(a, ACC);
        acc = Encode(a & rhs);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(BitAndConst)

    MOTH_BEGIN_INSTR(BitOrConst)
        VALUE_TO_INT(a, ACC);
        acc = Encode(a | rhs);
    MOTH_END_INSTR(BitOrConst)

    MOTH_BEGIN_INSTR(BitXorConst)
        VALUE_TO_INT(a, ACC);
        acc = Encode(a ^ rhs);
    MOTH_END_INSTR(BitXorConst)

    MOTH_BEGIN_INSTR(ShrConst)
        VALUE_TO_INT(a, ACC);
        acc = Encode(a >> rhs);
    MOTH_END_INSTR(ShrConst)

    MOTH_BEGIN_INSTR(ShlConst)
        VALUE_TO_INT(a, ACC);
        acc = Encode(a << rhs);
    MOTH_END_INSTR(ShlConst)

    MOTH_BEGIN_INSTR(BinopContext)
        STORE_ACC();
        QV4::Runtime::BinaryOperationContext op = *reinterpret_cast<QV4::Runtime::BinaryOperationContext *>(reinterpret_cast<char *>(&engine->runtime.runtimeMethods[alu]));
        acc = op(engine, STACK_VALUE(lhs), accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(BinopContext)

    MOTH_BEGIN_INSTR(Ret)
        goto functionExit;
    MOTH_END_INSTR(Ret)

#ifndef QT_NO_QML_DEBUGGER
    MOTH_BEGIN_INSTR(Debug)
        debug_slowPath(engine);
    MOTH_END_INSTR(Debug)
#endif // QT_NO_QML_DEBUGGER

    MOTH_BEGIN_INSTR(LoadQmlContext)
        STACK_VALUE(result) = Runtime::method_loadQmlContext(static_cast<QV4::NoThrowEngine*>(engine));
    MOTH_END_INSTR(LoadQmlContext)

    MOTH_BEGIN_INSTR(LoadQmlImportedScripts)
        STACK_VALUE(result) = Runtime::method_loadQmlImportedScripts(static_cast<QV4::NoThrowEngine*>(engine));
    MOTH_END_INSTR(LoadQmlImportedScripts)

    MOTH_BEGIN_INSTR(LoadQmlSingleton)
        acc = Runtime::method_loadQmlSingleton(static_cast<QV4::NoThrowEngine*>(engine), name);
    MOTH_END_INSTR(LoadQmlSingleton)

    catchException:
        Q_ASSERT(engine->hasException);
        if (!exceptionHandler) {
            acc = Encode::undefined();
            goto functionExit;
        }
        code = exceptionHandler;
    }

functionExit:
    if (QV4::Debugging::Debugger *debugger = engine->debugger())
        debugger->leavingFunction(ACC.asReturnedValue());
    engine->currentStackFrame = frame.parent;
    engine->jsStackTop = jsStackTop;

    if (function->hasQmlDependencies) {
        Q_ASSERT(context->type == Heap::ExecutionContext::Type_QmlContext);
        QQmlPropertyCapture::registerQmlDependencies(static_cast<Heap::QmlContext *>(context), engine, function->compiledFunction);
    }

    return ACC.asReturnedValue();
}
