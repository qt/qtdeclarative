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
#include <private/qv4regexp_p.h>
#include <private/qv4regexpobject_p.h>
#include <private/qv4string_p.h>
#include <private/qv4profiling_p.h>
#include <private/qv4jscall_p.h>
#include <private/qqmljavascriptexpression_p.h>
#include <iostream>

#include "qv4alloca_p.h"

#include <private/qv4jit_p.h>

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

#if QT_CONFIG(qml_debug)
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

#endif // QT_CONFIG(qml_debug)
// End of debugger interface

using namespace QV4;
using namespace QV4::Moth;

#ifdef COUNT_INSTRUCTIONS
static struct InstrCount {
    InstrCount() {
        fprintf(stderr, "Counting instructions...\n");
        for (int i = 0; i < MOTH_NUM_INSTRUCTIONS(); ++i)
            hits[i] = 0;
    }
    ~InstrCount() {
        fprintf(stderr, "Instruction count:\n");
#define BLAH(I) \
        fprintf(stderr, "%llu : %s\n", hits[int(Instr::Type::I)], #I);
        FOR_EACH_MOTH_INSTR(BLAH)
        #undef BLAH
    }
    quint64 hits[MOTH_NUM_INSTRUCTIONS()];
    void hit(Instr::Type i) { hits[int(i)]++; }
} instrCount;
#endif // COUNT_INSTRUCTIONS

#define MOTH_BEGIN_INSTR_COMMON(instr) \
    { \
        INSTR_##instr(MOTH_DECODE)

#ifdef COUNT_INSTRUCTIONS
#  define MOTH_BEGIN_INSTR(instr) \
    MOTH_BEGIN_INSTR_COMMON(instr) \
    instrCount.hit(Instr::Type::instr);
#else // !COUNT_INSTRUCTIONS
#  define MOTH_BEGIN_INSTR(instr) \
    MOTH_BEGIN_INSTR_COMMON(instr)
#endif // COUNT_INSTRUCTIONS

#ifdef MOTH_COMPUTED_GOTO
#define MOTH_END_INSTR(instr) \
        MOTH_DISPATCH() \
    }
#else // !MOTH_COMPUTED_GOTO
#define MOTH_END_INSTR(instr) \
        continue; \
    }
#endif

#define STACK_VALUE(temp) stack[temp]

// qv4scopedvalue_p.h also defines a CHECK_EXCEPTION macro
#ifdef CHECK_EXCEPTION
#undef CHECK_EXCEPTION
#endif
#define CHECK_EXCEPTION \
    if (engine->hasException) \
        goto catchException

static inline Heap::CallContext *getScope(QV4::Value *stack, int level)
{
    Heap::ExecutionContext *scope = static_cast<ExecutionContext &>(stack[CallData::Context]).d();
    while (level > 0) {
        --level;
        scope = scope->outer;
    }
    Q_ASSERT(scope);
    return static_cast<Heap::CallContext *>(scope);
}

static inline const QV4::Value &constant(Function *function, int index)
{
    return function->compilationUnit->constants[index];
}


