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
#include "qv4object.h"
#include "qv4functionobject.h"
#include "qv4regexpobject.h"
#include "qv4unwindhelper.h"

#include <assembler/LinkBuffer.h>
#include <WTFStubs.h>

#include <iostream>
#include <cassert>

#if USE(UDIS86)
#  include <udis86.h>
#endif

using namespace QQmlJS;
using namespace QQmlJS::MASM;
using namespace QQmlJS::VM;

/* Platform/Calling convention/Architecture specific section */

#if CPU(X86_64)
static const Assembler::RegisterID calleeSavedRegisters[] = {
    // Not used: JSC::X86Registers::rbx,
    // Not used: JSC::X86Registers::r10,
    JSC::X86Registers::r12, // LocalsRegister
    // Not used: JSC::X86Registers::r13,
    JSC::X86Registers::r14 // ContextRegister
    // Not used: JSC::X86Registers::r15,
};
#endif

#if CPU(X86)
static const Assembler::RegisterID calleeSavedRegisters[] = {
    // Not used: JSC::X86Registers::ebx,
    JSC::X86Registers::esi, // ContextRegister
    JSC::X86Registers::edi  // LocalsRegister
};
#endif

#if CPU(ARM)
static const Assembler::RegisterID calleeSavedRegisters[] = {
    // ### FIXME: push multi-register push instruction, remove unused registers.
    JSC::ARMRegisters::r4,
    JSC::ARMRegisters::r5,
    JSC::ARMRegisters::r6,
    JSC::ARMRegisters::r7,
    JSC::ARMRegisters::r8,
    JSC::ARMRegisters::r9,
    JSC::ARMRegisters::r10,
    JSC::ARMRegisters::r11
};
#endif

const int Assembler::calleeSavedRegisterCount = sizeof(calleeSavedRegisters) / sizeof(calleeSavedRegisters[0]);

/* End of platform/calling convention/architecture specific section */


const Assembler::VoidType Assembler::Void;

Assembler::Assembler(IR::Function* function, VM::Function *vmFunction)
    : _function(function), _vmFunction(vmFunction)
{
}

void Assembler::registerBlock(IR::BasicBlock* block)
{
    _addrs[block] = label();
}

void Assembler::jumpToBlock(IR::BasicBlock* current, IR::BasicBlock *target)
{
    if (current->index + 1 != target->index)
        _patches[target].append(jump());
}

void Assembler::addPatch(IR::BasicBlock* targetBlock, Jump targetJump)
{
    _patches[targetBlock].append(targetJump);
}

void Assembler::addPatch(DataLabelPtr patch, Label target)
{
    DataLabelPatch p;
    p.dataLabel = patch;
    p.target = target;
    _dataLabelPatches.append(p);
}

Assembler::Pointer Assembler::loadTempAddress(RegisterID reg, IR::Temp *t)
{
    int32_t offset = 0;
    int scope = t->scope;
    VM::Function *f = _vmFunction;
    RegisterID context = ContextRegister;
    if (scope) {
        loadPtr(Address(ContextRegister, offsetof(ExecutionContext, outer)), ScratchRegister);
        --scope;
        f = f->outer;
        context = ScratchRegister;
        while (scope) {
            loadPtr(Address(context, offsetof(ExecutionContext, outer)), context);
            f = f->outer;
            --scope;
        }
    }
    if (t->index < 0) {
        assert(t->scope == 0);
        const int arg = -t->index - 1;
        loadPtr(Address(context, offsetof(ExecutionContext, arguments)), reg);
        offset = arg * sizeof(Value);
    } else if (t->index < f->locals.size()) {
        loadPtr(Address(context, offsetof(ExecutionContext, locals)), reg);
        offset = t->index * sizeof(Value);
    } else {
        assert(t->scope == 0);
        const int arg = _function->maxNumberOfArguments + t->index - _function->locals.size() + 1;
        offset = - sizeof(Value) * (arg + 1);
        offset -= sizeof(void*) * calleeSavedRegisterCount;
        reg = LocalsRegister;
    }
    return Pointer(reg, offset);
}

template <typename Result, typename Source>
void Assembler::copyValue(Result result, Source source)
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

