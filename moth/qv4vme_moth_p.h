#ifndef QV4VME_MOTH_P_H
#define QV4VME_MOTH_P_H

#include "qmljs_runtime.h"
#include "qv4instr_moth_p.h"

namespace QQmlJS {
namespace Moth {

class VME
{
public:
    static void exec(VM::ExecutionContext *, const uchar *);

    void operator()(QQmlJS::VM::ExecutionContext *, const uchar *code
#ifdef MOTH_THREADED_INTERPRETER
            , void ***storeJumpTable = 0
#endif
            );

#ifdef MOTH_THREADED_INTERPRETER
    static void **instructionJumpTable();
#endif

private:
    static void restoreState(VM::ExecutionContext *context, int &targetTempIndex, const uchar *&code);
    static void saveState(VM::ExecutionContext *context, int targetTempIndex, const uchar *code);
};

} // namespace Moth
} // namespace QQmlJS

#endif // QV4VME_MOTH_P_H
