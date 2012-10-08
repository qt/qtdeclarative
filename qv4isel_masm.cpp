
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

    int locals = (_function->tempCount - _function->locals.size() + _function->maxNumberOfArguments) * sizeof(Value);
    locals = (locals + 15) & ~15;
    enterStandardStackFrame(locals);

    push(ContextRegister);
    loadPtr(addressForArgument(0), ContextRegister);

    foreach (IR::BasicBlock *block, _function->basicBlocks) {
        _block = block;
        _addrs[block] = label();
        foreach (IR::Stmt *s, block->statements) {
            s->accept(this);
        }
    }

    pop(ContextRegister);

    leaveStandardStackFrame(locals);
    ret();

    QHashIterator<IR::BasicBlock *, QVector<Jump> > it(_patches);
    while (it.hasNext()) {
        it.next();
        IR::BasicBlock *block = it.key();
        Label target = _addrs.value(block);
        assert(target.isSet());
        foreach (Jump jump, it.value())
            jump.linkTo(target, this);
    }

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

InstructionSelection::Pointer InstructionSelection::loadTempAddress(RegisterID reg, IR::Temp *t)
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
        offset = sizeof(Value) * (-arg - 1)
                 - sizeof(void*); // size of ebp
        reg = StackFrameRegister;
    }
    return Pointer(reg, offset);
}

void InstructionSelection::callActivationProperty(IR::Call *call, IR::Temp *result)
{
    IR::Name *baseName = call->base->asName();
    assert(baseName != 0);

    if (baseName->id)
        callRuntimeMethod(__qmljs_call_activation_property, result, call->base, call->args);
    else {
        switch (baseName->builtin) {
        case IR::Name::builtin_invalid:
            Q_UNREACHABLE();
            break;
        case IR::Name::builtin_typeof:
            callRuntimeMethod(__qmljs_builtin_typeof, result, call->args);
            break;
        case IR::Name::builtin_delete:
            Q_UNREACHABLE();
            break;
        case IR::Name::builtin_throw:
            callRuntimeMethod(__qmljs_builtin_throw, result, call->args);
            break;
        case IR::Name::builtin_rethrow:
            callRuntimeMethod(__qmljs_builtin_rethrow, result, call->args);
            return; // we need to return to avoid checking the exceptions
        }
    }
}


void InstructionSelection::callValue(IR::Call *call, IR::Temp *result)
{
    IR::Temp *baseTemp = call->base->asTemp();
    assert(baseTemp != 0);

    int argc = prepareVariableArguments(call->args);
    IR::Temp* thisObject = 0;
    generateFunctionCall(__qmljs_call_value, ContextRegister, result, baseTemp, thisObject, baseAddressForCallArguments(), TrustedImm32(argc));
    checkExceptions();
}

void InstructionSelection::callProperty(IR::Call *call, IR::Temp *result)
{
    IR::Member *member = call->base->asMember();
    assert(member != 0);
    assert(member->base->asTemp() != 0);

    int argc = prepareVariableArguments(call->args);
    IR::Temp* thisObject = 0;
    generateFunctionCall(__qmljs_call_property, ContextRegister, result, member->base->asTemp(), identifier(*member->name), baseAddressForCallArguments(), TrustedImm32(argc));
    checkExceptions();
}

void InstructionSelection::constructActivationProperty(IR::New *call, IR::Temp *result)
{
    IR::Name *baseName = call->base->asName();
    assert(baseName != 0);

    callRuntimeMethod(__qmljs_construct_activation_property, result, call->base, call->args);
}

void InstructionSelection::constructProperty(IR::New *call, IR::Temp *result)
{
    IR::Member *member = call->base->asMember();
    assert(member != 0);
    assert(member->base->asTemp() != 0);

    int argc = prepareVariableArguments(call->args);
    IR::Temp* thisObject = 0;
    generateFunctionCall(__qmljs_construct_property, ContextRegister, result, member->base->asTemp(), identifier(*member->name), baseAddressForCallArguments(), TrustedImm32(argc));
    checkExceptions();
}