template <typename Result>
void Assembler::copyValue(Result result, IR::Expr* source)
{
#ifdef VALUE_FITS_IN_REGISTER
    // Use ReturnValueRegister as "scratch" register because loadArgument
    // and storeArgument are functions that may need a scratch register themselves.
    loadArgument(source, ReturnValueRegister);
    storeArgument(ReturnValueRegister, result);
#else
    if (IR::Temp *temp = source->asTemp()) {
        loadDouble(temp, FPGpr0);
        storeDouble(FPGpr0, result);
    } else if (IR::Const *c = source->asConst()) {
        VM::Value v = convertToValue(c);
        storeValue(v, result);
    } else {
        assert(! "not implemented");
    }
#endif
}


void Assembler::storeValue(VM::Value value, IR::Temp* destination)
{
    Address addr = loadTempAddress(ScratchRegister, destination);
    storeValue(value, addr);
}

void Assembler::enterStandardStackFrame(int locals)
{
    platformEnterStandardStackFrame();

    // ### FIXME: Handle through calleeSavedRegisters mechanism
    // or eliminate StackFrameRegister altogether.
    push(StackFrameRegister);
    move(StackPointerRegister, StackFrameRegister);

    // space for the locals and callee saved registers
    int32_t frameSize = locals * sizeof(QQmlJS::VM::Value) + sizeof(void*) * calleeSavedRegisterCount;

#if CPU(X86) || CPU(X86_64)
    frameSize = (frameSize + 15) & ~15; // align on 16 byte boundaries for MMX
#endif
    subPtr(TrustedImm32(frameSize), StackPointerRegister);

    for (int i = 0; i < calleeSavedRegisterCount; ++i)
        storePtr(calleeSavedRegisters[i], Address(StackFrameRegister, -(i + 1) * sizeof(void*)));

    move(StackFrameRegister, LocalsRegister);
}

void Assembler::leaveStandardStackFrame(int locals)
{
    // restore the callee saved registers
    for (int i = calleeSavedRegisterCount - 1; i >= 0; --i)
        loadPtr(Address(StackFrameRegister, -(i + 1) * sizeof(void*)), calleeSavedRegisters[i]);

    // space for the locals and the callee saved registers
    int32_t frameSize = locals * sizeof(QQmlJS::VM::Value) + sizeof(void*) * calleeSavedRegisterCount;
#if CPU(X86) || CPU(X86_64)
    frameSize = (frameSize + 15) & ~15; // align on 16 byte boundaries for MMX
#endif
    // Work around bug in ARMv7Assembler.h where add32(imm, sp, sp) doesn't
    // work well for large immediates.
#if CPU(ARM_THUMB2)
    move(TrustedImm32(frameSize), Assembler::ScratchRegister);
    add32(Assembler::ScratchRegister, StackPointerRegister);
#else
    addPtr(TrustedImm32(frameSize), StackPointerRegister);
#endif

    pop(StackFrameRegister);
    platformLeaveStandardStackFrame();
}



#define OP(op) \
    { isel_stringIfy(op), op, 0, 0 }

#define INLINE_OP(op, memOp, immOp) \
    { isel_stringIfy(op), op, memOp, immOp }

#define NULL_OP \
    { 0, 0, 0, 0 }

const Assembler::BinaryOperationInfo Assembler::binaryOperations[QQmlJS::IR::LastAluOp + 1] = {
    NULL_OP, // OpInvalid
    NULL_OP, // OpIfTrue
    NULL_OP, // OpNot
    NULL_OP, // OpUMinus
    NULL_OP, // OpUPlus
    NULL_OP, // OpCompl
    NULL_OP, // OpIncrement
    NULL_OP, // OpDecrement

    INLINE_OP(__qmljs_bit_and, &Assembler::inline_and32, &Assembler::inline_and32), // OpBitAnd
    INLINE_OP(__qmljs_bit_or, &Assembler::inline_or32, &Assembler::inline_or32), // OpBitOr
    INLINE_OP(__qmljs_bit_xor, &Assembler::inline_xor32, &Assembler::inline_xor32), // OpBitXor

    INLINE_OP(__qmljs_add, &Assembler::inline_add32, &Assembler::inline_add32), // OpAdd
    INLINE_OP(__qmljs_sub, &Assembler::inline_sub32, &Assembler::inline_sub32), // OpSub
    INLINE_OP(__qmljs_mul, &Assembler::inline_mul32, &Assembler::inline_mul32), // OpMul

    OP(__qmljs_div), // OpDiv
    OP(__qmljs_mod), // OpMod

    INLINE_OP(__qmljs_shl, &Assembler::inline_shl32, &Assembler::inline_shl32), // OpLShift
    INLINE_OP(__qmljs_shr, &Assembler::inline_shr32, &Assembler::inline_shr32), // OpRShift
    INLINE_OP(__qmljs_ushr, &Assembler::inline_ushr32, &Assembler::inline_ushr32), // OpURShift

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

void Assembler::generateBinOp(IR::AluOp operation, IR::Temp* target, IR::Temp *left, IR::Temp *right)
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

            overflowCheck = (this->*info.inlineMemRegOp)(rightValue, IntegerOpRegister);
        } else { // right->asConst()
            overflowCheck = (this->*info.inlineImmRegOp)(TrustedImm32(rightConst.integerValue()), IntegerOpRegister);
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
    generateFunctionCallImp(Assembler::Void, info.name, info.fallbackImplementation, ContextRegister,
                            Assembler::PointerToValue(target), Assembler::Reference(left), Assembler::Reference(right));

    if (binOpFinished.isSet())
        binOpFinished.link(this);
}
#if OS(LINUX)
static void printDisassembledOutputWithCalls(const char* output, const QHash<void*, const char*>& functions)
{
    QByteArray processedOutput(output);
    for (QHash<void*, const char*>::ConstIterator it = functions.begin(), end = functions.end();
         it != end; ++it) {
        QByteArray ptrString = QByteArray::number(quintptr(it.key()), 16);
        ptrString.prepend("0x");
        processedOutput = processedOutput.replace(ptrString, it.value());
    }
    fprintf(stderr, "%s\n", processedOutput.constData());
}
#endif