static bool compareEqual(QV4::Value lhs, QV4::Value rhs)
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
    case QV4::Value::QT_ManagedOrUndefined:
        if (lhs.isUndefined())
            return rhs.isNullOrUndefined();
        Q_FALLTHROUGH();
    case QV4::Value::QT_ManagedOrUndefined1:
    case QV4::Value::QT_ManagedOrUndefined2:
    case QV4::Value::QT_ManagedOrUndefined3:
        // LHS: Managed
        switch (rt) {
        case QV4::Value::QT_ManagedOrUndefined:
            if (rhs.isUndefined())
                return false;
            Q_FALLTHROUGH();
        case QV4::Value::QT_ManagedOrUndefined1:
        case QV4::Value::QT_ManagedOrUndefined2:
        case QV4::Value::QT_ManagedOrUndefined3: {
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
        case QV4::Value::QT_Empty:
            Q_UNREACHABLE();
        case QV4::Value::QT_Null:
            return false;
        case QV4::Value::QT_Bool:
        case QV4::Value::QT_Int:
            rhs = Primitive::fromDouble(rhs.int_32());
            // fall through
        default: // double
            if (lhs.m()->vtable()->isString)
                return RuntimeHelpers::toNumber(lhs) == rhs.doubleValue();
            else
                lhs = Primitive::fromReturnedValue(RuntimeHelpers::objectDefaultValue(&static_cast<QV4::Object &>(lhs), PREFERREDTYPE_HINT));
        }
        goto redo;
    case QV4::Value::QT_Empty:
        Q_UNREACHABLE();
    case QV4::Value::QT_Null:
        return rhs.isNull();
    case QV4::Value::QT_Bool:
    case QV4::Value::QT_Int:
        switch (rt) {
        case QV4::Value::QT_ManagedOrUndefined:
        case QV4::Value::QT_ManagedOrUndefined1:
        case QV4::Value::QT_ManagedOrUndefined2:
        case QV4::Value::QT_ManagedOrUndefined3:
        case QV4::Value::QT_Empty:
        case QV4::Value::QT_Null:
            Q_UNREACHABLE();
        case QV4::Value::QT_Bool:
        case QV4::Value::QT_Int:
            return lhs.int_32() == rhs.int_32();
        default: // double
            return lhs.int_32() == rhs.doubleValue();
        }
    default: // double
        Q_ASSERT(rhs.isDouble());
        return lhs.doubleValue() == rhs.doubleValue();
    }
}

static bool compareEqualInt(QV4::Value &accumulator, QV4::Value lhs, int rhs)
{
  redo:
    switch (lhs.quickType()) {
    case QV4::Value::QT_ManagedOrUndefined:
        if (lhs.isUndefined())
            return false;
        Q_FALLTHROUGH();
    case QV4::Value::QT_ManagedOrUndefined1:
    case QV4::Value::QT_ManagedOrUndefined2:
    case QV4::Value::QT_ManagedOrUndefined3:
        // LHS: Managed
        if (lhs.m()->vtable()->isString)
            return RuntimeHelpers::stringToNumber(static_cast<String &>(lhs).toQString()) == rhs;
        accumulator = lhs;
        lhs = Primitive::fromReturnedValue(RuntimeHelpers::objectDefaultValue(&static_cast<QV4::Object &>(accumulator), PREFERREDTYPE_HINT));
        goto redo;
    case QV4::Value::QT_Empty:
        Q_UNREACHABLE();
    case QV4::Value::QT_Null:
        return false;
    case QV4::Value::QT_Bool:
    case QV4::Value::QT_Int:
        return lhs.int_32() == rhs;
    default: // double
        return lhs.doubleValue() == rhs;
    }
}

#define STORE_IP() frame.instructionPointer = int(code - codeStart);
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
                STORE_ACC(); \
                d = val.toNumberImpl(); \
                CHECK_EXCEPTION; \
            } \
            i = Double::toInt32(d); \
        } \
    } while (false)

