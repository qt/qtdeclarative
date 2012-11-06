#include "qv4vme_moth_p.h"
#include "qv4instr_moth_p.h"

#ifdef DO_TRACE_INSTR
#  define TRACE_INSTR(I) fprintf(stderr, "executing a %s\n", #I);
#  define TRACE(n, str, ...) { fprintf(stderr, "    %s : ", #n); fprintf(stderr, str, __VA_ARGS__); fprintf(stderr, "\n"); }
#else
#  define TRACE_INSTR(I)
#  define TRACE(n, str, ...)
#endif // DO_TRACE_INSTR

using namespace QQmlJS;
using namespace QQmlJS::Moth;

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

static inline VM::Value *tempValue(QQmlJS::VM::Context *context, QVector<VM::Value> &stack, int index)
{
    TRACE(tempValue, "index = %d / arg = %d / local = %d, stack size = %d", index, (-index-1), index - stack.count(), stack.size());

    if (index < 0) {
        const int arg = -index - 1;
        return context->arguments + arg;
    } else if (index < (int) context->varCount) {
        return context->locals + index;
    } else {
        int off = index - context->varCount;
        return stack.data() + off;
    }
}

#define TEMP(index) *tempValue(context, stack, index)

void VME::operator()(QQmlJS::VM::Context *context, const uchar *code
#ifdef MOTH_THREADED_INTERPRETER
        , void ***storeJumpTable
#endif
        )
{

#ifdef MOTH_THREADED_INTERPRETER
    if (storeJumpTable) {
#define MOTH_INSTR_ADDR(I, FMT) &&op_##I,
        static void *jumpTable[] = {
            FOR_EACH_MOTH_INSTR(MOTH_INSTR_ADDR)
        };
#undef MOTH_INSTR_ADDR
        *storeJumpTable = jumpTable;
        return;
    }
#endif

    QVector<VM::Value> stack;
    VM::Value tempRegister;

#ifdef MOTH_THREADED_INTERPRETER
    const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
    goto *genericInstr->common.code;
#else
    for (;;) {
        const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
        switch (genericInstr->common.instructionType) {
#endif

    MOTH_BEGIN_INSTR(StoreTemp)
        TEMP(instr.tempIndex) = tempRegister;
    MOTH_END_INSTR(StoreTemp)

    MOTH_BEGIN_INSTR(LoadTemp)
        tempRegister = TEMP(instr.tempIndex);
    MOTH_END_INSTR(LoadTemp)

    MOTH_BEGIN_INSTR(MoveTemp)
        TEMP(instr.toTempIndex) = TEMP(instr.fromTempIndex);
    MOTH_END_INSTR(MoveTemp)

    MOTH_BEGIN_INSTR(LoadUndefined)
        tempRegister = VM::Value::undefinedValue();
    MOTH_END_INSTR(LoadUndefined)

    MOTH_BEGIN_INSTR(LoadNull)
        tempRegister = VM::Value::nullValue();
    MOTH_END_INSTR(LoadNull)

    MOTH_BEGIN_INSTR(LoadTrue)
        tempRegister = VM::Value::fromBoolean(true);
    MOTH_END_INSTR(LoadTrue)

    MOTH_BEGIN_INSTR(LoadFalse)
        tempRegister = VM::Value::fromBoolean(false);
    MOTH_END_INSTR(LoadFalse)

    MOTH_BEGIN_INSTR(LoadNumber)
        TRACE(inline, "number = %f", instr.value);
        tempRegister = VM::Value::fromDouble(instr.value);
    MOTH_END_INSTR(LoadNumber)

    MOTH_BEGIN_INSTR(LoadString)
        tempRegister = VM::Value::fromString(instr.value);
    MOTH_END_INSTR(LoadString)

    MOTH_BEGIN_INSTR(LoadClosure)
        tempRegister = __qmljs_init_closure(instr.value, context);
    MOTH_END_INSTR(LoadClosure)

    MOTH_BEGIN_INSTR(LoadName)
        TRACE(inline, "property name = %s", instr.name->toQString().toUtf8().constData());
        tempRegister = __qmljs_get_activation_property(context, instr.name);
    MOTH_END_INSTR(LoadName)

    MOTH_BEGIN_INSTR(StoreName)
        TRACE(inline, "property name = %s", instr.name->toQString().toUtf8().constData());
        __qmljs_set_activation_property(context, instr.name, tempRegister);
    MOTH_END_INSTR(StoreName)

    MOTH_BEGIN_INSTR(Push)
        TRACE(inline, "stack size: %u", instr.value);
        stack.resize(instr.value);
    MOTH_END_INSTR(Push)

    MOTH_BEGIN_INSTR(CallValue)
        VM::Value *args = stack.data() + instr.args;
        tempRegister = __qmljs_call_value(context, VM::Value::undefinedValue(), tempRegister, args, instr.argc);
    MOTH_END_INSTR(CallValue)

    MOTH_BEGIN_INSTR(CallProperty)
        VM::Value *args = stack.data() + instr.args;
        tempRegister = __qmljs_call_property(context, tempRegister, instr.name, args, instr.argc);
    MOTH_END_INSTR(CallProperty)

    MOTH_BEGIN_INSTR(CallBuiltin)
        VM::Value *args = stack.data() + instr.args;
        void *buf;
        switch (instr.builtin) {
        case Instr::instr_callBuiltin::builtin_typeof:
            tempRegister = __qmljs_builtin_typeof(args[0], context);
            break;
        case Instr::instr_callBuiltin::builtin_throw:
            __qmljs_builtin_typeof(args[0], context);
            break;
        case Instr::instr_callBuiltin::builtin_create_exception_handler:
            buf = __qmljs_create_exception_handler(context);
            tempRegister = VM::Value::fromInt32(setjmp(* static_cast<jmp_buf *>(buf)));
            break;
        case Instr::instr_callBuiltin::builtin_delete_exception_handler:
            __qmljs_delete_exception_handler(context);
            break;
        case Instr::instr_callBuiltin::builtin_get_exception:
            tempRegister = __qmljs_get_exception(context);
            break;
        }
    MOTH_END_INSTR(CallBuiltin)

    MOTH_BEGIN_INSTR(Jump)
        code = ((uchar *)&instr.offset) + instr.offset;
    MOTH_END_INSTR(Jump)

    MOTH_BEGIN_INSTR(CJump)
        if (__qmljs_to_boolean(tempRegister, context))
            code = ((uchar *)&instr.offset) + instr.offset;
    MOTH_END_INSTR(CJump)

    MOTH_BEGIN_INSTR(Binop)
        tempRegister = instr.alu(TEMP(instr.lhsTempIndex), TEMP(instr.rhsTempIndex), context);
    MOTH_END_INSTR(Binop)

    MOTH_BEGIN_INSTR(Ret)
        context->result = TEMP(instr.tempIndex);
        return;
    MOTH_END_INSTR(Ret)

    MOTH_BEGIN_INSTR(LoadThis)
        tempRegister = __qmljs_get_thisObject(context);
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
        VME dummy;
        dummy(0, 0, &jumpTable);
    }
    return jumpTable;
}
#endif

void VME::exec(VM::Context *ctxt, const uchar *code)
{
    VME vme;
    vme(ctxt, code);
}

