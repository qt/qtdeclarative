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
#ifndef QV4ISEL_MASM_P_H
#define QV4ISEL_MASM_P_H

#include "qv4global_p.h"
#include "qv4jsir_p.h"
#include "qv4isel_p.h"
#include "qv4isel_util_p.h"
#include "qv4object_p.h"
#include "qv4runtime_p.h"
#include "qv4lookup_p.h"

#include <QtCore/QHash>
#include <config.h>
#include <wtf/Vector.h>
#include <assembler/MacroAssembler.h>

namespace QQmlJS {
namespace MASM {

class Assembler : public JSC::MacroAssembler
{
public:
    Assembler(V4IR::Function* function, QV4::Function *vmFunction, QV4::ExecutionEngine *engine);
#if CPU(X86)

#undef VALUE_FITS_IN_REGISTER
#undef ARGUMENTS_IN_REGISTERS

#if OS(WINDOWS)
    // Returned in EAX:EDX pair
#define RETURN_VALUE_IN_REGISTER
#else
#undef RETURN_VALUE_IN_REGISTER
#endif

#define HAVE_ALU_OPS_WITH_MEM_OPERAND 1

    static const RegisterID StackFrameRegister = JSC::X86Registers::ebp;
    static const RegisterID StackPointerRegister = JSC::X86Registers::esp;
    static const RegisterID LocalsRegister = JSC::X86Registers::edi;
    static const RegisterID ContextRegister = JSC::X86Registers::esi;
    static const RegisterID ReturnValueRegister = JSC::X86Registers::eax;
    static const RegisterID ScratchRegister = JSC::X86Registers::ecx;
    static const RegisterID IntegerOpRegister = JSC::X86Registers::eax;
    static const FPRegisterID FPGpr0 = JSC::X86Registers::xmm0;

    static const int RegisterSize = 4;

    static const int RegisterArgumentCount = 0;
    static RegisterID registerForArgument(int)
    {
        assert(false);
        // Not reached.
        return JSC::X86Registers::eax;
    }

    inline void platformEnterStandardStackFrame() {}
    inline void platformLeaveStandardStackFrame() {}
#elif CPU(X86_64)

#define VALUE_FITS_IN_REGISTER
#define ARGUMENTS_IN_REGISTERS
#define RETURN_VALUE_IN_REGISTER
#define HAVE_ALU_OPS_WITH_MEM_OPERAND 1

    static const RegisterID StackFrameRegister = JSC::X86Registers::ebp;
    static const RegisterID StackPointerRegister = JSC::X86Registers::esp;
    static const RegisterID LocalsRegister = JSC::X86Registers::r12;
    static const RegisterID ContextRegister = JSC::X86Registers::r14;
    static const RegisterID ReturnValueRegister = JSC::X86Registers::eax;
    static const RegisterID ScratchRegister = JSC::X86Registers::r10;
    static const RegisterID IntegerOpRegister = JSC::X86Registers::eax;
    static const FPRegisterID FPGpr0 = JSC::X86Registers::xmm0;

    static const int RegisterSize = 8;

    static const int RegisterArgumentCount = 6;
    static RegisterID registerForArgument(int index)
    {
        static RegisterID regs[RegisterArgumentCount] = {
            JSC::X86Registers::edi,
            JSC::X86Registers::esi,
            JSC::X86Registers::edx,
            JSC::X86Registers::ecx,
            JSC::X86Registers::r8,
            JSC::X86Registers::r9
        };
        assert(index >= 0 && index < RegisterArgumentCount);
        return regs[index];
    };
    inline void platformEnterStandardStackFrame() {}
    inline void platformLeaveStandardStackFrame() {}
#elif CPU(ARM)

#undef VALUE_FITS_IN_REGISTER
#define ARGUMENTS_IN_REGISTERS
#undef RETURN_VALUE_IN_REGISTER
#undef HAVE_ALU_OPS_WITH_MEM_OPERAND