void Assembler::link(VM::Function *vmFunc)
{
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

    foreach (const DataLabelPatch &p, _dataLabelPatches)
        linkBuffer.patch(p.dataLabel, linkBuffer.locationOf(p.target));

    static bool showCode = !qgetenv("SHOW_CODE").isNull();
    if (showCode) {
#if OS(LINUX)
        char* disasmOutput = 0;
        size_t disasmLength = 0;
        FILE* disasmStream = open_memstream(&disasmOutput, &disasmLength);
        WTF::setDataFile(disasmStream);
#endif

        QByteArray name = _function->name->toUtf8();
        vmFunc->codeRef = linkBuffer.finalizeCodeWithDisassembly("%s", name.data());

        WTF::setDataFile(stderr);
#if OS(LINUX)
        fclose(disasmStream);
#if CPU(X86) || CPU(X86_64)
        printDisassembledOutputWithCalls(disasmOutput, functions);
#endif
        free(disasmOutput);
#endif
    } else {
        vmFunc->codeRef = linkBuffer.finalizeCodeWithoutDisassembly();
    }

    vmFunc->code = (Value (*)(VM::ExecutionContext *, const uchar *)) vmFunc->codeRef.code().executableAddress();
}

InstructionSelection::InstructionSelection(VM::ExecutionEngine *engine, IR::Module *module)
    : EvalInstructionSelection(engine, module)
    , _block(0)
    , _function(0)
    , _vmFunction(0)
    , _as(0)
{
}

InstructionSelection::~InstructionSelection()
{
    delete _as;
}

void InstructionSelection::run(VM::Function *vmFunction, IR::Function *function)
{
    QVector<Lookup> lookups;
    qSwap(_function, function);
    qSwap(_vmFunction, vmFunction);
    qSwap(_lookups, lookups);
    Assembler* oldAssembler = _as;
    _as = new Assembler(_function, _vmFunction);

    int locals = (_function->tempCount - _function->locals.size() + _function->maxNumberOfArguments) + 1;
    locals = (locals + 1) & ~1;
    _as->enterStandardStackFrame(locals);

    int contextPointer = 0;
#ifndef VALUE_FITS_IN_REGISTER
    // When the return VM value doesn't fit into a register, then
    // the caller provides a pointer for storage as first argument.
    // That shifts the index the context pointer argument by one.
    contextPointer++;
#endif

#ifdef ARGUMENTS_IN_REGISTERS
    _as->move(_as->registerForArgument(contextPointer), Assembler::ContextRegister);
#else
    _as->loadPtr(addressForArgument(contextPointer), Assembler::ContextRegister);
#endif

    foreach (IR::BasicBlock *block, _function->basicBlocks) {
        _block = block;
        _as->registerBlock(_block);
        foreach (IR::Stmt *s, block->statements) {
            s->accept(this);
        }
    }

    _as->leaveStandardStackFrame(locals);
#ifndef ARGUMENTS_IN_REGISTERS
    // Emulate ret(n) instruction
    // Pop off return address into scratch register ...
    _as->pop(Assembler::ScratchRegister);
    // ... and overwrite the invisible argument with
    // the return address.
    _as->poke(Assembler::ScratchRegister);
#endif
    _as->ret();
    // TODO: add a label and a nop, so we can determine the exact function length

    _as->link(_vmFunction);

    if (_lookups.size()) {
        _vmFunction->lookups = new Lookup[_lookups.size()];
        memcpy(_vmFunction->lookups, _lookups.constData(), _lookups.size()*sizeof(Lookup));
    }

    if (engine()->unwindHelper)
        engine()->unwindHelper->registerFunction(_vmFunction);

    qSwap(_vmFunction, vmFunction);
    qSwap(_function, function);
    qSwap(_lookups, lookups);
    delete _as;
    _as = oldAssembler;
}

