/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
QTextStream qout(stderr, QIODevice::WriteOnly);
}

#if OS(LINUX)
static void printDisassembledOutputWithCalls(const char* output, const QHash<void*, const char*>& functions)
{
    QByteArray processedOutput(output);
    for (QHash<void*, const char*>::ConstIterator it = functions.begin(), end = functions.end();
         it != end; ++it) {
        QByteArray ptrString = QByteArray::number(qlonglong(it.key()), 16);
        ptrString.prepend("0x");
        processedOutput = processedOutput.replace(ptrString, it.value());
    }
    fprintf(stderr, "%s\n", processedOutput.constData());
}
#endif

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

#if OS(LINUX)
    char* disasmOutput = 0;
    size_t disasmLength = 0;
    FILE* disasmStream = open_memstream(&disasmOutput, &disasmLength);
    WTF::setDataFile(disasmStream);
#endif

    QByteArray name = _function->name->toUtf8();
    if (name.startsWith('%'))
        name.prepend('%');
    _function->codeRef = linkBuffer.finalizeCodeWithDisassembly(name.data());

    WTF::setDataFile(stderr);
#if OS(LINUX)
    fclose(disasmStream);
#if CPU(X86) || CPU(X86_64)
    printDisassembledOutputWithCalls(disasmOutput, functions);
#endif
    free(disasmOutput);