    static const RegisterID StackFrameRegister = JSC::ARMRegisters::r4;
    static const RegisterID StackPointerRegister = JSC::ARMRegisters::sp;
    static const RegisterID LocalsRegister = JSC::ARMRegisters::r7;
    static const RegisterID ContextRegister = JSC::ARMRegisters::r5;
    static const RegisterID ReturnValueRegister = JSC::ARMRegisters::r0;
    static const RegisterID ScratchRegister = JSC::ARMRegisters::r6;
    static const RegisterID IntegerOpRegister = JSC::ARMRegisters::r0;
    static const FPRegisterID FPGpr0 = JSC::ARMRegisters::d0;

    static const int RegisterSize = 4;

    static const RegisterID RegisterArgument1 = JSC::ARMRegisters::r0;
    static const RegisterID RegisterArgument2 = JSC::ARMRegisters::r1;
    static const RegisterID RegisterArgument3 = JSC::ARMRegisters::r2;
    static const RegisterID RegisterArgument4 = JSC::ARMRegisters::r3;

    static const int RegisterArgumentCount = 4;
    static RegisterID registerForArgument(int index)
    {
        assert(index >= 0 && index < RegisterArgumentCount);
        return static_cast<RegisterID>(JSC::ARMRegisters::r0 + index);
    };
    inline void platformEnterStandardStackFrame()
    {
        // Move the register arguments onto the stack as if they were
        // pushed by the caller, just like on ia32. This gives us consistent
        // access to the parameters if we need to.
        push(JSC::ARMRegisters::r3);
        push(JSC::ARMRegisters::r2);
        push(JSC::ARMRegisters::r1);
        push(JSC::ARMRegisters::r0);
        push(JSC::ARMRegisters::lr);
    }
    inline void platformLeaveStandardStackFrame()
    {
        pop(JSC::ARMRegisters::lr);
        pop(JSC::ARMRegisters::r0);
        pop(JSC::ARMRegisters::r1);
        pop(JSC::ARMRegisters::r2);
        pop(JSC::ARMRegisters::r3);
    }
#else
#error Argh.
#endif
    static const int calleeSavedRegisterCount;

    // Explicit type to allow distinguishing between
    // pushing an address itself or the value it points
    // to onto the stack when calling functions.
    struct Pointer : public Address
    {
        explicit Pointer(const Address& addr)
            : Address(addr)
        {}
        explicit Pointer(RegisterID reg, int32_t offset)
            : Address(reg, offset)
        {}
    };

    struct VoidType { VoidType() {} };
    static const VoidType Void;


    typedef JSC::FunctionPtr FunctionPtr;

    struct CallToLink {
        Call call;
        FunctionPtr externalFunction;
        const char* functionName;
    };
    struct PointerToValue {
        PointerToValue(V4IR::Temp *value) : value(value) {}
        V4IR::Temp *value;
    };
    struct Reference {
        Reference(V4IR::Temp *value) : value(value) {}
        V4IR::Temp *value;
    };

    struct ReentryBlock {
        ReentryBlock(V4IR::BasicBlock *b) : block(b) {}
        V4IR::BasicBlock *block;
    };

    void callAbsolute(const char* functionName, FunctionPtr function) {
        CallToLink ctl;
        ctl.call = call();
        ctl.externalFunction = function;
        ctl.functionName = functionName;
        _callsToLink.append(ctl);
    }

    void registerBlock(V4IR::BasicBlock*);
    void jumpToBlock(V4IR::BasicBlock* current, V4IR::BasicBlock *target);
    void addPatch(V4IR::BasicBlock* targetBlock, Jump targetJump);
    void addPatch(DataLabelPtr patch, Label target);
    void addPatch(DataLabelPtr patch, V4IR::BasicBlock *target);

    Pointer loadTempAddress(RegisterID reg, V4IR::Temp *t);

    void loadArgument(RegisterID source, RegisterID dest)
    {
        move(source, dest);
    }

