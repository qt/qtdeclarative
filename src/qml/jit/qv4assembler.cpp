/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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
#include "qv4runtime_p.h"
#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include "qv4regexpobject_p.h"
#include "qv4lookup_p.h"
#include "qv4function_p.h"
#include "qv4ssa_p.h"
#include "qv4regalloc_p.h"
#include "qv4assembler_p.h"

#include <assembler/LinkBuffer.h>
#include <WTFStubs.h>

#include <iostream>
#include <cassert>

#if ENABLE(ASSEMBLER)

#if USE(UDIS86)
#  include <udis86.h>
#endif

using namespace QV4;
using namespace QV4::JIT;

CompilationUnit::~CompilationUnit()
{
    foreach (Function *f, runtimeFunctions)
        engine->allFunctions.remove(reinterpret_cast<quintptr>(f->code));
}

void CompilationUnit::linkBackendToEngine(ExecutionEngine *engine)
{
    runtimeFunctions.resize(data->functionTableSize);
    runtimeFunctions.fill(0);
    for (int i = 0 ;i < runtimeFunctions.size(); ++i) {
        const CompiledData::Function *compiledFunction = data->functionAt(i);

        QV4::Function *runtimeFunction = new QV4::Function(engine, this, compiledFunction,
                                                           (ReturnedValue (*)(QV4::ExecutionContext *, const uchar *)) codeRefs[i].code().executableAddress());
        runtimeFunctions[i] = runtimeFunction;
    }

    foreach (Function *f, runtimeFunctions)
        engine->allFunctions.insert(reinterpret_cast<quintptr>(f->code), f);
}

QV4::ExecutableAllocator::ChunkOfPages *CompilationUnit::chunkForFunction(int functionIndex)
{
    if (functionIndex < 0 || functionIndex >= codeRefs.count())
        return 0;
    JSC::ExecutableMemoryHandle *handle = codeRefs[functionIndex].executableMemory();
    if (!handle)
        return 0;
    return handle->chunk();
}



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
    JSC::X86Registers::ebx, // temporary register
    JSC::X86Registers::esi, // ContextRegister
    JSC::X86Registers::edi  // LocalsRegister
};
#endif

#if CPU(ARM)
static const Assembler::RegisterID calleeSavedRegisters[] = {
    JSC::ARMRegisters::r11,
    JSC::ARMRegisters::r10,
    JSC::ARMRegisters::r9,
    JSC::ARMRegisters::r8,
    JSC::ARMRegisters::r7,
    JSC::ARMRegisters::r6,
    JSC::ARMRegisters::r5,
    JSC::ARMRegisters::r4
};
#endif

const int Assembler::calleeSavedRegisterCount = sizeof(calleeSavedRegisters) / sizeof(calleeSavedRegisters[0]);

/* End of platform/calling convention/architecture specific section */


const Assembler::VoidType Assembler::Void;

Assembler::Assembler(InstructionSelection *isel, IR::Function* function, QV4::ExecutableAllocator *executableAllocator,
                     int maxArgCountForBuiltins)
    : _stackLayout(function, maxArgCountForBuiltins)
    , _constTable(this)
    , _function(function)
    , _nextBlock(0)
    , _executableAllocator(executableAllocator)
    , _isel(isel)
{
}

void Assembler::registerBlock(IR::BasicBlock* block, IR::BasicBlock *nextBlock)
{
    _addrs[block] = label();
    catchBlock = block->catchBlock;
    _nextBlock = nextBlock;
}

