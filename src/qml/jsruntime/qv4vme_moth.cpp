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

#include "qv4vme_moth_p.h"
#include "qv4instr_moth_p.h"
#include <private/qv4value_p.h>
#include <private/qv4debugging_p.h>
#include <private/qv4exception_p.h>
#include <private/qv4math_p.h>
#include <private/qv4scopedvalue_p.h>
#include <iostream>

#include "qv4alloca_p.h"

#ifdef DO_TRACE_INSTR
#  define TRACE_INSTR(I) fprintf(stderr, "executing a %s\n", #I);
#  define TRACE(n, str, ...) { char buf[4096]; snprintf(buf, 4096, str, __VA_ARGS__); fprintf(stderr, "    %s : %s\n", #n, buf); }
#else
#  define TRACE_INSTR(I)
#  define TRACE(n, str, ...)
#endif // DO_TRACE_INSTR

using namespace QQmlJS;
using namespace QQmlJS::Moth;

#define MOTH_BEGIN_INSTR_COMMON(I) { \
    const InstrMeta<(int)Instr::I>::DataType &instr = InstrMeta<(int)Instr::I>::data(*genericInstr); \
    code += InstrMeta<(int)Instr::I>::Size; \
    if (context->engine->debugger && (instr.breakPoint || context->engine->debugger->pauseAtNextOpportunity())) \
        context->engine->debugger->maybeBreakAtInstruction(code, instr.breakPoint); \
    Q_UNUSED(instr); \
    TRACE_INSTR(I)

#ifdef MOTH_THREADED_INTERPRETER

#  define MOTH_BEGIN_INSTR(I) op_##I: \
    MOTH_BEGIN_INSTR_COMMON(I)

#  define MOTH_NEXT_INSTR(I) { \
    genericInstr = reinterpret_cast<const Instr *>(code); \
    goto *genericInstr->common.code; \
    }

#  define MOTH_END_INSTR(I) } \
    genericInstr = reinterpret_cast<const Instr *>(code); \
    goto *genericInstr->common.code; \

#else

#  define MOTH_BEGIN_INSTR(I) \
    case Instr::I: \
    MOTH_BEGIN_INSTR_COMMON(I)

#  define MOTH_NEXT_INSTR(I) { \
    break; \
    }

#  define MOTH_END_INSTR(I) } \
    break;

#endif

#ifdef WITH_STATS
namespace {
struct VMStats {
    quint64 paramIsValue;
    quint64 paramIsArg;
    quint64 paramIsLocal;
    quint64 paramIsTemp;
    quint64 paramIsScopedLocal;

    VMStats()
        : paramIsValue(0)
        , paramIsArg(0)
        , paramIsLocal(0)
        , paramIsTemp(0)
        , paramIsScopedLocal(0)
    {}

    ~VMStats()
    { show(); }

    void show() {
        fprintf(stderr, "VM stats:\n");
        fprintf(stderr, "         value: %lu\n", paramIsValue);
        fprintf(stderr, "           arg: %lu\n", paramIsArg);
        fprintf(stderr, "         local: %lu\n", paramIsLocal);
        fprintf(stderr, "          temp: %lu\n", paramIsTemp);
        fprintf(stderr, "  scoped local: %lu\n", paramIsScopedLocal);
    }
};
static VMStats vmStats;
#define VMSTATS(what) ++vmStats.what
}
#else // !WITH_STATS
#define VMSTATS(what) {}
#endif // WITH_STATS