    void loadArgument(TrustedImmPtr ptr, RegisterID dest)
    {
        move(TrustedImmPtr(ptr), dest);
    }

    void loadArgument(const Pointer& ptr, RegisterID dest)
    {
        addPtr(TrustedImm32(ptr.offset), ptr.base, dest);
    }

    void loadArgument(PointerToValue temp, RegisterID dest)
    {
        if (!temp.value) {
            loadArgument(TrustedImmPtr(0), dest);
        } else {
            Pointer addr = loadTempAddress(dest, temp.value);
            loadArgument(addr, dest);
        }
    }

    void loadArgument(Reference temp, RegisterID dest)
    {
        assert(temp.value);
        Pointer addr = loadTempAddress(dest, temp.value);
        loadArgument(addr, dest);
    }

    void loadArgument(ReentryBlock block, RegisterID dest)
    {
        assert(block.block);
        DataLabelPtr patch = moveWithPatch(TrustedImmPtr(0), dest);
        addPatch(patch, block.block);
    }

#ifdef VALUE_FITS_IN_REGISTER
    void loadArgument(V4IR::Temp* temp, RegisterID dest)
    {
        if (!temp) {
            QV4::Value undefined = QV4::Value::undefinedValue();
            move(TrustedImm64(undefined.val), dest);
        } else {
            Pointer addr = loadTempAddress(dest, temp);
            load64(addr, dest);
        }
    }

    void loadArgument(V4IR::Const* c, RegisterID dest)
    {
        QV4::Value v = convertToValue(c);
        move(TrustedImm64(v.val), dest);
    }

    void loadArgument(V4IR::Expr* expr, RegisterID dest)
    {
        if (!expr) {
            QV4::Value undefined = QV4::Value::undefinedValue();
            move(TrustedImm64(undefined.val), dest);
        } else if (expr->asTemp()){
            loadArgument(expr->asTemp(), dest);
        } else if (expr->asConst()) {
            loadArgument(expr->asConst(), dest);
        } else {
            assert(!"unimplemented expression type in loadArgument");
        }
    }
#else
    void loadArgument(V4IR::Expr*, RegisterID)
    {
        assert(!"unimplemented: expression in loadArgument");
    }
#endif

    void loadArgument(QV4::String* string, RegisterID dest)
    {
        loadArgument(TrustedImmPtr(string), dest);
    }

    void loadArgument(TrustedImm32 imm32, RegisterID dest)
    {
        xorPtr(dest, dest);
        if (imm32.m_value)
            move(imm32, dest);
    }

    void storeArgument(RegisterID src, V4IR::Temp *temp)
    {
        if (temp) {
            Pointer addr = loadTempAddress(ScratchRegister, temp);
#ifdef VALUE_FITS_IN_REGISTER
            store64(src, addr);
#else
            // If the value doesn't fit into a register, then the
            // register contains the address to where the argument
            // (return value) is stored. Copy it from there.
            copyValue(addr, Pointer(src, 0));
#endif
        }
    }

#ifdef VALUE_FITS_IN_REGISTER
    void storeArgument(RegisterID src, const Pointer &dest)
    {
        store64(src, dest);
    }
#endif

    void storeArgument(RegisterID src, RegisterID dest)
    {
        move(src, dest);
    }

    void storeArgument(RegisterID, VoidType)
    {
    }

    using JSC::MacroAssembler::push;

    void push(const Pointer& ptr)
    {
        addPtr(TrustedImm32(ptr.offset), ptr.base, ScratchRegister);
        push(ScratchRegister);
    }

    void push(QV4::Value value)
    {
#ifdef VALUE_FITS_IN_REGISTER
        move(TrustedImm64(value.val), ScratchRegister);
        push(ScratchRegister);
#else
        move(TrustedImm32(value.tag), ScratchRegister);
        push(ScratchRegister);
        move(TrustedImm32(value.int_32), ScratchRegister);
        push(ScratchRegister);
#endif
    }

