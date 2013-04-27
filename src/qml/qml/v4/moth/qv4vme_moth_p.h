#ifndef QV4VME_MOTH_P_H
#define QV4VME_MOTH_P_H

#include <private/qv4runtime_p.h>
#include "qv4instr_moth_p.h"

namespace QV4 {
    struct Value;
}

namespace QQmlJS {
namespace Moth {

class VME
{
public:
    static QV4::Value exec(QV4::ExecutionContext *, const uchar *);

#ifdef MOTH_THREADED_INTERPRETER
    static void **instructionJumpTable();
#endif

private:
    QV4::Value run(QV4::ExecutionContext *, const uchar *&code,
            QV4::Value *stack = 0, unsigned stackSize = 0
#ifdef MOTH_THREADED_INTERPRETER
            , void ***storeJumpTable = 0
#endif
            );
};

} // namespace Moth
} // namespace QQmlJS

#endif // QV4VME_MOTH_P_H
