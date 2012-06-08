#ifndef QV4ISEL_MOTH_P_H
#define QV4ISEL_MOTH_P_H

#include "qv4ir_p.h"
#include "qmljs_objects.h"

namespace QQmlJS {
namespace Moth {

class InstructionSelection : public IR::StmtVisitor
{
public:
    InstructionSelection(VM::ExecutionEngine *engine, IR::Module *module, uchar *code);
    ~InstructionSelection();

    void operator()(IR::Function *function);

protected:
    virtual void visitExp(IR::Exp *);
    virtual void visitEnter(IR::Enter *);
    virtual void visitLeave(IR::Leave *);
    virtual void visitMove(IR::Move *);
    virtual void visitJump(IR::Jump *);
    virtual void visitCJump(IR::CJump *);
    virtual void visitRet(IR::Ret *);

private:
    IR::Function *_function;
    IR::BasicBlock *_block;
};

} // namespace Moth
} // namespace QQmlJS

#endif // QV4ISEL_MOTH_P_H