static inline QV4::Value *getValueRef(QV4::ExecutionContext *context,
                                     QV4::Value* stack,
                                     const Param &param
#if !defined(QT_NO_DEBUG)
                                     , unsigned stackSize
#endif
                                     )
{
#ifdef DO_TRACE_INSTR
    if (param.isValue()) {
        fprintf(stderr, "    value %s\n", param.value.toString(context)->toQString().toUtf8().constData());
    } else if (param.isArgument()) {
        fprintf(stderr, "    argument %d@%d\n", param.index, param.scope);
    } else if (param.isLocal()) {
        fprintf(stderr, "    local %d\n", param.index);
    } else if (param.isTemp()) {
        fprintf(stderr, "    temp %d\n", param.index);
    } else if (param.isScopedLocal()) {
        fprintf(stderr, "    temp %d@%d\n", param.index, param.scope);
    } else {
        Q_ASSERT(!"INVALID");
    }
#endif // DO_TRACE_INSTR

    if (param.isValue()) {
        VMSTATS(paramIsValue);
        return const_cast<QV4::Value *>(&param.value);
    } else if (param.isArgument()) {
        VMSTATS(paramIsArg);
        QV4::ExecutionContext *c = context;
        uint scope = param.scope;
        while (scope--)
            c = c->outer;
        QV4::CallContext *cc = static_cast<QV4::CallContext *>(c);
        const unsigned arg = param.index;
        Q_ASSERT(arg >= 0);
        Q_ASSERT((unsigned) arg < cc->callData->argc);
        Q_ASSERT(cc->callData->args);
        return cc->callData->args + arg;
    } else if (param.isLocal()) {
        VMSTATS(paramIsLocal);
        const unsigned index = param.index;
        QV4::CallContext *c = static_cast<QV4::CallContext *>(context);
        Q_ASSERT(index >= 0);
        Q_ASSERT(index < context->variableCount());
        Q_ASSERT(c->locals);
        return c->locals + index;
    } else if (param.isTemp()) {
        VMSTATS(paramIsTemp);
#if !defined(QT_NO_DEBUG)
        Q_ASSERT(param.index < stackSize);
#endif
        return stack + param.index;
    } else if (param.isScopedLocal()) {
        VMSTATS(paramIsScopedLocal);
        QV4::ExecutionContext *c = context;
        uint scope = param.scope;
        while (scope--)
            c = c->outer;
        const unsigned index = param.index;
        QV4::CallContext *cc = static_cast<QV4::CallContext *>(c);
        Q_ASSERT(index >= 0);
        Q_ASSERT(index < cc->variableCount());
        Q_ASSERT(cc->locals);
        return cc->locals + index;
    } else {
        Q_UNIMPLEMENTED();
        return 0;
    }
}

#if defined(QT_NO_DEBUG)
# define VALUE(param) (*VALUEPTR(param))

// The non-temp case might need some tweaking for QML: there it would probably be a value instead of a local.
# define VALUEPTR(param) \
    (param.isTemp() ? stack + param.index \
                    : (param.isLocal() ? static_cast<QV4::CallContext *>(context)->locals + param.index \
                                       : getValueRef(context, stack, param)))
#else
# define VALUE(param) (*getValueRef(context, stack, param, stackSize))
# define VALUEPTR(param) getValueRef(context, stack, param, stackSize)
#endif
#define STOREVALUE(param, value) VALUE(param) = QV4::Value::fromReturnedValue((value))

