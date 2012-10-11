
#include "qv4isel_masm_p.h"
#include "qmljs_runtime.h"
#include "qmljs_objects.h"

#include <assembler/LinkBuffer.h>
#include <WTFStubs.h>

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

static void printDisassmbleOutputWithCalls(const char* output, const QHash<void*, const char*>& functions)
{
    QByteArray processedOutput(output);
    for (QHash<void*, const char*>::ConstIterator it = functions.begin(), end = functions.end();
         it != end; ++it) {
        QByteArray ptrString = QByteArray::number(qlonglong(it.key()), 16);
        ptrString.prepend("0x");
        processedOutput = processedOutput.replace(ptrString, it.value());
    }
    fprintf(stdout, "%s\n", processedOutput.constData());
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

    int locals = (_function->tempCount - _function->locals.size() + _function->maxNumberOfArguments);
    locals = (locals + 1) & ~1;
    enterStandardStackFrame(locals);

    push(ContextRegister);
#if CPU(X86)
    loadPtr(addressForArgument(0), ContextRegister);
#elif CPU(X86_64) || CPU(ARM)
    move(RegisterArgument1, ContextRegister);
#else
    assert(!"TODO");
#endif

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
    QHash<void*, const char*> functions;
    foreach (CallToLink ctl, _callsToLink) {
        linkBuffer.link(ctl.call, ctl.externalFunction);
        functions[ctl.externalFunction.value()] = ctl.functionName;
    }

    char* disasmOutput = 0;
    size_t disasmLength = 0;
    FILE* disasmStream = open_memstream(&disasmOutput, &disasmLength);
    WTF::setDataFile(disasmStream);

    _function->codeRef = linkBuffer.finalizeCodeWithDisassembly("operator()(IR::Function*)");

    WTF::setDataFile(stdout);
    fclose(disasmStream);
#if CPU(X86) || CPU(X86_64)
    printDisassmbleOutputWithCalls(disasmOutput, functions);
#endif
    free(disasmOutput);

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
        // StackFrameRegister points to its old value on the stack, so even for the first temp we need to
        // subtract at least sizeof(Value).
        offset = - sizeof(Value) * (arg + 1);
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
        case IR::Name::builtin_rethrow: {
            int argc = prepareVariableArguments(call->args);
            generateFunctionCall(__qmljs_builtin_rethrow, ContextRegister, result, baseAddressForCallArguments(), TrustedImm32(argc));
            return; // we need to return to avoid checking the exceptions
        }
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
                copyValue(t, t2);
                return;
            } else if (IR::String *str = s->source->asString()) {
                // ### inline
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
                    switch (u->op) {
                    case IR::OpIfTrue: assert(!"unreachable"); break;
                    case IR::OpNot: generateFunctionCall(__qmljs_not, ContextRegister, t, e); break;
                    case IR::OpUMinus: generateFunctionCall(__qmljs_uminus, ContextRegister, t, e); break;
                    case IR::OpUPlus: generateFunctionCall(__qmljs_uplus, ContextRegister, t, e); break;
                    case IR::OpCompl: generateFunctionCall(__qmljs_compl, ContextRegister, t, e); break;
                    default: assert(!"unreachable"); break;
                    } // switch
                    return;
                }
            } else if (IR::Binop *b = s->source->asBinop()) {
                IR::Temp *l = b->left->asTemp();
                IR::Temp *r = b->right->asTemp();
                if (l && r) {
                    switch ((IR::AluOp) b->op) {
                    case IR::OpInvalid:
                    case IR::OpIfTrue:
                    case IR::OpNot:
                    case IR::OpUMinus:
                    case IR::OpUPlus:
                    case IR::OpCompl:
                        assert(!"unreachable");
                        break;

                    case IR::OpBitAnd: generateFunctionCall(__qmljs_bit_and, ContextRegister, t, l, r); break;
                    case IR::OpBitOr: generateFunctionCall(__qmljs_bit_or, ContextRegister, t, l, r); break;
                    case IR::OpBitXor: generateFunctionCall(__qmljs_bit_xor, ContextRegister, t, l, r); break;
                    case IR::OpAdd: generateFunctionCall(__qmljs_add, ContextRegister, t, l, r); break;
                    case IR::OpSub: generateFunctionCall(__qmljs_sub, ContextRegister, t, l, r); break;
                    case IR::OpMul: generateFunctionCall(__qmljs_mul, ContextRegister, t, l, r); break;
                    case IR::OpDiv: generateFunctionCall(__qmljs_div, ContextRegister, t, l, r); break;
                    case IR::OpMod: generateFunctionCall(__qmljs_mod, ContextRegister, t, l, r); break;
                    case IR::OpLShift: generateFunctionCall(__qmljs_shl, ContextRegister, t, l, r); break;
                    case IR::OpRShift: generateFunctionCall(__qmljs_shr, ContextRegister, t, l, r); break;
                    case IR::OpURShift: generateFunctionCall(__qmljs_ushr, ContextRegister, t, l, r); break;
                    case IR::OpGt: generateFunctionCall(__qmljs_gt, ContextRegister, t, l, r); break;
                    case IR::OpLt: generateFunctionCall(__qmljs_lt, ContextRegister, t, l, r); break;
                    case IR::OpGe: generateFunctionCall(__qmljs_ge, ContextRegister, t, l, r); break;
                    case IR::OpLe: generateFunctionCall(__qmljs_le, ContextRegister, t, l, r); break;
                    case IR::OpEqual: generateFunctionCall(__qmljs_eq, ContextRegister, t, l, r); break;
                    case IR::OpNotEqual: generateFunctionCall(__qmljs_ne, ContextRegister, t, l, r); break;
                    case IR::OpStrictEqual: generateFunctionCall(__qmljs_se, ContextRegister, t, l, r); break;
                    case IR::OpStrictNotEqual: generateFunctionCall(__qmljs_sne, ContextRegister, t, l, r); break;
                    case IR::OpInstanceof: generateFunctionCall(__qmljs_instanceof, ContextRegister, t, l, r); break;
                    case IR::OpIn: generateFunctionCall(__qmljs_in, ContextRegister, t, l, r); break;

                    case IR::OpAnd:
                    case IR::OpOr:
                        assert(!"unreachable");
                        break;
                    }
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
                switch (s->op) {
                case IR::OpBitAnd: generateFunctionCall(__qmljs_bit_and, ContextRegister, t, t, t2); break;
                case IR::OpBitOr: generateFunctionCall(__qmljs_bit_or, ContextRegister, t, t, t2); break;
                case IR::OpBitXor: generateFunctionCall(__qmljs_bit_xor, ContextRegister, t, t, t2); break;
                case IR::OpAdd: generateFunctionCall(__qmljs_add, ContextRegister, t, t, t2); break;
                case IR::OpSub: generateFunctionCall(__qmljs_sub, ContextRegister, t, t, t2); break;
                case IR::OpMul: generateFunctionCall(__qmljs_mul, ContextRegister, t, t, t2); break;
                case IR::OpDiv: generateFunctionCall(__qmljs_div, ContextRegister, t, t, t2); break;
                case IR::OpMod: generateFunctionCall(__qmljs_mod, ContextRegister, t, t, t2); break;
                case IR::OpLShift: generateFunctionCall(__qmljs_shl, ContextRegister, t, t, t2); break;
                case IR::OpRShift: generateFunctionCall(__qmljs_shr, ContextRegister, t, t, t2); break;
                case IR::OpURShift: generateFunctionCall(__qmljs_ushr, ContextRegister, t, t, t2); break;
                default:
                    Q_UNREACHABLE();
                    break;
                }
                return;
            }
        } else if (IR::Name *n = s->target->asName()) {
            if (IR::Temp *t = s->source->asTemp()) {
                switch (s->op) {
                case IR::OpBitAnd: generateFunctionCall(__qmljs_inplace_bit_and_name, ContextRegister, identifier(*n->id), t); break;
                case IR::OpBitOr: generateFunctionCall(__qmljs_inplace_bit_or_name, ContextRegister, identifier(*n->id), t); break;
                case IR::OpBitXor: generateFunctionCall(__qmljs_inplace_bit_xor_name, ContextRegister, identifier(*n->id), t); break;
                case IR::OpAdd: generateFunctionCall(__qmljs_inplace_add_name, ContextRegister, identifier(*n->id), t); break;
                case IR::OpSub: generateFunctionCall(__qmljs_inplace_sub_name, ContextRegister, identifier(*n->id), t); break;
                case IR::OpMul: generateFunctionCall(__qmljs_inplace_mul_name, ContextRegister, identifier(*n->id), t); break;
                case IR::OpDiv: generateFunctionCall(__qmljs_inplace_div_name, ContextRegister, identifier(*n->id), t); break;
                case IR::OpMod: generateFunctionCall(__qmljs_inplace_mod_name, ContextRegister, identifier(*n->id), t); break;
                case IR::OpLShift: generateFunctionCall(__qmljs_inplace_shl_name, ContextRegister, identifier(*n->id), t); break;
                case IR::OpRShift: generateFunctionCall(__qmljs_inplace_shr_name, ContextRegister, identifier(*n->id), t); break;
                case IR::OpURShift: generateFunctionCall(__qmljs_inplace_ushr_name, ContextRegister, identifier(*n->id), t); break;
                default:
                    Q_UNREACHABLE();
                    break;
                }
                checkExceptions();
                return;
            }
        } else if (IR::Subscript *ss = s->target->asSubscript()) {
            if (IR::Temp *t = s->source->asTemp()) {
                IR::Temp* base = ss->base->asTemp();
                IR::Temp* index = ss->index->asTemp();
                switch (s->op) {
                case IR::OpBitAnd: generateFunctionCall(__qmljs_inplace_bit_and_element, ContextRegister, base, index, t); break;
                case IR::OpBitOr: generateFunctionCall(__qmljs_inplace_bit_or_element, ContextRegister, base, index, t); break;
                case IR::OpBitXor: generateFunctionCall(__qmljs_inplace_bit_xor_element, ContextRegister, base, index, t); break;
                case IR::OpAdd: generateFunctionCall(__qmljs_inplace_add_element, ContextRegister, base, index, t); break;
                case IR::OpSub: generateFunctionCall(__qmljs_inplace_sub_element, ContextRegister, base, index, t); break;
                case IR::OpMul: generateFunctionCall(__qmljs_inplace_mul_element, ContextRegister, base, index, t); break;
                case IR::OpDiv: generateFunctionCall(__qmljs_inplace_div_element, ContextRegister, base, index, t); break;
                case IR::OpMod: generateFunctionCall(__qmljs_inplace_mod_element, ContextRegister, base, index, t); break;
                case IR::OpLShift: generateFunctionCall(__qmljs_inplace_shl_element, ContextRegister, base, index, t); break;
                case IR::OpRShift: generateFunctionCall(__qmljs_inplace_shr_element, ContextRegister, base, index, t); break;
                case IR::OpURShift: generateFunctionCall(__qmljs_inplace_ushr_element, ContextRegister, base, index, t); break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

                checkExceptions();
                return;
            }
        } else if (IR::Member *m = s->target->asMember()) {
            if (IR::Temp *t = s->source->asTemp()) {
                IR::Temp* base = m->base->asTemp();
                String* member = identifier(*m->name);
                switch (s->op) {
                case IR::OpBitAnd: generateFunctionCall(__qmljs_inplace_bit_and_member, ContextRegister, base, member, t); break;
                case IR::OpBitOr: generateFunctionCall(__qmljs_inplace_bit_or_member, ContextRegister, base, member, t); break;
                case IR::OpBitXor: generateFunctionCall(__qmljs_inplace_bit_xor_member, ContextRegister, base, member, t); break;
                case IR::OpAdd: generateFunctionCall(__qmljs_inplace_add_member, ContextRegister, base, member, t); break;
                case IR::OpSub: generateFunctionCall(__qmljs_inplace_sub_member, ContextRegister, base, member, t); break;
                case IR::OpMul: generateFunctionCall(__qmljs_inplace_mul_member, ContextRegister, base, member, t); break;
                case IR::OpDiv: generateFunctionCall(__qmljs_inplace_div_member, ContextRegister, base, member, t); break;
                case IR::OpMod: generateFunctionCall(__qmljs_inplace_mod_member, ContextRegister, base, member, t); break;
                case IR::OpLShift: generateFunctionCall(__qmljs_inplace_shl_member, ContextRegister, base, member, t); break;
                case IR::OpRShift: generateFunctionCall(__qmljs_inplace_shr_member, ContextRegister, base, member, t); break;
                case IR::OpURShift: generateFunctionCall(__qmljs_inplace_ushr_member, ContextRegister, base, member, t); break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

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
        Address temp = loadTempAddress(Gpr0, t);
        Address tag = temp;
        tag.offset += offsetof(VM::ValueData, tag);
        Jump booleanConversion = branch32(NotEqual, tag, TrustedImm32(VM::Value::Boolean_Type));

        Address data = temp;
        data.offset += offsetof(VM::ValueData, b);
        load32(data, Gpr0);
        Jump testBoolean = jump();

        booleanConversion.link(this);
        {
            generateFunctionCall(__qmljs_to_boolean, ContextRegister, t);
            move(ReturnValueRegister, Gpr0);
        }

        testBoolean.link(this);
        Jump target = branch32(Equal, Gpr0, TrustedImm32(1));
        _patches[s->iftrue].append(target);

        jumpToBlock(s->iffalse);
        return;
    } else if (IR::Binop *b = s->cond->asBinop()) {
        IR::Temp *l = b->left->asTemp();
        IR::Temp *r = b->right->asTemp();
        if (l && r) {
            switch (b->op) {
            default: Q_UNREACHABLE(); assert(!"todo"); break;
            case IR::OpGt: generateFunctionCall(__qmljs_cmp_gt, ContextRegister, l, r); break;
            case IR::OpLt: generateFunctionCall(__qmljs_cmp_lt, ContextRegister, l, r); break;
            case IR::OpGe: generateFunctionCall(__qmljs_cmp_ge, ContextRegister, l, r); break;
            case IR::OpLe: generateFunctionCall(__qmljs_cmp_le, ContextRegister, l, r); break;
            case IR::OpEqual: generateFunctionCall(__qmljs_cmp_eq, ContextRegister, l, r); break;
            case IR::OpNotEqual: generateFunctionCall(__qmljs_cmp_ne, ContextRegister, l, r); break;
            case IR::OpStrictEqual: generateFunctionCall(__qmljs_cmp_se, ContextRegister, l, r); break;
            case IR::OpStrictNotEqual: generateFunctionCall(__qmljs_cmp_sne, ContextRegister, l, r); break;
            case IR::OpInstanceof: generateFunctionCall(__qmljs_cmp_instanceof, ContextRegister, l, r); break;
            case IR::OpIn: generateFunctionCall(__qmljs_cmp_in, ContextRegister, l, r); break;
            } // switch

            move(ReturnValueRegister, Gpr0);

            Jump target = branch32(Equal, Gpr0, TrustedImm32(1));
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
        copyValue(Pointer(ContextRegister, offsetof(Context, result)), t);
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
        copyValue(argumentAddressForCall(i), arg);
    }

    return argc;
}

void InstructionSelection::callRuntimeMethodImp(const char* name, ActivationMethod method, IR::Temp *result, IR::Expr *base, IR::ExprList *args)
{
    IR::Name *baseName = base->asName();
    assert(baseName != 0);

    int argc = prepareVariableArguments(args);
    generateFunctionCallImp(name, method, ContextRegister, result, identifier(*baseName->id), baseAddressForCallArguments(), TrustedImm32(argc));
    checkExceptions();
}

void InstructionSelection::callRuntimeMethodImp(const char* name, BuiltinMethod method, IR::Temp *result, IR::ExprList *args)
{
    int argc = prepareVariableArguments(args);
    generateFunctionCallImp(name, method, ContextRegister, result, baseAddressForCallArguments(), TrustedImm32(argc));
    checkExceptions();
}

template <typename Result, typename Source>
void InstructionSelection::copyValue(Result result, Source source)
{
    loadDouble(source, FPGpr0);
    storeDouble(FPGpr0, result);
}
