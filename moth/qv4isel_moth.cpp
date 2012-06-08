#include "qv4isel_moth_p.h"

using namespace QQmlJS;
using namespace QQmlJS::Moth;

InstructionSelection::InstructionSelection(VM::ExecutionEngine *engine, IR::Module *module,
                                           uchar *code)
{
}

InstructionSelection::~InstructionSelection()
{
}

void InstructionSelection::operator()(IR::Function *function)
{
    qSwap(_function, function);

    foreach (_block, _function->basicBlocks) {
        foreach (IR::Stmt *s, _block->statements)
            s->accept(this);
    }

    qSwap(_function, function);
}

void InstructionSelection::visitExp(IR::Exp *)
{
    qWarning(__PRETTY_FUNCTION__);
}

void InstructionSelection::visitEnter(IR::Enter *)
{
    qWarning(__PRETTY_FUNCTION__);
}

void InstructionSelection::visitLeave(IR::Leave *)
{
    qWarning(__PRETTY_FUNCTION__);
}

void InstructionSelection::visitMove(IR::Move *)
{
    qWarning(__PRETTY_FUNCTION__);
}

void InstructionSelection::visitJump(IR::Jump *)
{
    qWarning(__PRETTY_FUNCTION__);
}

void InstructionSelection::visitCJump(IR::CJump *)
{
    qWarning(__PRETTY_FUNCTION__);
}

void InstructionSelection::visitRet(IR::Ret *)
{
    qWarning(__PRETTY_FUNCTION__);
}