void InstructionSelection::constructValue(IR::New *call, IR::Temp *result)
{
    IR::Temp *baseTemp = call->base->asTemp();
    assert(baseTemp != 0);

    int argc = prepareVariableArguments(call->args);
    generateFunctionCall(__qmljs_construct_value, ContextRegister, result, baseTemp, baseAddressForCallArguments(), TrustedImm32(argc));
    checkExceptions();
}

void InstructionSelection::checkExceptions()
{
    Address addr(ContextRegister, offsetof(Context, hasUncaughtException));
    Jump jmp = branch8(Equal, addr, TrustedImm32(1));
    _patches[_function->handlersBlock].append(jmp);
}

void InstructionSelection::visitExp(IR::Exp *s)
{
    if (IR::Call *c = s->expr->asCall()) {
        if (c->base->asName()) {
            callActivationProperty(c, 0);
            return;
        } else if (c->base->asTemp()) {
            callValue(c, 0);
            return;
        } else if (c->base->asMember()) {
            callProperty(c, 0);
            return;
        }
    }
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
        if (IR::Name *n = s->target->asName()) {
            String *propertyName = identifier(*n->id);

            if (IR::Temp *t = s->source->asTemp()) {
                generateFunctionCall(__qmljs_set_activation_property, ContextRegister, propertyName, t);
                checkExceptions();
                return;
            } else {
                Q_UNREACHABLE();
            }
        } else if (IR::Temp *t = s->target->asTemp()) {
            if (IR::Name *n = s->source->asName()) {
                if (*n->id == QStringLiteral("this")) { // ### `this' should be a builtin.
                    generateFunctionCall(__qmljs_get_thisObject, ContextRegister, t);
                } else {
                    String *propertyName = identifier(*n->id);
                    generateFunctionCall(__qmljs_get_activation_property, ContextRegister, t, propertyName);
                    checkExceptions();
                }
                return;
            } else if (IR::Const *c = s->source->asConst()) {
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
                return;
            } else if (IR::Temp *t2 = s->source->asTemp()) {
                generateFunctionCall(__qmljs_copy, t, t2);
                return;
            } else if (IR::String *str = s->source->asString()) {
                generateFunctionCall(__qmljs_init_string, t, _engine->newString(*str->value));
                return;
            } else if (IR::Closure *clos = s->source->asClosure()) {
                generateFunctionCall(__qmljs_init_closure, ContextRegister, t, TrustedImmPtr(clos->value));
                return;
            } else if (IR::New *ctor = s->source->asNew()) {
                if (ctor->base->asName()) {
                    constructActivationProperty(ctor, t);
                    return;
                } else if (ctor->base->asMember()) {
                    constructProperty(ctor, t);
                    return;
                } else if (ctor->base->asTemp()) {
                    constructValue(ctor, t);
                    return;
                }
            } else if (IR::Member *m = s->source->asMember()) {
                //__qmljs_get_property(ctx, result, object, name);
                if (IR::Temp *base = m->base->asTemp()) {
                    generateFunctionCall(__qmljs_get_property, ContextRegister, t, base, identifier(*m->name));
                    checkExceptions();
                    return;
                }
                assert(!"wip");
                return;
            } else if (IR::Subscript *ss = s->source->asSubscript()) {
                generateFunctionCall(__qmljs_get_element, ContextRegister, t, ss->base->asTemp(), ss->index->asTemp());
                checkExceptions();
                return;
            } else if (IR::Unop *u = s->source->asUnop()) {
                if (IR::Temp *e = u->expr->asTemp()) {
                    void (*op)(Context *, Value *, const Value *) = 0;
                    switch (u->op) {
                    case IR::OpIfTrue: assert(!"unreachable"); break;
                    case IR::OpNot: op = __qmljs_not; break;
                    case IR::OpUMinus: op = __qmljs_uminus; break;
                    case IR::OpUPlus: op = __qmljs_uplus; break;
                    case IR::OpCompl: op = __qmljs_compl; break;
                    default: assert(!"unreachable"); break;
                    } // switch
                    generateFunctionCall(op, ContextRegister, t, e);
                    return;
                }
            } else if (IR::Binop *b = s->source->asBinop()) {
                IR::Temp *l = b->left->asTemp();
                IR::Temp *r = b->right->asTemp();
                if (l && r) {
                    void (*op)(Context *, Value *, const Value *, const Value *) = 0;

                    switch ((IR::AluOp) b->op) {
                    case IR::OpInvalid:
                    case IR::OpIfTrue:
                    case IR::OpNot:
                    case IR::OpUMinus:
                    case IR::OpUPlus:
                    case IR::OpCompl:
                        assert(!"unreachable");
                        break;

                    case IR::OpBitAnd: op = __qmljs_bit_and; break;
                    case IR::OpBitOr: op = __qmljs_bit_or; break;
                    case IR::OpBitXor: op = __qmljs_bit_xor; break;
                    case IR::OpAdd: op = __qmljs_add; break;
                    case IR::OpSub: op = __qmljs_sub; break;
                    case IR::OpMul: op = __qmljs_mul; break;
                    case IR::OpDiv: op = __qmljs_div; break;
                    case IR::OpMod: op = __qmljs_mod; break;
                    case IR::OpLShift: op = __qmljs_shl; break;
                    case IR::OpRShift: op = __qmljs_shr; break;
                    case IR::OpURShift: op = __qmljs_ushr; break;
                    case IR::OpGt: op = __qmljs_gt; break;
                    case IR::OpLt: op = __qmljs_lt; break;
                    case IR::OpGe: op = __qmljs_ge; break;
                    case IR::OpLe: op = __qmljs_le; break;
                    case IR::OpEqual: op = __qmljs_eq; break;
                    case IR::OpNotEqual: op = __qmljs_ne; break;
                    case IR::OpStrictEqual: op = __qmljs_se; break;
                    case IR::OpStrictNotEqual: op = __qmljs_sne; break;
                    case IR::OpInstanceof: op = __qmljs_instanceof; break;
                    case IR::OpIn: op = __qmljs_in; break;

                    case IR::OpAnd:
                    case IR::OpOr:
                        assert(!"unreachable");
                        break;
                    }
                    generateFunctionCall(op, ContextRegister, t, l, r);
                    return;
                }
            } else if (IR::Call *c = s->source->asCall()) {
                if (c->base->asName()) {
                    callActivationProperty(c, t);
                    return;
                } else if (c->base->asMember()) {
                    callProperty(c, t);
                    return;
                } else if (c->base->asTemp()) {
                    callValue(c, t);
                    return;
                }
            }
        } else if (IR::Member *m = s->target->asMember()) {
            if (IR::Temp *base = m->base->asTemp()) {
                if (IR::Temp *t = s->source->asTemp()) {
                    generateFunctionCall(__qmljs_set_property, ContextRegister, base, identifier(*m->name), t);
                    checkExceptions();
                    return;
                } else {
                    Q_UNREACHABLE();
                }
            }
        } else if (IR::Subscript *ss = s->target->asSubscript()) {
            if (IR::Temp *t2 = s->source->asTemp()) {
                generateFunctionCall(__qmljs_set_element, ContextRegister, ss->base->asTemp(), ss->index->asTemp(), t2);
                checkExceptions();
                return;
            } else {
                Q_UNIMPLEMENTED();
            }
        }
    } else {
        // inplace assignment, e.g. x += 1, ++x, ...
        if (IR::Temp *t = s->target->asTemp()) {
            if (IR::Temp *t2 = s->source->asTemp()) {
                void (*op)(Context *, Value *, const Value *, const Value *) = 0;
                switch (s->op) {
                case IR::OpBitAnd: op = __qmljs_bit_and; break;
                case IR::OpBitOr: op = __qmljs_bit_or; break;
                case IR::OpBitXor: op = __qmljs_bit_xor; break;
                case IR::OpAdd: op = __qmljs_add; break;
                case IR::OpSub: op = __qmljs_sub; break;
                case IR::OpMul: op = __qmljs_mul; break;
                case IR::OpDiv: op = __qmljs_div; break;
                case IR::OpMod: op = __qmljs_mod; break;
                case IR::OpLShift: op = __qmljs_shl; break;
                case IR::OpRShift: op = __qmljs_shr; break;
                case IR::OpURShift: op = __qmljs_ushr; break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

                generateFunctionCall(op, ContextRegister, t, t, t2);
                return;
            }
        } else if (IR::Name *n = s->target->asName()) {
            if (IR::Temp *t = s->source->asTemp()) {
                void (*op)(Context *, String *, Value *) = 0;
                switch (s->op) {
                case IR::OpBitAnd: op = __qmljs_inplace_bit_and_name; break;
                case IR::OpBitOr: op = __qmljs_inplace_bit_or_name; break;
                case IR::OpBitXor: op = __qmljs_inplace_bit_xor_name; break;
                case IR::OpAdd: op = __qmljs_inplace_add_name; break;
                case IR::OpSub: op = __qmljs_inplace_sub_name; break;
                case IR::OpMul: op = __qmljs_inplace_mul_name; break;
                case IR::OpDiv: op = __qmljs_inplace_div_name; break;
                case IR::OpMod: op = __qmljs_inplace_mod_name; break;
                case IR::OpLShift: op = __qmljs_inplace_shl_name; break;
                case IR::OpRShift: op = __qmljs_inplace_shr_name; break;
                case IR::OpURShift: op = __qmljs_inplace_ushr_name; break;
                default:
                    Q_UNREACHABLE();
                    break;
                }
                generateFunctionCall(op, ContextRegister, identifier(*n->id), t);
                checkExceptions();
                return;
            }
        } else if (IR::Subscript *ss = s->target->asSubscript()) {
            if (IR::Temp *t = s->source->asTemp()) {
                void (*op)(Context *, Value *, Value *, Value *) = 0;
                switch (s->op) {
                case IR::OpBitAnd: op = __qmljs_inplace_bit_and_element; break;
                case IR::OpBitOr: op = __qmljs_inplace_bit_or_element; break;
                case IR::OpBitXor: op = __qmljs_inplace_bit_xor_element; break;
                case IR::OpAdd: op = __qmljs_inplace_add_element; break;
                case IR::OpSub: op = __qmljs_inplace_sub_element; break;
                case IR::OpMul: op = __qmljs_inplace_mul_element; break;
                case IR::OpDiv: op = __qmljs_inplace_div_element; break;
                case IR::OpMod: op = __qmljs_inplace_mod_element; break;
                case IR::OpLShift: op = __qmljs_inplace_shl_element; break;
                case IR::OpRShift: op = __qmljs_inplace_shr_element; break;
                case IR::OpURShift: op = __qmljs_inplace_ushr_element; break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

                generateFunctionCall(op, ContextRegister, ss->base->asTemp(), ss->index->asTemp(), t);
                checkExceptions();
                return;
            }
        } else if (IR::Member *m = s->target->asMember()) {
            if (IR::Temp *t = s->source->asTemp()) {
                void (*op)(Context *, Value *, String *, Value *) = 0;
                switch (s->op) {
                case IR::OpBitAnd: op = __qmljs_inplace_bit_and_member; break;
                case IR::OpBitOr: op = __qmljs_inplace_bit_or_member; break;
                case IR::OpBitXor: op = __qmljs_inplace_bit_xor_member; break;
                case IR::OpAdd: op = __qmljs_inplace_add_member; break;
                case IR::OpSub: op = __qmljs_inplace_sub_member; break;
                case IR::OpMul: op = __qmljs_inplace_mul_member; break;
                case IR::OpDiv: op = __qmljs_inplace_div_member; break;
                case IR::OpMod: op = __qmljs_inplace_mod_member; break;
                case IR::OpLShift: op = __qmljs_inplace_shl_member; break;
                case IR::OpRShift: op = __qmljs_inplace_shr_member; break;
                case IR::OpURShift: op = __qmljs_inplace_ushr_member; break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

                generateFunctionCall(op, ContextRegister, m->base->asTemp(), identifier(*m->name), t);
                checkExceptions();
                return;
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
    jumpToBlock(s->target);
}

void InstructionSelection::jumpToBlock(IR::BasicBlock *target)
{
    if (_block->index + 1 != target->index)
        _patches[target].append(jump());
}

void InstructionSelection::visitCJump(IR::CJump *s)
{
    if (IR::Temp *t = s->cond->asTemp()) {
        Address temp = loadTempAddress(Gpr1, t);
        Address tag = temp;
        tag.offset += offsetof(VM::ValueData, tag);
        Jump booleanConversion = branch32(NotEqual, tag, TrustedImm32(VM::Value::Boolean_Type));

        Address data = temp;
        data.offset += offsetof(VM::ValueData, b);
        load32(data, Gpr1);
        Jump testBoolean = jump();

        booleanConversion.link(this);
        {
            generateFunctionCall(__qmljs_to_boolean, ContextRegister, t);
            move(ReturnValueRegister, Gpr1);
        }

        testBoolean.link(this);
        move(TrustedImm32(1), Gpr0);
        Jump target = branch32(Equal, Gpr1, Gpr0);
        _patches[s->iftrue].append(target);

        jumpToBlock(s->iffalse);
        return;
    } else if (IR::Binop *b = s->cond->asBinop()) {
        IR::Temp *l = b->left->asTemp();
        IR::Temp *r = b->right->asTemp();
        if (l && r) {
            bool (*op)(Context *, const Value *, const Value *);
            switch (b->op) {
            default: Q_UNREACHABLE(); assert(!"todo"); break;
            case IR::OpGt: op = __qmljs_cmp_gt; break;
            case IR::OpLt: op = __qmljs_cmp_lt; break;
            case IR::OpGe: op = __qmljs_cmp_ge; break;
            case IR::OpLe: op = __qmljs_cmp_le; break;
            case IR::OpEqual: op = __qmljs_cmp_eq; break;
            case IR::OpNotEqual: op = __qmljs_cmp_ne; break;
            case IR::OpStrictEqual: op = __qmljs_cmp_se; break;
            case IR::OpStrictNotEqual: op = __qmljs_cmp_sne; break;
            case IR::OpInstanceof: op = __qmljs_cmp_instanceof; break;
            case IR::OpIn: op = __qmljs_cmp_in; break;
            } // switch

            generateFunctionCall(op, ContextRegister, l, r);
            move(ReturnValueRegister, Gpr0);

            move(TrustedImm32(1), Gpr1);
            Jump target = branch32(Equal, Gpr0, Gpr1);
            _patches[s->iftrue].append(target);

            jumpToBlock(s->iffalse);
            return;
        } else {
            assert(!"wip");
        }
        Q_UNIMPLEMENTED();
    }
    Q_UNIMPLEMENTED();
    assert(!"TODO");
}

void InstructionSelection::visitRet(IR::Ret *s)
{
    if (IR::Temp *t = s->expr->asTemp()) {
        generateFunctionCall(__qmljs_copy, Pointer(ContextRegister, offsetof(Context, result)), t);
        return;
    }
    Q_UNIMPLEMENTED();
    Q_UNUSED(s);
}

int InstructionSelection::prepareVariableArguments(IR::ExprList* args)
{
    int argc = 0;
    for (IR::ExprList *it = args; it; it = it->next) {
        ++argc;
    }

    int i = 0;
    for (IR::ExprList *it = args; it; it = it->next, ++i) {
        IR::Temp *arg = it->expr->asTemp();
        assert(arg != 0);
        generateFunctionCall(__qmljs_copy, argumentAddressForCall(i), arg);
    }

    return argc;
}

void InstructionSelection::callRuntimeMethod(ActivationMethod method, IR::Temp *result, IR::Expr *base, IR::ExprList *args)
{
    IR::Name *baseName = base->asName();
    assert(baseName != 0);

    int argc = prepareVariableArguments(args);
    generateFunctionCall(method, ContextRegister, result, identifier(*baseName->id), baseAddressForCallArguments(), TrustedImm32(argc));
    checkExceptions();
}

void InstructionSelection::callRuntimeMethod(BuiltinMethod method, IR::Temp *result, IR::ExprList *args)
{
    int argc = prepareVariableArguments(args);
    generateFunctionCall(method, ContextRegister, result, baseAddressForCallArguments(), TrustedImm32(argc));
    checkExceptions();
}