QV4::ReturnedValue VME::run(QV4::ExecutionContext *context, const uchar *&code,
                            QV4::Value *stack, unsigned stackSize
#ifdef MOTH_THREADED_INTERPRETER
        , void ***storeJumpTable
#endif
        )
{
#ifdef DO_TRACE_INSTR
    qDebug("Starting VME with context=%p and code=%p", context, code);
#endif // DO_TRACE_INSTR

#ifdef MOTH_THREADED_INTERPRETER
    if (storeJumpTable) {
#define MOTH_INSTR_ADDR(I, FMT) &&op_##I,
        static void *jumpTable[] = {
            FOR_EACH_MOTH_INSTR(MOTH_INSTR_ADDR)
        };
#undef MOTH_INSTR_ADDR
        *storeJumpTable = jumpTable;
        return QV4::Primitive::undefinedValue().asReturnedValue();
    }
#endif

    QV4::SafeString * const runtimeStrings = context->compilationUnit->runtimeStrings;
    context->interpreterInstructionPointer = &code;

#ifdef MOTH_THREADED_INTERPRETER
    const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
    goto *genericInstr->common.code;
#else
    for (;;) {
        const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
        switch (genericInstr->common.instructionType) {
#endif

    MOTH_BEGIN_INSTR(MoveTemp)
        VALUE(instr.result) = VALUE(instr.source);
    MOTH_END_INSTR(MoveTemp)

    MOTH_BEGIN_INSTR(SwapTemps)
        qSwap(VALUE(instr.left),  VALUE(instr.right));
    MOTH_END_INSTR(MoveTemp)

    MOTH_BEGIN_INSTR(LoadValue)
//        TRACE(value, "%s", instr.value.toString(context)->toQString().toUtf8().constData());
        VALUE(instr.result) = VALUE(instr.value);
    MOTH_END_INSTR(LoadValue)

    MOTH_BEGIN_INSTR(LoadRuntimeString)
//        TRACE(value, "%s", instr.value.toString(context)->toQString().toUtf8().constData());
        VALUE(instr.result) = runtimeStrings[instr.stringId].asReturnedValue();
    MOTH_END_INSTR(LoadRuntimeString)

    MOTH_BEGIN_INSTR(LoadRegExp)
//        TRACE(value, "%s", instr.value.toString(context)->toQString().toUtf8().constData());
        VALUE(instr.result) = context->compilationUnit->runtimeRegularExpressions[instr.regExpId];
    MOTH_END_INSTR(LoadRegExp)

    MOTH_BEGIN_INSTR(LoadClosure)
        STOREVALUE(instr.result, __qmljs_init_closure(context, instr.value));
    MOTH_END_INSTR(LoadClosure)

    MOTH_BEGIN_INSTR(LoadName)
        TRACE(inline, "property name = %s", runtimeStrings[instr.name]->toQString().toUtf8().constData());
        STOREVALUE(instr.result, __qmljs_get_activation_property(context, runtimeStrings[instr.name]));
    MOTH_END_INSTR(LoadName)

    MOTH_BEGIN_INSTR(StoreName)
        TRACE(inline, "property name = %s", runtimeStrings[instr.name]->toQString().toUtf8().constData());
        __qmljs_set_activation_property(context, runtimeStrings[instr.name], VALUEPTR(instr.source));
    MOTH_END_INSTR(StoreName)

    MOTH_BEGIN_INSTR(LoadElement)
        STOREVALUE(instr.result, __qmljs_get_element(context, VALUEPTR(instr.base), VALUEPTR(instr.index)));
    MOTH_END_INSTR(LoadElement)

    MOTH_BEGIN_INSTR(StoreElement)
        __qmljs_set_element(context, VALUEPTR(instr.base), VALUEPTR(instr.index), VALUEPTR(instr.source));
    MOTH_END_INSTR(StoreElement)

    MOTH_BEGIN_INSTR(LoadProperty)
        STOREVALUE(instr.result, __qmljs_get_property(context, VALUEPTR(instr.base), runtimeStrings[instr.name]));
    MOTH_END_INSTR(LoadProperty)

    MOTH_BEGIN_INSTR(StoreProperty)
        __qmljs_set_property(context, VALUEPTR(instr.base), runtimeStrings[instr.name], VALUEPTR(instr.source));
    MOTH_END_INSTR(StoreProperty)

    MOTH_BEGIN_INSTR(Push)
        TRACE(inline, "stack size: %u", instr.value);
        stackSize = instr.value;
        stack = context->engine->stackPush(stackSize);
        memset(stack, 0, stackSize * sizeof(QV4::Value));
    MOTH_END_INSTR(Push)

    MOTH_BEGIN_INSTR(CallValue)
#if 0 //def DO_TRACE_INSTR
        if (Debugging::Debugger *debugger = context->engine->debugger) {
            if (QV4::FunctionObject *o = (VALUE(instr.dest)).asFunctionObject()) {
                if (Debugging::FunctionDebugInfo *info = debugger->debugInfo(o)) {
                    QString n = debugger->name(o);
                    std::cerr << "*** Call to \"" << (n.isNull() ? "<no name>" : qPrintable(n)) << "\" defined @" << info->startLine << ":" << info->startColumn << std::endl;
                }
            }
        }
#endif // DO_TRACE_INSTR
        Q_ASSERT(instr.callData + instr.argc + qOffsetOf(QV4::CallData, args)/sizeof(QV4::SafeValue) <= stackSize);
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData);
        callData->tag = QV4::Value::Integer_Type;
        callData->argc = instr.argc;
        callData->thisObject = QV4::Primitive::undefinedValue();
        STOREVALUE(instr.result, __qmljs_call_value(context, VALUEPTR(instr.dest), callData));
    MOTH_END_INSTR(CallValue)

    MOTH_BEGIN_INSTR(CallProperty)
        TRACE(property name, "%s, args=%u, argc=%u, this=%s", qPrintable(runtimeStrings[instr.name]->toQString()), instr.callData, instr.argc, (VALUE(instr.base)).toString(context)->toQString().toUtf8().constData());
        Q_ASSERT(instr.callData + instr.argc + qOffsetOf(QV4::CallData, args)/sizeof(QV4::SafeValue) <= stackSize);
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData);
        callData->tag = QV4::Value::Integer_Type;
        callData->argc = instr.argc;
        callData->thisObject = VALUE(instr.base);
        STOREVALUE(instr.result, __qmljs_call_property(context, runtimeStrings[instr.name], callData));
    MOTH_END_INSTR(CallProperty)

    MOTH_BEGIN_INSTR(CallElement)
        Q_ASSERT(instr.callData + instr.argc + qOffsetOf(QV4::CallData, args)/sizeof(QV4::SafeValue) <= stackSize);
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData);
        callData->tag = QV4::Value::Integer_Type;
        callData->argc = instr.argc;
        callData->thisObject = VALUE(instr.base);
        STOREVALUE(instr.result, __qmljs_call_element(context, VALUEPTR(instr.index), callData));
    MOTH_END_INSTR(CallElement)

    MOTH_BEGIN_INSTR(CallActivationProperty)
        TRACE(args, "starting at %d, length %d", instr.args, instr.argc);
        Q_ASSERT(instr.callData + instr.argc + qOffsetOf(QV4::CallData, args)/sizeof(QV4::SafeValue) <= stackSize);
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData);
        callData->tag = QV4::Value::Integer_Type;
        callData->argc = instr.argc;
        callData->thisObject = QV4::Primitive::undefinedValue();
        STOREVALUE(instr.result, __qmljs_call_activation_property(context, runtimeStrings[instr.name], callData));
    MOTH_END_INSTR(CallActivationProperty)

    MOTH_BEGIN_INSTR(CallBuiltinThrow)
        __qmljs_throw(context, VALUEPTR(instr.arg));
    MOTH_END_INSTR(CallBuiltinThrow)

    MOTH_BEGIN_INSTR(EnterTry)
        VALUE(instr.exceptionVar) = QV4::Primitive::undefinedValue();
        try {
            const uchar *tryCode = ((uchar *)&instr.tryOffset) + instr.tryOffset;
            run(context, tryCode, stack, stackSize);
            code = tryCode;
            context->interpreterInstructionPointer = &code;
        } catch (QV4::Exception &ex) {
            ex.accept(context);
            STOREVALUE(instr.exceptionVar, ex.value());
            try {
                QV4::ExecutionContext *catchContext = __qmljs_builtin_push_catch_scope(runtimeStrings[instr.exceptionVarName], VALUEPTR(instr.exceptionVar), context);
                const uchar *catchCode = ((uchar *)&instr.catchOffset) + instr.catchOffset;
                run(catchContext, catchCode, stack, stackSize);
                code = catchCode;
                context->interpreterInstructionPointer = &code;
                context = __qmljs_builtin_pop_scope(catchContext);
            } catch (QV4::Exception &ex) {
                ex.accept(context);
                STOREVALUE(instr.exceptionVar, ex.value());
                const uchar *catchCode = ((uchar *)&instr.catchOffset) + instr.catchOffset;
                run(context, catchCode, stack, stackSize);
                code = catchCode;
                context->interpreterInstructionPointer = &code;
            }
        }
    MOTH_END_INSTR(EnterTry)

    MOTH_BEGIN_INSTR(CallBuiltinFinishTry)
         return QV4::ReturnedValue(0);
    MOTH_END_INSTR(CallBuiltinFinishTry)

    MOTH_BEGIN_INSTR(CallBuiltinPushScope)
        context = __qmljs_builtin_push_with_scope(VALUEPTR(instr.arg), context);
    MOTH_END_INSTR(CallBuiltinPushScope)

    MOTH_BEGIN_INSTR(CallBuiltinPopScope)
        context = __qmljs_builtin_pop_scope(context);
    MOTH_END_INSTR(CallBuiltinPopScope)

    MOTH_BEGIN_INSTR(CallBuiltinForeachIteratorObject)
        STOREVALUE(instr.result, __qmljs_foreach_iterator_object(context, VALUEPTR(instr.arg)));
    MOTH_END_INSTR(CallBuiltinForeachIteratorObject)

    MOTH_BEGIN_INSTR(CallBuiltinForeachNextPropertyName)
        STOREVALUE(instr.result, __qmljs_foreach_next_property_name(VALUEPTR(instr.arg)));
    MOTH_END_INSTR(CallBuiltinForeachNextPropertyName)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteMember)
        STOREVALUE(instr.result, __qmljs_delete_member(context, VALUEPTR(instr.base), runtimeStrings[instr.member]));
    MOTH_END_INSTR(CallBuiltinDeleteMember)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteSubscript)
        STOREVALUE(instr.result, __qmljs_delete_subscript(context, VALUEPTR(instr.base), VALUEPTR(instr.index)));
    MOTH_END_INSTR(CallBuiltinDeleteSubscript)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteName)
        STOREVALUE(instr.result, __qmljs_delete_name(context, runtimeStrings[instr.name]));
    MOTH_END_INSTR(CallBuiltinDeleteName)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofMember)
        STOREVALUE(instr.result, __qmljs_builtin_typeof_member(context, VALUEPTR(instr.base), runtimeStrings[instr.member]));
    MOTH_END_INSTR(CallBuiltinTypeofMember)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofSubscript)
        STOREVALUE(instr.result, __qmljs_builtin_typeof_element(context, VALUEPTR(instr.base), VALUEPTR(instr.index)));
    MOTH_END_INSTR(CallBuiltinTypeofSubscript)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofName)
        STOREVALUE(instr.result, __qmljs_builtin_typeof_name(context, runtimeStrings[instr.name]));
    MOTH_END_INSTR(CallBuiltinTypeofName)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofValue)
        STOREVALUE(instr.result, __qmljs_builtin_typeof(context, VALUEPTR(instr.value)));
    MOTH_END_INSTR(CallBuiltinTypeofValue)

    MOTH_BEGIN_INSTR(CallBuiltinDeclareVar)
        __qmljs_builtin_declare_var(context, instr.isDeletable, runtimeStrings[instr.varName]);
    MOTH_END_INSTR(CallBuiltinDeclareVar)

    MOTH_BEGIN_INSTR(CallBuiltinDefineGetterSetter)
        __qmljs_builtin_define_getter_setter(context, VALUEPTR(instr.object), runtimeStrings[instr.name], VALUEPTR(instr.getter), VALUEPTR(instr.setter));
    MOTH_END_INSTR(CallBuiltinDefineGetterSetter)

    MOTH_BEGIN_INSTR(CallBuiltinDefineProperty)
        __qmljs_builtin_define_property(context, VALUEPTR(instr.object), runtimeStrings[instr.name], VALUEPTR(instr.value));
    MOTH_END_INSTR(CallBuiltinDefineProperty)

    MOTH_BEGIN_INSTR(CallBuiltinDefineArray)
        Q_ASSERT(instr.args + instr.argc <= stackSize);
        QV4::Value *args = stack + instr.args;
        STOREVALUE(instr.result, __qmljs_builtin_define_array(context, args, instr.argc));
    MOTH_END_INSTR(CallBuiltinDefineArray)

    MOTH_BEGIN_INSTR(CallBuiltinDefineObjectLiteral)
        QV4::Value *args = stack + instr.args;
    STOREVALUE(instr.result, __qmljs_builtin_define_object_literal(context, args, instr.internalClassId));
    MOTH_END_INSTR(CallBuiltinDefineObjectLiteral)

    MOTH_BEGIN_INSTR(CallBuiltinSetupArgumentsObject)
        STOREVALUE(instr.result, __qmljs_builtin_setup_arguments_object(context));
    MOTH_END_INSTR(CallBuiltinSetupArgumentsObject)

    MOTH_BEGIN_INSTR(CreateValue)
        Q_ASSERT(instr.callData + instr.argc + qOffsetOf(QV4::CallData, args)/sizeof(QV4::SafeValue) <= stackSize);
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData);
        callData->tag = QV4::Value::Integer_Type;
        callData->argc = instr.argc;
        callData->thisObject = QV4::Primitive::undefinedValue();
        STOREVALUE(instr.result, __qmljs_construct_value(context, VALUEPTR(instr.func), callData));
    MOTH_END_INSTR(CreateValue)

    MOTH_BEGIN_INSTR(CreateProperty)
        Q_ASSERT(instr.callData + instr.argc + qOffsetOf(QV4::CallData, args)/sizeof(QV4::SafeValue) <= stackSize);
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData);
        callData->tag = QV4::Value::Integer_Type;
        callData->argc = instr.argc;
        callData->thisObject = QV4::Primitive::undefinedValue();
        STOREVALUE(instr.result, __qmljs_construct_property(context, VALUEPTR(instr.base), runtimeStrings[instr.name], callData));
    MOTH_END_INSTR(CreateProperty)

    MOTH_BEGIN_INSTR(CreateActivationProperty)
        TRACE(inline, "property name = %s, args = %d, argc = %d", runtimeStrings[instr.name]->toQString().toUtf8().constData(), instr.args, instr.argc);
        Q_ASSERT(instr.callData + instr.argc + qOffsetOf(QV4::CallData, args)/sizeof(QV4::SafeValue) <= stackSize);
        QV4::CallData *callData = reinterpret_cast<QV4::CallData *>(stack + instr.callData);
        callData->tag = QV4::Value::Integer_Type;
        callData->argc = instr.argc;
        callData->thisObject = QV4::Primitive::undefinedValue();
        STOREVALUE(instr.result, __qmljs_construct_activation_property(context, runtimeStrings[instr.name], callData));
    MOTH_END_INSTR(CreateActivationProperty)

    MOTH_BEGIN_INSTR(Jump)
        code = ((uchar *)&instr.offset) + instr.offset;
    MOTH_END_INSTR(Jump)

    MOTH_BEGIN_INSTR(CJump)
        uint cond = __qmljs_to_boolean(VALUEPTR(instr.condition));
        TRACE(condition, "%s", cond ? "TRUE" : "FALSE");
        if (cond)
            code = ((uchar *)&instr.offset) + instr.offset;
    MOTH_END_INSTR(CJump)

    MOTH_BEGIN_INSTR(Unop)
        STOREVALUE(instr.result, instr.alu(VALUEPTR(instr.source)));
    MOTH_END_INSTR(Unop)

    MOTH_BEGIN_INSTR(Binop)
        STOREVALUE(instr.result, instr.alu(VALUEPTR(instr.lhs), VALUEPTR(instr.rhs)));
    MOTH_END_INSTR(Binop)

    MOTH_BEGIN_INSTR(BinopContext)
        STOREVALUE(instr.result, instr.alu(context, VALUEPTR(instr.lhs), VALUEPTR(instr.rhs)));
    MOTH_END_INSTR(BinopContext)

    MOTH_BEGIN_INSTR(AddNumberParams)
        double lhs = VALUE(instr.lhs).asDouble();
        double rhs = VALUE(instr.rhs).asDouble();
        VALUEPTR(instr.result)->setDouble(lhs + rhs);
    MOTH_END_INSTR(AddNumberParams)

    MOTH_BEGIN_INSTR(MulNumberParams)
        double lhs = VALUE(instr.lhs).asDouble();
        double rhs = VALUE(instr.rhs).asDouble();
        VALUEPTR(instr.result)->setDouble(lhs * rhs);
    MOTH_END_INSTR(MulNumberParams)

    MOTH_BEGIN_INSTR(SubNumberParams)
        double lhs = VALUE(instr.lhs).asDouble();
        double rhs = VALUE(instr.rhs).asDouble();
        VALUEPTR(instr.result)->setDouble(lhs - rhs);
    MOTH_END_INSTR(SubNumberParams)

    MOTH_BEGIN_INSTR(Ret)
        context->engine->stackPop(stackSize);
//        TRACE(Ret, "returning value %s", result.toString(context)->toQString().toUtf8().constData());
        return VALUE(instr.result).asReturnedValue();
    MOTH_END_INSTR(Ret)

    MOTH_BEGIN_INSTR(LoadThis)
        VALUE(instr.result) = context->callData->thisObject;
    MOTH_END_INSTR(LoadThis)

#ifdef MOTH_THREADED_INTERPRETER
    // nothing to do
#else
        default:
            qFatal("QQmlJS::Moth::VME: Internal error - unknown instruction %d", genericInstr->common.instructionType);
            break;
        }
    }
#endif

}

#ifdef MOTH_THREADED_INTERPRETER
void **VME::instructionJumpTable()
{
    static void **jumpTable = 0;
    if (!jumpTable) {
        const uchar *code = 0;
        VME().run(0, code, 0, 0, &jumpTable);
    }
    return jumpTable;
}
#endif

QV4::ReturnedValue VME::exec(QV4::ExecutionContext *ctxt, const uchar *code)
{
    VME vme;
    return vme.run(ctxt, code);
}
