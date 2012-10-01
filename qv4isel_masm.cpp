
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
    qSwap(_function, function);

    enterStandardStackFrame();

    int locals = (_function->tempCount - _function->locals.size() + _function->maxNumberOfArguments) * sizeof(Value);
    locals = (locals + 15) & ~15;
    sub32(TrustedImm32(locals), StackPointerRegister);

    push(ContextRegister);
    loadPtr(addressForArgument(0), ContextRegister);

    foreach (IR::BasicBlock *block, _function->basicBlocks) {
        _block = block;
    //    _addrs[block] = _codePtr;
        foreach (IR::Stmt *s, block->statements) {
            s->accept(this);
        }
    }

    pop(ContextRegister);

    add32(TrustedImm32(locals), StackPointerRegister);

    leaveStandardStackFrame();
    ret();

    JSC::JSGlobalData dummy;
    JSC::LinkBuffer linkBuffer(dummy, this, 0);
    foreach (CallToLink ctl, _callsToLink)
        linkBuffer.link(ctl.call, ctl.externalFunction);
    _function->codeRef = linkBuffer.finalizeCodeWithDisassembly("operator()(IR::Function*)");
    _function->code = (void (*)(VM::Context *, const uchar *)) _function->codeRef.code().executableAddress();

    qSwap(_function, function);
}

String *InstructionSelection::identifier(const QString &s)
{
    return _engine->identifier(s);
}

JSC::MacroAssembler::Address InstructionSelection::loadTempAddress(RegisterID reg, IR::Temp *t)
{
    int32_t offset = 0;
    if (t->index < 0) {
        const int arg = -t->index - 1;
        loadPtr(Address(ContextRegister, offsetof(Context, arguments)), reg);
        offset = arg * sizeof(Value);
    } else if (t->index < _function->locals.size()) {
        loadPtr(Address(ContextRegister, offsetof(Context, locals)), reg);
        offset = t->index * sizeof(Value);
    } else {
        const int arg = _function->maxNumberOfArguments + t->index - _function->locals.size();
        offset = sizeof(Value) * (-arg)
                 - sizeof(void*); // size of ebp
        reg = StackFrameRegister;
    }
    return Address(reg, offset);
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
    if (s->op == IR::OpInvalid) {
        if (IR::Temp *t = s->target->asTemp()) {
            if (IR::Const *c = s->source->asConst()) {
                Address dest = loadTempAddress(Gpr0, t);
                switch (c->type) {
                case IR::NullType:
                    storeValue<Value::Null_Type>(TrustedImm32(0), dest);
                    break;
                case IR::UndefinedType:
                    storeValue<Value::Undefined_Type>(TrustedImm32(0), dest);
                    break;
                case IR::BoolType:
                    storeValue<Value::Boolean_Type>(TrustedImm32(c->value != 0), dest);
                    break;
                case IR::NumberType:
                    // ### Taking address of pointer inside IR.
                    loadDouble(&c->value, FPGpr0);
                    storeDouble(FPGpr0, dest);
                    break;
                default:
                    Q_UNIMPLEMENTED();
                    assert(!"TODO");
                }
            }
        }
    }
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
    if (IR::Temp *t = s->expr->asTemp()) {
        Address addr = loadTempAddress(Gpr0, t);
        add32(TrustedImm32(addr.offset), addr.base, Gpr0);
        add32(TrustedImm32(offsetof(Context, result)), ContextRegister, Gpr1);
        callHelper(__qmljs_copy, Gpr1, Gpr0);
        return;
    }
    Q_UNIMPLEMENTED();
    Q_UNUSED(s);
}

