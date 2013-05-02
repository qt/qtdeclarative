#include "qv4vme_moth_p.h"
#include "qv4instr_moth_p.h"
#include <private/qv4value_p.h>
#include <private/qv4debugging_p.h>

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

class FunctionState: public Debugging::FunctionState
{
public:
    FunctionState(QV4::ExecutionContext *context, const uchar **code)
        : Debugging::FunctionState(context)
        , stack(0)
        , stackSize(0)
        , code(code)
    {
        previousInstructionPointer = context->interpreterInstructionPointer;
        context->interpreterInstructionPointer = code;
    }
    ~FunctionState()
    {
        context()->interpreterInstructionPointer = previousInstructionPointer;
    }

    virtual QV4::Value *temp(unsigned idx) { return stack + idx; }

    void setStack(QV4::Value *stack, unsigned stackSize)
    { this->stack = stack; this->stackSize = stackSize; }

private:
    QV4::Value *stack;
    unsigned stackSize;
    const uchar **code;
    const uchar **previousInstructionPointer;
};

#define MOTH_BEGIN_INSTR_COMMON(I) { \
    const InstrMeta<(int)Instr::I>::DataType &instr = InstrMeta<(int)Instr::I>::data(*genericInstr); \
    code += InstrMeta<(int)Instr::I>::Size; \
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
                                     const Instr::Param &param
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
        Q_ASSERT((unsigned) arg < cc->argumentCount);
        Q_ASSERT(cc->arguments);
        return cc->arguments + arg;
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
        Q_ASSERT(param.index < stackSize);
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
# define VALUE(param) *getValueRef(context, stack, param, stackSize)
# define VALUEPTR(param) getValueRef(context, stack, param, stackSize)
#endif

QV4::Value VME::run(QV4::ExecutionContext *context, const uchar *&code,
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
        return QV4::Value::undefinedValue();
    }
#endif

    FunctionState state(context, &code);

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

    MOTH_BEGIN_INSTR(LoadValue)
//        TRACE(value, "%s", instr.value.toString(context)->toQString().toUtf8().constData());
        VALUE(instr.result) = VALUE(instr.value);
    MOTH_END_INSTR(LoadValue)

    MOTH_BEGIN_INSTR(LoadClosure)
        __qmljs_init_closure(context, VALUEPTR(instr.result), instr.value);
    MOTH_END_INSTR(LoadClosure)

    MOTH_BEGIN_INSTR(LoadName)
        TRACE(inline, "property name = %s", instr.name->toQString().toUtf8().constData());
        __qmljs_get_activation_property(context, VALUEPTR(instr.result), instr.name);
    MOTH_END_INSTR(LoadName)

    MOTH_BEGIN_INSTR(StoreName)
        TRACE(inline, "property name = %s", instr.name->toQString().toUtf8().constData());
        __qmljs_set_activation_property(context, instr.name, VALUE(instr.source));
    MOTH_END_INSTR(StoreName)

    MOTH_BEGIN_INSTR(LoadElement)
         __qmljs_get_element(context, VALUEPTR(instr.result), VALUE(instr.base), VALUE(instr.index));
    MOTH_END_INSTR(LoadElement)

    MOTH_BEGIN_INSTR(StoreElement)
        __qmljs_set_element(context, VALUE(instr.base), VALUE(instr.index), VALUE(instr.source));
    MOTH_END_INSTR(StoreElement)

    MOTH_BEGIN_INSTR(LoadProperty)
        __qmljs_get_property(context, VALUEPTR(instr.result), VALUE(instr.base), instr.name);
    MOTH_END_INSTR(LoadProperty)

    MOTH_BEGIN_INSTR(StoreProperty)
        __qmljs_set_property(context, VALUE(instr.base), instr.name, VALUE(instr.source));
    MOTH_END_INSTR(StoreProperty)

    MOTH_BEGIN_INSTR(Push)
        TRACE(inline, "stack size: %u", instr.value);
        stackSize = instr.value;
        stack = static_cast<QV4::Value *>(alloca(stackSize * sizeof(QV4::Value)));
        state.setStack(stack, stackSize);
    MOTH_END_INSTR(Push)

    MOTH_BEGIN_INSTR(CallValue)
#ifdef DO_TRACE_INSTR
        if (Debugging::Debugger *debugger = context->engine->debugger) {
            if (QV4::FunctionObject *o = (VALUE(instr.dest)).asFunctionObject()) {
                if (Debugging::FunctionDebugInfo *info = debugger->debugInfo(o)) {
                    QString n = debugger->name(o);
                    std::cerr << "*** Call to \"" << (n.isNull() ? "<no name>" : qPrintable(n)) << "\" defined @" << info->startLine << ":" << info->startColumn << std::endl;
                }
            }
        }