    void push(PointerToValue temp)
    {
        if (temp.value) {
            Pointer ptr = loadTempAddress(ScratchRegister, temp.value);
            push(ptr);
        } else {
            push(TrustedImmPtr(0));
        }
    }

    void push(Reference temp)
    {
        assert (temp.value);

        Pointer ptr = loadTempAddress(ScratchRegister, temp.value);
        push(ptr);
    }

    void push(ReentryBlock block)
    {
        assert(block.block);
        DataLabelPtr patch = moveWithPatch(TrustedImmPtr(0), ScratchRegister);
        push(ScratchRegister);
        addPatch(patch, block.block);
    }

    void push(V4IR::Temp* temp)
    {
        if (temp) {
            Address addr = loadTempAddress(ScratchRegister, temp);
            addr.offset += 4;
            push(addr);
            addr.offset -= 4;
            push(addr);
        } else {
            QV4::Value undefined = QV4::Value::undefinedValue();
            push(undefined);
        }
    }

    void push(V4IR::Const* c)
    {
        QV4::Value v = convertToValue(c);
        push(v);
    }

    void push(V4IR::Expr* e)
    {
        if (!e) {
            QV4::Value undefined = QV4::Value::undefinedValue();
            push(undefined);
        } else if (V4IR::Const *c = e->asConst())
            push(c);
        else if (V4IR::Temp *t = e->asTemp()) {
            push(t);
        } else {
            assert(!"Trying to push an expression that is not a Temp or Const");
        }
    }

    void push(TrustedImmPtr ptr)
    {
        move(TrustedImmPtr(ptr), ScratchRegister);
        push(ScratchRegister);
    }

    void push(QV4::String* name)
    {
        push(TrustedImmPtr(name));
    }

    using JSC::MacroAssembler::loadDouble;
    void loadDouble(V4IR::Temp* temp, FPRegisterID dest)
    {
        Pointer ptr = loadTempAddress(ScratchRegister, temp);
        loadDouble(ptr, dest);
    }

    using JSC::MacroAssembler::storeDouble;
    void storeDouble(FPRegisterID source, V4IR::Temp* temp)
    {
        Pointer ptr = loadTempAddress(ScratchRegister, temp);
        storeDouble(source, ptr);
    }

    template <typename Result, typename Source>
    void copyValue(Result result, Source source);
    template <typename Result>
    void copyValue(Result result, V4IR::Expr* source);

    void storeValue(QV4::Value value, Address destination)
    {
#ifdef VALUE_FITS_IN_REGISTER
        store64(TrustedImm64(value.val), destination);
#else
        store32(TrustedImm32(value.int_32), destination);
        destination.offset += 4;
        store32(TrustedImm32(value.tag), destination);
#endif
    }

    void storeValue(QV4::Value value, V4IR::Temp* temp);

    void enterStandardStackFrame(int locals);
    void leaveStandardStackFrame(int locals);

    static inline int sizeOfArgument(VoidType)
    { return 0; }
    static inline int sizeOfArgument(RegisterID)
    { return RegisterSize; }
    static inline int sizeOfArgument(V4IR::Temp*)
    { return 8; } // Size of value
    static inline int sizeOfArgument(V4IR::Expr*)
    { return 8; } // Size of value
    static inline int sizeOfArgument(const Pointer&)
    { return sizeof(void*); }
    static inline int sizeOfArgument(QV4::String*)
    { return sizeof(QV4::String*); }
    static inline int sizeOfArgument(const PointerToValue &)
    { return sizeof(void *); }
    static inline int sizeOfArgument(const Reference &)
    { return sizeof(void *); }
    static inline int sizeOfArgument(const ReentryBlock &)
    { return sizeof(void *); }
    static inline int sizeOfArgument(TrustedImmPtr)
    { return sizeof(void*); }
    static inline int sizeOfArgument(TrustedImm32)
    { return 4; }