void Assembler::jumpToBlock(IR::BasicBlock* current, IR::BasicBlock *target)
{
    Q_UNUSED(current);

    if (target != _nextBlock)
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

void Assembler::addPatch(DataLabelPtr patch, IR::BasicBlock *target)
{
    _labelPatches[target].append(patch);
}

void Assembler::generateCJumpOnNonZero(RegisterID reg, IR::BasicBlock *currentBlock,
                                       IR::BasicBlock *trueBlock, IR::BasicBlock *falseBlock)
{
    generateCJumpOnCompare(NotEqual, reg, TrustedImm32(0), currentBlock, trueBlock, falseBlock);
}

void Assembler::generateCJumpOnCompare(RelationalCondition cond, RegisterID left,TrustedImm32 right,
                                       IR::BasicBlock *currentBlock,  IR::BasicBlock *trueBlock,
                                       IR::BasicBlock *falseBlock)
{
    if (trueBlock == _nextBlock) {
        Jump target = branch32(invert(cond), left, right);
        addPatch(falseBlock, target);
    } else {
        Jump target = branch32(cond, left, right);
        addPatch(trueBlock, target);
        jumpToBlock(currentBlock, falseBlock);
    }
}

void Assembler::generateCJumpOnCompare(RelationalCondition cond, RegisterID left, RegisterID right,
                                       IR::BasicBlock *currentBlock,  IR::BasicBlock *trueBlock,
                                       IR::BasicBlock *falseBlock)
{
    if (trueBlock == _nextBlock) {
        Jump target = branch32(invert(cond), left, right);
        addPatch(falseBlock, target);
    } else {
        Jump target = branch32(cond, left, right);
        addPatch(trueBlock, target);
        jumpToBlock(currentBlock, falseBlock);
    }
}

Assembler::Pointer Assembler::loadTempAddress(RegisterID baseReg, IR::Temp *t)
{
    int32_t offset = 0;
    int scope = t->scope;
    RegisterID context = ContextRegister;
    if (scope) {
        loadPtr(Address(ContextRegister, qOffsetOf(ExecutionContext, outer)), baseReg);
        --scope;
        context = baseReg;
        while (scope) {
            loadPtr(Address(context, qOffsetOf(ExecutionContext, outer)), context);
            --scope;
        }
    }
    switch (t->kind) {
    case IR::Temp::Formal:
    case IR::Temp::ScopedFormal: {
        loadPtr(Address(context, qOffsetOf(ExecutionContext, callData)), baseReg);
        offset = sizeof(CallData) + (t->index - 1) * sizeof(Value);
    } break;
    case IR::Temp::Local:
    case IR::Temp::ScopedLocal: {
        loadPtr(Address(context, qOffsetOf(CallContext, locals)), baseReg);
        offset = t->index * sizeof(Value);
    } break;
    case IR::Temp::StackSlot: {
        return stackSlotPointer(t);
    } break;
    default:
        Q_UNREACHABLE();
    }
    return Pointer(baseReg, offset);
}

Assembler::Pointer Assembler::loadStringAddress(RegisterID reg, const QString &string)
{
    loadPtr(Address(Assembler::ContextRegister, qOffsetOf(QV4::ExecutionContext, compilationUnit)), Assembler::ScratchRegister);
    loadPtr(Address(Assembler::ScratchRegister, qOffsetOf(QV4::CompiledData::CompilationUnit, runtimeStrings)), reg);
    const int id = _isel->registerString(string);
    return Pointer(reg, id * sizeof(QV4::StringValue));
}

void Assembler::loadStringRef(RegisterID reg, const QString &string)
{
    loadPtr(Address(Assembler::ContextRegister, qOffsetOf(QV4::ExecutionContext, compilationUnit)), reg);
    loadPtr(Address(reg, qOffsetOf(QV4::CompiledData::CompilationUnit, runtimeStrings)), reg);
    const int id = _isel->registerString(string);
    addPtr(TrustedImmPtr(id * sizeof(QV4::StringValue)), reg);
}

void Assembler::storeValue(QV4::Primitive value, IR::Temp* destination)
{
    Address addr = loadTempAddress(ScratchRegister, destination);
    storeValue(value, addr);
}

void Assembler::enterStandardStackFrame()
{
    platformEnterStandardStackFrame();

    // ### FIXME: Handle through calleeSavedRegisters mechanism
    // or eliminate StackFrameRegister altogether.
    push(StackFrameRegister);
    move(StackPointerRegister, StackFrameRegister);

    int frameSize = _stackLayout.calculateStackFrameSize();

    subPtr(TrustedImm32(frameSize), StackPointerRegister);

    for (int i = 0; i < calleeSavedRegisterCount; ++i)
        storePtr(calleeSavedRegisters[i], Address(StackFrameRegister, -(i + 1) * sizeof(void*)));

}

void Assembler::leaveStandardStackFrame()
{
    // restore the callee saved registers
    for (int i = calleeSavedRegisterCount - 1; i >= 0; --i)
        loadPtr(Address(StackFrameRegister, -(i + 1) * sizeof(void*)), calleeSavedRegisters[i]);

    int frameSize = _stackLayout.calculateStackFrameSize();
    // Work around bug in ARMv7Assembler.h where add32(imm, sp, sp) doesn't
    // work well for large immediates.
#if CPU(ARM_THUMB2)
    move(TrustedImm32(frameSize), JSC::ARMRegisters::r3);
    add32(JSC::ARMRegisters::r3, StackPointerRegister);
#else
    addPtr(TrustedImm32(frameSize), StackPointerRegister);
#endif

    pop(StackFrameRegister);
    platformLeaveStandardStackFrame();
}



#define OP(op) \
    { isel_stringIfy(op), op, 0, 0, 0 }
#define OPCONTEXT(op) \
    { isel_stringIfy(op), 0, op, 0, 0 }

#define INLINE_OP(op, memOp, immOp) \
    { isel_stringIfy(op), op, 0, memOp, immOp }
#define INLINE_OPCONTEXT(op, memOp, immOp) \
    { isel_stringIfy(op), 0, op, memOp, immOp }

#define NULL_OP \
    { 0, 0, 0, 0, 0 }

const Assembler::BinaryOperationInfo Assembler::binaryOperations[IR::LastAluOp + 1] = {
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

    INLINE_OPCONTEXT(__qmljs_add, &Assembler::inline_add32, &Assembler::inline_add32), // OpAdd
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

    OPCONTEXT(__qmljs_instanceof), // OpInstanceof
    OPCONTEXT(__qmljs_in), // OpIn

    NULL_OP, // OpAnd
    NULL_OP // OpOr
};


// Try to load the source expression into the destination FP register. This assumes that two
// general purpose (integer) registers are available: the ScratchRegister and the
// ReturnValueRegister. It returns a Jump if no conversion can be performed.
Assembler::Jump Assembler::genTryDoubleConversion(IR::Expr *src, Assembler::FPRegisterID dest)
{
    switch (src->type) {
    case IR::DoubleType:
        moveDouble(toDoubleRegister(src, dest), dest);
        return Assembler::Jump();
    case IR::SInt32Type:
        convertInt32ToDouble(toInt32Register(src, Assembler::ScratchRegister),
                                  dest);
        return Assembler::Jump();
    case IR::UInt32Type:
        convertUInt32ToDouble(toUInt32Register(src, Assembler::ScratchRegister),
                                   dest, Assembler::ReturnValueRegister);
        return Assembler::Jump();
    case IR::BoolType:
        // TODO?
        return jump();
    default:
        break;
    }

    IR::Temp *sourceTemp = src->asTemp();
    Q_ASSERT(sourceTemp);

    // It's not a number type, so it cannot be in a register.
    Q_ASSERT(sourceTemp->kind != IR::Temp::PhysicalRegister || sourceTemp->type == IR::BoolType);

    Assembler::Pointer tagAddr = loadTempAddress(Assembler::ScratchRegister, sourceTemp);
    tagAddr.offset += 4;
    load32(tagAddr, Assembler::ScratchRegister);

    // check if it's an int32:
    Assembler::Jump isNoInt = branch32(Assembler::NotEqual, Assembler::ScratchRegister,
                                            Assembler::TrustedImm32(Value::_Integer_Type));
    convertInt32ToDouble(toInt32Register(src, Assembler::ScratchRegister), dest);
    Assembler::Jump intDone = jump();

    // not an int, check if it's a double:
    isNoInt.link(this);
#if QT_POINTER_SIZE == 8
    and32(Assembler::TrustedImm32(Value::IsDouble_Mask), Assembler::ScratchRegister);
    Assembler::Jump isNoDbl = branch32(Assembler::Equal, Assembler::ScratchRegister,
                                            Assembler::TrustedImm32(0));
#else
    and32(Assembler::TrustedImm32(Value::NotDouble_Mask), Assembler::ScratchRegister);
    Assembler::Jump isNoDbl = branch32(Assembler::Equal, Assembler::ScratchRegister,
                                            Assembler::TrustedImm32(Value::NotDouble_Mask));
#endif
    toDoubleRegister(src, dest);
    intDone.link(this);

    return isNoDbl;
}

#if !defined(QT_NO_DEBUG) || defined(QT_FORCE_ASSERTS)
namespace {
inline bool isPregOrConst(IR::Expr *e)
{
    if (IR::Temp *t = e->asTemp())
        return t->kind == IR::Temp::PhysicalRegister;
    return e->asConst() != 0;
}
} // anonymous namespace
#endif

Assembler::Jump Assembler::branchDouble(bool invertCondition, IR::AluOp op,
                                                   IR::Expr *left, IR::Expr *right)
{
    Q_ASSERT(isPregOrConst(left));
    Q_ASSERT(isPregOrConst(right));
    Q_ASSERT(left->asConst() == 0 || right->asConst() == 0);

    Assembler::DoubleCondition cond;
    switch (op) {
    case IR::OpGt: cond = Assembler::DoubleGreaterThan; break;
    case IR::OpLt: cond = Assembler::DoubleLessThan; break;
    case IR::OpGe: cond = Assembler::DoubleGreaterThanOrEqual; break;
    case IR::OpLe: cond = Assembler::DoubleLessThanOrEqual; break;
    case IR::OpEqual:
    case IR::OpStrictEqual: cond = Assembler::DoubleEqual; break;
    case IR::OpNotEqual:
    case IR::OpStrictNotEqual: cond = Assembler::DoubleNotEqualOrUnordered; break; // No, the inversion of DoubleEqual is NOT DoubleNotEqual.
    default:
        Q_UNREACHABLE();
    }
    if (invertCondition)
        cond = JSC::MacroAssembler::invert(cond);

    return JSC::MacroAssembler::branchDouble(cond, toDoubleRegister(left), toDoubleRegister(right));
}


#endif