#endif

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
        callRuntimeMethod(result, __qmljs_call_activation_property, call->base, call->args);
    else {
        switch (baseName->builtin) {
        case IR::Name::builtin_invalid:
            Q_UNREACHABLE();
            break;
        case IR::Name::builtin_typeof:
            callRuntimeMethod(result, __qmljs_builtin_typeof, call->args);
            break;
        case IR::Name::builtin_delete:
            Q_UNREACHABLE();
            break;
        case IR::Name::builtin_throw:
            callRuntimeMethod(result, __qmljs_builtin_throw, call->args);
            break;
        case IR::Name::builtin_rethrow: {
            int argc = prepareVariableArguments(call->args);
            generateFunctionCall2(result, __qmljs_builtin_rethrow, ContextRegister, baseAddressForCallArguments(), TrustedImm32(argc));
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
    generateFunctionCall2(result, __qmljs_call_value, ContextRegister, baseTemp, thisObject, baseAddressForCallArguments(), TrustedImm32(argc));
    checkExceptions();
}

void InstructionSelection::callProperty(IR::Call *call, IR::Temp *result)
{
    IR::Member *member = call->base->asMember();
    assert(member != 0);
    assert(member->base->asTemp() != 0);

    int argc = prepareVariableArguments(call->args);
    generateFunctionCall2(result, __qmljs_call_property, ContextRegister, member->base->asTemp(), identifier(*member->name), baseAddressForCallArguments(), TrustedImm32(argc));
    checkExceptions();
}

void InstructionSelection::constructActivationProperty(IR::New *call, IR::Temp *result)
{
    IR::Name *baseName = call->base->asName();
    assert(baseName != 0);

    callRuntimeMethod(result, __qmljs_construct_activation_property, call->base, call->args);
}

void InstructionSelection::constructProperty(IR::New *call, IR::Temp *result)
{
    IR::Member *member = call->base->asMember();
    assert(member != 0);
    assert(member->base->asTemp() != 0);

    int argc = prepareVariableArguments(call->args);
    generateFunctionCall2(result, __qmljs_construct_property, ContextRegister, member->base->asTemp(), identifier(*member->name), baseAddressForCallArguments(), TrustedImm32(argc));
    checkExceptions();
}

void InstructionSelection::constructValue(IR::New *call, IR::Temp *result)
{
    IR::Temp *baseTemp = call->base->asTemp();
    assert(baseTemp != 0);

    int argc = prepareVariableArguments(call->args);
    generateFunctionCall2(result, __qmljs_construct_value, ContextRegister, baseTemp, baseAddressForCallArguments(), TrustedImm32(argc));
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

#define setOp(op, opName, operation) \
    do { op = operation; opName = isel_stringIfy(operation); } while (0)

void InstructionSelection::visitMove(IR::Move *s)
{

    if (s->op == IR::OpInvalid) {
        if (IR::Name *n = s->target->asName()) {
            String *propertyName = identifier(*n->id);

            if (IR::Temp *t = s->source->asTemp()) {
                generateFunctionCall2(Void, __qmljs_set_activation_property, ContextRegister, propertyName, t);
                checkExceptions();
                return;
            } else {
                Q_UNREACHABLE();
            }
        } else if (IR::Temp *t = s->target->asTemp()) {
            if (IR::Name *n = s->source->asName()) {
                if (*n->id == QStringLiteral("this")) { // ### `this' should be a builtin.
                    generateFunctionCall2(t, __qmljs_get_thisObject, ContextRegister);
                } else {
                    String *propertyName = identifier(*n->id);
                    generateFunctionCall2(t, __qmljs_get_activation_property, ContextRegister, propertyName);
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
                case IR::NumberType: {
                    int ival = (int)c->value;
                    if (ival == c->value) {
                        storeValue<Value::Integer_Type>(TrustedImm32(ival), dest);
                    } else {
                        // ### Taking address of pointer inside IR.
                        copyValue(dest, &c->value);
                    }
                }
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
                generateFunctionCall2(t, __qmljs_init_string, _engine->newString(*str->value));
                return;
            } else if (IR::Closure *clos = s->source->asClosure()) {
                generateFunctionCall2(t, __qmljs_init_closure, TrustedImmPtr(clos->value), ContextRegister);
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
                    generateFunctionCall2(t, __qmljs_get_property, ContextRegister, base, identifier(*m->name));
                    checkExceptions();
                    return;
                }
                assert(!"wip");
                return;
            } else if (IR::Subscript *ss = s->source->asSubscript()) {
                generateFunctionCall2(t, __qmljs_get_element, ContextRegister, ss->base->asTemp(), ss->index->asTemp());
                checkExceptions();
                return;
            } else if (IR::Unop *u = s->source->asUnop()) {
                if (IR::Temp *e = u->expr->asTemp()) {
                    Value (*op)(const Value value, Context *ctx) = 0;
                    const char *opName = 0;
                    switch (u->op) {
                    case IR::OpIfTrue: assert(!"unreachable"); break;
                    case IR::OpNot: setOp(op, opName, __qmljs_not); break;
                    case IR::OpUMinus: setOp(op, opName, __qmljs_uminus); break;
                    case IR::OpUPlus: setOp(op, opName, __qmljs_uplus); break;
                    case IR::OpCompl: setOp(op, opName, __qmljs_compl); break;
                    default: assert(!"unreachable"); break;
                    } // switch

                    if (op)
                        generateFunctionCallImp2(t, opName, op, e, ContextRegister);
                    return;
                }
            } else if (IR::Binop *b = s->source->asBinop()) {
                IR::Temp *l = b->left->asTemp();
                IR::Temp *r = b->right->asTemp();
                if (l && r) {
                    Value (*op)(const Value, const Value, Context *) = 0;
                    const char* opName = 0;

                    switch ((IR::AluOp) b->op) {
                    case IR::OpInvalid:
                    case IR::OpIfTrue:
                    case IR::OpNot:
                    case IR::OpUMinus:
                    case IR::OpUPlus:
                    case IR::OpCompl:
                        assert(!"unreachable");
                        break;

                    case IR::OpBitAnd: setOp(op, opName, __qmljs_bit_and); break;
                    case IR::OpBitOr: setOp(op, opName, __qmljs_bit_or); break;
                    case IR::OpBitXor: setOp(op, opName, __qmljs_bit_xor); break;
                    case IR::OpAdd: setOp(op, opName, __qmljs_add); break;
                    case IR::OpSub: setOp(op, opName, __qmljs_sub); break;
                    case IR::OpMul: setOp(op, opName, __qmljs_mul); break;
                    case IR::OpDiv: setOp(op, opName, __qmljs_div); break;
                    case IR::OpMod: setOp(op, opName, __qmljs_mod); break;
                    case IR::OpLShift: setOp(op, opName, __qmljs_shl); break;
                    case IR::OpRShift: setOp(op, opName, __qmljs_shr); break;
                    case IR::OpURShift: setOp(op, opName, __qmljs_ushr); break;
                    case IR::OpGt: setOp(op, opName, __qmljs_gt); break;
                    case IR::OpLt: setOp(op, opName, __qmljs_lt); break;
                    case IR::OpGe: setOp(op, opName, __qmljs_ge); break;
                    case IR::OpLe: setOp(op, opName, __qmljs_le); break;
                    case IR::OpEqual: setOp(op, opName, __qmljs_eq); break;
                    case IR::OpNotEqual: setOp(op, opName, __qmljs_ne); break;
                    case IR::OpStrictEqual: setOp(op, opName, __qmljs_se); break;
                    case IR::OpStrictNotEqual: setOp(op, opName, __qmljs_sne); break;
                    case IR::OpInstanceof: setOp(op, opName, __qmljs_instanceof); break;
                    case IR::OpIn: setOp(op, opName, __qmljs_in);

                    case IR::OpAnd:
                    case IR::OpOr:
                        assert(!"unreachable");
                        break;
                    }

                    if (op) {
                        generateFunctionCallImp2(t, opName, op, l, r, ContextRegister);
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
                    generateFunctionCall2(Void, __qmljs_set_property, ContextRegister, base, identifier(*m->name), t);
                    checkExceptions();
                    return;
                } else {
                    Q_UNREACHABLE();
                }
            }
        } else if (IR::Subscript *ss = s->target->asSubscript()) {
            if (IR::Temp *t2 = s->source->asTemp()) {
                generateFunctionCall2(Void, __qmljs_set_element, ContextRegister, ss->base->asTemp(), ss->index->asTemp(), t2);
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
                Value (*op)(const Value left, const Value right, Context *ctx) = 0;
                const char *opName = 0;
                switch (s->op) {
                case IR::OpBitAnd: setOp(op, opName, __qmljs_bit_and); break;
                case IR::OpBitOr: setOp(op, opName, __qmljs_bit_or); break;
                case IR::OpBitXor: setOp(op, opName, __qmljs_bit_xor); break;
                case IR::OpAdd: setOp(op, opName, __qmljs_add); break;
                case IR::OpSub: setOp(op, opName, __qmljs_sub); break;
                case IR::OpMul: setOp(op, opName, __qmljs_mul); break;
                case IR::OpDiv: setOp(op, opName, __qmljs_div); break;
                case IR::OpMod: setOp(op, opName, __qmljs_mod); break;
                case IR::OpLShift: setOp(op, opName, __qmljs_shl); break;
                case IR::OpRShift: setOp(op, opName, __qmljs_shr); break;
                case IR::OpURShift: setOp(op, opName, __qmljs_ushr); break;
                default:
                    Q_UNREACHABLE();
                    break;
                }
                if (op)
                    generateFunctionCallImp2(t, opName, op, t, t2, ContextRegister);
                return;
            }
        } else if (IR::Name *n = s->target->asName()) {
            if (IR::Temp *t = s->source->asTemp()) {
                void (*op)(Context *ctx, String *name, Value *value) = 0;
                const char *opName = 0;
                switch (s->op) {
                case IR::OpBitAnd: setOp(op, opName, __qmljs_inplace_bit_and_name); break;
                case IR::OpBitOr: setOp(op, opName, __qmljs_inplace_bit_or_name); break;
                case IR::OpBitXor: setOp(op, opName, __qmljs_inplace_bit_xor_name); break;
                case IR::OpAdd: setOp(op, opName, __qmljs_inplace_add_name); break;
                case IR::OpSub: setOp(op, opName, __qmljs_inplace_sub_name); break;
                case IR::OpMul: setOp(op, opName, __qmljs_inplace_mul_name); break;
                case IR::OpDiv: setOp(op, opName, __qmljs_inplace_div_name); break;
                case IR::OpMod: setOp(op, opName, __qmljs_inplace_mod_name); break;
                case IR::OpLShift: setOp(op, opName, __qmljs_inplace_shl_name); break;
                case IR::OpRShift: setOp(op, opName, __qmljs_inplace_shr_name); break;
                case IR::OpURShift: setOp(op, opName, __qmljs_inplace_ushr_name); break;
                default:
                    Q_UNREACHABLE();
                    break;
                }
                if (op) {
                    generateFunctionCallImp(opName, op, ContextRegister, identifier(*n->id), t);
                    checkExceptions();
                }
                return;
            }
        } else if (IR::Subscript *ss = s->target->asSubscript()) {
            if (IR::Temp *t = s->source->asTemp()) {
                void (*op)(Context *ctx, Value *base, Value *index, Value *value) = 0;
                const char *opName = 0;
                switch (s->op) {
                case IR::OpBitAnd: setOp(op, opName, __qmljs_inplace_bit_and_element); break;
                case IR::OpBitOr: setOp(op, opName, __qmljs_inplace_bit_or_element); break;
                case IR::OpBitXor: setOp(op, opName, __qmljs_inplace_bit_xor_element); break;
                case IR::OpAdd: setOp(op, opName, __qmljs_inplace_add_element); break;
                case IR::OpSub: setOp(op, opName, __qmljs_inplace_sub_element); break;
                case IR::OpMul: setOp(op, opName, __qmljs_inplace_mul_element); break;
                case IR::OpDiv: setOp(op, opName, __qmljs_inplace_div_element); break;
                case IR::OpMod: setOp(op, opName, __qmljs_inplace_mod_element); break;
                case IR::OpLShift: setOp(op, opName, __qmljs_inplace_shl_element); break;
                case IR::OpRShift: setOp(op, opName, __qmljs_inplace_shr_element); break;
                case IR::OpURShift: setOp(op, opName, __qmljs_inplace_ushr_element); break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

                if (op) {
                    IR::Temp* base = ss->base->asTemp();
                    IR::Temp* index = ss->index->asTemp();
                    generateFunctionCallImp(opName, op, ContextRegister, base, index, t);
                    checkExceptions();
                }
                return;
            }
        } else if (IR::Member *m = s->target->asMember()) {
            if (IR::Temp *t = s->source->asTemp()) {
                void (*op)(Context *ctx, Value *base, String *name, Value *value) = 0;
                const char *opName = 0;
                switch (s->op) {
                case IR::OpBitAnd: setOp(op, opName, __qmljs_inplace_bit_and_member); break;
                case IR::OpBitOr: setOp(op, opName, __qmljs_inplace_bit_or_member); break;
                case IR::OpBitXor: setOp(op, opName, __qmljs_inplace_bit_xor_member); break;
                case IR::OpAdd: setOp(op, opName, __qmljs_inplace_add_member); break;
                case IR::OpSub: setOp(op, opName, __qmljs_inplace_sub_member); break;
                case IR::OpMul: setOp(op, opName, __qmljs_inplace_mul_member); break;
                case IR::OpDiv: setOp(op, opName, __qmljs_inplace_div_member); break;
                case IR::OpMod: setOp(op, opName, __qmljs_inplace_mod_member); break;
                case IR::OpLShift: setOp(op, opName, __qmljs_inplace_shl_member); break;
                case IR::OpRShift: setOp(op, opName, __qmljs_inplace_shr_member); break;
                case IR::OpURShift: setOp(op, opName, __qmljs_inplace_ushr_member); break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

                if (op) {
                    IR::Temp* base = m->base->asTemp();
                    String* member = identifier(*m->name);
                    generateFunctionCallImp(opName, op, ContextRegister, base, member, t);
                    checkExceptions();
                }
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
        data.offset += offsetof(VM::ValueData, int_32);
        load32(data, Gpr0);
        Jump testBoolean = jump();

        booleanConversion.link(this);
        {
            generateFunctionCall2(Gpr0, __qmljs_to_boolean, t, ContextRegister);
        }

        testBoolean.link(this);
        Jump target = branch32(NotEqual, Gpr0, TrustedImm32(0));
        _patches[s->iftrue].append(target);

        jumpToBlock(s->iffalse);
        return;
    } else if (IR::Binop *b = s->cond->asBinop()) {
        IR::Temp *l = b->left->asTemp();
        IR::Temp *r = b->right->asTemp();
        if (l && r) {
            Bool (*op)(Context *ctx, const Value, const Value) = 0;
            const char *opName = 0;
            switch (b->op) {
            default: Q_UNREACHABLE(); assert(!"todo"); break;
            case IR::OpGt: setOp(op, opName, __qmljs_cmp_gt); break;
            case IR::OpLt: setOp(op, opName, __qmljs_cmp_lt); break;
            case IR::OpGe: setOp(op, opName, __qmljs_cmp_ge); break;
            case IR::OpLe: setOp(op, opName, __qmljs_cmp_le); break;
            case IR::OpEqual: setOp(op, opName, __qmljs_cmp_eq); break;
            case IR::OpNotEqual: setOp(op, opName, __qmljs_cmp_ne); break;
            case IR::OpStrictEqual: setOp(op, opName, __qmljs_cmp_se); break;
            case IR::OpStrictNotEqual: setOp(op, opName, __qmljs_cmp_sne); break;
            case IR::OpInstanceof: setOp(op, opName, __qmljs_cmp_instanceof); break;
            case IR::OpIn: setOp(op, opName, __qmljs_cmp_in); break;
            } // switch

            generateFunctionCall2(ReturnValueRegister, op, ContextRegister, l, r);
            move(ReturnValueRegister, Gpr0);

            Jump target = branch32(NotEqual, Gpr0, TrustedImm32(0));
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

void InstructionSelection::callRuntimeMethodImp(IR::Temp *result, const char* name, ActivationMethod method, IR::Expr *base, IR::ExprList *args)
{
    IR::Name *baseName = base->asName();
    assert(baseName != 0);

    int argc = prepareVariableArguments(args);
    generateFunctionCallImp2(result, name, method, ContextRegister, identifier(*baseName->id), baseAddressForCallArguments(), TrustedImm32(argc));
    checkExceptions();
}

void InstructionSelection::callRuntimeMethodImp(IR::Temp *result, const char* name, BuiltinMethod method, IR::ExprList *args)
{
    int argc = prepareVariableArguments(args);
    generateFunctionCallImp2(result, name, method, ContextRegister, baseAddressForCallArguments(), TrustedImm32(argc));
    checkExceptions();
}

template <typename Result, typename Source>
void InstructionSelection::copyValue(Result result, Source source)
{
    loadDouble(source, FPGpr0);
    storeDouble(FPGpr0, result);
}