    template <int argumentNumber, typename T>
    int loadArgumentOnStackOrRegister(const T &value)
    {
        if (argumentNumber < RegisterArgumentCount) {
            loadArgument(value, registerForArgument(argumentNumber));
            return 0;
        } else {
            push(value);
            return sizeOfArgument(value);
        }
    }

    template <int argumentNumber>
    int loadArgumentOnStackOrRegister(const VoidType &value)
    {
        Q_UNUSED(value);
        return 0;
    }

    template <typename ArgRet, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
    void generateFunctionCallImp(ArgRet r, const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
    {
        int totalNumberOfArgs = 6;

        // If necessary reserve space for the return value on the stack and
        // pass the pointer to it as the first hidden parameter.
        bool returnValueOnStack = false;
        int sizeOfReturnValueOnStack = sizeOfArgument(r);
        if (sizeOfReturnValueOnStack > RegisterSize) {
            sub32(TrustedImm32(sizeOfReturnValueOnStack), StackPointerRegister);
            ++totalNumberOfArgs;
            returnValueOnStack = true;
        }

        int stackSpaceUsedForArgs = 0;
        stackSpaceUsedForArgs += loadArgumentOnStackOrRegister<5>(arg6);
        stackSpaceUsedForArgs += loadArgumentOnStackOrRegister<4>(arg5);
        stackSpaceUsedForArgs += loadArgumentOnStackOrRegister<3>(arg4);
        stackSpaceUsedForArgs += loadArgumentOnStackOrRegister<2>(arg3);
        stackSpaceUsedForArgs += loadArgumentOnStackOrRegister<1>(arg2);
        stackSpaceUsedForArgs += loadArgumentOnStackOrRegister<0>(arg1);

        if (returnValueOnStack) {
            // Load address of return value
            push(Pointer(StackPointerRegister, stackSpaceUsedForArgs));
        }

        callAbsolute(functionName, function);

        int stackSizeToCorrect = stackSpaceUsedForArgs;
        if (returnValueOnStack)
            stackSizeToCorrect += sizeOfReturnValueOnStack;

        storeArgument(ReturnValueRegister, r);

        if (stackSizeToCorrect)
            add32(TrustedImm32(stackSizeToCorrect), StackPointerRegister);
    }

    template <typename ArgRet, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    void generateFunctionCallImp(ArgRet r, const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
    {
        generateFunctionCallImp(r, functionName, function, arg1, arg2, arg3, arg4, arg5, VoidType());
    }

    template <typename ArgRet, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    void generateFunctionCallImp(ArgRet r, const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
    {
        generateFunctionCallImp(r, functionName, function, arg1, arg2, arg3, arg4, VoidType());
    }

    template <typename ArgRet, typename Arg1, typename Arg2, typename Arg3>
    void generateFunctionCallImp(ArgRet r, const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3)
    {
        generateFunctionCallImp(r, functionName, function, arg1, arg2, arg3, VoidType(), VoidType());
    }

    template <typename ArgRet, typename Arg1, typename Arg2>
    void generateFunctionCallImp(ArgRet r, const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2)
    {
        generateFunctionCallImp(r, functionName, function, arg1, arg2, VoidType(), VoidType(), VoidType());
    }

    template <typename ArgRet, typename Arg1>
    void generateFunctionCallImp(ArgRet r, const char* functionName, FunctionPtr function, Arg1 arg1)
    {
        generateFunctionCallImp(r, functionName, function, arg1, VoidType(), VoidType(), VoidType(), VoidType());
    }

    typedef Jump (Assembler::*MemRegBinOp)(Address, RegisterID);
    typedef Jump (Assembler::*ImmRegBinOp)(TrustedImm32, RegisterID);

    struct BinaryOperationInfo {
        const char *name;
        QV4::BinOp fallbackImplementation;
        MemRegBinOp inlineMemRegOp;
        ImmRegBinOp inlineImmRegOp;
    };

    static const BinaryOperationInfo binaryOperations[QQmlJS::V4IR::LastAluOp + 1];

    void generateBinOp(V4IR::AluOp operation, V4IR::Temp* target, V4IR::Temp* left, V4IR::Temp* right);

    Jump inline_add32(Address addr, RegisterID reg)
    {
#if HAVE(ALU_OPS_WITH_MEM_OPERAND)
        return branchAdd32(Overflow, addr, reg);
#else
        load32(addr, ScratchRegister);
        return branchAdd32(Overflow, ScratchRegister, reg);
#endif
    }

    Jump inline_add32(TrustedImm32 imm, RegisterID reg)
    {
        return branchAdd32(Overflow, imm, reg);
    }

    Jump inline_sub32(Address addr, RegisterID reg)
    {
#if HAVE(ALU_OPS_WITH_MEM_OPERAND)
        return branchSub32(Overflow, addr, reg);
#else
        load32(addr, ScratchRegister);
        return branchSub32(Overflow, ScratchRegister, reg);
#endif
    }

    Jump inline_sub32(TrustedImm32 imm, RegisterID reg)
    {
        return branchSub32(Overflow, imm, reg);
    }

    Jump inline_mul32(Address addr, RegisterID reg)
    {
#if HAVE(ALU_OPS_WITH_MEM_OPERAND)
        return branchMul32(Overflow, addr, reg);
#else
        load32(addr, ScratchRegister);
        return branchMul32(Overflow, ScratchRegister, reg);
#endif
    }

    Jump inline_mul32(TrustedImm32 imm, RegisterID reg)
    {
        return branchMul32(Overflow, imm, reg, reg);
    }

    Jump inline_shl32(Address addr, RegisterID reg)
    {
        load32(addr, ScratchRegister);
        and32(TrustedImm32(0x1f), ScratchRegister);
        lshift32(ScratchRegister, reg);
        return Jump();
    }

    Jump inline_shl32(TrustedImm32 imm, RegisterID reg)
    {
        imm.m_value &= 0x1f;
        lshift32(imm, reg);
        return Jump();
    }

    Jump inline_shr32(Address addr, RegisterID reg)
    {
        load32(addr, ScratchRegister);
        and32(TrustedImm32(0x1f), ScratchRegister);
        rshift32(ScratchRegister, reg);
        return Jump();
    }

    Jump inline_shr32(TrustedImm32 imm, RegisterID reg)
    {
        imm.m_value &= 0x1f;
        rshift32(imm, reg);
        return Jump();
    }

    Jump inline_ushr32(Address addr, RegisterID reg)
    {
        load32(addr, ScratchRegister);
        and32(TrustedImm32(0x1f), ScratchRegister);
        urshift32(ScratchRegister, reg);
        return branchTest32(Signed, reg, reg);
    }

    Jump inline_ushr32(TrustedImm32 imm, RegisterID reg)
    {
        imm.m_value &= 0x1f;
        urshift32(imm, reg);
        return branchTest32(Signed, reg, reg);
    }

    Jump inline_and32(Address addr, RegisterID reg)
    {
#if HAVE(ALU_OPS_WITH_MEM_OPERAND)
        and32(addr, reg);
#else
        load32(addr, ScratchRegister);
        and32(ScratchRegister, reg);
#endif
        return Jump();
    }

    Jump inline_and32(TrustedImm32 imm, RegisterID reg)
    {
        and32(imm, reg);
        return Jump();
    }

    Jump inline_or32(Address addr, RegisterID reg)
    {
#if HAVE(ALU_OPS_WITH_MEM_OPERAND)
        or32(addr, reg);
#else
        load32(addr, ScratchRegister);
        or32(ScratchRegister, reg);
#endif
        return Jump();
    }

    Jump inline_or32(TrustedImm32 imm, RegisterID reg)
    {
        or32(imm, reg);
        return Jump();
    }

    Jump inline_xor32(Address addr, RegisterID reg)
    {
#if HAVE(ALU_OPS_WITH_MEM_OPERAND)
        xor32(addr, reg);
#else
        load32(addr, ScratchRegister);
        xor32(ScratchRegister, reg);
#endif
        return Jump();
    }

    Jump inline_xor32(TrustedImm32 imm, RegisterID reg)
    {
        xor32(imm, reg);
        return Jump();
    }

    void link(QV4::Function *vmFunc);

private:
    V4IR::Function *_function;
    QV4::Function *_vmFunction;
    QHash<V4IR::BasicBlock *, Label> _addrs;
    QHash<V4IR::BasicBlock *, QVector<Jump> > _patches;
    QList<CallToLink> _callsToLink;

    struct DataLabelPatch {
        DataLabelPtr dataLabel;
        Label target;
    };
    QList<DataLabelPatch> _dataLabelPatches;

    QHash<V4IR::BasicBlock *, QVector<DataLabelPtr> > _labelPatches;

    QV4::ExecutionEngine *_engine;
};

class Q_QML_EXPORT InstructionSelection:
        protected V4IR::InstructionSelection,
        public EvalInstructionSelection
{
public:
    InstructionSelection(QV4::ExecutionEngine *engine, V4IR::Module *module);
    ~InstructionSelection();

    virtual void run(QV4::Function *vmFunction, V4IR::Function *function);

protected:
    virtual void callBuiltinInvalid(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void callBuiltinTypeofMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinTypeofSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result);
    virtual void callBuiltinTypeofName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinTypeofValue(V4IR::Temp *value, V4IR::Temp *result);
    virtual void callBuiltinDeleteMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinDeleteSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result);
    virtual void callBuiltinDeleteName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinDeleteValue(V4IR::Temp *result);
    virtual void callBuiltinPostDecrementMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinPostDecrementSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result);
    virtual void callBuiltinPostDecrementName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinPostDecrementValue(V4IR::Temp *value, V4IR::Temp *result);
    virtual void callBuiltinPostIncrementMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result);
    virtual void callBuiltinPostIncrementSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result);
    virtual void callBuiltinPostIncrementName(const QString &name, V4IR::Temp *result);
    virtual void callBuiltinPostIncrementValue(V4IR::Temp *value, V4IR::Temp *result);
    virtual void callBuiltinThrow(V4IR::Temp *arg);
    virtual void callBuiltinFinishTry();
    virtual void callBuiltinForeachIteratorObject(V4IR::Temp *arg, V4IR::Temp *result);
    virtual void callBuiltinForeachNextPropertyname(V4IR::Temp *arg, V4IR::Temp *result);
    virtual void callBuiltinPushWithScope(V4IR::Temp *arg);
    virtual void callBuiltinPopScope();
    virtual void callBuiltinDeclareVar(bool deletable, const QString &name);
    virtual void callBuiltinDefineGetterSetter(V4IR::Temp *object, const QString &name, V4IR::Temp *getter, V4IR::Temp *setter);
    virtual void callBuiltinDefineProperty(V4IR::Temp *object, const QString &name, V4IR::Temp *value);
    virtual void callBuiltinDefineArray(V4IR::Temp *result, V4IR::ExprList *args);
    virtual void callProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void callSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void callValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void loadThisObject(V4IR::Temp *temp);
    virtual void loadConst(V4IR::Const *sourceConst, V4IR::Temp *targetTemp);
    virtual void loadString(const QString &str, V4IR::Temp *targetTemp);
    virtual void loadRegexp(V4IR::RegExp *sourceRegexp, V4IR::Temp *targetTemp);
    virtual void getActivationProperty(const V4IR::Name *name, V4IR::Temp *temp);
    virtual void setActivationProperty(V4IR::Temp *source, const QString &targetName);
    virtual void initClosure(V4IR::Closure *closure, V4IR::Temp *target);
    virtual void getProperty(V4IR::Temp *base, const QString &name, V4IR::Temp *target);
    virtual void setProperty(V4IR::Temp *source, V4IR::Temp *targetBase, const QString &targetName);
    virtual void getElement(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *target);
    virtual void setElement(V4IR::Temp *source, V4IR::Temp *targetBase, V4IR::Temp *targetIndex);
    virtual void copyValue(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp);
    virtual void unop(V4IR::AluOp oper, V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp);
    virtual void binop(V4IR::AluOp oper, V4IR::Temp *leftSource, V4IR::Temp *rightSource, V4IR::Temp *target);
    virtual void inplaceNameOp(V4IR::AluOp oper, V4IR::Temp *rightSource, const QString &targetName);
    virtual void inplaceElementOp(V4IR::AluOp oper, V4IR::Temp *source, V4IR::Temp *targetBaseTemp, V4IR::Temp *targetIndexTemp);
    virtual void inplaceMemberOp(V4IR::AluOp oper, V4IR::Temp *source, V4IR::Temp *targetBase, const QString &targetName);

