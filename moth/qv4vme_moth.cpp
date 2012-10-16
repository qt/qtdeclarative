#include "qv4vme_moth_p.h"
#include "qv4instr_moth_p.h"

using namespace QQmlJS;
using namespace QQmlJS::Moth;

#define MOTH_BEGIN_INSTR_COMMON(I) { \
    const InstrMeta<(int)Instr::I>::DataType &instr = InstrMeta<(int)Instr::I>::data(*genericInstr); \
    code += InstrMeta<(int)Instr::I>::Size; \
    Q_UNUSED(instr);

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
    if (index < 0) {
        const int arg = -index - 1;
        return context->arguments + arg;
    } else if (index < stack.count()) {
        return stack.data() + index;
    } else {
        return context->locals + index - stack.count();
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
        tempRegister = VM::Value::fromDouble(instr.value);
    MOTH_END_INSTR(LoadNumber)

    MOTH_BEGIN_INSTR(LoadString)
        tempRegister = VM::Value::fromString(instr.value);
    MOTH_END_INSTR(LoadString)

    MOTH_BEGIN_INSTR(LoadClosure)
        tempRegister = __qmljs_init_closure(instr.value, context);
    MOTH_END_INSTR(LoadClosure)

    MOTH_BEGIN_INSTR(LoadName)
        __qmljs_get_activation_property(context, &tempRegister, instr.value);  
    MOTH_END_INSTR(LoadName)

    MOTH_BEGIN_INSTR(Push)
        stack.resize(instr.value);
    MOTH_END_INSTR(Push)

    MOTH_BEGIN_INSTR(Call)
        VM::Value *args = stack.data() + instr.args;
        tempRegister = __qmljs_call_value(context, VM::Value::undefinedValue(), &tempRegister, args, instr.argc);
    MOTH_END_INSTR(Call)

    MOTH_BEGIN_INSTR(Jump)
        code = ((uchar *)&instr.offset) + instr.offset;
    MOTH_END_INSTR(Jump)

    MOTH_BEGIN_INSTR(CJump)
        if (__qmljs_to_boolean(tempRegister, context))
            code = ((uchar *)&instr.offset) + instr.offset;
    MOTH_END_INSTR(CJump)

    MOTH_BEGIN_INSTR(Binop)
        instr.alu(context, &tempRegister, &TEMP(instr.lhsTempIndex), &TEMP(instr.rhsTempIndex));
    MOTH_END_INSTR(Binop)

    MOTH_BEGIN_INSTR(Ret)
        context->result = TEMP(instr.tempIndex);
        return;
    MOTH_END_INSTR(Ret)

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