void InstructionSelection::callBuiltinInvalid(IR::Name *func, IR::ExprList *args, IR::Temp *result)
{
    callRuntimeMethod(result, __qmljs_call_activation_property, func, args);
}

void InstructionSelection::callBuiltinTypeofMember(IR::Temp *base, const QString &name, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_typeof_member, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::Reference(base), identifier(name));
}

void InstructionSelection::callBuiltinTypeofSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_typeof_element,
            Assembler::ContextRegister, Assembler::PointerToValue(result),
            Assembler::Reference(base), Assembler::Reference(index));
}

void InstructionSelection::callBuiltinTypeofName(const QString &name, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_typeof_name, Assembler::ContextRegister, Assembler::PointerToValue(result), identifier(name));
}

void InstructionSelection::callBuiltinTypeofValue(IR::Temp *value, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_typeof, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::Reference(value));
}

void InstructionSelection::callBuiltinDeleteMember(IR::Temp *base, const QString &name, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_delete_member, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::Reference(base), identifier(name));
}

void InstructionSelection::callBuiltinDeleteSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_delete_subscript, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::Reference(base), Assembler::Reference(index));
}

void InstructionSelection::callBuiltinDeleteName(const QString &name, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_delete_name, Assembler::ContextRegister, Assembler::PointerToValue(result), identifier(name));
}

void InstructionSelection::callBuiltinDeleteValue(IR::Temp *result)
{
    _as->storeValue(Value::fromBoolean(false), result);
}

void InstructionSelection::callBuiltinPostIncrementMember(IR::Temp *base, const QString &name, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_increment_member, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), Assembler::PointerToValue(base), identifier(name));
}

void InstructionSelection::callBuiltinPostIncrementSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_increment_element, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), Assembler::Reference(base), Assembler::PointerToValue(index));
}

void InstructionSelection::callBuiltinPostIncrementName(const QString &name, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_increment_name, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), identifier(name));
}

void InstructionSelection::callBuiltinPostIncrementValue(IR::Temp *value, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_increment, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), Assembler::PointerToValue(value));
}

void InstructionSelection::callBuiltinPostDecrementMember(IR::Temp *base, const QString &name, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_decrement_member, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), Assembler::Reference(base), identifier(name));
}

void InstructionSelection::callBuiltinPostDecrementSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_decrement_element, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), Assembler::Reference(base),
                         Assembler::Reference(index));
}

void InstructionSelection::callBuiltinPostDecrementName(const QString &name, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_decrement_name, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), identifier(name));
}

void InstructionSelection::callBuiltinPostDecrementValue(IR::Temp *value, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_decrement, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), Assembler::PointerToValue(value));
}

void InstructionSelection::callBuiltinThrow(IR::Temp *arg)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_throw, Assembler::ContextRegister, Assembler::Reference(arg));
}

static void *tryWrapper(ExecutionContext *context, void *localsPtr, void *(*exceptionEntryPointInCallingFunction)(ExecutionContext*, void*, int))
{
    void *addressToContinueAt = 0;
    try {
        addressToContinueAt = exceptionEntryPointInCallingFunction(context, localsPtr, 0);
    } catch (Exception& ex) {
        ex.accept(context);
        try {
            addressToContinueAt = exceptionEntryPointInCallingFunction(context, localsPtr, 1);
        } catch (Exception& ex) {
            ex.accept(context);
            addressToContinueAt = exceptionEntryPointInCallingFunction(context, localsPtr, 1);
        }
    }
    return addressToContinueAt;
}

