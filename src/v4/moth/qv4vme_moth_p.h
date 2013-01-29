#ifndef QV4VME_MOTH_P_H
#define QV4VME_MOTH_P_H

#include "qmljs_runtime.h"
#include "qv4instr_moth_p.h"

namespace QQmlJS {
namespace VM {
    struct Value;
}

namespace Moth {

class VME
{
public:
    static VM::Value exec(VM::ExecutionContext *, const uchar *);

    VM::Value operator()(QQmlJS::VM::ExecutionContext *, const uchar *code
#ifdef MOTH_THREADED_INTERPRETER
            , void ***storeJumpTable = 0
#endif
            );

#ifdef MOTH_THREADED_INTERPRETER
    static void **instructionJumpTable();
#endif

private:
    static void restoreState(VM::ExecutionContext *context, VM::Value *&target, const uchar *&code);
    static void saveState(VM::ExecutionContext *context, VM::Value *target, const uchar *code);
};

} // namespace Moth
} // namespace QQmlJS

#endif // QV4VME_MOTH_P_H