    typedef Assembler::Address Address;
    typedef Assembler::Pointer Pointer;

    Address addressForArgument(int index) const
    {
        // StackFrameRegister points to its old value on the stack, and above
        // it we have the return address, hence the need to step over two
        // values before reaching the first argument.
        return Address(Assembler::StackFrameRegister, (index + 2) * sizeof(void*));
    }

    // Some run-time functions take (Value* args, int argc). This function is for populating
    // the args.
    Pointer argumentAddressForCall(int argument)
    {
        const int index = _function->maxNumberOfArguments - argument;
        return Pointer(Assembler::LocalsRegister, sizeof(QV4::Value) * (-index)
                                                      - sizeof(void*) // size of ebp
                                                  - sizeof(void*) * Assembler::calleeSavedRegisterCount
                       );
    }
    Pointer baseAddressForCallArguments()
    {
        return argumentAddressForCall(0);
    }

    QV4::String *identifier(const QString &s);
    virtual void constructActivationProperty(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void constructProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result);
    virtual void constructValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result);

    virtual void visitJump(V4IR::Jump *);
    virtual void visitCJump(V4IR::CJump *);
    virtual void visitRet(V4IR::Ret *);
    virtual void visitTry(V4IR::Try *);

private:
    #define isel_stringIfyx(s) #s
    #define isel_stringIfy(s) isel_stringIfyx(s)