void InstructionSelection::callBuiltinCreateExceptionHandler(IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_create_exception_handler, Assembler::ContextRegister);

    // Call tryWrapper, which is going to re-enter the same function again below.
    // When tryWrapper returns, it returns the with address of where to continue.
    Assembler::DataLabelPtr movePatch = _as->moveWithPatch(Assembler::TrustedImmPtr(0), Assembler::ScratchRegister);
    generateFunctionCall(Assembler::ReturnValueRegister, tryWrapper, Assembler::ContextRegister, Assembler::LocalsRegister, Assembler::ScratchRegister);
    _as->jump(Assembler::ReturnValueRegister);

    // tryWrapper calls us at this place with arg3 == 0 if we're supposed to execute the try block
    // and arg == 1 if we caught an exception. The generated IR takes care of returning from this
    // call when deleteExceptionHandler is called.
    _as->addPatch(movePatch, _as->label());
    _as->enterStandardStackFrame(/*locals*/0);
#ifdef ARGUMENTS_IN_REGISTERS
    _as->move(Assembler::registerForArgument(0), Assembler::ContextRegister);
    _as->move(Assembler::registerForArgument(1), Assembler::LocalsRegister);
#else
    _as->loadPtr(addressForArgument(0), Assembler::ContextRegister);
    _as->loadPtr(addressForArgument(1), Assembler::LocalsRegister);
#endif

    Address addr = _as->loadTempAddress(Assembler::ScratchRegister, result);
#ifdef ARGUMENTS_IN_REGISTERS
    _as->store32(Assembler::registerForArgument(2), addr);
#else
    _as->load32(addressForArgument(2), Assembler::ReturnValueRegister);
    _as->store32(Assembler::ReturnValueRegister, addr);
#endif
    addr.offset += 4;
    _as->store32(Assembler::TrustedImm32(Value::Boolean_Type), addr);
}

void InstructionSelection::callBuiltinDeleteExceptionHandler()
{
    // This assumes that we're in code that was called by tryWrapper, so we return to try wrapper
    // with the address that we'd like to continue at, which is right after the ret below.
    Assembler::DataLabelPtr continuation = _as->moveWithPatch(Assembler::TrustedImmPtr(0), Assembler::ReturnValueRegister);
    _as->leaveStandardStackFrame(/*locals*/0);
    _as->ret();
    _as->addPatch(continuation, _as->label());
}

void InstructionSelection::callBuiltinGetException(IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_get_exception, Assembler::ContextRegister, Assembler::PointerToValue(result));
}

void InstructionSelection::callBuiltinForeachIteratorObject(IR::Temp *arg, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_foreach_iterator_object, Assembler::ContextRegister, Assembler::PointerToValue(result), Assembler::Reference(arg), Assembler::ContextRegister);
}

void InstructionSelection::callBuiltinForeachNextPropertyname(IR::Temp *arg, IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_foreach_next_property_name, Assembler::PointerToValue(result), Assembler::Reference(arg));
}

void InstructionSelection::callBuiltinPushWithScope(IR::Temp *arg)
{
    generateFunctionCall(Assembler::ContextRegister, __qmljs_builtin_push_with_scope, Assembler::Reference(arg), Assembler::ContextRegister);
}

void InstructionSelection::callBuiltinPushCatchScope(const QString &exceptionVarName)
{
    generateFunctionCall(Assembler::ContextRegister, __qmljs_builtin_push_catch_scope, identifier(exceptionVarName), Assembler::ContextRegister);
}

void InstructionSelection::callBuiltinPopScope()
{
    generateFunctionCall(Assembler::ContextRegister, __qmljs_builtin_pop_scope, Assembler::ContextRegister);
}

void InstructionSelection::callBuiltinDeclareVar(bool deletable, const QString &name)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_declare_var, Assembler::ContextRegister,
                         Assembler::TrustedImm32(deletable), identifier(name));
}

void InstructionSelection::callBuiltinDefineGetterSetter(IR::Temp *object, const QString &name, IR::Temp *getter, IR::Temp *setter)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_define_getter_setter, Assembler::ContextRegister,
                         Assembler::Reference(object), identifier(name), Assembler::PointerToValue(getter), Assembler::PointerToValue(setter));
}

void InstructionSelection::callBuiltinDefineProperty(IR::Temp *object, const QString &name, IR::Temp *value)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_define_property, Assembler::ContextRegister,
                         Assembler::Reference(object), identifier(name), Assembler::PointerToValue(value));
}

