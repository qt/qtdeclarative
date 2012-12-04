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

typedef JSC::MacroAssembler::Jump (*MemRegBinOp)(JSC::MacroAssembler*, JSC::MacroAssembler::Address, JSC::MacroAssembler::RegisterID);
typedef JSC::MacroAssembler::Jump (*ImmRegBinOp)(JSC::MacroAssembler*, JSC::MacroAssembler::TrustedImm32, JSC::MacroAssembler::RegisterID);

static JSC::MacroAssembler::Jump masm_add32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::Address addr, JSC::MacroAssembler::RegisterID reg)
{
    return assembler->branchAdd32(JSC::MacroAssembler::Overflow, addr, reg);
}

static JSC::MacroAssembler::Jump masm_add32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::TrustedImm32 imm, JSC::MacroAssembler::RegisterID reg)
{
    return assembler->branchAdd32(JSC::MacroAssembler::Overflow, imm, reg);
}

static JSC::MacroAssembler::Jump masm_sub32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::Address addr, JSC::MacroAssembler::RegisterID reg)
{
    return assembler->branchSub32(JSC::MacroAssembler::Overflow, addr, reg);
}

static JSC::MacroAssembler::Jump masm_sub32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::TrustedImm32 imm, JSC::MacroAssembler::RegisterID reg)
{
    return assembler->branchSub32(JSC::MacroAssembler::Overflow, imm, reg);
}

static JSC::MacroAssembler::Jump masm_mul32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::Address addr, JSC::MacroAssembler::RegisterID reg)
{
    return assembler->branchMul32(JSC::MacroAssembler::Overflow, addr, reg);
}

static JSC::MacroAssembler::Jump masm_mul32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::TrustedImm32 imm, JSC::MacroAssembler::RegisterID reg)
{
    return assembler->branchMul32(JSC::MacroAssembler::Overflow, imm, reg, reg);
}

static JSC::MacroAssembler::Jump masm_shl32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::Address addr, JSC::MacroAssembler::RegisterID reg)
{
    assembler->load32(addr, InstructionSelection::ScratchRegister);
    assembler->and32(JSC::MacroAssembler::TrustedImm32(0x1f), InstructionSelection::ScratchRegister);
    assembler->lshift32(InstructionSelection::ScratchRegister, reg);
    return JSC::MacroAssembler::Jump();
}

static JSC::MacroAssembler::Jump masm_shl32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::TrustedImm32 imm, JSC::MacroAssembler::RegisterID reg)
{
    imm.m_value &= 0x1f;
    assembler->lshift32(imm, reg);
    return JSC::MacroAssembler::Jump();
}

static JSC::MacroAssembler::Jump masm_shr32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::Address addr, JSC::MacroAssembler::RegisterID reg)
{
    assembler->load32(addr, InstructionSelection::ScratchRegister);
    assembler->and32(JSC::MacroAssembler::TrustedImm32(0x1f), InstructionSelection::ScratchRegister);
    assembler->rshift32(InstructionSelection::ScratchRegister, reg);
    return JSC::MacroAssembler::Jump();
}

static JSC::MacroAssembler::Jump masm_shr32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::TrustedImm32 imm, JSC::MacroAssembler::RegisterID reg)
{
    imm.m_value &= 0x1f;
    assembler->rshift32(imm, reg);
    return JSC::MacroAssembler::Jump();
}

static JSC::MacroAssembler::Jump masm_and32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::Address addr, JSC::MacroAssembler::RegisterID reg)
{
    assembler->and32(addr, reg);
    return JSC::MacroAssembler::Jump();
}

static JSC::MacroAssembler::Jump masm_and32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::TrustedImm32 imm, JSC::MacroAssembler::RegisterID reg)
{
    assembler->and32(imm, reg);
    return JSC::MacroAssembler::Jump();
}

static JSC::MacroAssembler::Jump masm_or32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::Address addr, JSC::MacroAssembler::RegisterID reg)
{
    assembler->or32(addr, reg);
    return JSC::MacroAssembler::Jump();
}

static JSC::MacroAssembler::Jump masm_or32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::TrustedImm32 imm, JSC::MacroAssembler::RegisterID reg)
{
    assembler->or32(imm, reg);
    return JSC::MacroAssembler::Jump();
}

