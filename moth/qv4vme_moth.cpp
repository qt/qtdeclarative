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

void VME::operator()(QQmlJS::VM::Context *, const uchar *code
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

#ifdef MOTH_THREADED_INTERPRETER
    const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
    goto *genericInstr->common.code;
#else
    for (;;) {
        const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
        switch (genericInstr->common.instructionType) {
#endif

    MOTH_BEGIN_INSTR(Nop)
        qWarning("NOP");
    MOTH_END_INSTR(Nop)

    MOTH_BEGIN_INSTR(Done)
        return;
    MOTH_END_INSTR(Done)

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