void InstructionSelection::callBuiltinDefineArray(IR::Temp *result, IR::ExprList *args)
{
    int length = prepareVariableArguments(args);
    generateFunctionCall(Assembler::Void, __qmljs_builtin_define_array, Assembler::ContextRegister,
                         Assembler::PointerToValue(result),
                         baseAddressForCallArguments(), Assembler::TrustedImm32(length));
}

void InstructionSelection::callValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result)
{
    int argc = prepareVariableArguments(args);
    IR::Temp* thisObject = 0;
    generateFunctionCall(Assembler::Void, __qmljs_call_value, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::PointerToValue(thisObject),
            Assembler::Reference(value), baseAddressForCallArguments(), Assembler::TrustedImm32(argc));
}

void InstructionSelection::loadThisObject(IR::Temp *temp)
{
    generateFunctionCall(Assembler::Void, __qmljs_get_thisObject, Assembler::ContextRegister, Assembler::PointerToValue(temp));
}

void InstructionSelection::loadConst(IR::Const *sourceConst, IR::Temp *targetTemp)
{
    _as->storeValue(convertToValue(sourceConst), targetTemp);
}

void InstructionSelection::loadString(const QString &str, IR::Temp *targetTemp)
{
    Value v = Value::fromString(identifier(str));
    _as->storeValue(v, targetTemp);
}

void InstructionSelection::loadRegexp(IR::RegExp *sourceRegexp, IR::Temp *targetTemp)
{
    Value v = Value::fromObject(engine()->newRegExpObject(*sourceRegexp->value,
                                                          sourceRegexp->flags));
    _vmFunction->generatedValues.append(v);
    _as->storeValue(v, targetTemp);
}

void InstructionSelection::getActivationProperty(const QString &name, IR::Temp *temp)
{
    String *propertyName = identifier(name);
    generateFunctionCall(Assembler::Void, __qmljs_get_activation_property, Assembler::ContextRegister, Assembler::PointerToValue(temp), propertyName);
}

void InstructionSelection::setActivationProperty(IR::Temp *source, const QString &targetName)
{
    String *propertyName = identifier(targetName);
    generateFunctionCall(Assembler::Void, __qmljs_set_activation_property,
            Assembler::ContextRegister, propertyName, Assembler::Reference(source));
}

void InstructionSelection::initClosure(IR::Closure *closure, IR::Temp *target)
{
    VM::Function *vmFunc = vmFunction(closure->value);
    assert(vmFunc);
    generateFunctionCall(Assembler::Void, __qmljs_init_closure, Assembler::ContextRegister, Assembler::PointerToValue(target), Assembler::TrustedImmPtr(vmFunc));
}

void InstructionSelection::getProperty(IR::Temp *base, const QString &name, IR::Temp *target)
{
    if (useFastLookups) {
        VM::String *s = identifier(name);
        uint index = addLookup(s);
        generateFunctionCall(Assembler::Void, __qmljs_get_property_lookup, Assembler::ContextRegister, Assembler::PointerToValue(target),
                             Assembler::Reference(base), Assembler::TrustedImm32(index));
    } else {
        generateFunctionCall(Assembler::Void, __qmljs_get_property, Assembler::ContextRegister, Assembler::PointerToValue(target),
                             Assembler::Reference(base), identifier(name));
    }
}

void InstructionSelection::setProperty(IR::Temp *source, IR::Temp *targetBase, const QString &targetName)
{
    if (useFastLookups) {
        VM::String *s = identifier(targetName);
        uint index = addLookup(s);
        generateFunctionCall(Assembler::Void, __qmljs_set_property_lookup,
                Assembler::ContextRegister, Assembler::Reference(targetBase),
                Assembler::TrustedImm32(index), Assembler::Reference(source));
    } else {
        generateFunctionCall(Assembler::Void, __qmljs_set_property, Assembler::ContextRegister,
                Assembler::Reference(targetBase),
                identifier(targetName), Assembler::Reference(source));
    }
}

void InstructionSelection::getElement(IR::Temp *base, IR::Temp *index, IR::Temp *target)
{
    generateFunctionCall(Assembler::Void, __qmljs_get_element, Assembler::ContextRegister,
                         Assembler::PointerToValue(target), Assembler::Reference(base),
                         Assembler::Reference(index));
}

void InstructionSelection::setElement(IR::Temp *source, IR::Temp *targetBase, IR::Temp *targetIndex)
{
    generateFunctionCall(Assembler::Void, __qmljs_set_element, Assembler::ContextRegister,
                         Assembler::Reference(targetBase), Assembler::Reference(targetIndex),
                         Assembler::Reference(source));
}