static JSC::MacroAssembler::Jump masm_xor32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::Address addr, JSC::MacroAssembler::RegisterID reg)
{
    assembler->xor32(addr, reg);
    return JSC::MacroAssembler::Jump();
}

static JSC::MacroAssembler::Jump masm_xor32(JSC::MacroAssembler* assembler, JSC::MacroAssembler::TrustedImm32 imm, JSC::MacroAssembler::RegisterID reg)
{
    assembler->xor32(imm, reg);
    return JSC::MacroAssembler::Jump();
}


#define OP(op) \
    { isel_stringIfy(op), op, 0, 0 }

#define INLINE_OP(op, memOp, immOp) \
    { isel_stringIfy(op), op, memOp, immOp }

#define NULL_OP \
    { 0, 0, 0, 0 }

static const struct BinaryOperationInfo {
    const char *name;
    Value (*fallbackImplementation)(const Value, const Value, ExecutionContext *);
    MemRegBinOp inlineMemRegOp;
    ImmRegBinOp inlineImmRegOp;
} binaryOperations[QQmlJS::IR::LastAluOp + 1] = {
    NULL_OP, // OpInvalid
    NULL_OP, // OpIfTrue
    NULL_OP, // OpNot
    NULL_OP, // OpUMinus
    NULL_OP, // OpUPlus
    NULL_OP, // OpCompl

    INLINE_OP(__qmljs_bit_and, &masm_and32, &masm_and32), // OpBitAnd
    INLINE_OP(__qmljs_bit_or, &masm_or32, &masm_or32), // OpBitOr
    INLINE_OP(__qmljs_bit_xor, &masm_xor32, &masm_xor32), // OpBitXor

    INLINE_OP(__qmljs_add, &masm_add32, &masm_add32), // OpAdd
    INLINE_OP(__qmljs_sub, &masm_sub32, &masm_sub32), // OpSub
    INLINE_OP(__qmljs_mul, &masm_mul32, &masm_mul32), // OpMul

    OP(__qmljs_div), // OpDiv
    OP(__qmljs_mod), // OpMod

    INLINE_OP(__qmljs_shl, &masm_shl32, &masm_shl32), // OpLShift
    INLINE_OP(__qmljs_shr, &masm_shr32, &masm_shr32), // OpRShift
    OP(__qmljs_ushr), // OpURShift

    OP(__qmljs_gt), // OpGt
    OP(__qmljs_lt), // OpLt
    OP(__qmljs_ge), // OpGe
    OP(__qmljs_le), // OpLe
    OP(__qmljs_eq), // OpEqual
    OP(__qmljs_ne), // OpNotEqual
    OP(__qmljs_se), // OpStrictEqual
    OP(__qmljs_sne), // OpStrictNotEqual

    OP(__qmljs_instanceof), // OpInstanceof
    OP(__qmljs_in), // OpIn

    NULL_OP, // OpAnd
    NULL_OP // OpOr
};

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

InstructionSelection::InstructionSelection(VM::ExecutionEngine *engine)
    : _engine(engine)
    , _function(0)
    , _block(0)
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

    int contextPointer = 0;
#ifndef VALUE_FITS_IN_REGISTER
    // When the return VM value doesn't fit into a register, then
    // the caller provides a pointer for storage as first argument.
    // That shifts the index the context pointer argument by one.
    contextPointer++;
#endif
#if CPU(X86)
    loadPtr(addressForArgument(contextPointer), ContextRegister);
#elif CPU(X86_64) || CPU(ARM)
    move(registerForArgument(contextPointer), ContextRegister);
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

    leaveStandardStackFrame(locals);
#ifndef VALUE_FITS_IN_REGISTER
    // Emulate ret(n) instruction
    // Pop off return address into scratch register ...
    pop(ScratchRegister);
    // ... and overwrite the invisible argument with
    // the return address.
    poke(ScratchRegister);
