#include "qv4isel_moth_p.h"
#include "qv4vme_moth_p.h"

using namespace QQmlJS;
using namespace QQmlJS::Moth;

InstructionSelection::InstructionSelection(VM::ExecutionEngine *engine, IR::Module *module,
                                           uchar *code)
: _code(code), _ccode(code)
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

    addInstruction(Instruction::Done());

    qSwap(_function, function);
}

void InstructionSelection::visitExp(IR::Exp *)
{
    qWarning("%s", __PRETTY_FUNCTION__);
}

void InstructionSelection::visitEnter(IR::Enter *)
{
    qWarning("%s", __PRETTY_FUNCTION__);
}

void InstructionSelection::visitLeave(IR::Leave *)
{
    qWarning("%s", __PRETTY_FUNCTION__);
}

void InstructionSelection::visitMove(IR::Move *)
{
    qWarning("%s", __PRETTY_FUNCTION__);
}

void InstructionSelection::visitJump(IR::Jump *)
{
    qWarning("%s", __PRETTY_FUNCTION__);
}

void InstructionSelection::visitCJump(IR::CJump *)
{
    qWarning("%s", __PRETTY_FUNCTION__);
}

void InstructionSelection::visitRet(IR::Ret *)
{
    qWarning("%s", __PRETTY_FUNCTION__);
}

int InstructionSelection::addInstructionHelper(Instr::Type type, Instr &instr)
{
#ifdef MOTH_THREADED_INTERPRETER
    instr.common.code = VME::instructionJumpTable()[static_cast<int>(type)];
#else
    instr.common.instructionType = type;
#endif

    // XXX - int is wrong size for both of these
    int ptrOffset = (int)(_ccode - _code);
    int size = Instr::size(type);

    ::memcpy(_ccode, reinterpret_cast<const char *>(&instr), size);
    _ccode += size;

    return ptrOffset;
}

