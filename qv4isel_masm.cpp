
#include "qv4isel_masm_p.h"
#include "qmljs_runtime.h"
#include "qmljs_objects.h"

#include <assembler/LinkBuffer.h>

#include <sys/mman.h>
#include <iostream>
#include <cassert>

#ifndef NO_UDIS86
#  include <udis86.h>
#endif

using namespace QQmlJS;
using namespace QQmlJS::MASM;
using namespace QQmlJS::VM;

namespace {
QTextStream qout(stdout, QIODevice::WriteOnly);
}

InstructionSelection::InstructionSelection(VM::ExecutionEngine *engine, IR::Module *module, uchar *buffer)
    : _engine(engine)
    , _module(module)
    , _function(0)
    , _block(0)
    , _buffer(buffer)
    , _code(buffer)
    , _codePtr(buffer)
{
}

InstructionSelection::~InstructionSelection()
{
}

void InstructionSelection::operator()(IR::Function *function)
{
    _assembler.ret();
    JSC::JSGlobalData dummy;
    JSC::LinkBuffer linkBuffer(dummy, &_assembler, 0);
    JSC::MacroAssembler::CodeRef code = linkBuffer.finalizeCodeWithDisassembly("operator()(IR::Function*)");
    memcpy(_buffer, code.executableMemory()->start(), code.size());
    function->code = (void (*)(VM::Context *, const uchar *)) _buffer;
}

String *InstructionSelection::identifier(const QString &s)
{
    return _engine->identifier(s);
}

void InstructionSelection::loadTempAddress(int reg, IR::Temp *t)
{
}

void InstructionSelection::callActivationProperty(IR::Call *call, IR::Temp *result)
{
}


void InstructionSelection::callValue(IR::Call *call, IR::Temp *result)
{
}

void InstructionSelection::callProperty(IR::Call *call, IR::Temp *result)
{
}

void InstructionSelection::constructActivationProperty(IR::New *call, IR::Temp *result)
{
}

void InstructionSelection::constructProperty(IR::New *call, IR::Temp *result)
{
}

void InstructionSelection::constructValue(IR::New *call, IR::Temp *result)
{
}

void InstructionSelection::checkExceptions()
{
}

void InstructionSelection::visitExp(IR::Exp *s)
{
    Q_UNIMPLEMENTED();
    assert(!"TODO");
}

void InstructionSelection::visitEnter(IR::Enter *)
{
    Q_UNIMPLEMENTED();
    assert(!"TODO");
}

void InstructionSelection::visitLeave(IR::Leave *)
{
    Q_UNIMPLEMENTED();
    assert(!"TODO");
}

void InstructionSelection::visitMove(IR::Move *s)
{
    Q_UNIMPLEMENTED();
    s->dump(qout, IR::Stmt::MIR);
    qout << endl;
    assert(!"TODO");
}

void InstructionSelection::visitJump(IR::Jump *s)
{
}

void InstructionSelection::visitCJump(IR::CJump *s)
{
    Q_UNIMPLEMENTED();
    assert(!"TODO");
}

void InstructionSelection::visitRet(IR::Ret *s)
{
    Q_UNIMPLEMENTED();
    Q_UNUSED(s);
}

