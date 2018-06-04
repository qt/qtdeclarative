/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QBuffer>
#include <QFile>

#include "qv4engine_p.h"
#include "qv4assembler_p.h"
#include <private/qv4function_p.h>
#include <private/qv4runtime_p.h>

#include <wtf/Vector.h>
#include <assembler/MacroAssembler.h>
#include <assembler/MacroAssemblerCodeRef.h>
#include <assembler/LinkBuffer.h>
#include <WTFStubs.h>

#undef ENABLE_ALL_ASSEMBLERS_FOR_REFACTORING_PURPOSES

#ifdef V4_ENABLE_JIT

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace JIT {

#define callHelper(x) PlatformAssemblerCommon::callRuntime(#x, reinterpret_cast<void *>(&x))

const QV4::Value::ValueTypeInternal IntegerTag = QV4::Value::ValueTypeInternal::Integer;

static ReturnedValue toNumberHelper(ReturnedValue v)
{
    return Encode(Value::fromReturnedValue(v).toNumber());
}

static ReturnedValue toInt32Helper(ReturnedValue v)
{
    return Encode(Value::fromReturnedValue(v).toInt32());
}

#if defined(Q_PROCESSOR_X86_64) || defined(ENABLE_ALL_ASSEMBLERS_FOR_REFACTORING_PURPOSES)
#if defined(Q_OS_LINUX) || defined(Q_OS_QNX) || defined(Q_OS_FREEBSD) || defined(Q_OS_DARWIN)

struct PlatformAssembler_X86_64_SysV : JSC::MacroAssembler<JSC::MacroAssemblerX86_64>
{
    static const RegisterID NoRegister = RegisterID(-1);

    static const RegisterID ReturnValueRegister   = RegisterID::eax;
    static const RegisterID ReturnValueRegisterValue = ReturnValueRegister;
    static const RegisterID AccumulatorRegister   = RegisterID::eax;
    static const RegisterID AccumulatorRegisterValue = AccumulatorRegister;
    static const RegisterID ScratchRegister       = RegisterID::r10;
    static const RegisterID ScratchRegister2      = RegisterID::r9; // Note: overlaps with Arg5Reg, so do not use while setting up a call!
    static const RegisterID JSStackFrameRegister  = RegisterID::r12;
    static const RegisterID CppStackFrameRegister = RegisterID::r13;
    static const RegisterID EngineRegister        = RegisterID::r14;
    static const RegisterID StackPointerRegister  = RegisterID::esp;
    static const RegisterID FramePointerRegister  = RegisterID::ebp;
    static const FPRegisterID FPScratchRegister   = FPRegisterID::xmm1;

    static const RegisterID Arg0Reg = RegisterID::edi;
    static const RegisterID Arg1Reg = RegisterID::esi;
    static const RegisterID Arg2Reg = RegisterID::edx;
    static const RegisterID Arg3Reg = RegisterID::ecx;
    static const RegisterID Arg4Reg = RegisterID::r8;
    static const RegisterID Arg5Reg = RegisterID::r9;
    static const RegisterID Arg6Reg = NoRegister;
    static const RegisterID Arg7Reg = NoRegister;
    static const int ArgInRegCount = 6;

    void popValue()
    {
        addPtr(TrustedImmPtr(sizeof(ReturnedValue)), StackPointerRegister);
    }

    void generatePlatformFunctionEntry()
    {
        push(RegisterID::ebp);
        move(RegisterID::esp, RegisterID::ebp);
        move(TrustedImmPtr(nullptr), AccumulatorRegister); push(AccumulatorRegister); // exceptionHandler
        push(JSStackFrameRegister);
        push(CppStackFrameRegister);
        push(EngineRegister);
        move(Arg0Reg, CppStackFrameRegister);
        move(Arg1Reg, EngineRegister);
    }

    void generatePlatformFunctionExit()
    {
        pop(EngineRegister);
        pop(CppStackFrameRegister);
        pop(JSStackFrameRegister);
        pop(); // exceptionHandler
        pop(RegisterID::ebp);
        ret();
    }

    void callAbsolute(const void *funcPtr)
    {
        move(TrustedImmPtr(funcPtr), ScratchRegister);
        call(ScratchRegister);
    }

    void pushAligned(RegisterID reg)
    {
        subPtr(TrustedImm32(PointerSize), StackPointerRegister);
        push(reg);
    }

    void popAligned(RegisterID reg)
    {
        pop(reg);
        addPtr(TrustedImm32(PointerSize), StackPointerRegister);
    }
};

typedef PlatformAssembler_X86_64_SysV PlatformAssemblerBase;

#endif
#if defined(Q_OS_WIN)

struct PlatformAssembler_Win64 : JSC::MacroAssembler<JSC::MacroAssemblerX86_64>
{
    static const RegisterID NoRegister = RegisterID(-1);

    static const RegisterID ReturnValueRegister   = RegisterID::eax;
    static const RegisterID ReturnValueRegisterValue = ReturnValueRegister;
    static const RegisterID AccumulatorRegister   = RegisterID::eax;
    static const RegisterID AccumulatorRegisterValue = AccumulatorRegister;
    static const RegisterID ScratchRegister       = RegisterID::r10;
    static const RegisterID ScratchRegister2      = RegisterID::r9; // Note: overlaps with Arg3Reg, so do not use while setting up a call!
    static const RegisterID JSStackFrameRegister  = RegisterID::r12;
    static const RegisterID CppStackFrameRegister = RegisterID::r13;
    static const RegisterID EngineRegister        = RegisterID::r14;
    static const RegisterID StackPointerRegister  = RegisterID::esp;
    static const RegisterID FramePointerRegister  = RegisterID::ebp;
    static const FPRegisterID FPScratchRegister   = FPRegisterID::xmm1;

    static const RegisterID Arg0Reg = RegisterID::ecx;
    static const RegisterID Arg1Reg = RegisterID::edx;
    static const RegisterID Arg2Reg = RegisterID::r8;
    static const RegisterID Arg3Reg = RegisterID::r9;
    static const RegisterID Arg4Reg = NoRegister;
    static const RegisterID Arg5Reg = NoRegister;
    static const RegisterID Arg6Reg = NoRegister;
    static const RegisterID Arg7Reg = NoRegister;
    static const int ArgInRegCount = 4;

    void popValue()
    {
        addPtr(TrustedImmPtr(sizeof(ReturnedValue)), StackPointerRegister);
    }

    void generatePlatformFunctionEntry()
    {
        push(RegisterID::ebp);
        move(RegisterID::esp, RegisterID::ebp);
        move(TrustedImmPtr(nullptr), AccumulatorRegister); push(AccumulatorRegister); // exceptionHandler
        push(JSStackFrameRegister);
        push(CppStackFrameRegister);
        push(EngineRegister);
        move(Arg0Reg, CppStackFrameRegister);
        move(Arg1Reg, EngineRegister);
    }

    void generatePlatformFunctionExit()
    {
        pop(EngineRegister);
        pop(CppStackFrameRegister);
        pop(JSStackFrameRegister);
        pop(); // exceptionHandler
        pop(RegisterID::ebp);
        ret();
    }

    void callAbsolute(const void *funcPtr)
    {
        move(TrustedImmPtr(funcPtr), ScratchRegister);
        subPtr(TrustedImm32(4 * PointerSize), StackPointerRegister);
        call(ScratchRegister);
        addPtr(TrustedImm32(4 * PointerSize), StackPointerRegister);
    }

    void pushAligned(RegisterID reg)
    {
        subPtr(TrustedImm32(PointerSize), StackPointerRegister);
        push(reg);
    }

    void popAligned(RegisterID reg)
    {
        pop(reg);
        addPtr(TrustedImm32(PointerSize), StackPointerRegister);
    }
};

typedef PlatformAssembler_Win64 PlatformAssemblerBase;

#endif
#endif

#if (defined(Q_PROCESSOR_X86) && !defined(Q_PROCESSOR_X86_64)) || defined(ENABLE_ALL_ASSEMBLERS_FOR_REFACTORING_PURPOSES)

struct PlatformAssembler_X86_All : JSC::MacroAssembler<JSC::MacroAssemblerX86>
{
    static const RegisterID NoRegister = RegisterID(-1);

    static const RegisterID ReturnValueRegisterValue = RegisterID::eax;
    static const RegisterID ReturnValueRegisterTag   = RegisterID::edx;
    static const RegisterID ScratchRegister          = RegisterID::ecx;
    static const RegisterID AccumulatorRegisterValue = ReturnValueRegisterValue;
    static const RegisterID AccumulatorRegisterTag   = ReturnValueRegisterTag;
    static const RegisterID JSStackFrameRegister  = RegisterID::ebx;
    static const RegisterID CppStackFrameRegister = RegisterID::esi;
    static const RegisterID EngineRegister        = RegisterID::edi;
    static const RegisterID StackPointerRegister  = RegisterID::esp;
    static const RegisterID FramePointerRegister  = RegisterID::ebp;
    static const FPRegisterID FPScratchRegister   = FPRegisterID::xmm1;

    static const RegisterID Arg0Reg = NoRegister;
    static const RegisterID Arg1Reg = NoRegister;
    static const RegisterID Arg2Reg = NoRegister;
    static const RegisterID Arg3Reg = NoRegister;
    static const RegisterID Arg4Reg = NoRegister;
    static const RegisterID Arg5Reg = NoRegister;
    static const RegisterID Arg6Reg = NoRegister;
    static const RegisterID Arg7Reg = NoRegister;
    static const int ArgInRegCount = 0;

    void popValue()
    {
        addPtr(TrustedImmPtr(sizeof(ReturnedValue)), StackPointerRegister);
    }

    void generatePlatformFunctionEntry()
    {
        push(RegisterID::ebp);
        move(RegisterID::esp, RegisterID::ebp);
        move(TrustedImmPtr(nullptr), AccumulatorRegisterValue); push(AccumulatorRegisterValue); // exceptionHandler
        push(JSStackFrameRegister);
        push(CppStackFrameRegister);
        push(EngineRegister);
        // Ensure the stack is 16-byte aligned in order for compiler generated aligned SSE2
        // instructions to be able to target the stack.
        subPtr(TrustedImm32(8), StackPointerRegister);
        loadPtr(Address(FramePointerRegister, 2 * PointerSize), CppStackFrameRegister);
        loadPtr(Address(FramePointerRegister, 3 * PointerSize), EngineRegister);
    }

    void generatePlatformFunctionExit()
    {
        addPtr(TrustedImm32(8), StackPointerRegister);
        pop(EngineRegister);
        pop(CppStackFrameRegister);
        pop(JSStackFrameRegister);
        pop(); // exceptionHandler
        pop(RegisterID::ebp);
        ret();
    }

    void callAbsolute(const void *funcPtr)
    {
        move(TrustedImmPtr(funcPtr), ScratchRegister);
        call(ScratchRegister);
    }

    void pushAligned(RegisterID reg)
    {
        subPtr(TrustedImm32(PointerSize), StackPointerRegister);
        push(reg);
    }

    void popAligned(RegisterID reg)
    {
        pop(reg);
        addPtr(TrustedImm32(PointerSize), StackPointerRegister);
    }
};

typedef PlatformAssembler_X86_All PlatformAssemblerBase;

#endif

#if defined(Q_PROCESSOR_ARM_64) || defined(ENABLE_ALL_ASSEMBLERS_FOR_REFACTORING_PURPOSES)

struct PlatformAssembler_ARM64 : JSC::MacroAssembler<JSC::MacroAssemblerARM64>
{
    static const RegisterID NoRegister = RegisterID(-1);

    static const RegisterID ReturnValueRegister   = JSC::ARM64Registers::x0;
    static const RegisterID ReturnValueRegisterValue = ReturnValueRegister;
    static const RegisterID AccumulatorRegister   = JSC::ARM64Registers::x9;
    static const RegisterID AccumulatorRegisterValue = AccumulatorRegister;
    static const RegisterID ScratchRegister       = JSC::ARM64Registers::x10;
    static const RegisterID ScratchRegister2      = JSC::ARM64Registers::x7; // Note: overlaps with Arg7Reg, so do not use while setting up a call!
    static const RegisterID JSStackFrameRegister  = JSC::ARM64Registers::x19;
    static const RegisterID CppStackFrameRegister = JSC::ARM64Registers::x20;
    static const RegisterID EngineRegister        = JSC::ARM64Registers::x21;
    static const RegisterID StackPointerRegister  = JSC::ARM64Registers::sp;
    static const RegisterID FramePointerRegister  = JSC::ARM64Registers::fp;
    static const FPRegisterID FPScratchRegister   = JSC::ARM64Registers::q1;

    static const RegisterID Arg0Reg = JSC::ARM64Registers::x0;
    static const RegisterID Arg1Reg = JSC::ARM64Registers::x1;
    static const RegisterID Arg2Reg = JSC::ARM64Registers::x2;
    static const RegisterID Arg3Reg = JSC::ARM64Registers::x3;
    static const RegisterID Arg4Reg = JSC::ARM64Registers::x4;
    static const RegisterID Arg5Reg = JSC::ARM64Registers::x5;
    static const RegisterID Arg6Reg = JSC::ARM64Registers::x6;
    static const RegisterID Arg7Reg = JSC::ARM64Registers::x7;
    static const int ArgInRegCount = 8;

    void push(RegisterID src)
    {
        pushToSave(src);
    }

    void pop(RegisterID dest)
    {
        popToRestore(dest);
    }

    void pop()
    {
        add64(TrustedImm32(16), stackPointerRegister);
    }

    void popValue()
    {
        pop();
    }

    void generatePlatformFunctionEntry()
    {
        pushPair(JSC::ARM64Registers::fp, JSC::ARM64Registers::lr);
        move(RegisterID::sp, RegisterID::fp);
        move(TrustedImmPtr(nullptr), AccumulatorRegister); // exceptionHandler
        pushPair(JSStackFrameRegister, AccumulatorRegister);
        pushPair(EngineRegister, CppStackFrameRegister);
        move(Arg0Reg, CppStackFrameRegister);
        move(Arg1Reg, EngineRegister);
    }

    void generatePlatformFunctionExit()
    {
        move(AccumulatorRegister, ReturnValueRegister);
        popPair(EngineRegister, CppStackFrameRegister);
        popPair(JSStackFrameRegister, AccumulatorRegister);
        popPair(JSC::ARM64Registers::fp, JSC::ARM64Registers::lr);
        ret();
    }

    void callAbsolute(const void *funcPtr)
    {
        move(TrustedImmPtr(funcPtr), ScratchRegister);
        call(ScratchRegister);
    }

    void pushAligned(RegisterID reg)
    {
        pushToSave(reg);
    }

    void popAligned(RegisterID reg)
    {
        popToRestore(reg);
    }
};

typedef PlatformAssembler_ARM64 PlatformAssemblerBase;

#endif

#if defined(Q_PROCESSOR_ARM_32) || defined(ENABLE_ALL_ASSEMBLERS_FOR_REFACTORING_PURPOSES)

struct PlatformAssembler_ARM32 : JSC::MacroAssembler<JSC::MacroAssemblerARMv7>
{
    static const RegisterID NoRegister = RegisterID(-1);

    static const RegisterID ReturnValueRegisterValue = JSC::ARMRegisters::r0;
    static const RegisterID ReturnValueRegisterTag   = JSC::ARMRegisters::r1;
    static const RegisterID ScratchRegister          = JSC::ARMRegisters::r2;
    static const RegisterID AccumulatorRegisterValue = JSC::ARMRegisters::r4;
    static const RegisterID AccumulatorRegisterTag   = JSC::ARMRegisters::r5;
    // r6 is used by MacroAssemblerARMv7
    static const RegisterID JSStackFrameRegister     = JSC::ARMRegisters::r8;
    static const RegisterID CppStackFrameRegister    = JSC::ARMRegisters::r10;
#if CPU(ARM_THUMB2) || defined(V4_BOOTSTRAP)
    static const RegisterID FramePointerRegister     = JSC::ARMRegisters::r7;
    static const RegisterID EngineRegister           = JSC::ARMRegisters::r11;
#else // Thumbs down
    static const RegisterID FramePointerRegister     = JSC::ARMRegisters::r11;
    static const RegisterID EngineRegister           = JSC::ARMRegisters::r7;
#endif
    static const RegisterID StackPointerRegister     = JSC::ARMRegisters::r13;
    static const FPRegisterID FPScratchRegister      = JSC::ARMRegisters::d1;

    static const RegisterID Arg0Reg = JSC::ARMRegisters::r0;
    static const RegisterID Arg1Reg = JSC::ARMRegisters::r1;
    static const RegisterID Arg2Reg = JSC::ARMRegisters::r2;
    static const RegisterID Arg3Reg = JSC::ARMRegisters::r3;
    static const RegisterID Arg4Reg = NoRegister;
    static const RegisterID Arg5Reg = NoRegister;
    static const RegisterID Arg6Reg = NoRegister;
    static const RegisterID Arg7Reg = NoRegister;
    static const int ArgInRegCount = 4;

    void popValue()
    {
        addPtr(TrustedImm32(sizeof(ReturnedValue)), StackPointerRegister);
    }

    void generatePlatformFunctionEntry()
    {
        push(JSC::ARMRegisters::lr);
        push(FramePointerRegister);
        move(StackPointerRegister, FramePointerRegister);
        push(TrustedImm32(0)); // exceptionHandler
        push(AccumulatorRegisterValue);
        push(AccumulatorRegisterTag);
        push(addressTempRegister);
        push(JSStackFrameRegister);
        push(CppStackFrameRegister);
        push(EngineRegister);
        subPtr(TrustedImm32(4), StackPointerRegister); // stack alignment
        move(Arg0Reg, CppStackFrameRegister);
        move(Arg1Reg, EngineRegister);
    }

    void generatePlatformFunctionExit()
    {
        move(AccumulatorRegisterValue, ReturnValueRegisterValue);
        move(AccumulatorRegisterTag, ReturnValueRegisterTag);
        addPtr(TrustedImm32(4), StackPointerRegister); // stack alignment
        pop(EngineRegister);
        pop(CppStackFrameRegister);
        pop(JSStackFrameRegister);
        pop(addressTempRegister);
        pop(AccumulatorRegisterTag);
        pop(AccumulatorRegisterValue);
        pop(); // exceptionHandler
        pop(FramePointerRegister);
        pop(JSC::ARMRegisters::lr);
        ret();
    }

    void callAbsolute(const void *funcPtr)
    {
        move(TrustedImmPtr(funcPtr), dataTempRegister);
        call(dataTempRegister);
    }

    void pushAligned(RegisterID reg)
    {
        subPtr(TrustedImm32(PointerSize), StackPointerRegister);
        push(reg);
    }

    void popAligned(RegisterID reg)
    {
        pop(reg);
        addPtr(TrustedImm32(PointerSize), StackPointerRegister);
    }
};

typedef PlatformAssembler_ARM32 PlatformAssemblerBase;

#endif

struct PlatformAssemblerCommon : PlatformAssemblerBase
{
    const Value* constantTable;
    struct JumpTarget { JSC::MacroAssemblerBase::Jump jump; int offset; };
    std::vector<JumpTarget> patches;
    struct ExceptionHanlderTarget { JSC::MacroAssemblerBase::DataLabelPtr label; int offset; };
    std::vector<ExceptionHanlderTarget> ehTargets;
    QHash<int, JSC::MacroAssemblerBase::Label> labelsByOffset;
    QHash<const void *, const char *> functions;
    std::vector<Jump> catchyJumps;
    Label functionExit;

    Address exceptionHandlerAddress() const
    {
        return Address(FramePointerRegister, -1 * PointerSize);
    }

    Address contextAddress() const
    {
        return Address(JSStackFrameRegister, offsetof(CallData, context));
    }

    RegisterID registerForArg(int arg) const
    {
        Q_ASSERT(arg >= 0);
        Q_ASSERT(arg < ArgInRegCount);
        switch (arg) {
        case 0: return Arg0Reg;
        case 1: return Arg1Reg;
        case 2: return Arg2Reg;
        case 3: return Arg3Reg;
        case 4: return Arg4Reg;
        case 5: return Arg5Reg;
        case 6: return Arg6Reg;
        case 7: return Arg7Reg;
        default:
            Q_UNIMPLEMENTED();
            Q_UNREACHABLE();
        }
    }

    void callRuntime(const char *functionName, const void *funcPtr)
    {
        functions.insert(funcPtr, functionName);
        callAbsolute(funcPtr);
    }

    Address loadFunctionPtr(RegisterID target)
    {
        Address addr(CppStackFrameRegister, offsetof(CppStackFrame, v4Function));
        loadPtr(addr, target);
        return Address(target);
    }

    Address loadCompilationUnitPtr(RegisterID target)
    {
        Address addr = loadFunctionPtr(target);
        addr.offset = offsetof(QV4::Function, compilationUnit);
        loadPtr(addr, target);
        return Address(target);
    }

    Address loadConstAddress(int constIndex, RegisterID baseReg = ScratchRegister)
    {
        Address addr = loadCompilationUnitPtr(baseReg);
        addr.offset = offsetof(QV4::CompiledData::CompilationUnitBase, constants);
        loadPtr(addr, baseReg);
        addr.offset = constIndex * int(sizeof(QV4::Value));
        return addr;
    }

    Address loadStringAddress(int stringId)
    {
        Address addr = loadCompilationUnitPtr(ScratchRegister);
        addr.offset = offsetof(QV4::CompiledData::CompilationUnitBase, runtimeStrings);
        loadPtr(addr, ScratchRegister);
        return Address(ScratchRegister, stringId * PointerSize);
    }

    void passAsArg(RegisterID src, int arg)
    {
        move(src, registerForArg(arg));
    }

    void generateCatchTrampoline(std::function<void()> loadUndefined)
    {
        for (Jump j : catchyJumps)
            j.link(this);

        loadPtr(exceptionHandlerAddress(), ScratchRegister);
        Jump exitFunction = branchPtr(Equal, ScratchRegister, TrustedImmPtr(0));
        jump(ScratchRegister);
        exitFunction.link(this);
        loadUndefined();

        if (functionExit.isSet())
            jump(functionExit);
        else
            generateFunctionExit();
    }

    void addCatchyJump(Jump j)
    {
        Q_ASSERT(j.isSet());
        catchyJumps.push_back(j);
    }

    void generateFunctionEntry()
    {
        generatePlatformFunctionEntry();
        loadPtr(Address(CppStackFrameRegister, offsetof(CppStackFrame, jsFrame)), JSStackFrameRegister);
    }

    void generateFunctionExit()
    {
        if (functionExit.isSet()) {
            jump(functionExit);
            return;
        }

        functionExit = label();
        generatePlatformFunctionExit();
    }
};

#if QT_POINTER_SIZE == 8 || defined(ENABLE_ALL_ASSEMBLERS_FOR_REFACTORING_PURPOSES)
struct PlatformAssembler64 : PlatformAssemblerCommon
{
    void callRuntime(const char *functionName, const void *funcPtr,
                     Assembler::CallResultDestination dest)
    {
        PlatformAssemblerCommon::callRuntime(functionName, funcPtr);
        if (dest == Assembler::ResultInAccumulator)
            saveReturnValueInAccumulator();
    }

    void saveReturnValueInAccumulator()
    {
        move(ReturnValueRegister, AccumulatorRegister);
    }

    void loadUndefined(RegisterID dest = AccumulatorRegister)
    {
        move(TrustedImm64(0), dest);
    }

    void copyConst(int constIndex, Address dest)
    {
        //###
        if (constantTable[constIndex].isUndefined()) {
            loadUndefined(ScratchRegister);
        } else {
            load64(loadConstAddress(constIndex, ScratchRegister), ScratchRegister);
        }
        store64(ScratchRegister, dest);
    }

    void copyReg(Address src, Address dst)
    {
        load64(src, ScratchRegister);
        store64(ScratchRegister, dst);
    }

    void loadPointerFromValue(Address addr, RegisterID dest = AccumulatorRegister)
    {
        load64(addr, dest);
    }

    void loadAccumulator(Address addr)
    {
        load64(addr, AccumulatorRegister);
    }

    void storeAccumulator(Address addr)
    {
        store64(AccumulatorRegister, addr);
    }

    void loadString(int stringId)
    {
        loadAccumulator(loadStringAddress(stringId));
    }

    void loadValue(ReturnedValue value)
    {
        move(TrustedImm64(value), AccumulatorRegister);
    }

    void storeHeapObject(RegisterID source, Address addr)
    {
        store64(source, addr);
    }

    void generateCatchTrampoline()
    {
        PlatformAssemblerCommon::generateCatchTrampoline([this](){loadUndefined();});
    }

    void toBoolean(std::function<void(RegisterID)> continuation)
    {
        urshift64(AccumulatorRegister, TrustedImm32(Value::IsIntegerConvertible_Shift), ScratchRegister);
        auto needsConversion = branch32(NotEqual, TrustedImm32(1), ScratchRegister);
        continuation(AccumulatorRegister);
        Jump done = jump();

        // slow path:
        needsConversion.link(this);
        push(AccumulatorRegister);
        move(AccumulatorRegister, registerForArg(0));
        callHelper(Value::toBooleanImpl);
        and32(TrustedImm32(1), ReturnValueRegister, ScratchRegister);
        pop(AccumulatorRegister);
        continuation(ScratchRegister);

        done.link(this);
    }

    void toNumber()
    {
        urshift64(AccumulatorRegister, TrustedImm32(Value::QuickType_Shift), ScratchRegister);
        auto isNumber = branch32(GreaterThanOrEqual, ScratchRegister, TrustedImm32(Value::QT_Int));

        move(AccumulatorRegister, registerForArg(0));
        callHelper(toNumberHelper);
        saveReturnValueInAccumulator();

        isNumber.link(this);
    }

    void toInt32LhsAcc(Address lhs, RegisterID lhsTarget)
    {
        load64(lhs, lhsTarget);
        urshift64(lhsTarget, TrustedImm32(Value::QuickType_Shift), ScratchRegister2);
        auto lhsIsInt = branch32(Equal, TrustedImm32(Value::QT_Int), ScratchRegister2);

        pushAligned(AccumulatorRegister);
        move(lhsTarget, registerForArg(0));
        callHelper(toInt32Helper);
        move(ReturnValueRegister, lhsTarget);
        popAligned(AccumulatorRegister);

        lhsIsInt.link(this);
        urshift64(AccumulatorRegister, TrustedImm32(Value::QuickType_Shift), ScratchRegister2);
        auto isInt = branch32(Equal, TrustedImm32(Value::QT_Int), ScratchRegister2);

        pushAligned(lhsTarget);
        move(AccumulatorRegister, registerForArg(0));
        callHelper(toInt32Helper);
        saveReturnValueInAccumulator();
        popAligned(lhsTarget);

        isInt.link(this);
    }

    void toInt32()
    {
        urshift64(AccumulatorRegister, TrustedImm32(Value::QuickType_Shift), ScratchRegister2);
        auto isInt = branch32(Equal, TrustedImm32(Value::QT_Int), ScratchRegister2);

        move(AccumulatorRegister, registerForArg(0));
        callRuntime("toInt32Helper", reinterpret_cast<void *>(&toInt32Helper),
                    Assembler::ResultInAccumulator);

        isInt.link(this);
    }

    void regToInt32(Address srcReg, RegisterID targetReg)
    {
        load64(srcReg, targetReg);
        urshift64(targetReg, TrustedImm32(Value::QuickType_Shift), ScratchRegister2);
        auto isInt = branch32(Equal, TrustedImm32(Value::QT_Int), ScratchRegister2);

        pushAligned(AccumulatorRegister);
        move(targetReg, registerForArg(0));
        callHelper(toInt32Helper);
        move(ReturnValueRegister, targetReg);
        popAligned(AccumulatorRegister);

        isInt.link(this);
    }

    void isNullOrUndefined()
    {
        move(AccumulatorRegister, ScratchRegister);
        compare64(Equal, ScratchRegister, TrustedImm32(0), AccumulatorRegister);
        Jump isUndef = branch32(NotEqual, TrustedImm32(0), AccumulatorRegister);

        // not undefined
        rshift64(TrustedImm32(32), ScratchRegister);
        compare32(Equal, ScratchRegister, TrustedImm32(int(QV4::Value::ValueTypeInternal::Null)),
                  AccumulatorRegister);

        isUndef.link(this);
    }

    Jump isIntOrBool()
    {
        urshift64(AccumulatorRegister, TrustedImm32(Value::IsIntegerOrBool_Shift), ScratchRegister);
        return branch32(Equal, TrustedImm32(3), ScratchRegister);
    }

    void jumpStrictEqualStackSlotInt(int lhs, int rhs, int offset)
    {
        Address lhsAddr(JSStackFrameRegister, lhs * int(sizeof(Value)));
        load64(lhsAddr, ScratchRegister);
        Jump isUndef = branch64(Equal, ScratchRegister, TrustedImm64(0));
        Jump equal = branch32(Equal, TrustedImm32(rhs), ScratchRegister);
        patches.push_back({ equal, offset });
        isUndef.link(this);
    }

    void jumpStrictNotEqualStackSlotInt(int lhs, int rhs, int offset)
    {
        Address lhsAddr(JSStackFrameRegister, lhs * int(sizeof(Value)));
        load64(lhsAddr, ScratchRegister);
        Jump isUndef = branch64(Equal, ScratchRegister, TrustedImm64(0));
        patches.push_back({ isUndef, offset });
        Jump notEqual = branch32(NotEqual, TrustedImm32(rhs), ScratchRegister);
        patches.push_back({ notEqual, offset });
    }

    void setAccumulatorTag(QV4::Value::ValueTypeInternal tag, RegisterID sourceReg = NoRegister)
    {
        if (sourceReg == NoRegister)
            or64(TrustedImm64(int64_t(tag) << 32), AccumulatorRegister);
        else
            or64(TrustedImm64(int64_t(tag) << 32), sourceReg, AccumulatorRegister);
    }

    void encodeDoubleIntoAccumulator(FPRegisterID src)
    {
        moveDoubleTo64(src, AccumulatorRegister);
        move(TrustedImm64(Value::NaNEncodeMask), ScratchRegister);
        xor64(ScratchRegister, AccumulatorRegister);
    }

    void pushValueAligned(ReturnedValue v)
    {
        loadValue(v);
        pushAligned(AccumulatorRegister);
    }

    void popValueAligned()
    {
        addPtr(TrustedImm32(2 * PointerSize), StackPointerRegister);
    }

    Jump binopBothIntPath(Address lhsAddr, std::function<Jump(void)> fastPath)
    {
        urshift64(AccumulatorRegister, TrustedImm32(32), ScratchRegister);
        Jump accNotInt = branch32(NotEqual, TrustedImm32(int(IntegerTag)), ScratchRegister);
        load64(lhsAddr, ScratchRegister);
        urshift64(ScratchRegister, TrustedImm32(32), ScratchRegister2);
        Jump lhsNotInt = branch32(NotEqual, TrustedImm32(int(IntegerTag)), ScratchRegister2);

        // both integer
        Jump failure = fastPath();
        Jump done = jump();

        // all other cases
        if (failure.isSet())
            failure.link(this);
        accNotInt.link(this);
        lhsNotInt.link(this);

        return done;
    }

    Jump unopIntPath(std::function<Jump(void)> fastPath)
    {
        urshift64(AccumulatorRegister, TrustedImm32(Value::IsIntegerConvertible_Shift), ScratchRegister);
        Jump accNotIntConvertible = branch32(NotEqual, TrustedImm32(1), ScratchRegister);

        // both integer
        Jump failure = fastPath();
        Jump done = jump();

        // all other cases
        if (failure.isSet())
            failure.link(this);
        accNotIntConvertible.link(this);

        return done;
    }

    void callWithAccumulatorByValueAsFirstArgument(std::function<void()> doCall)
    {
        passAsArg(AccumulatorRegister, 0);
        doCall();
    }
};

typedef PlatformAssembler64 PlatformAssembler;
#endif

#if QT_POINTER_SIZE == 4 || defined(ENABLE_ALL_ASSEMBLERS_FOR_REFACTORING_PURPOSES)
struct PlatformAssembler32 : PlatformAssemblerCommon
{
    void callRuntime(const char *functionName, const void *funcPtr,
                     Assembler::CallResultDestination dest)
    {
        PlatformAssemblerCommon::callRuntime(functionName, funcPtr);
        if (dest == Assembler::ResultInAccumulator)
            saveReturnValueInAccumulator();
    }

    void saveReturnValueInAccumulator()
    {
        move(ReturnValueRegisterValue, AccumulatorRegisterValue);
        move(ReturnValueRegisterTag, AccumulatorRegisterTag);
    }

    void loadUndefined()
    {
        move(TrustedImm32(0), AccumulatorRegisterValue);
        move(TrustedImm32(0), AccumulatorRegisterTag);
    }

    void copyConst(int constIndex, Address destRegAddr)
    {
        //###
        if (constantTable[constIndex].isUndefined()) {
            move(TrustedImm32(0), ScratchRegister);
            store32(ScratchRegister, destRegAddr);
            destRegAddr.offset += 4;
            store32(ScratchRegister, destRegAddr);
        } else {
            Address src = loadConstAddress(constIndex);
            loadDouble(src, FPScratchRegister);
            storeDouble(FPScratchRegister, destRegAddr);
        }
    }

    void copyReg(Address src, Address dest)
    {
        loadDouble(src, FPScratchRegister);
        storeDouble(FPScratchRegister, dest);
    }

    void loadPointerFromValue(Address addr, RegisterID dest = AccumulatorRegisterValue)
    {
        load32(addr, dest);
    }

    void loadAccumulator(Address src)
    {
        load32(src, AccumulatorRegisterValue);
        src.offset += 4;
        load32(src, AccumulatorRegisterTag);
    }

    void storeAccumulator(Address addr)
    {
        store32(AccumulatorRegisterValue, addr);
        addr.offset += 4;
        store32(AccumulatorRegisterTag, addr);
    }

    void loadString(int stringId)
    {
        load32(loadStringAddress(stringId), AccumulatorRegisterValue);
        move(TrustedImm32(0), AccumulatorRegisterTag);
    }

    void loadValue(ReturnedValue value)
    {
        move(TrustedImm32(Value::fromReturnedValue(value).value()), AccumulatorRegisterValue);
        move(TrustedImm32(Value::fromReturnedValue(value).tag()), AccumulatorRegisterTag);
    }

    void storeHeapObject(RegisterID source, Address addr)
    {
        store32(source, addr);
        addr.offset += 4;
        store32(TrustedImm32(0), addr);
    }


    void generateCatchTrampoline()
    {
        PlatformAssemblerCommon::generateCatchTrampoline([this](){loadUndefined();});
    }

    void toNumber()
    {
        urshift32(AccumulatorRegisterTag, TrustedImm32(Value::QuickType_Shift - 32), ScratchRegister);
        auto isNumber = branch32(GreaterThanOrEqual, ScratchRegister, TrustedImm32(Value::QT_Int));

        if (ArgInRegCount < 2) {
            push(AccumulatorRegisterTag);
            push(AccumulatorRegisterValue);
        } else {
            move(AccumulatorRegisterValue, registerForArg(0));
            move(AccumulatorRegisterTag, registerForArg(1));
        }
        callRuntime("toNumberHelper", reinterpret_cast<void *>(&toNumberHelper),
                    Assembler::ResultInAccumulator);
        saveReturnValueInAccumulator();
        if (ArgInRegCount < 2)
            addPtr(TrustedImm32(2 * PointerSize), StackPointerRegister);

        isNumber.link(this);
    }

    void toInt32LhsAcc(Address lhs, RegisterID lhsTarget)
    {
        bool accumulatorNeedsSaving = AccumulatorRegisterValue == ReturnValueRegisterValue
                || AccumulatorRegisterTag == ReturnValueRegisterTag;
        lhs.offset += 4;
        load32(lhs, lhsTarget);
        lhs.offset -= 4;
        auto lhsIsNotInt = branch32(NotEqual, TrustedImm32(int(IntegerTag)), lhsTarget);
        load32(lhs, lhsTarget);
        auto lhsIsInt = jump();

        lhsIsNotInt.link(this);
        if (accumulatorNeedsSaving) {
            push(AccumulatorRegisterTag);
            push(AccumulatorRegisterValue);
        }
        if (ArgInRegCount < 2) {
            push(lhsTarget);
            load32(lhs, lhsTarget);
            push(lhsTarget);
        } else {
            move(lhsTarget, registerForArg(1));
            load32(lhs, registerForArg(0));
        }
        callHelper(toInt32Helper);
        move(ReturnValueRegisterValue, lhsTarget);
        if (ArgInRegCount < 2)
            addPtr(TrustedImm32(2 * PointerSize), StackPointerRegister);
        if (accumulatorNeedsSaving) {
            pop(AccumulatorRegisterValue);
            pop(AccumulatorRegisterTag);
        }
        lhsIsInt.link(this);

        auto rhsIsInt = branch32(Equal, TrustedImm32(int(IntegerTag)), AccumulatorRegisterTag);

        pushAligned(lhsTarget);
        if (ArgInRegCount < 2) {
            push(AccumulatorRegisterTag);
            push(AccumulatorRegisterValue);
        } else {
            move(AccumulatorRegisterValue, registerForArg(0));
            move(AccumulatorRegisterTag, registerForArg(1));
        }
        callRuntime("toInt32Helper", reinterpret_cast<void *>(&toInt32Helper),
                    Assembler::ResultInAccumulator);
        if (ArgInRegCount < 2)
            addPtr(TrustedImm32(2 * PointerSize), StackPointerRegister);
        popAligned(lhsTarget);

        rhsIsInt.link(this);
    }

    void toInt32()
    {
        urshift32(AccumulatorRegisterTag, TrustedImm32(Value::QuickType_Shift - 32), ScratchRegister);
        auto isInt = branch32(Equal, TrustedImm32(Value::QT_Int), ScratchRegister);

        if (ArgInRegCount < 2) {
            push(AccumulatorRegisterTag);
            push(AccumulatorRegisterValue);
        } else {
            move(AccumulatorRegisterValue, registerForArg(0));
            move(AccumulatorRegisterTag, registerForArg(1));
        }
        callRuntime("toInt32Helper", reinterpret_cast<void *>(&toInt32Helper),
                    Assembler::ResultInAccumulator);
        if (ArgInRegCount < 2)
            addPtr(TrustedImm32(2 * PointerSize), StackPointerRegister);

        isInt.link(this);
    }

    void regToInt32(Address srcReg, RegisterID targetReg)
    {
        bool accumulatorNeedsSaving = AccumulatorRegisterValue == ReturnValueRegisterValue
                || AccumulatorRegisterTag == ReturnValueRegisterTag;
        if (accumulatorNeedsSaving) {
            push(AccumulatorRegisterTag);
            push(AccumulatorRegisterValue);
        }
        if (ArgInRegCount < 2) {
            srcReg.offset += 4;
            load32(srcReg, targetReg);
            push(targetReg);
            srcReg.offset -= 4;
            load32(srcReg, targetReg);
            push(targetReg);
        } else {
            load32(srcReg, registerForArg(0));
            srcReg.offset += 4;
            load32(srcReg, registerForArg(1));
        }
        callHelper(toInt32Helper);
        move(ReturnValueRegisterValue, targetReg);
        if (ArgInRegCount < 2)
            addPtr(TrustedImm32(2 * PointerSize), StackPointerRegister);
        if (accumulatorNeedsSaving) {
            pop(AccumulatorRegisterValue);
            pop(AccumulatorRegisterTag);
        }
    }

    void isNullOrUndefined()
    {
        Jump notUndefOrPtr = branch32(NotEqual, TrustedImm32(0), AccumulatorRegisterTag);
        compare32(Equal, AccumulatorRegisterValue, TrustedImm32(0), AccumulatorRegisterValue);
        auto done = jump();

        // not undefined or managed
        notUndefOrPtr.link(this);
        compare32(Equal, AccumulatorRegisterTag, TrustedImm32(int(QV4::Value::ValueTypeInternal::Null)),
                  AccumulatorRegisterValue);

        done.link(this);
    }

    Jump isIntOrBool()
    {
        urshift32(AccumulatorRegisterTag, TrustedImm32(Value::IsIntegerOrBool_Shift - 32), ScratchRegister);
        return branch32(Equal, TrustedImm32(3), ScratchRegister);
    }

    void pushValue(ReturnedValue v)
    {
        push(TrustedImm32(v >> 32));
        push(TrustedImm32(v));
    }

    void toBoolean(std::function<void(RegisterID)> continuation)
    {
        urshift32(AccumulatorRegisterTag, TrustedImm32(Value::IsIntegerConvertible_Shift - 32),
                  ScratchRegister);
        auto needsConversion = branch32(NotEqual, TrustedImm32(1), ScratchRegister);
        continuation(AccumulatorRegisterValue);
        Jump done = jump();

        // slow path:
        needsConversion.link(this);

        bool accumulatorNeedsSaving = AccumulatorRegisterValue == ReturnValueRegisterValue
                || AccumulatorRegisterTag == ReturnValueRegisterTag;
        if (accumulatorNeedsSaving) {
            push(AccumulatorRegisterTag);
            push(AccumulatorRegisterValue);
        }

        if (ArgInRegCount < 2) {
            push(AccumulatorRegisterTag);
            push(AccumulatorRegisterValue);
        } else {
            move(AccumulatorRegisterValue, registerForArg(0));
            move(AccumulatorRegisterTag, registerForArg(1));
        }
        callHelper(Value::toBooleanImpl);
        if (ArgInRegCount < 2)
            addPtr(TrustedImm32(2 * PointerSize), StackPointerRegister);

        and32(TrustedImm32(1), ReturnValueRegisterValue, ScratchRegister);
        if (accumulatorNeedsSaving) {
            pop(AccumulatorRegisterValue);
            pop(AccumulatorRegisterTag);
        }
        continuation(ScratchRegister);

        done.link(this);
    }

    void jumpStrictEqualStackSlotInt(int lhs, int rhs, int offset)
    {
        Address lhsAddr(JSStackFrameRegister, lhs * int(sizeof(Value)));
        load32(lhsAddr, ScratchRegister);
        Jump notEqInt = branch32(NotEqual, ScratchRegister, TrustedImm32(rhs));
        Jump notEqUndefVal = branch32(NotEqual, ScratchRegister, TrustedImm32(0));
        patches.push_back({ notEqUndefVal, offset });
        lhsAddr.offset += 4;
        load32(lhsAddr, ScratchRegister);
        Jump notEqUndefTag = branch32(NotEqual, ScratchRegister, TrustedImm32(0));
        patches.push_back({ notEqUndefTag, offset });
        notEqInt.link(this);
    }

    void jumpStrictNotEqualStackSlotInt(int lhs, int rhs, int offset)
    {
        Address lhsAddr(JSStackFrameRegister, lhs * int(sizeof(Value)));
        load32(lhsAddr, ScratchRegister);
        Jump notEqual = branch32(NotEqual, TrustedImm32(rhs), ScratchRegister);
        patches.push_back({ notEqual, offset });
        Jump notUndefValue = branch32(NotEqual, TrustedImm32(0), ScratchRegister);
        lhsAddr.offset += 4;
        load32(lhsAddr, ScratchRegister);
        Jump equalUndef = branch32(Equal, TrustedImm32(0), ScratchRegister);
        patches.push_back({ equalUndef, offset });
        notUndefValue.link(this);
    }

    void setAccumulatorTag(QV4::Value::ValueTypeInternal tag, RegisterID sourceReg = NoRegister)
    {
        if (sourceReg != NoRegister)
            move(sourceReg, AccumulatorRegisterValue);
        move(TrustedImm32(int(tag)), AccumulatorRegisterTag);
    }

    void encodeDoubleIntoAccumulator(FPRegisterID src)
    {
        moveDoubleToInts(src, AccumulatorRegisterValue, AccumulatorRegisterTag);
        xor32(TrustedImm32(Value::NaNEncodeMask >> 32), AccumulatorRegisterTag);
    }

    void pushValueAligned(ReturnedValue v)
    {
        pushValue(v);
    }

    void popValueAligned()
    {
        popValue();
    }

    Jump binopBothIntPath(Address lhsAddr, std::function<Jump(void)> fastPath)
    {
        Jump accNotInt = branch32(NotEqual, TrustedImm32(int(IntegerTag)), AccumulatorRegisterTag);
        Address lhsAddrTag = lhsAddr; lhsAddrTag.offset += Value::tagOffset();
        load32(lhsAddrTag, ScratchRegister);
        Jump lhsNotInt = branch32(NotEqual, TrustedImm32(int(IntegerTag)), ScratchRegister);

        // both integer
        Address lhsAddrValue = lhsAddr; lhsAddrValue.offset += Value::valueOffset();
        load32(lhsAddrValue, ScratchRegister);
        Jump failure = fastPath();
        Jump done = jump();

        // all other cases
        if (failure.isSet())
            failure.link(this);
        accNotInt.link(this);
        lhsNotInt.link(this);

        return done;
    }

    Jump unopIntPath(std::function<Jump(void)> fastPath)
    {
        Jump accNotInt = branch32(NotEqual, TrustedImm32(int(IntegerTag)), AccumulatorRegisterTag);

        // both integer
        Jump failure = fastPath();
        Jump done = jump();

        // all other cases
        if (failure.isSet())
            failure.link(this);
        accNotInt.link(this);

        return done;
    }

    void callWithAccumulatorByValueAsFirstArgument(std::function<void()> doCall)
    {
        if (ArgInRegCount < 2) {
            push(AccumulatorRegisterTag);
            push(AccumulatorRegisterValue);
        } else {
            move(AccumulatorRegisterValue, registerForArg(0));
            move(AccumulatorRegisterTag, registerForArg(1));
        }
        doCall();
        if (ArgInRegCount < 2)
            addPtr(TrustedImm32(2 * PointerSize), StackPointerRegister);
    }
};

typedef PlatformAssembler32 PlatformAssembler;
#endif

typedef PlatformAssembler::TrustedImmPtr TrustedImmPtr;
typedef PlatformAssembler::TrustedImm32 TrustedImm32;
typedef PlatformAssembler::TrustedImm64 TrustedImm64;
typedef PlatformAssembler::Address Address;
typedef PlatformAssembler::RegisterID RegisterID;
typedef PlatformAssembler::FPRegisterID FPRegisterID;

#define pasm() reinterpret_cast<PlatformAssembler *>(this->d)

static Address regAddr(int reg)
{
    return Address(PlatformAssembler::JSStackFrameRegister, reg * int(sizeof(QV4::Value)));
}

Assembler::Assembler(const Value *constantTable)
    : d(new PlatformAssembler)
{
    pasm()->constantTable = constantTable;
}

Assembler::~Assembler()
{
    delete pasm();
}

void Assembler::generatePrologue()
{
    pasm()->generateFunctionEntry();
}

void Assembler::generateEpilogue()
{
    pasm()->generateCatchTrampoline();
}

namespace {
class QIODevicePrintStream: public FilePrintStream
{
    Q_DISABLE_COPY(QIODevicePrintStream)

public:
    explicit QIODevicePrintStream(QIODevice *dest)
        : FilePrintStream(nullptr)
        , dest(dest)
        , buf(4096, '0')
    {
        Q_ASSERT(dest);
    }

    ~QIODevicePrintStream()
    {}

    void vprintf(const char* format, va_list argList) WTF_ATTRIBUTE_PRINTF(2, 0)
    {
        const int written = qvsnprintf(buf.data(), buf.size(), format, argList);
        if (written > 0)
            dest->write(buf.constData(), written);
        memset(buf.data(), 0, qMin(written, buf.size()));
    }

    void flush()
    {}

private:
    QIODevice *dest;
    QByteArray buf;
};
} // anonymous namespace

static void printDisassembledOutputWithCalls(QByteArray processedOutput,
                                             const QHash<const void*, const char*>& functions)
{
    for (QHash<const void*, const char*>::ConstIterator it = functions.begin(), end = functions.end();
         it != end; ++it) {
        const QByteArray ptrString = "0x" + QByteArray::number(quintptr(it.key()), 16);
        int idx = 0;
        while (idx >= 0) {
            idx = processedOutput.indexOf(ptrString, idx);
            if (idx < 0)
                break;
            idx = processedOutput.indexOf('\n', idx);
            if (idx < 0)
                break;
            processedOutput = processedOutput.insert(idx, QByteArrayLiteral("                          ; ") + it.value());
        }
    }

    qDebug("%s", processedOutput.constData());
}

static QByteArray functionName(Function *function)
{
    QByteArray name = function->name()->toQString().toUtf8();
    if (name.isEmpty()) {
        name = QByteArray::number(reinterpret_cast<quintptr>(function), 16);
        name.prepend("QV4::Function(0x");
        name.append(')');
    }
    return name;
}

void Assembler::link(Function *function)
{
    for (const auto &jumpTarget : pasm()->patches)
        jumpTarget.jump.linkTo(pasm()->labelsByOffset[jumpTarget.offset], pasm());

    JSC::JSGlobalData dummy(function->internalClass->engine->executableAllocator);
    JSC::LinkBuffer<PlatformAssembler::MacroAssembler> linkBuffer(dummy, pasm(), nullptr);

    for (const auto &ehTarget : pasm()->ehTargets) {
        auto targetLabel = pasm()->labelsByOffset.value(ehTarget.offset);
        linkBuffer.patch(ehTarget.label, linkBuffer.locationOf(targetLabel));
    }

    JSC::MacroAssemblerCodeRef codeRef;

    static const bool showCode = qEnvironmentVariableIsSet("QV4_SHOW_ASM");
    if (showCode) {
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        WTF::setDataFile(new QIODevicePrintStream(&buf));

        QByteArray name = functionName(function);
        codeRef = linkBuffer.finalizeCodeWithDisassembly("%s", name.data());

        WTF::setDataFile(stderr);
        printDisassembledOutputWithCalls(buf.data(), pasm()->functions);
    } else {
        codeRef = linkBuffer.finalizeCodeWithoutDisassembly();
    }

    function->codeRef = new JSC::MacroAssemblerCodeRef(codeRef);
    function->jittedCode = reinterpret_cast<Function::JittedCode>(function->codeRef->code().executableAddress());

#if defined(Q_OS_LINUX)
    // This implements writing of JIT'd addresses so that perf can find the
    // symbol names.
    //
    // Perf expects the mapping to be in a certain place and have certain
    // content, for more information, see:
    // https://github.com/torvalds/linux/blob/master/tools/perf/Documentation/jit-interface.txt
    static bool doProfile = !qEnvironmentVariableIsEmpty("QV4_PROFILE_WRITE_PERF_MAP");
    if (doProfile) {
        static QFile perfMapFile(QString::fromLatin1("/tmp/perf-%1.map")
                                 .arg(QCoreApplication::applicationPid()));
        static const bool isOpen = perfMapFile.open(QIODevice::WriteOnly);
        if (!isOpen) {
            qWarning("QV4::JIT::Assembler: Cannot write perf map file.");
            doProfile = false;
        } else {
            perfMapFile.write(QByteArray::number(reinterpret_cast<quintptr>(
                                                     codeRef.code().executableAddress()), 16));
            perfMapFile.putChar(' ');
            perfMapFile.write(QByteArray::number(static_cast<qsizetype>(codeRef.size()), 16));
            perfMapFile.putChar(' ');
            perfMapFile.write(functionName(function));
            perfMapFile.putChar('\n');
            perfMapFile.flush();
        }
    }
#endif
}

void Assembler::addLabel(int offset)
{
    pasm()->labelsByOffset[offset] = pasm()->label();
}

void Assembler::loadConst(int constIndex)
{
    //###
    if (pasm()->constantTable[constIndex].isUndefined()) {
        pasm()->loadUndefined();
    } else {
        pasm()->loadAccumulator(pasm()->loadConstAddress(constIndex));
    }
}

void Assembler::copyConst(int constIndex, int destReg)
{
    pasm()->copyConst(constIndex, regAddr(destReg));
}

void Assembler::loadReg(int reg)
{
    pasm()->loadAccumulator(regAddr(reg));
}

void Assembler::storeReg(int reg)
{
    pasm()->storeAccumulator(regAddr(reg));
}

void Assembler::loadLocal(int index, int level)
{
    Heap::CallContext ctx;
    Q_UNUSED(ctx)
    pasm()->loadPointerFromValue(regAddr(CallData::Context), PlatformAssembler::ScratchRegister);
    while (level) {
        pasm()->loadPtr(Address(PlatformAssembler::ScratchRegister, ctx.outer.offset), PlatformAssembler::ScratchRegister);
        --level;
    }
    pasm()->loadAccumulator(Address(PlatformAssembler::ScratchRegister, ctx.locals.offset + offsetof(ValueArray<0>, values) + sizeof(Value)*index));
}

void Assembler::storeLocal(int index, int level)
{
    Heap::CallContext ctx;
    Q_UNUSED(ctx)
    pasm()->loadPtr(regAddr(CallData::Context), PlatformAssembler::ScratchRegister);
    while (level) {
        pasm()->loadPtr(Address(PlatformAssembler::ScratchRegister, ctx.outer.offset), PlatformAssembler::ScratchRegister);
        --level;
    }
    pasm()->storeAccumulator(Address(PlatformAssembler::ScratchRegister, ctx.locals.offset + offsetof(ValueArray<0>, values) + sizeof(Value)*index));
}

void Assembler::loadString(int stringId)
{
    pasm()->loadString(stringId);
}

void Assembler::loadValue(ReturnedValue value)
{
    pasm()->loadValue(value);
}

void JIT::Assembler::storeHeapObject(int reg)
{
    pasm()->storeHeapObject(PlatformAssembler::ReturnValueRegisterValue, regAddr(reg));
}

void Assembler::toNumber()
{
    pasm()->toNumber();
}

void Assembler::uminus()
{
    saveAccumulatorInFrame();
    prepareCallWithArgCount(1);
    passAccumulatorAsArg(0);
    IN_JIT_GENERATE_RUNTIME_CALL(Runtime::method_uMinus, ResultInAccumulator);
    checkException();
}

void Assembler::ucompl()
{
    pasm()->toInt32();
    pasm()->xor32(TrustedImm32(-1), PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(IntegerTag);
}

static ReturnedValue incHelper(const Value v)
{
    double d;
    if (Q_LIKELY(v.isDouble()))
        d =  v.doubleValue();
    else
        d = v.toNumberImpl();
    return Encode(d + 1.);
}

void Assembler::inc()
{
    auto done = pasm()->unopIntPath([this](){
        auto overflowed = pasm()->branchAdd32(PlatformAssembler::Overflow,
                                              PlatformAssembler::AccumulatorRegisterValue,
                                              TrustedImm32(1),
                                              PlatformAssembler::ScratchRegister);
        pasm()->setAccumulatorTag(IntegerTag, PlatformAssembler::ScratchRegister);
        return overflowed;
    });

    // slow path:
    pasm()->callWithAccumulatorByValueAsFirstArgument([this]() {
        pasm()->callHelper(incHelper);
        pasm()->saveReturnValueInAccumulator();
    });
    checkException();

    // done.
    done.link(pasm());
}

static ReturnedValue decHelper(const Value v)
{
    double d;
    if (Q_LIKELY(v.isDouble()))
        d =  v.doubleValue();
    else
        d = v.toNumberImpl();
    return Encode(d - 1.);
}

void Assembler::dec()
{
    auto done = pasm()->unopIntPath([this](){
        auto overflowed = pasm()->branchSub32(PlatformAssembler::Overflow,
                                              PlatformAssembler::AccumulatorRegisterValue,
                                              TrustedImm32(1),
                                              PlatformAssembler::ScratchRegister);
        pasm()->setAccumulatorTag(IntegerTag, PlatformAssembler::ScratchRegister);
        return overflowed;
    });

    // slow path:
    pasm()->callWithAccumulatorByValueAsFirstArgument([this]() {
        pasm()->callHelper(decHelper);
        pasm()->saveReturnValueInAccumulator();
    });
    checkException();

    // done.
    done.link(pasm());
}

void Assembler::unot()
{
    pasm()->toBoolean([this](PlatformAssembler::RegisterID resultReg){
        pasm()->compare32(PlatformAssembler::Equal, resultReg,
                          TrustedImm32(0), PlatformAssembler::AccumulatorRegisterValue);
        pasm()->setAccumulatorTag(QV4::Value::ValueTypeInternal::Boolean);
    });
}

void Assembler::add(int lhs)
{
    auto done = pasm()->binopBothIntPath(regAddr(lhs), [this](){
        auto overflowed = pasm()->branchAdd32(PlatformAssembler::Overflow,
                                              PlatformAssembler::AccumulatorRegisterValue,
                                              PlatformAssembler::ScratchRegister);
        pasm()->setAccumulatorTag(IntegerTag,
                                  PlatformAssembler::ScratchRegister);
        return overflowed;
    });

    // slow path:
    saveAccumulatorInFrame();
    prepareCallWithArgCount(3);
    passAccumulatorAsArg(2);
    passRegAsArg(lhs, 1);
    passEngineAsArg(0);
    IN_JIT_GENERATE_RUNTIME_CALL(Runtime::method_add, ResultInAccumulator);
    checkException();

    // done.
    done.link(pasm());
}

void Assembler::bitAnd(int lhs)
{
    PlatformAssembler::Address lhsAddr = regAddr(lhs);
    pasm()->toInt32LhsAcc(lhsAddr, PlatformAssembler::ScratchRegister);
    pasm()->and32(PlatformAssembler::ScratchRegister, PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(IntegerTag);
}

void Assembler::bitOr(int lhs)
{
    PlatformAssembler::Address lhsAddr = regAddr(lhs);
    pasm()->toInt32LhsAcc(lhsAddr, PlatformAssembler::ScratchRegister);
    pasm()->or32(PlatformAssembler::ScratchRegister, PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(IntegerTag);
}

void Assembler::bitXor(int lhs)
{
    PlatformAssembler::Address lhsAddr = regAddr(lhs);
    pasm()->toInt32LhsAcc(lhsAddr, PlatformAssembler::ScratchRegister);
    pasm()->xor32(PlatformAssembler::ScratchRegister, PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(IntegerTag);
}

void Assembler::ushr(int lhs)
{
    PlatformAssembler::Address lhsAddr = regAddr(lhs);
    pasm()->toInt32LhsAcc(lhsAddr, PlatformAssembler::ScratchRegister);
    pasm()->and32(TrustedImm32(0x1f), PlatformAssembler::AccumulatorRegisterValue);
    pasm()->urshift32(PlatformAssembler::AccumulatorRegisterValue, PlatformAssembler::ScratchRegister);
    pasm()->move(PlatformAssembler::ScratchRegister, PlatformAssembler::AccumulatorRegisterValue);
    auto doubleEncode = pasm()->branch32(PlatformAssembler::LessThan,
                                         PlatformAssembler::AccumulatorRegisterValue,
                                         TrustedImm32(0));
    pasm()->setAccumulatorTag(IntegerTag);
    auto done = pasm()->jump();

    doubleEncode.link(pasm());
    pasm()->convertUInt32ToDouble(PlatformAssembler::AccumulatorRegisterValue,
                                  PlatformAssembler::FPScratchRegister,
                                  PlatformAssembler::ScratchRegister);
    pasm()->encodeDoubleIntoAccumulator(PlatformAssembler::FPScratchRegister);
    done.link(pasm());
}

void Assembler::shr(int lhs)
{
    PlatformAssembler::Address lhsAddr = regAddr(lhs);
    pasm()->toInt32LhsAcc(lhsAddr, PlatformAssembler::ScratchRegister);
    pasm()->and32(TrustedImm32(0x1f), PlatformAssembler::AccumulatorRegisterValue);
    pasm()->rshift32(PlatformAssembler::AccumulatorRegisterValue, PlatformAssembler::ScratchRegister);
    pasm()->move(PlatformAssembler::ScratchRegister, PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(IntegerTag);
}

void Assembler::shl(int lhs)
{
    PlatformAssembler::Address lhsAddr = regAddr(lhs);
    pasm()->toInt32LhsAcc(lhsAddr, PlatformAssembler::ScratchRegister);
    pasm()->and32(TrustedImm32(0x1f), PlatformAssembler::AccumulatorRegisterValue);
    pasm()->lshift32(PlatformAssembler::AccumulatorRegisterValue, PlatformAssembler::ScratchRegister);
    pasm()->move(PlatformAssembler::ScratchRegister, PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(IntegerTag);
}

void Assembler::bitAndConst(int rhs)
{
    pasm()->toInt32();
    pasm()->and32(TrustedImm32(rhs), PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(IntegerTag);
}

void Assembler::bitOrConst(int rhs)
{
    pasm()->toInt32();
    pasm()->or32(TrustedImm32(rhs), PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(IntegerTag);
}

void Assembler::bitXorConst(int rhs)
{
    pasm()->toInt32();
    pasm()->xor32(TrustedImm32(rhs), PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(IntegerTag);
}

void Assembler::ushrConst(int rhs)
{
    rhs &= 0x1f;
    pasm()->toInt32();
    if (rhs) {
        // a non zero shift will always give a number encodable as an int
        pasm()->urshift32(TrustedImm32(rhs), PlatformAssembler::AccumulatorRegisterValue);
        pasm()->setAccumulatorTag(IntegerTag);
    } else {
        // shift with 0 can lead to a negative result
        auto doubleEncode = pasm()->branch32(PlatformAssembler::LessThan,
                                             PlatformAssembler::AccumulatorRegisterValue,
                                             TrustedImm32(0));
        pasm()->setAccumulatorTag(IntegerTag);
        auto done = pasm()->jump();

        doubleEncode.link(pasm());
        pasm()->convertUInt32ToDouble(PlatformAssembler::AccumulatorRegisterValue,
                                      PlatformAssembler::FPScratchRegister,
                                      PlatformAssembler::ScratchRegister);
        pasm()->encodeDoubleIntoAccumulator(PlatformAssembler::FPScratchRegister);
        done.link(pasm());
    }
}

void Assembler::shrConst(int rhs)
{
    rhs &= 0x1f;
    pasm()->toInt32();
    if (rhs)
        pasm()->rshift32(TrustedImm32(rhs), PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(IntegerTag);
}

void Assembler::shlConst(int rhs)
{
    rhs &= 0x1f;
    pasm()->toInt32();
    if (rhs)
        pasm()->lshift32(TrustedImm32(rhs), PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(IntegerTag);
}

void Assembler::mul(int lhs)
{
    auto done = pasm()->binopBothIntPath(regAddr(lhs), [this](){
        auto overflowed = pasm()->branchMul32(PlatformAssembler::Overflow,
                                              PlatformAssembler::AccumulatorRegisterValue,
                                              PlatformAssembler::ScratchRegister);
        pasm()->setAccumulatorTag(IntegerTag,
                                  PlatformAssembler::ScratchRegister);
        return overflowed;
    });

    // slow path:
    saveAccumulatorInFrame();
    prepareCallWithArgCount(2);
    passAccumulatorAsArg(1);
    passRegAsArg(lhs, 0);
    IN_JIT_GENERATE_RUNTIME_CALL(Runtime::method_mul, ResultInAccumulator);
    checkException();

    // done.
    done.link(pasm());
}

void Assembler::div(int lhs)
{
    saveAccumulatorInFrame();
    prepareCallWithArgCount(2);
    passAccumulatorAsArg(1);
    passRegAsArg(lhs, 0);
    IN_JIT_GENERATE_RUNTIME_CALL(Runtime::method_div, ResultInAccumulator);
    checkException();
}

void Assembler::mod(int lhs)
{
    saveAccumulatorInFrame();
    prepareCallWithArgCount(2);
    passAccumulatorAsArg(1);
    passRegAsArg(lhs, 0);
    IN_JIT_GENERATE_RUNTIME_CALL(Runtime::method_mod, ResultInAccumulator);
    checkException();
}

void Assembler::sub(int lhs)
{
    auto done = pasm()->binopBothIntPath(regAddr(lhs), [this](){
        auto overflowed = pasm()->branchSub32(PlatformAssembler::Overflow,
                                              PlatformAssembler::AccumulatorRegisterValue,
                                              PlatformAssembler::ScratchRegister);
        pasm()->setAccumulatorTag(IntegerTag,
                                  PlatformAssembler::ScratchRegister);
        return overflowed;
    });

    // slow path:
    saveAccumulatorInFrame();
    prepareCallWithArgCount(2);
    passAccumulatorAsArg(1);
    passRegAsArg(lhs, 0);
    IN_JIT_GENERATE_RUNTIME_CALL(Runtime::method_sub, ResultInAccumulator);
    checkException();

    // done.
    done.link(pasm());
}

void Assembler::cmpeqNull()
{
    pasm()->isNullOrUndefined();
    pasm()->setAccumulatorTag(QV4::Value::ValueTypeInternal::Boolean);
}

void Assembler::cmpneNull()
{
    pasm()->isNullOrUndefined();
    pasm()->xor32(TrustedImm32(1), PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(QV4::Value::ValueTypeInternal::Boolean);
}

void Assembler::cmpeqInt(int lhs)
{
    auto isIntOrBool = pasm()->isIntOrBool();
    saveAccumulatorInFrame();
    pasm()->pushValueAligned(Encode(lhs));
    if (PlatformAssembler::ArgInRegCount < 2)
        pasm()->push(PlatformAssembler::StackPointerRegister);
    else
        pasm()->move(PlatformAssembler::StackPointerRegister, pasm()->registerForArg(1));
    passAccumulatorAsArg_internal(0, true);
    pasm()->callRuntime("Runtime::method_equal", (void*)Runtime::method_equal, ResultInAccumulator);
    if (PlatformAssembler::ArgInRegCount < 2)
        pasm()->addPtr(TrustedImm32(2 * PlatformAssembler::PointerSize), PlatformAssembler::StackPointerRegister);
    pasm()->popValueAligned();
    auto done = pasm()->jump();
    isIntOrBool.link(pasm());
    pasm()->compare32(PlatformAssembler::Equal, PlatformAssembler::AccumulatorRegisterValue,
                      TrustedImm32(lhs),
                      PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(QV4::Value::ValueTypeInternal::Boolean);
    done.link(pasm());
}

void Assembler::cmpneInt(int lhs)
{
    auto isIntOrBool = pasm()->isIntOrBool();
    saveAccumulatorInFrame();
    pasm()->pushValueAligned(Encode(lhs));
    if (PlatformAssembler::ArgInRegCount < 2)
        pasm()->push(PlatformAssembler::StackPointerRegister);
    else
        pasm()->move(PlatformAssembler::StackPointerRegister, pasm()->registerForArg(1));
    passAccumulatorAsArg_internal(0, true);
    pasm()->callRuntime("Runtime::method_notEqual", (void*)Runtime::method_notEqual, ResultInAccumulator);
    if (PlatformAssembler::ArgInRegCount < 2)
        pasm()->addPtr(TrustedImm32(2 * PlatformAssembler::PointerSize), PlatformAssembler::StackPointerRegister);
    pasm()->popValueAligned();
    auto done = pasm()->jump();
    isIntOrBool.link(pasm());
    pasm()->compare32(PlatformAssembler::NotEqual, PlatformAssembler::AccumulatorRegisterValue,
                      TrustedImm32(lhs),
                      PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(QV4::Value::ValueTypeInternal::Boolean);
    done.link(pasm());
}

void Assembler::cmp(int cond, CmpFunc function, const char *functionName, int lhs)
{
    auto c = static_cast<PlatformAssembler::RelationalCondition>(cond);
    auto done = pasm()->binopBothIntPath(regAddr(lhs), [this, c](){
        pasm()->compare32(c, PlatformAssembler::ScratchRegister,
                          PlatformAssembler::AccumulatorRegisterValue,
                          PlatformAssembler::AccumulatorRegisterValue);
        pasm()->setAccumulatorTag(QV4::Value::ValueTypeInternal::Boolean);
        return PlatformAssembler::Jump();
    });

    // slow path:
    saveAccumulatorInFrame();
    prepareCallWithArgCount(2);
    passAccumulatorAsArg(1);
    passRegAsArg(lhs, 0);

    callRuntime(functionName, reinterpret_cast<void*>(function), ResultInAccumulator);
    checkException();
    pasm()->setAccumulatorTag(QV4::Value::ValueTypeInternal::Boolean);

    // done.
    done.link(pasm());
}

void Assembler::cmpeq(int lhs)
{
    cmp(PlatformAssembler::Equal, &Runtime::method_compareEqual,
        "Runtime::method_compareEqual", lhs);
}

void Assembler::cmpne(int lhs)
{
    cmp(PlatformAssembler::NotEqual, &Runtime::method_compareNotEqual,
        "Runtime::method_compareNotEqual", lhs);
}

void Assembler::cmpgt(int lhs)
{
    cmp(PlatformAssembler::GreaterThan, &Runtime::method_compareGreaterThan,
        "Runtime::method_compareGreaterThan", lhs);
}

void Assembler::cmpge(int lhs)
{
    cmp(PlatformAssembler::GreaterThanOrEqual, &Runtime::method_compareGreaterEqual,
        "Runtime::method_compareGreaterEqual", lhs);
}

void Assembler::cmplt(int lhs)
{
    cmp(PlatformAssembler::LessThan, &Runtime::method_compareLessThan,
        "Runtime::method_compareLessThan", lhs);
}

void Assembler::cmple(int lhs)
{
    cmp(PlatformAssembler::LessThanOrEqual, &Runtime::method_compareLessEqual,
        "Runtime::method_compareLessEqual", lhs);
}

void Assembler::cmpStrictEqual(int lhs)
{
    cmp(PlatformAssembler::Equal, &RuntimeHelpers::strictEqual,
        "RuntimeHelpers::strictEqual", lhs);
}

void Assembler::cmpStrictNotEqual(int lhs)
{
    cmp(PlatformAssembler::Equal, &RuntimeHelpers::strictEqual,
        "RuntimeHelpers::strictEqual", lhs);
    pasm()->xor32(TrustedImm32(1), PlatformAssembler::AccumulatorRegisterValue);
    pasm()->setAccumulatorTag(QV4::Value::ValueTypeInternal::Boolean);
}

void Assembler::jump(int offset)
{
    pasm()->patches.push_back({ pasm()->jump(), offset });
}

void Assembler::jumpTrue(int offset)
{
    pasm()->toBoolean([this, offset](PlatformAssembler::RegisterID resultReg) {
        auto jump = pasm()->branch32(PlatformAssembler::NotEqual, TrustedImm32(0), resultReg);
        pasm()->patches.push_back({ jump, offset });
    });
}

void Assembler::jumpFalse(int offset)
{
    pasm()->toBoolean([this, offset](PlatformAssembler::RegisterID resultReg) {
        auto jump = pasm()->branch32(PlatformAssembler::Equal, TrustedImm32(0), resultReg);
        pasm()->patches.push_back({ jump, offset });
    });
}

void Assembler::jumpStrictEqualStackSlotInt(int lhs, int rhs, int offset)
{
    pasm()->jumpStrictEqualStackSlotInt(lhs, rhs, offset);
}

void Assembler::jumpStrictNotEqualStackSlotInt(int lhs, int rhs, int offset)
{
    pasm()->jumpStrictNotEqualStackSlotInt(lhs, rhs, offset);
}

void Assembler::prepareCallWithArgCount(int argc)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(remainingArgcForCall == NoCall);
    remainingArgcForCall = argc;
#endif

    if (argc > PlatformAssembler::ArgInRegCount) {
        argcOnStackForCall = int(WTF::roundUpToMultipleOf(16, size_t(argc - PlatformAssembler::ArgInRegCount) * PlatformAssembler::PointerSize));
        pasm()->subPtr(TrustedImm32(argcOnStackForCall), PlatformAssembler::StackPointerRegister);
    }
}

void Assembler::storeInstructionPointer(int instructionOffset)
{
    PlatformAssembler::Address addr(PlatformAssembler::CppStackFrameRegister,
                                    offsetof(QV4::CppStackFrame, instructionPointer));
    pasm()->store32(TrustedImm32(instructionOffset), addr);
}

Address argStackAddress(int arg)
{
    int offset = arg - PlatformAssembler::ArgInRegCount;
    Q_ASSERT(offset >= 0);
    return Address(PlatformAssembler::StackPointerRegister, offset * PlatformAssembler::PointerSize);
}

void Assembler::passAccumulatorAsArg(int arg)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(arg < remainingArgcForCall);
    --remainingArgcForCall;
#endif

    passAccumulatorAsArg_internal(arg, false);
}

void Assembler::passAccumulatorAsArg_internal(int arg, bool push)
{
    if (arg < PlatformAssembler::ArgInRegCount) {
        pasm()->addPtr(TrustedImm32(offsetof(CallData, accumulator)),
                       PlatformAssembler::JSStackFrameRegister,
                       pasm()->registerForArg(arg));
    } else {
        pasm()->addPtr(TrustedImm32(offsetof(CallData, accumulator)),
                       PlatformAssembler::JSStackFrameRegister,
                       PlatformAssembler::ScratchRegister);
        if (push)
            pasm()->push(PlatformAssembler::ScratchRegister);
        else
            pasm()->storePtr(PlatformAssembler::ScratchRegister,
                             argStackAddress(arg));
    }
}

void Assembler::passFunctionAsArg(int arg)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(arg < remainingArgcForCall);
    --remainingArgcForCall;
#endif

    if (arg < PlatformAssembler::ArgInRegCount) {
        pasm()->loadFunctionPtr(pasm()->registerForArg(arg));
    } else {
        pasm()->loadFunctionPtr(PlatformAssembler::ScratchRegister);
        pasm()->storePtr(PlatformAssembler::ScratchRegister,
                         argStackAddress(arg));
    }
}

void Assembler::passEngineAsArg(int arg)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(arg < remainingArgcForCall);
    --remainingArgcForCall;
#endif

    if (arg < PlatformAssembler::ArgInRegCount) {
        pasm()->move(PlatformAssembler::EngineRegister, pasm()->registerForArg(arg));
    } else {
        pasm()->storePtr(PlatformAssembler::EngineRegister, argStackAddress(arg));
    }
}

void Assembler::passRegAsArg(int reg, int arg)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(arg < remainingArgcForCall);
    --remainingArgcForCall;
#endif

    if (arg < PlatformAssembler::ArgInRegCount) {
        pasm()->addPtr(TrustedImm32(reg * int(sizeof(QV4::Value))),
                       PlatformAssembler::JSStackFrameRegister,
                       pasm()->registerForArg(arg));
    } else {
        pasm()->addPtr(TrustedImm32(reg * int(sizeof(QV4::Value))),
                       PlatformAssembler::JSStackFrameRegister,
                       PlatformAssembler::ScratchRegister);
        pasm()->storePtr(PlatformAssembler::ScratchRegister,
                         argStackAddress(arg));
    }
}

void JIT::Assembler::passCppFrameAsArg(int arg)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(arg < remainingArgcForCall);
    --remainingArgcForCall;
#endif

    if (arg < PlatformAssembler::ArgInRegCount) {
        pasm()->move(PlatformAssembler::CppStackFrameRegister, pasm()->registerForArg(arg));
    } else {
        pasm()->store32(PlatformAssembler::CppStackFrameRegister, argStackAddress(arg));
    }
}

void Assembler::passInt32AsArg(int value, int arg)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(arg < remainingArgcForCall);
    --remainingArgcForCall;
#endif

    if (arg < PlatformAssembler::ArgInRegCount) {
        pasm()->move(TrustedImm32(value), pasm()->registerForArg(arg));
    } else {
        pasm()->store32(TrustedImm32(value), argStackAddress(arg));
    }
}

void Assembler::callRuntime(const char *functionName, const void *funcPtr,
                            Assembler::CallResultDestination dest)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(remainingArgcForCall == 0);
    remainingArgcForCall = NoCall;
#endif
    pasm()->callRuntime(functionName, funcPtr, dest);
    if (argcOnStackForCall > 0) {
        pasm()->addPtr(TrustedImm32(argcOnStackForCall), PlatformAssembler::StackPointerRegister);
        argcOnStackForCall = 0;
    }
}

void Assembler::saveAccumulatorInFrame()
{
    pasm()->storeAccumulator(PlatformAssembler::Address(PlatformAssembler::JSStackFrameRegister,
                                                       offsetof(CallData, accumulator)));
}

void Assembler::checkException()
{
    pasm()->addCatchyJump(
                pasm()->branch32(
                    PlatformAssembler::NotEqual,
                    PlatformAssembler::Address(PlatformAssembler::EngineRegister,
                                               offsetof(EngineBase, hasException)),
                    TrustedImm32(0)));
}

void Assembler::gotoCatchException()
{
    pasm()->addCatchyJump(pasm()->jump());
}

void Assembler::getException()
{
    Q_STATIC_ASSERT(sizeof(QV4::EngineBase::hasException) == 1);

    Address hasExceptionAddr(PlatformAssembler::EngineRegister,
                             offsetof(EngineBase, hasException));
    PlatformAssembler::Jump nope = pasm()->branch8(PlatformAssembler::Equal,
                                                  hasExceptionAddr,
                                                  TrustedImm32(0));
    pasm()->loadPtr(Address(PlatformAssembler::EngineRegister,
                            offsetof(EngineBase, exceptionValue)),
                    PlatformAssembler::ScratchRegister);
    pasm()->loadAccumulator(Address(PlatformAssembler::ScratchRegister));
    pasm()->store8(TrustedImm32(0), hasExceptionAddr);
    auto done = pasm()->jump();
    nope.link(pasm());
    pasm()->loadValue(Primitive::emptyValue().asReturnedValue());

    done.link(pasm());
}

void Assembler::setException()
{
    Address addr(PlatformAssembler::EngineRegister, offsetof(EngineBase, exceptionValue));
    pasm()->loadPtr(addr, PlatformAssembler::ScratchRegister);
    pasm()->storeAccumulator(Address(PlatformAssembler::ScratchRegister));
    addr.offset = offsetof(EngineBase, hasException);
    Q_STATIC_ASSERT(sizeof(QV4::EngineBase::hasException) == 1);
    pasm()->store8(TrustedImm32(1), addr);
}

void Assembler::setExceptionHandler(int offset)
{
    auto l = pasm()->storePtrWithPatch(TrustedImmPtr(nullptr), pasm()->exceptionHandlerAddress());
    pasm()->ehTargets.push_back({ l, offset });
}


void Assembler::clearExceptionHandler()
{
    pasm()->storePtr(TrustedImmPtr(nullptr), pasm()->exceptionHandlerAddress());
}

void Assembler::pushCatchContext(int name, int reg)
{
    pasm()->copyReg(pasm()->contextAddress(), regAddr(reg));
    prepareCallWithArgCount(2);
    passInt32AsArg(name, 1);
    passRegAsArg(CallData::Context, 0);
    IN_JIT_GENERATE_RUNTIME_CALL(Runtime::method_createCatchContext, ResultInAccumulator);
    pasm()->storeAccumulator(pasm()->contextAddress());
}

void Assembler::popContext(int reg)
{
    pasm()->copyReg(regAddr(reg), pasm()->contextAddress());
}

void Assembler::ret()
{
    pasm()->generateFunctionExit();
}

} // JIT namespace
} // QV4 namepsace

QT_END_NAMESPACE

#endif // V4_ENABLE_JIT