QV4::ReturnedValue VME::exec(const FunctionObject *fo, const QV4::Value *thisObject, const QV4::Value *argv, int argc)
{
    qt_v4ResolvePendingBreakpointsHook();
    ExecutionEngine *engine;
    QV4::Value *stack;
    CppStackFrame frame;
    frame.originalArguments = argv;
    frame.originalArgumentsCount = argc;
    Function *function;

    {
        Heap::ExecutionContext *scope;

        quintptr d = reinterpret_cast<quintptr>(fo);
        if (d & 0x1) {
            // we don't have a FunctionObject, but a ExecData
            ExecData *data = reinterpret_cast<ExecData *>(d - 1);
            function = data->function;
            scope = data->scope->d();
            fo = nullptr;
        } else {
            function = fo->function();
            scope = fo->scope();
        }

        engine = function->internalClass->engine;

        stack = engine->jsStackTop;
        CallData *callData = reinterpret_cast<CallData *>(stack);
        callData->function = fo ? fo->asReturnedValue() : Encode::undefined();
        callData->context = scope;
        callData->accumulator = Encode::undefined();
        callData->thisObject = thisObject ? *thisObject : Primitive::undefinedValue();
        if (argc > int(function->nFormals))
            argc = int(function->nFormals);
        callData->setArgc(argc);

        int jsStackFrameSize = offsetof(CallData, args)/sizeof(Value) + function->compiledFunction->nRegisters;
        engine->jsStackTop += jsStackFrameSize;
        memcpy(callData->args, argv, argc*sizeof(Value));
        for (Value *v = callData->args + argc; v < engine->jsStackTop; ++v)
            *v = Encode::undefined();

        frame.parent = engine->currentStackFrame;
        frame.v4Function = function;
        frame.instructionPointer = 0;
        frame.jsFrame = callData;
        engine->currentStackFrame = &frame;
    }
    CHECK_STACK_LIMITS(engine);

    Profiling::FunctionCallProfiler profiler(engine, function); // start execution profiling
    QV4::Debugging::Debugger *debugger = engine->debugger();
    const uchar *exceptionHandler = nullptr;

    QV4::Value &accumulator = frame.jsFrame->accumulator;
    QV4::ReturnedValue acc = Encode::undefined();

#ifdef V4_ENABLE_JIT
    if (function->jittedCode == nullptr && debugger == nullptr) {
        if (engine->canJIT(function))
            QV4::JIT::BaselineJIT(function).generate();
        else
            ++function->interpreterCallCount;
    }
#endif // V4_ENABLE_JIT

    if (debugger)
        debugger->enteringFunction();

    if (function->jittedCode != nullptr && debugger == nullptr) {
        acc = function->jittedCode(&frame, engine);
    } else {
    // interpreter
    const uchar *code = function->codeData;
    const uchar *codeStart = code;

    MOTH_JUMP_TABLE;

    for (;;) {
    MOTH_DISPATCH()
    Q_UNREACHABLE(); // only reached when the dispatch doesn't jump somewhere

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
        auto cc = static_cast<Heap::CallContext *>(stack[CallData::Context].m());
        acc = cc->locals[index].asReturnedValue();
    MOTH_END_INSTR(LoadLocal)

    MOTH_BEGIN_INSTR(StoreLocal)
        CHECK_EXCEPTION;
        auto cc = static_cast<Heap::CallContext *>(stack[CallData::Context].m());
        QV4::WriteBarrier::write(engine, cc, cc->locals.values[index].data_ptr(), acc);
    MOTH_END_INSTR(StoreLocal)

    MOTH_BEGIN_INSTR(LoadScopedLocal)
        auto cc = getScope(stack, scope);
        acc = cc->locals[index].asReturnedValue();
    MOTH_END_INSTR(LoadScopedLocal)

    MOTH_BEGIN_INSTR(StoreScopedLocal)
        CHECK_EXCEPTION;
        auto cc = getScope(stack, scope);
        QV4::WriteBarrier::write(engine, cc, cc->locals.values[index].data_ptr(), acc);
    MOTH_END_INSTR(StoreScopedLocal)

    MOTH_BEGIN_INSTR(LoadRuntimeString)
        acc = function->compilationUnit->runtimeStrings[stringId]->asReturnedValue();
    MOTH_END_INSTR(LoadRuntimeString)

    MOTH_BEGIN_INSTR(MoveRegExp)
        STACK_VALUE(destReg) = Runtime::method_regexpLiteral(engine, regExpId);
    MOTH_END_INSTR(MoveRegExp)

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
    MOTH_END_INSTR(StoreNameStrict)

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
        Value func = STACK_VALUE(name);
        if (Q_UNLIKELY(!func.isFunctionObject())) {
            acc = engine->throwTypeError(QStringLiteral("%1 is not a function").arg(func.toQStringNoThrow()));
            goto catchException;
        }
        acc = static_cast<const FunctionObject &>(func).call(nullptr, stack + argv, argc);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallValue)

    MOTH_BEGIN_INSTR(CallProperty)
        STORE_IP();
        acc = Runtime::method_callProperty(engine, stack + base, name, stack + argv, argc);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallProperty)

    MOTH_BEGIN_INSTR(CallPropertyLookup)
        STORE_IP();
        Lookup *l = function->compilationUnit->runtimeLookups + lookupIndex;
        // ok to have the value on the stack here
        Value f = Value::fromReturnedValue(l->getter(l, engine, stack[base]));

        if (Q_UNLIKELY(!f.isFunctionObject())) {
            acc = engine->throwTypeError();
            goto catchException;
        }

        acc = static_cast<FunctionObject &>(f).call(stack + base, stack + argv, argc);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallPropertyLookup)

    MOTH_BEGIN_INSTR(CallElement)
        STORE_IP();
        acc = Runtime::method_callElement(engine, stack + base, STACK_VALUE(index), stack + argv, argc);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallElement)

    MOTH_BEGIN_INSTR(CallName)
        STORE_IP();
        acc = Runtime::method_callName(engine, name, stack + argv, argc);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallName)

    MOTH_BEGIN_INSTR(CallPossiblyDirectEval)
        STORE_IP();
        acc = Runtime::method_callPossiblyDirectEval(engine, stack + argv, argc);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallPossiblyDirectEval)

    MOTH_BEGIN_INSTR(CallGlobalLookup)
        STORE_IP();
        acc = Runtime::method_callGlobalLookup(engine, index, stack + argv, argc);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallGlobalLookup)

    MOTH_BEGIN_INSTR(CallScopeObjectProperty)
        STORE_IP();
        acc = Runtime::method_callQmlScopeObjectProperty(engine, stack + base, name, stack + argv, argc);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallScopeObjectProperty)

    MOTH_BEGIN_INSTR(CallContextObjectProperty)
        STORE_IP();
        acc = Runtime::method_callQmlContextObjectProperty(engine, stack + base, name, stack + argv, argc);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CallContextObjectProperty)

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
        acc = engine->hasException ? engine->exceptionValue->asReturnedValue()
                                   : Primitive::emptyValue().asReturnedValue();
        engine->hasException = false;
    MOTH_END_INSTR(HasException)

    MOTH_BEGIN_INSTR(SetException)
        *engine->exceptionValue = acc;
        engine->hasException = true;
    MOTH_END_INSTR(SetException)

    MOTH_BEGIN_INSTR(PushCatchContext)
        STACK_VALUE(reg) = STACK_VALUE(CallData::Context);
        ExecutionContext *c = static_cast<ExecutionContext *>(stack + CallData::Context);
        STACK_VALUE(CallData::Context) = Runtime::method_createCatchContext(c, name);
    MOTH_END_INSTR(PushCatchContext)

    MOTH_BEGIN_INSTR(CreateCallContext)
        stack[CallData::Context] = ExecutionContext::newCallContext(&frame);
    MOTH_END_INSTR(CreateCallContext)

    MOTH_BEGIN_INSTR(PushWithContext)
        STORE_IP();
        STORE_ACC();
        accumulator = accumulator.toObject(engine);
        CHECK_EXCEPTION;
        STACK_VALUE(reg) = STACK_VALUE(CallData::Context);
        ExecutionContext *c = static_cast<ExecutionContext *>(stack + CallData::Context);
        STACK_VALUE(CallData::Context) = Runtime::method_createWithContext(c, accumulator);
    MOTH_END_INSTR(PushWithContext)

    MOTH_BEGIN_INSTR(PopContext)
        STACK_VALUE(CallData::Context) = STACK_VALUE(reg);
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
        Value *t = &stack[CallData::This];
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
        acc = Runtime::method_construct(engine, STACK_VALUE(func), stack + argv, argc);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(Construct)

    MOTH_BEGIN_INSTR(Jump)
        code += offset;
    MOTH_END_INSTR(Jump)

    MOTH_BEGIN_INSTR(JumpTrue)
        if (Q_LIKELY(ACC.integerCompatible())) {
            if (ACC.int_32())
                code += offset;
        } else {
            if (ACC.toBoolean())
                code += offset;
        }
    MOTH_END_INSTR(JumpTrue)

    MOTH_BEGIN_INSTR(JumpFalse)
        if (Q_LIKELY(ACC.integerCompatible())) {
            if (!ACC.int_32())
                code += offset;
        } else {
            if (!ACC.toBoolean())
                code += offset;
        }
    MOTH_END_INSTR(JumpFalse)

    MOTH_BEGIN_INSTR(CmpEqNull)
        acc = Encode(ACC.isNullOrUndefined());
    MOTH_END_INSTR(CmpEqNull)

    MOTH_BEGIN_INSTR(CmpNeNull)
        acc = Encode(!ACC.isNullOrUndefined());
    MOTH_END_INSTR(CmpNeNull)

    MOTH_BEGIN_INSTR(CmpEqInt)
        if (ACC.isIntOrBool()) {
            acc = Encode(ACC.int_32() == lhs);
        } else {
            STORE_ACC();
            acc = Encode(compareEqualInt(accumulator, ACC, lhs));
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(CmpEqInt)

    MOTH_BEGIN_INSTR(CmpNeInt)
        if (ACC.isIntOrBool()) {
            acc = Encode(bool(ACC.int_32() != lhs));
        } else {
            STORE_ACC();
            acc = Encode(!compareEqualInt(accumulator, ACC, lhs));
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(CmpNeInt)

    MOTH_BEGIN_INSTR(CmpEq)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(left.asReturnedValue() == ACC.asReturnedValue())) {
            acc = Encode(!ACC.isNaN());
        } else if (Q_LIKELY(left.isInteger() && ACC.isInteger())) {
            acc = Encode(left.int_32() == ACC.int_32());
        } else {
            STORE_ACC();
            acc = Encode(compareEqual(left, accumulator));
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(CmpEq)

    MOTH_BEGIN_INSTR(CmpNe)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(left.isInteger() && ACC.isInteger())) {
            acc = Encode(bool(left.int_32() != ACC.int_32()));
        } else {
            STORE_ACC();
            acc = Encode(!compareEqual(left, accumulator));
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(CmpNe)

    MOTH_BEGIN_INSTR(CmpGt)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(left.isInteger() && ACC.isInteger())) {
            acc = Encode(left.int_32() > ACC.int_32());
        } else if (left.isNumber() && ACC.isNumber()) {
            acc = Encode(left.asDouble() > ACC.asDouble());
        } else {
            STORE_ACC();
            acc = Encode(bool(Runtime::method_compareGreaterThan(left, accumulator)));
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(CmpGt)

    MOTH_BEGIN_INSTR(CmpGe)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(left.isInteger() && ACC.isInteger())) {
            acc = Encode(left.int_32() >= ACC.int_32());
        } else if (left.isNumber() && ACC.isNumber()) {
            acc = Encode(left.asDouble() >= ACC.asDouble());
        } else {
            STORE_ACC();
            acc = Encode(bool(Runtime::method_compareGreaterEqual(left, accumulator)));
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(CmpGe)

    MOTH_BEGIN_INSTR(CmpLt)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(left.isInteger() && ACC.isInteger())) {
            acc = Encode(left.int_32() < ACC.int_32());
        } else if (left.isNumber() && ACC.isNumber()) {
            acc = Encode(left.asDouble() < ACC.asDouble());
        } else {
            STORE_ACC();
            acc = Encode(bool(Runtime::method_compareLessThan(left, accumulator)));
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(CmpLt)

    MOTH_BEGIN_INSTR(CmpLe)
        const Value left = STACK_VALUE(lhs);
        if (Q_LIKELY(left.isInteger() && ACC.isInteger())) {
            acc = Encode(left.int_32() <= ACC.int_32());
        } else if (left.isNumber() && ACC.isNumber()) {
            acc = Encode(left.asDouble() <= ACC.asDouble());
        } else {
            STORE_ACC();
            acc = Encode(bool(Runtime::method_compareLessEqual(left, accumulator)));
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(CmpLe)

    MOTH_BEGIN_INSTR(CmpStrictEqual)
        if (STACK_VALUE(lhs).rawValue() == ACC.rawValue() && !ACC.isNaN()) {
            acc = Encode(true);
        } else {
            STORE_ACC();
            acc = Encode(bool(RuntimeHelpers::strictEqual(STACK_VALUE(lhs), accumulator)));
            CHECK_EXCEPTION;
        }
    MOTH_END_INSTR(CmpStrictEqual)

    MOTH_BEGIN_INSTR(CmpStrictNotEqual)
        if (STACK_VALUE(lhs).rawValue() != ACC.rawValue() || ACC.isNaN()) {
            STORE_ACC();
            acc = Encode(!RuntimeHelpers::strictEqual(STACK_VALUE(lhs), accumulator));
            CHECK_EXCEPTION;
        } else {
            acc = Encode(false);
        }
    MOTH_END_INSTR(CmpStrictNotEqual)

    MOTH_BEGIN_INSTR(CmpIn)
        STORE_ACC();
        acc = Runtime::method_in(engine, STACK_VALUE(lhs), accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CmpIn)

    MOTH_BEGIN_INSTR(CmpInstanceOf)
        // 11.8.6, 5: rval must be an Object
        if (Q_UNLIKELY(!Primitive::fromReturnedValue(acc).isObject())) {
           acc = engine->throwTypeError();
           goto catchException;
        }

        // 11.8.6, 7: call "HasInstance", which we term instanceOf, and return the result.
        acc = Primitive::fromReturnedValue(acc).objectValue()->instanceOf(STACK_VALUE(lhs));
        CHECK_EXCEPTION;
    MOTH_END_INSTR(CmpInstanceOf)

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
        if (Q_UNLIKELY(!ACC.isNumber())) {
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

    MOTH_BEGIN_INSTR(Div)
        STORE_ACC();
        acc = Runtime::method_div(STACK_VALUE(lhs), accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(Div)

    MOTH_BEGIN_INSTR(Mod)
        STORE_ACC();
        acc = Runtime::method_mod(STACK_VALUE(lhs), accumulator);
        CHECK_EXCEPTION;
    MOTH_END_INSTR(Mod)

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

    MOTH_BEGIN_INSTR(UShr)
        VALUE_TO_INT(l, STACK_VALUE(lhs));
        VALUE_TO_INT(a, ACC);
        acc = Encode(static_cast<uint>(l) >> uint(a & 0x1f));
    MOTH_END_INSTR(UShr)

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

    MOTH_BEGIN_INSTR(UShrConst)
        acc = Encode(ACC.toUInt32() >> uint(rhs));
    MOTH_END_INSTR(UShrConst)

    MOTH_BEGIN_INSTR(ShrConst)
        VALUE_TO_INT(a, ACC);
        acc = Encode(a >> rhs);
    MOTH_END_INSTR(ShrConst)

    MOTH_BEGIN_INSTR(ShlConst)
        VALUE_TO_INT(a, ACC);
        acc = Encode(a << rhs);
    MOTH_END_INSTR(ShlConst)

    MOTH_BEGIN_INSTR(Ret)
        goto functionExit;
    MOTH_END_INSTR(Ret)

    MOTH_BEGIN_INSTR(Debug)
#if QT_CONFIG(qml_debug)
        STORE_IP();
        debug_slowPath(engine);
#endif // QT_CONFIG(qml_debug)
    MOTH_END_INSTR(Debug)

    MOTH_BEGIN_INSTR(LoadQmlContext)
        STACK_VALUE(result) = Runtime::method_loadQmlContext(static_cast<QV4::NoThrowEngine*>(engine));
    MOTH_END_INSTR(LoadQmlContext)

    MOTH_BEGIN_INSTR(LoadQmlImportedScripts)
        STACK_VALUE(result) = Runtime::method_loadQmlImportedScripts(static_cast<QV4::NoThrowEngine*>(engine));
    MOTH_END_INSTR(LoadQmlImportedScripts)

    catchException:
        Q_ASSERT(engine->hasException);
        if (!exceptionHandler) {
            acc = Encode::undefined();
            goto functionExit;
        }
        code = exceptionHandler;
    }
    }

functionExit:
    if (QV4::Debugging::Debugger *debugger = engine->debugger())
        debugger->leavingFunction(ACC.asReturnedValue());
    engine->currentStackFrame = frame.parent;
    engine->jsStackTop = stack;

    return acc;
}