#endif
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

    foreach (CatchBlockToLink cbl, _catchHandlers) {
        Label target = _addrs.value(cbl.catchBlock);
        linkBuffer.patch(cbl.ptr, linkBuffer.locationOf(target));
    }

    static bool showCode = !qgetenv("SHOW_CODE").isNull();
    if (showCode) {
#if OS(LINUX)
        char* disasmOutput = 0;
        size_t disasmLength = 0;
        FILE* disasmStream = open_memstream(&disasmOutput, &disasmLength);
        WTF::setDataFile(disasmStream);
#endif

        QByteArray name = _function->name->toUtf8();
        _function->codeRef = linkBuffer.finalizeCodeWithDisassembly("%s", name.data());

        WTF::setDataFile(stderr);
#if OS(LINUX)
        fclose(disasmStream);
#if CPU(X86) || CPU(X86_64)
        printDisassembledOutputWithCalls(disasmOutput, functions);
#endif
        free(disasmOutput);
#endif
    } else {
        _function->codeRef = linkBuffer.finalizeCodeWithoutDisassembly();
    }

    _function->code = (Value (*)(VM::ExecutionContext *, const uchar *)) _function->codeRef.code().executableAddress();

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
        loadPtr(Address(ContextRegister, offsetof(ExecutionContext, arguments)), reg);
        offset = arg * sizeof(Value);
    } else if (t->index < _function->locals.size()) {
        loadPtr(Address(ContextRegister, offsetof(ExecutionContext, locals)), reg);
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

    switch (baseName->builtin) {
    case IR::Name::builtin_invalid:
        callRuntimeMethod(result, __qmljs_call_activation_property, call->base, call->args);
        break;
    case IR::Name::builtin_typeof: {
        IR::Temp *arg = call->args->expr->asTemp();
        assert(arg != 0);
        generateFunctionCall(result, __qmljs_builtin_typeof, arg, ContextRegister);
    }
        break;
    case IR::Name::builtin_delete: {
        if (IR::Member *m = call->args->expr->asMember()) {
            generateFunctionCall(result, __qmljs_delete_member, ContextRegister, m->base->asTemp(), identifier(*m->name));
            return;
        } else if (IR::Subscript *ss = call->args->expr->asSubscript()) {
            generateFunctionCall(result, __qmljs_delete_subscript, ContextRegister, ss->base->asTemp(), ss->index->asTemp());
            return;
        } else if (IR::Name *n = call->args->expr->asName()) {
            generateFunctionCall(result, __qmljs_delete_name, ContextRegister, n);
            return;
        } else if (call->args->expr->asTemp()){
            // ### should throw in strict mode
            Address dest = loadTempAddress(ScratchRegister, result);
            Value v = Value::fromBoolean(false);
            storeValue(v, dest);
            return;
        }
        break;
    }
    case IR::Name::builtin_throw: {
        IR::Temp *arg = call->args->expr->asTemp();
        assert(arg != 0);
        generateFunctionCall(result, __qmljs_builtin_throw, arg, ContextRegister);
    }
        break;
    case IR::Name::builtin_create_exception_handler:
        generateFunctionCall(ReturnValueRegister, __qmljs_create_exception_handler, ContextRegister);
        generateFunctionCall(result, setjmp, ReturnValueRegister);
        break;
    case IR::Name::builtin_delete_exception_handler:
        generateFunctionCall(Void, __qmljs_delete_exception_handler, ContextRegister);
        break;
    case IR::Name::builtin_get_exception:
        generateFunctionCall(result, __qmljs_get_exception, ContextRegister);
        break;
    case IR::Name::builtin_foreach_iterator_object: {
        IR::Temp *arg = call->args->expr->asTemp();
        assert(arg != 0);
        generateFunctionCall(result, __qmljs_foreach_iterator_object, arg, ContextRegister);
    }
        break;
    case IR::Name::builtin_foreach_next_property_name: {
        IR::Temp *arg = call->args->expr->asTemp();
        assert(arg != 0);
        generateFunctionCall(result, __qmljs_foreach_next_property_name, arg);
    }
        break;
    case IR::Name::builtin_push_with: {
        IR::Temp *arg = call->args->expr->asTemp();
        assert(arg != 0);
        generateFunctionCall(Void, __qmljs_builtin_push_with, arg, ContextRegister);
    }
        break;
    case IR::Name::builtin_pop_with:
        generateFunctionCall(Void, __qmljs_builtin_pop_with, ContextRegister);
        break;
    case IR::Name::builtin_declare_vars: {
        if (!call->args)
            return;
        IR::Const *deletable = call->args->expr->asConst();
        assert(deletable->type == IR::BoolType);
        for (IR::ExprList *it = call->args->next; it; it = it->next) {
            IR::Name *arg = it->expr->asName();
            assert(arg != 0);
            generateFunctionCall(Void, __qmljs_builtin_declare_var, ContextRegister,
                                 TrustedImm32(deletable->value != 0), identifier(*arg->id));
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
    generateFunctionCall(result, __qmljs_call_value, ContextRegister, thisObject, baseTemp, baseAddressForCallArguments(), TrustedImm32(argc));
}

void InstructionSelection::callProperty(IR::Call *call, IR::Temp *result)
{
    IR::Member *member = call->base->asMember();
    assert(member != 0);
    assert(member->base->asTemp() != 0);

    int argc = prepareVariableArguments(call->args);
    generateFunctionCall(result, __qmljs_call_property, ContextRegister, member->base->asTemp(), identifier(*member->name), baseAddressForCallArguments(), TrustedImm32(argc));
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
    generateFunctionCall(result, __qmljs_construct_property, ContextRegister, member->base->asTemp(), identifier(*member->name), baseAddressForCallArguments(), TrustedImm32(argc));
}

void InstructionSelection::constructValue(IR::New *call, IR::Temp *result)
{
    IR::Temp *baseTemp = call->base->asTemp();
    assert(baseTemp != 0);

    int argc = prepareVariableArguments(call->args);
    generateFunctionCall(result, __qmljs_construct_value, ContextRegister, baseTemp, baseAddressForCallArguments(), TrustedImm32(argc));
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

            if (s->source->asTemp() || s->source->asConst()) {
                generateFunctionCall(Void, __qmljs_set_activation_property, ContextRegister, propertyName, s->source);
                return;
            } else {
                Q_UNREACHABLE();
            }
        } else if (IR::Temp *t = s->target->asTemp()) {
            if (IR::Name *n = s->source->asName()) {
                if (*n->id == QStringLiteral("this")) { // ### `this' should be a builtin.
                    generateFunctionCall(t, __qmljs_get_thisObject, ContextRegister);
                } else {
                    String *propertyName = identifier(*n->id);
                    generateFunctionCall(t, __qmljs_get_activation_property, ContextRegister, propertyName);
                }
                return;
            } else if (IR::Const *c = s->source->asConst()) {
                Address dest = loadTempAddress(ScratchRegister, t);
                Value v;
                switch (c->type) {
                case IR::NullType:
                    v = Value::nullValue();
                    break;
                case IR::UndefinedType:
                    v = Value::undefinedValue();
                    break;
                case IR::BoolType:
                    v = Value::fromBoolean(c->value != 0);
                    break;
                case IR::NumberType: {
                    int ival = (int)c->value;
                    if (ival == c->value) {
                        v = Value::fromInt32(ival);
                    } else {
                        v = Value::fromDouble(c->value);
                    }
                }
                    break;
                default:
                    Q_UNIMPLEMENTED();
                    assert(!"TODO");
                }
                storeValue(v, dest);
                return;
            } else if (IR::Temp *t2 = s->source->asTemp()) {
                copyValue(t, t2);
                return;
            } else if (IR::String *str = s->source->asString()) {
                Address dest = loadTempAddress(ScratchRegister, t);
                Value v = Value::fromString(_engine->newString(*str->value));
                storeValue(v, dest);
                return;
            } else if (IR::RegExp *re = s->source->asRegExp()) {
                Address dest = loadTempAddress(ScratchRegister, t);
                Value v = Value::fromObject(_engine->newRegExpObject(*re->value, re->flags));
                storeValue(v, dest);
                return;
            } else if (IR::Closure *clos = s->source->asClosure()) {
                generateFunctionCall(t, __qmljs_init_closure, TrustedImmPtr(clos->value), ContextRegister);
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
                    generateFunctionCall(t, __qmljs_get_property, ContextRegister, base, identifier(*m->name));
                    return;
                }
                assert(!"wip");
                return;
            } else if (IR::Subscript *ss = s->source->asSubscript()) {
                generateFunctionCall(t, __qmljs_get_element, ContextRegister, ss->base->asTemp(), ss->index->asTemp());
                return;
            } else if (IR::Unop *u = s->source->asUnop()) {
                if (IR::Temp *e = u->expr->asTemp()) {
                    Value (*op)(const Value value, ExecutionContext *ctx) = 0;
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
                        generateFunctionCallImp(t, opName, op, e, ContextRegister);
                    return;
                }
            } else if (IR::Binop *b = s->source->asBinop()) {
                if ((b->left->asTemp() || b->left->asConst()) &&
                    (b->right->asTemp() || b->right->asConst())) {
                    generateBinOp((IR::AluOp)b->op, t, b->left, b->right);
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
                if (s->source->asTemp() || s->source->asConst()) {
                    generateFunctionCall(Void, __qmljs_set_property, ContextRegister, base, identifier(*m->name), s->source);
                    return;
                } else {
                    Q_UNREACHABLE();
                }
            }
        } else if (IR::Subscript *ss = s->target->asSubscript()) {
            if (s->source->asTemp() || s->source->asConst()) {
                generateFunctionCall(Void, __qmljs_set_element, ContextRegister, ss->base->asTemp(), ss->index->asTemp(), s->source);
                return;
            } else {
                Q_UNIMPLEMENTED();
            }
        }
    } else {
        // inplace assignment, e.g. x += 1, ++x, ...
        if (IR::Temp *t = s->target->asTemp()) {
            if (s->source->asTemp() || s->source->asConst()) {
                generateBinOp((IR::AluOp)s->op, t, t, s->source);
                return;
            }
        } else if (IR::Name *n = s->target->asName()) {
            if (s->source->asTemp() || s->source->asConst()) {
                void (*op)(const Value value, String *name, ExecutionContext *ctx) = 0;
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
                    generateFunctionCallImp(Void, opName, op, s->source, identifier(*n->id), ContextRegister);
                }
                return;
            }
        } else if (IR::Subscript *ss = s->target->asSubscript()) {
            if (s->source->asTemp() || s->source->asConst()) {
                void (*op)(Value base, Value index, Value value, ExecutionContext *ctx) = 0;
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
                    generateFunctionCallImp(Void, opName, op, base, index, s->source, ContextRegister);
                }
                return;
            }
        } else if (IR::Member *m = s->target->asMember()) {
            if (s->source->asTemp() || s->source->asConst()) {
                void (*op)(Value value, Value base, String *name, ExecutionContext *ctx) = 0;
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
                    generateFunctionCallImp(Void, opName, op, s->source, base, member, ContextRegister);
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
        Address temp = loadTempAddress(ScratchRegister, t);
        Address tag = temp;
        tag.offset += offsetof(VM::Value, tag);
        Jump booleanConversion = branch32(NotEqual, tag, TrustedImm32(VM::Value::Boolean_Type));

        Address data = temp;
        data.offset += offsetof(VM::Value, int_32);
        load32(data, ReturnValueRegister);
        Jump testBoolean = jump();

        booleanConversion.link(this);
        {
            generateFunctionCall(ReturnValueRegister, __qmljs_to_boolean, t, ContextRegister);
        }

        testBoolean.link(this);
        Jump target = branch32(NotEqual, ReturnValueRegister, TrustedImm32(0));
        _patches[s->iftrue].append(target);

        jumpToBlock(s->iffalse);
        return;
    } else if (IR::Binop *b = s->cond->asBinop()) {
        if ((b->left->asTemp() || b->left->asConst()) &&
            (b->right->asTemp() || b->right->asConst())) {
            Bool (*op)(const Value, const Value, ExecutionContext *ctx) = 0;
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

            generateFunctionCallImp(ReturnValueRegister, opName, op, b->left, b->right, ContextRegister);

            Jump target = branch32(NotEqual, ReturnValueRegister, TrustedImm32(0));
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
#ifdef VALUE_FITS_IN_REGISTER
        copyValue(ReturnValueRegister, t);
#else
        loadPtr(addressForArgument(0), ReturnValueRegister);
        copyValue(Address(ReturnValueRegister, 0), t);
#endif
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
    generateFunctionCallImp(result, name, method, ContextRegister, identifier(*baseName->id), baseAddressForCallArguments(), TrustedImm32(argc));
}

void InstructionSelection::callRuntimeMethodImp(IR::Temp *result, const char* name, BuiltinMethod method, IR::ExprList *args)
{
    int argc = prepareVariableArguments(args);
    generateFunctionCallImp(result, name, method, ContextRegister, baseAddressForCallArguments(), TrustedImm32(argc));
}

template <typename Result, typename Source>
void InstructionSelection::copyValue(Result result, Source source)
{
#ifdef VALUE_FITS_IN_REGISTER
    // Use ReturnValueRegister as "scratch" register because loadArgument
    // and storeArgument are functions that may need a scratch register themselves.
    loadArgument(source, ReturnValueRegister);
    storeArgument(ReturnValueRegister, result);
#else
    loadDouble(source, FPGpr0);
    storeDouble(FPGpr0, result);
#endif
}

void InstructionSelection::generateBinOp(IR::AluOp operation, IR::Temp* target, IR::Expr* left, IR::Expr* right)
{
    const BinaryOperationInfo& info = binaryOperations[operation];
    if (!info.fallbackImplementation) {
        assert(!"unreachable");
        return;
    }

    Value leftConst = Value::undefinedValue();
    Value rightConst = Value::undefinedValue();

    bool canDoInline = info.inlineMemRegOp && info.inlineImmRegOp;

    if (canDoInline) {
        if (left->asConst()) {
            leftConst = convertToValue(left->asConst());
            canDoInline = canDoInline && leftConst.tryIntegerConversion();
        }
        if (right->asConst()) {
            rightConst = convertToValue(right->asConst());
            canDoInline = canDoInline && rightConst.tryIntegerConversion();
        }
    }

    Jump binOpFinished;

    if (canDoInline) {

        Jump leftTypeCheck;
        if (left->asTemp()) {
            Address typeAddress = loadTempAddress(ScratchRegister, left->asTemp());
            typeAddress.offset += offsetof(VM::Value, tag);
            leftTypeCheck = branch32(NotEqual, typeAddress, TrustedImm32(VM::Value::_Integer_Type));
        }

        Jump rightTypeCheck;
        if (right->asTemp()) {
            Address typeAddress = loadTempAddress(ScratchRegister, right->asTemp());
            typeAddress.offset += offsetof(VM::Value, tag);
            rightTypeCheck = branch32(NotEqual, typeAddress, TrustedImm32(VM::Value::_Integer_Type));
        }

        if (left->asTemp()) {
            Address leftValue = loadTempAddress(ScratchRegister, left->asTemp());
            leftValue.offset += offsetof(VM::Value, int_32);
            load32(leftValue, IntegerOpRegister);
        } else { // left->asConst()
            move(TrustedImm32(leftConst.integerValue()), IntegerOpRegister);
        }

        Jump overflowCheck;

        if (right->asTemp()) {
            Address rightValue = loadTempAddress(ScratchRegister, right->asTemp());
            rightValue.offset += offsetof(VM::Value, int_32);

            overflowCheck = info.inlineMemRegOp(this, rightValue, IntegerOpRegister);
        } else { // right->asConst()
            overflowCheck = info.inlineImmRegOp(this, TrustedImm32(rightConst.integerValue()), IntegerOpRegister);
        }

        Address resultAddr = loadTempAddress(ScratchRegister, target);
        Address resultValueAddr = resultAddr;
        resultValueAddr.offset += offsetof(VM::Value, int_32);
        store32(IntegerOpRegister, resultValueAddr);

        Address resultTypeAddr = resultAddr;
        resultTypeAddr.offset += offsetof(VM::Value, tag);
        store32(TrustedImm32(VM::Value::_Integer_Type), resultTypeAddr);

        binOpFinished = jump();

        if (leftTypeCheck.isSet())
            leftTypeCheck.link(this);
        if (rightTypeCheck.isSet())
            rightTypeCheck.link(this);
        if (overflowCheck.isSet())
            overflowCheck.link(this);
    }

    // Fallback
    generateFunctionCallImp(target, info.name, info.fallbackImplementation, left, right, ContextRegister);

    if (binOpFinished.isSet())
        binOpFinished.link(this);
}