    #define generateFunctionCall(t, function, ...) \
        _as->generateFunctionCallImp(t, isel_stringIfy(function), function, __VA_ARGS__)

    int prepareVariableArguments(V4IR::ExprList* args);

    typedef void (*ActivationMethod)(QV4::ExecutionContext *, QV4::Value *result, QV4::String *name, QV4::Value *args, int argc);
    void callRuntimeMethodImp(V4IR::Temp *result, const char* name, ActivationMethod method, V4IR::Expr *base, V4IR::ExprList *args);
#define callRuntimeMethod(result, function, ...) \
    callRuntimeMethodImp(result, isel_stringIfy(function), function, __VA_ARGS__)

    uint addLookup(QV4::String *name);
    uint addGlobalLookup(QV4::String *name);

    V4IR::BasicBlock *_block;
    V4IR::Function* _function;
    QV4::Function* _vmFunction;
    QVector<QV4::Lookup> _lookups;
    Assembler* _as;
    QSet<V4IR::BasicBlock*> _reentryBlocks;
};

class Q_QML_EXPORT ISelFactory: public EvalISelFactory
{
public:
    virtual ~ISelFactory() {}
    virtual EvalInstructionSelection *create(QV4::ExecutionEngine *engine, V4IR::Module *module)
    { return new InstructionSelection(engine, module); }
};

} // end of namespace MASM
} // end of namespace QQmlJS

#endif // QV4ISEL_MASM_P_H