#endif // DO_TRACE_INSTR
        Q_ASSERT(instr.args + instr.argc <= stackSize);
        QV4::Value *args = stack + instr.args;
        __qmljs_call_value(context, VALUEPTR(instr.result), /*thisObject*/0, VALUE(instr.dest), args, instr.argc);
    MOTH_END_INSTR(CallValue)

    MOTH_BEGIN_INSTR(CallProperty)
        TRACE(property name, "%s, args=%u, argc=%u, this=%s", qPrintable(instr.name->toQString()), instr.args, instr.argc, (VALUE(instr.base)).toString(context)->toQString().toUtf8().constData());
        Q_ASSERT(instr.args + instr.argc <= stackSize);
        QV4::Value *args = stack + instr.args;
        __qmljs_call_property(context, VALUEPTR(instr.result), VALUE(instr.base), instr.name, args, instr.argc);
    MOTH_END_INSTR(CallProperty)

    MOTH_BEGIN_INSTR(CallElement)
        Q_ASSERT(instr.args + instr.argc <= stackSize);
        QV4::Value *args = stack + instr.args;
        __qmljs_call_element(context, VALUEPTR(instr.result), VALUE(instr.base), VALUE(instr.index), args, instr.argc);
    MOTH_END_INSTR(CallElement)

    MOTH_BEGIN_INSTR(CallActivationProperty)
        Q_ASSERT(instr.args + instr.argc <= stackSize);
        QV4::Value *args = stack + instr.args;
        __qmljs_call_activation_property(context, VALUEPTR(instr.result), instr.name, args, instr.argc);
    MOTH_END_INSTR(CallActivationProperty)

    MOTH_BEGIN_INSTR(CallBuiltinThrow)
        __qmljs_throw(context, VALUE(instr.arg));
    MOTH_END_INSTR(CallBuiltinThrow)

    MOTH_BEGIN_INSTR(EnterTry)
        VALUE(instr.exceptionVar) = QV4::Value::undefinedValue();
        try {
            const uchar *tryCode = ((uchar *)&instr.tryOffset) + instr.tryOffset;
            run(context, tryCode, stack, stackSize);
            code = tryCode;
        } catch (QV4::Exception &ex) {
            ex.accept(context);
            VALUE(instr.exceptionVar) = ex.value();
            try {
                QV4::ExecutionContext *catchContext = __qmljs_builtin_push_catch_scope(instr.exceptionVarName, ex.value(), context);
                const uchar *catchCode = ((uchar *)&instr.catchOffset) + instr.catchOffset;
                run(catchContext, catchCode, stack, stackSize);
                code = catchCode;
                context = __qmljs_builtin_pop_scope(catchContext);
            } catch (QV4::Exception &ex) {
                ex.accept(context);
                VALUE(instr.exceptionVar) = ex.value();
                const uchar *catchCode = ((uchar *)&instr.catchOffset) + instr.catchOffset;
                run(context, catchCode, stack, stackSize);
                code = catchCode;
            }
        }
    MOTH_END_INSTR(EnterTry)

    MOTH_BEGIN_INSTR(CallBuiltinFinishTry)
        return QV4::Value();
    MOTH_END_INSTR(CallBuiltinFinishTry)

    MOTH_BEGIN_INSTR(CallBuiltinPushScope)
        context = __qmljs_builtin_push_with_scope(VALUE(instr.arg), context);
    MOTH_END_INSTR(CallBuiltinPushScope)

    MOTH_BEGIN_INSTR(CallBuiltinPopScope)
        context = __qmljs_builtin_pop_scope(context);
    MOTH_END_INSTR(CallBuiltinPopScope)

    MOTH_BEGIN_INSTR(CallBuiltinForeachIteratorObject)
        __qmljs_foreach_iterator_object(context, VALUEPTR(instr.result), VALUE(instr.arg));
    MOTH_END_INSTR(CallBuiltinForeachIteratorObject)

    MOTH_BEGIN_INSTR(CallBuiltinForeachNextPropertyName)
        __qmljs_foreach_next_property_name(VALUEPTR(instr.result), VALUE(instr.arg));
    MOTH_END_INSTR(CallBuiltinForeachNextPropertyName)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteMember)
        __qmljs_delete_member(context, VALUEPTR(instr.result), VALUE(instr.base), instr.member);
    MOTH_END_INSTR(CallBuiltinDeleteMember)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteSubscript)
        __qmljs_delete_subscript(context, VALUEPTR(instr.result), VALUE(instr.base), VALUE(instr.index));
    MOTH_END_INSTR(CallBuiltinDeleteSubscript)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteName)
        __qmljs_delete_name(context, VALUEPTR(instr.result), instr.name);
    MOTH_END_INSTR(CallBuiltinDeleteName)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofMember)
        __qmljs_builtin_typeof_member(context, VALUEPTR(instr.result), VALUE(instr.base), instr.member);
    MOTH_END_INSTR(CallBuiltinTypeofMember)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofSubscript)
        __qmljs_builtin_typeof_element(context, VALUEPTR(instr.result), VALUE(instr.base), VALUE(instr.index));
    MOTH_END_INSTR(CallBuiltinTypeofSubscript)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofName)
        __qmljs_builtin_typeof_name(context, VALUEPTR(instr.result), instr.name);
    MOTH_END_INSTR(CallBuiltinTypeofName)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofValue)
        __qmljs_builtin_typeof(context, VALUEPTR(instr.result), VALUE(instr.value));
    MOTH_END_INSTR(CallBuiltinTypeofValue)

    MOTH_BEGIN_INSTR(CallBuiltinPostIncMember)
        __qmljs_builtin_post_increment_member(context, VALUEPTR(instr.result), VALUE(instr.base), instr.member);
    MOTH_END_INSTR(CallBuiltinTypeofMember)

    MOTH_BEGIN_INSTR(CallBuiltinPostIncSubscript)
        __qmljs_builtin_post_increment_element(context, VALUEPTR(instr.result), VALUE(instr.base), VALUEPTR(instr.index));
    MOTH_END_INSTR(CallBuiltinTypeofSubscript)

    MOTH_BEGIN_INSTR(CallBuiltinPostIncName)
        __qmljs_builtin_post_increment_name(context, VALUEPTR(instr.result), instr.name);
    MOTH_END_INSTR(CallBuiltinTypeofName)

    MOTH_BEGIN_INSTR(CallBuiltinPostIncValue)
        __qmljs_builtin_post_increment(VALUEPTR(instr.result), VALUEPTR(instr.value));
    MOTH_END_INSTR(CallBuiltinTypeofValue)

    MOTH_BEGIN_INSTR(CallBuiltinPostDecMember)
        __qmljs_builtin_post_decrement_member(context, VALUEPTR(instr.result), VALUE(instr.base), instr.member);
    MOTH_END_INSTR(CallBuiltinTypeofMember)

    MOTH_BEGIN_INSTR(CallBuiltinPostDecSubscript)
        __qmljs_builtin_post_decrement_element(context, VALUEPTR(instr.result), VALUE(instr.base), VALUE(instr.index));
    MOTH_END_INSTR(CallBuiltinTypeofSubscript)

    MOTH_BEGIN_INSTR(CallBuiltinPostDecName)
        __qmljs_builtin_post_decrement_name(context, VALUEPTR(instr.result), instr.name);
    MOTH_END_INSTR(CallBuiltinTypeofName)

    MOTH_BEGIN_INSTR(CallBuiltinPostDecValue)
        __qmljs_builtin_post_decrement(VALUEPTR(instr.result), VALUEPTR(instr.value));
    MOTH_END_INSTR(CallBuiltinTypeofValue)

    MOTH_BEGIN_INSTR(CallBuiltinDeclareVar)
        __qmljs_builtin_declare_var(context, instr.isDeletable, instr.varName);
    MOTH_END_INSTR(CallBuiltinDeclareVar)

    MOTH_BEGIN_INSTR(CallBuiltinDefineGetterSetter)
        __qmljs_builtin_define_getter_setter(context, VALUE(instr.object), instr.name, VALUEPTR(instr.getter), VALUEPTR(instr.setter));
    MOTH_END_INSTR(CallBuiltinDefineGetterSetter)

    MOTH_BEGIN_INSTR(CallBuiltinDefineProperty)
        __qmljs_builtin_define_property(context, VALUE(instr.object), instr.name, VALUEPTR(instr.value));
    MOTH_END_INSTR(CallBuiltinDefineProperty)

    MOTH_BEGIN_INSTR(CallBuiltinDefineArray)
        Q_ASSERT(instr.args + instr.argc <= stackSize);
        QV4::Value *args = stack + instr.args;
        __qmljs_builtin_define_array(context, VALUEPTR(instr.result), args, instr.argc);
    MOTH_END_INSTR(CallBuiltinDefineArray)

    MOTH_BEGIN_INSTR(CallBuiltinDefineObjectLiteral)
        QV4::Value *args = stack + instr.args;
        __qmljs_builtin_define_object_literal(context, VALUEPTR(instr.result), args, instr.internalClass);
    MOTH_END_INSTR(CallBuiltinDefineObjectLiteral)

    MOTH_BEGIN_INSTR(CreateValue)
        Q_ASSERT(instr.args + instr.argc <= stackSize);
        QV4::Value *args = stack + instr.args;
        __qmljs_construct_value(context, VALUEPTR(instr.result), VALUE(instr.func), args, instr.argc);
    MOTH_END_INSTR(CreateValue)

    MOTH_BEGIN_INSTR(CreateProperty)
        Q_ASSERT(instr.args + instr.argc <= stackSize);
        QV4::Value *args = stack + instr.args;
        __qmljs_construct_property(context, VALUEPTR(instr.result), VALUE(instr.base), instr.name, args, instr.argc);
    MOTH_END_INSTR(CreateProperty)

    MOTH_BEGIN_INSTR(CreateActivationProperty)
        TRACE(inline, "property name = %s, args = %d, argc = %d", instr.name->toQString().toUtf8().constData(), instr.args, instr.argc);
        Q_ASSERT(instr.args + instr.argc <= stackSize);
        QV4::Value *args = stack + instr.args;
        __qmljs_construct_activation_property(context, VALUEPTR(instr.result), instr.name, args, instr.argc);
    MOTH_END_INSTR(CreateActivationProperty)

    MOTH_BEGIN_INSTR(Jump)
        code = ((uchar *)&instr.offset) + instr.offset;
    MOTH_END_INSTR(Jump)

    MOTH_BEGIN_INSTR(CJump)
        uint cond = __qmljs_to_boolean(VALUE(instr.condition));
        TRACE(condition, "%s", cond ? "TRUE" : "FALSE");
        if (cond)
            code = ((uchar *)&instr.offset) + instr.offset;
    MOTH_END_INSTR(CJump)

    MOTH_BEGIN_INSTR(Unop)
        instr.alu(VALUEPTR(instr.result), VALUE(instr.source));
    MOTH_END_INSTR(Unop)

    MOTH_BEGIN_INSTR(Binop)
        instr.alu(context, VALUEPTR(instr.result), VALUE(instr.lhs), VALUE(instr.rhs));
    MOTH_END_INSTR(Binop)

    MOTH_BEGIN_INSTR(AddNumberParams)
        double lhs = VALUEPTR(instr.lhs)->asDouble();
        double rhs = VALUEPTR(instr.rhs)->asDouble();
        VALUEPTR(instr.result)->setDouble(lhs + rhs);
    MOTH_END_INSTR(AddNumberParams)

    MOTH_BEGIN_INSTR(MulNumberParams)
        double lhs = VALUEPTR(instr.lhs)->asDouble();
        double rhs = VALUEPTR(instr.rhs)->asDouble();
        VALUEPTR(instr.result)->setDouble(lhs * rhs);
    MOTH_END_INSTR(MulNumberParams)

    MOTH_BEGIN_INSTR(SubNumberParams)
        double lhs = VALUEPTR(instr.lhs)->asDouble();
        double rhs = VALUEPTR(instr.rhs)->asDouble();
        VALUEPTR(instr.result)->setDouble(lhs - rhs);
    MOTH_END_INSTR(SubNumberParams)

    MOTH_BEGIN_INSTR(Ret)
        QV4::Value &result = VALUE(instr.result);
//        TRACE(Ret, "returning value %s", result.toString(context)->toQString().toUtf8().constData());
        return result;
    MOTH_END_INSTR(Ret)

    MOTH_BEGIN_INSTR(LoadThis)
        VALUE(instr.result) = context->thisObject;
    MOTH_END_INSTR(LoadThis)

    MOTH_BEGIN_INSTR(InplaceElementOp)
        instr.alu(context,
                  VALUE(instr.base),
                  VALUE(instr.index),
                  VALUE(instr.source));
    MOTH_END_INSTR(InplaceElementOp)

    MOTH_BEGIN_INSTR(InplaceMemberOp)
        instr.alu(context,
                  VALUE(instr.base),
                  instr.member,
                  VALUE(instr.source));
    MOTH_END_INSTR(InplaceMemberOp)

    MOTH_BEGIN_INSTR(InplaceNameOp)
        TRACE(name, "%s", instr.name->toQString().toUtf8().constData());
        instr.alu(context, instr.name, VALUE(instr.source));
    MOTH_END_INSTR(InplaceNameOp)

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

QV4::Value VME::exec(QV4::ExecutionContext *ctxt, const uchar *code)
{
    VME vme;
    return vme.run(ctxt, code);
}