void InstructionSelection::copyValue(IR::Temp *sourceTemp, IR::Temp *targetTemp)
{
    _as->copyValue(targetTemp, sourceTemp);
}

#define setOp(op, opName, operation) \
    do { op = operation; opName = isel_stringIfy(operation); } while (0)

void InstructionSelection::unop(IR::AluOp oper, IR::Temp *sourceTemp, IR::Temp *targetTemp)
{
    VM::UnaryOpName op = 0;
    const char *opName = 0;
    switch (oper) {
    case IR::OpIfTrue: assert(!"unreachable"); break;
    case IR::OpNot: setOp(op, opName, __qmljs_not); break;
    case IR::OpUMinus: setOp(op, opName, __qmljs_uminus); break;
    case IR::OpUPlus: setOp(op, opName, __qmljs_uplus); break;
    case IR::OpCompl: setOp(op, opName, __qmljs_compl); break;
    case IR::OpIncrement: setOp(op, opName, __qmljs_increment); break;
    case IR::OpDecrement: setOp(op, opName, __qmljs_decrement); break;
    default: assert(!"unreachable"); break;
    } // switch

    if (op)
        _as->generateFunctionCallImp(Assembler::Void, opName, op, Assembler::ContextRegister, Assembler::PointerToValue(targetTemp),
                                     Assembler::Reference(sourceTemp));
}

void InstructionSelection::binop(IR::AluOp oper, IR::Temp *leftSource, IR::Temp *rightSource, IR::Temp *target)
{
    _as->generateBinOp(oper, target, leftSource, rightSource);
}

void InstructionSelection::inplaceNameOp(IR::AluOp oper, IR::Temp *rightSource, const QString &targetName)
{
    VM::InplaceBinOpName op = 0;
    const char *opName = 0;
    switch (oper) {
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
        _as->generateFunctionCallImp(Assembler::Void, opName, op, Assembler::ContextRegister,
                identifier(targetName), Assembler::Reference(rightSource));
    }
}

void InstructionSelection::inplaceElementOp(IR::AluOp oper, IR::Temp *source, IR::Temp *targetBaseTemp, IR::Temp *targetIndexTemp)
{
    VM::InplaceBinOpElement op = 0;
    const char *opName = 0;
    switch (oper) {
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
        _as->generateFunctionCallImp(Assembler::Void, opName, op, Assembler::ContextRegister,
                                     Assembler::Reference(targetBaseTemp), Assembler::Reference(targetIndexTemp),
                                     Assembler::Reference(source));
    }
}

void InstructionSelection::inplaceMemberOp(IR::AluOp oper, IR::Temp *source, IR::Temp *targetBase, const QString &targetName)
{
    VM::InplaceBinOpMember op = 0;
    const char *opName = 0;
    switch (oper) {
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
        String* member = identifier(targetName);
        _as->generateFunctionCallImp(Assembler::Void, opName, op, Assembler::ContextRegister,
                                     Assembler::Reference(targetBase), identifier(targetName),
                                     Assembler::Reference(source));
    }
}

void InstructionSelection::callProperty(IR::Temp *base, const QString &name,
                                        IR::ExprList *args, IR::Temp *result)
{
    assert(base != 0);

    int argc = prepareVariableArguments(args);
    VM::String *s = identifier(name);

    if (useFastLookups) {
        uint index = addLookup(s);
        generateFunctionCall(Assembler::Void, __qmljs_call_property_lookup,
                             Assembler::ContextRegister, Assembler::PointerToValue(result),
                             Assembler::Reference(base), Assembler::TrustedImm32(index),
                             baseAddressForCallArguments(),
                             Assembler::TrustedImm32(argc));
    } else {
        generateFunctionCall(Assembler::Void, __qmljs_call_property,
                             Assembler::ContextRegister, Assembler::PointerToValue(result),
                             Assembler::Reference(base), s,
                             baseAddressForCallArguments(),
                             Assembler::TrustedImm32(argc));
    }
}

void InstructionSelection::callSubscript(IR::Temp *base, IR::Temp *index, IR::ExprList *args, IR::Temp *result)
{
    assert(base != 0);

    int argc = prepareVariableArguments(args);
    generateFunctionCall(Assembler::Void, __qmljs_call_element,
                         Assembler::ContextRegister, Assembler::PointerToValue(result),
                         Assembler::Reference(base), Assembler::Reference(index),
                         baseAddressForCallArguments(),
                         Assembler::TrustedImm32(argc));
}

