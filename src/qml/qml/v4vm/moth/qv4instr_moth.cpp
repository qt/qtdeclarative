#include "qv4instr_moth_p.h"

using namespace QQmlJS;
using namespace QQmlJS::Moth;

int Instr::size(Type type)
{
#define MOTH_RETURN_INSTR_SIZE(I, FMT) case I: return InstrMeta<(int)I>::Size;
    switch (type) {
    FOR_EACH_MOTH_INSTR(MOTH_RETURN_INSTR_SIZE)
    default: return 0;
    }
#undef MOTH_RETURN_INSTR_SIZE
}