String *InstructionSelection::identifier(const QString &s)
{
    String *str = engine()->newIdentifier(s);
    _vmFunction->identifiers.append(str);
    return str;
}

void InstructionSelection::constructActivationProperty(IR::Name *func, IR::ExprList *args, IR::Temp *result)
{
    assert(func != 0);

    callRuntimeMethod(result, __qmljs_construct_activation_property, func, args);
}

void InstructionSelection::constructProperty(IR::Temp *base, const QString &name, IR::ExprList *args, IR::Temp *result)
{
    int argc = prepareVariableArguments(args);
    generateFunctionCall(Assembler::Void, __qmljs_construct_property, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::Reference(base), identifier(name), baseAddressForCallArguments(), Assembler::TrustedImm32(argc));
}

void InstructionSelection::constructValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result)
{
    assert(value != 0);

    int argc = prepareVariableArguments(args);
    generateFunctionCall(Assembler::Void, __qmljs_construct_value, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::Reference(value), baseAddressForCallArguments(), Assembler::TrustedImm32(argc));
}

void InstructionSelection::visitJump(IR::Jump *s)
{
    _as->jumpToBlock(_block, s->target);
}

void InstructionSelection::visitCJump(IR::CJump *s)
{
    if (IR::Temp *t = s->cond->asTemp()) {
        Address temp = _as->loadTempAddress(Assembler::ScratchRegister, t);
        Address tag = temp;
        tag.offset += offsetof(VM::Value, tag);
        Assembler::Jump booleanConversion = _as->branch32(Assembler::NotEqual, tag, Assembler::TrustedImm32(VM::Value::Boolean_Type));

        Address data = temp;
        data.offset += offsetof(VM::Value, int_32);
        _as->load32(data, Assembler::ReturnValueRegister);
        Assembler::Jump testBoolean = _as->jump();

        booleanConversion.link(_as);
        {
            generateFunctionCall(Assembler::ReturnValueRegister, __qmljs_to_boolean, Assembler::Reference(t));
        }

        testBoolean.link(_as);
        Assembler::Jump target = _as->branch32(Assembler::NotEqual, Assembler::ReturnValueRegister, Assembler::TrustedImm32(0));
        _as->addPatch(s->iftrue, target);

        _as->jumpToBlock(_block, s->iffalse);
        return;
    } else if (IR::Binop *b = s->cond->asBinop()) {
        if (b->left->asTemp() && b->right->asTemp()) {
            VM::CmpOp op = 0;
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

            _as->generateFunctionCallImp(Assembler::ReturnValueRegister, opName, op, Assembler::ContextRegister,
                                         Assembler::Reference(b->left->asTemp()),
                                         Assembler::Reference(b->right->asTemp()));

            Assembler::Jump target = _as->branch32(Assembler::NotEqual, Assembler::ReturnValueRegister, Assembler::TrustedImm32(0));
            _as->addPatch(s->iftrue, target);

            _as->jumpToBlock(_block, s->iffalse);
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
#if defined(ARGUMENTS_IN_REGISTERS) && defined(VALUE_FITS_IN_REGISTER)
        _as->copyValue(Assembler::ReturnValueRegister, t);
#else
        _as->loadPtr(addressForArgument(0), Assembler::ReturnValueRegister);
        _as->copyValue(Address(Assembler::ReturnValueRegister, 0), t);
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
//        IR::Temp *arg = it->expr->asTemp();
//        assert(arg != 0);
        _as->copyValue(argumentAddressForCall(i), it->expr);
    }

    return argc;
}

void InstructionSelection::callRuntimeMethodImp(IR::Temp *result, const char* name, ActivationMethod method, IR::Expr *base, IR::ExprList *args)
{
    IR::Name *baseName = base->asName();
    assert(baseName != 0);

    int argc = prepareVariableArguments(args);
    _as->generateFunctionCallImp(Assembler::Void, name, method, Assembler::ContextRegister, Assembler::PointerToValue(result),
                                 identifier(*baseName->id), baseAddressForCallArguments(),
                                 Assembler::TrustedImm32(argc));
}


uint InstructionSelection::addLookup(VM::String *name)
{
    uint index = (uint)_lookups.size();
    VM::Lookup l;
    l.mainClass = 0;
    l.protoClass = 0;
    l.index = 0;
    l.name = name;
    _lookups.append(l);
    return index;
}
