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

#include "qv4global.h"
#include "qv4ir_p.h"
#include "qv4isel_p.h"
#include "qv4isel_util_p.h"
#include "qv4object.h"
#include "qmljs_runtime.h"

#include <QtCore/QHash>
#include <config.h>
#include <wtf/Vector.h>
#include <assembler/MacroAssembler.h>

namespace QQmlJS {
namespace MASM {

class Assembler : public JSC::MacroAssembler
{
public:
    Assembler(IR::Function* function);
#if CPU(X86)

#undef VALUE_FITS_IN_REGISTER
#undef ARGUMENTS_IN_REGISTERS

    static const RegisterID StackFrameRegister = JSC::X86Registers::ebp;
    static const RegisterID StackPointerRegister = JSC::X86Registers::esp;
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

    static const RegisterID StackFrameRegister = JSC::X86Registers::ebp;
    static const RegisterID StackPointerRegister = JSC::X86Registers::esp;
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

    static const RegisterID StackFrameRegister = JSC::ARMRegisters::r4;
    static const RegisterID StackPointerRegister = JSC::ARMRegisters::sp;
    static const RegisterID ContextRegister = JSC::ARMRegisters::r5;
    static const RegisterID ReturnValueRegister = JSC::ARMRegisters::r0;
    static const RegisterID ScratchRegister = JSC::ARMRegisters::r6;
    static const RegisterID IntegerOpRegister = JSC::X86Registers::r0;
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
        push(JSC::ARMRegisters::lr);
    }
    inline void platformLeaveStandardStackFrame()
    {
        pop(JSC::ARMRegisters::lr);
    }
#else
#error Argh.
#endif

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

    struct VoidType {};
    static const VoidType Void;


    typedef JSC::FunctionPtr FunctionPtr;

    struct CallToLink {
        Call call;
        FunctionPtr externalFunction;
        const char* functionName;
    };
    struct PointerToValue {
        PointerToValue(IR::Temp *value) : value(value) {}
        IR::Temp *value;
    };

    void callAbsolute(const char* functionName, FunctionPtr function) {
        CallToLink ctl;
        ctl.call = call();
        ctl.externalFunction = function;
        ctl.functionName = functionName;
        _callsToLink.append(ctl);
    }

    void registerBlock(IR::BasicBlock*);
    void jumpToBlock(IR::BasicBlock* current, IR::BasicBlock *target);
    void addPatch(IR::BasicBlock* targetBlock, Jump targetJump);

    Pointer loadTempAddress(RegisterID reg, IR::Temp *t);

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

#ifdef VALUE_FITS_IN_REGISTER
    void loadArgument(IR::Temp* temp, RegisterID dest)
    {
        if (!temp) {
            VM::Value undefined = VM::Value::undefinedValue();
            move(TrustedImm64(undefined.val), dest);
        } else {
            Pointer addr = loadTempAddress(dest, temp);
            load64(addr, dest);
        }
    }

    void loadArgument(IR::Const* c, RegisterID dest)
    {
        VM::Value v = convertToValue(c);
        move(TrustedImm64(v.val), dest);
    }

    void loadArgument(IR::Expr* expr, RegisterID dest)
    {
        if (!expr) {
            VM::Value undefined = VM::Value::undefinedValue();
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
    void loadArgument(IR::Expr*, RegisterID)
    {
        assert(!"unimplemented: expression in loadArgument");
    }
#endif

    void loadArgument(VM::String* string, RegisterID dest)
    {
        loadArgument(TrustedImmPtr(string), dest);
    }

    void loadArgument(TrustedImm32 imm32, RegisterID dest)
    {
        xorPtr(dest, dest);
        if (imm32.m_value)
            move(imm32, dest);
    }

    void storeArgument(RegisterID src, IR::Temp *temp)
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

    void push(VM::Value value)
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
        assert (temp.value);

        Pointer ptr = loadTempAddress(ScratchRegister, temp.value);
        push(ptr);
    }

    void push(IR::Temp* temp)
    {
        if (temp) {
            Address addr = loadTempAddress(ScratchRegister, temp);
            addr.offset += 4;
            push(addr);
            addr.offset -= 4;
            push(addr);
        } else {
            VM::Value undefined = VM::Value::undefinedValue();
            push(undefined);
        }
    }

    void push(IR::Const* c)
    {
        VM::Value v = convertToValue(c);
        push(v);
    }

    void push(IR::Expr* e)
    {
        if (!e) {
            VM::Value undefined = VM::Value::undefinedValue();
            push(undefined);
        } else if (IR::Const *c = e->asConst())
            push(c);
        else if (IR::Temp *t = e->asTemp()) {
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

    void push(VM::String* name)
    {
        push(TrustedImmPtr(name));
    }

    using JSC::MacroAssembler::loadDouble;
    void loadDouble(IR::Temp* temp, FPRegisterID dest)
    {
        Pointer ptr = loadTempAddress(ScratchRegister, temp);
        loadDouble(ptr, dest);
    }

    using JSC::MacroAssembler::storeDouble;
    void storeDouble(FPRegisterID source, IR::Temp* temp)
    {
        Pointer ptr = loadTempAddress(ScratchRegister, temp);
        storeDouble(source, ptr);
    }

    template <typename Result, typename Source>
    void copyValue(Result result, Source source);

    void storeValue(VM::Value value, Address destination)
    {
#ifdef VALUE_FITS_IN_REGISTER
        store64(TrustedImm64(value.val), destination);
#else
        store32(TrustedImm32(value.int_32), destination);
        destination.offset += 4;
        store32(TrustedImm32(value.tag), destination);
#endif
    }

    void storeValue(VM::Value value, IR::Temp* temp);

    void enterStandardStackFrame(int locals);
    void leaveStandardStackFrame(int locals);

    static inline int sizeOfArgument(VoidType)
    { return 0; }
    static inline int sizeOfArgument(RegisterID)
    { return RegisterSize; }
    static inline int sizeOfArgument(IR::Temp*)
    { return 8; } // Size of value
    static inline int sizeOfArgument(IR::Expr*)
    { return 8; } // Size of value
    static inline int sizeOfArgument(const Pointer&)
    { return sizeof(void*); }
    static inline int sizeOfArgument(VM::String* string)
    { return sizeof(string); }
    static inline int sizeOfArgument(const PointerToValue &)
    { return sizeof(void *); }
    static inline int sizeOfArgument(TrustedImmPtr)
    { return sizeof(void*); }
    static inline int sizeOfArgument(TrustedImm32)
    { return 4; }

    struct ArgumentLoader
    {
        ArgumentLoader(Assembler* _assembler, int totalNumberOfArguments)
            : assembler(_assembler)
            , stackSpaceForArguments(0)
            , currentRegisterIndex(qMin(totalNumberOfArguments - 1, RegisterArgumentCount - 1))
        {
        }

        template <typename T>
        void load(T argument)
        {
            if (currentRegisterIndex >= 0) {
                assembler->loadArgument(argument, registerForArgument(currentRegisterIndex));
                --currentRegisterIndex;
            } else {
                assembler->push(argument);
                stackSpaceForArguments += sizeOfArgument(argument);
            }
        }

        void load(VoidType)
        {
            if (currentRegisterIndex >= 0)
                --currentRegisterIndex;
        }

        Assembler *assembler;
        int stackSpaceForArguments;
        int currentRegisterIndex;
    };

    template <typename ArgRet, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    void generateFunctionCallImp(ArgRet r, const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
    {
        int totalNumberOfArgs = 5;

        // If necessary reserve space for the return value on the stack and
        // pass the pointer to it as the first hidden parameter.
        bool returnValueOnStack = false;
        int sizeOfReturnValueOnStack = sizeOfArgument(r);
        if (sizeOfReturnValueOnStack > RegisterSize) {
            sub32(TrustedImm32(sizeOfReturnValueOnStack), StackPointerRegister);
            ++totalNumberOfArgs;
            returnValueOnStack = true;
        }

        ArgumentLoader l(this, totalNumberOfArgs);
        l.load(arg5);
        l.load(arg4);
        l.load(arg3);
        l.load(arg2);
        l.load(arg1);

        if (returnValueOnStack) {
            // Load address of return value
            l.load(Pointer(StackPointerRegister, l.stackSpaceForArguments));
        }

        callAbsolute(functionName, function);

        int stackSizeToCorrect = l.stackSpaceForArguments;
        if (returnValueOnStack) {
            stackSizeToCorrect -= sizeof(void*); // Callee removed the hidden argument (address of return value)
            stackSizeToCorrect += sizeOfReturnValueOnStack;
        }

        storeArgument(ReturnValueRegister, r);

        if (stackSizeToCorrect)
            add32(TrustedImm32(stackSizeToCorrect), StackPointerRegister);
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
        VM::Value (*fallbackImplementation)(const VM::Value, const VM::Value, VM::ExecutionContext *);
        MemRegBinOp inlineMemRegOp;
        ImmRegBinOp inlineImmRegOp;
    };

    static const BinaryOperationInfo binaryOperations[QQmlJS::IR::LastAluOp + 1];

    void generateBinOp(IR::AluOp operation, IR::Temp* target, IR::Expr* left, IR::Expr* right);

    Jump inline_add32(Address addr, RegisterID reg)
    {
        return branchAdd32(Overflow, addr, reg);
    }

    Jump inline_add32(TrustedImm32 imm, RegisterID reg)
    {
        return branchAdd32(Overflow, imm, reg);
    }

    Jump inline_sub32(Address addr, RegisterID reg)
    {
        return branchSub32(Overflow, addr, reg);
    }

    Jump inline_sub32(TrustedImm32 imm, RegisterID reg)
    {
        return branchSub32(Overflow, imm, reg);
    }

    Jump inline_mul32(Address addr, RegisterID reg)
    {
        return branchMul32(Overflow, addr, reg);
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
        return Jump();
    }

    Jump inline_ushr32(TrustedImm32 imm, RegisterID reg)
    {
        imm.m_value &= 0x1f;
        urshift32(imm, reg);
        return Jump();
    }

    Jump inline_and32(Address addr, RegisterID reg)
    {
        and32(addr, reg);
        return Jump();
    }

    Jump inline_and32(TrustedImm32 imm, RegisterID reg)
    {
        and32(imm, reg);
        return Jump();
    }

    Jump inline_or32(Address addr, RegisterID reg)
    {
        or32(addr, reg);
        return Jump();
    }

    Jump inline_or32(TrustedImm32 imm, RegisterID reg)
    {
        or32(imm, reg);
        return Jump();
    }

    Jump inline_xor32(Address addr, RegisterID reg)
    {
        xor32(addr, reg);
        return Jump();
    }

    Jump inline_xor32(TrustedImm32 imm, RegisterID reg)
    {
        xor32(imm, reg);
        return Jump();
    }

    void link(VM::Function *vmFunc);

private:
    IR::Function* _function;
    QHash<IR::BasicBlock *, Label> _addrs;
    QHash<IR::BasicBlock *, QVector<Jump> > _patches;
    QList<CallToLink> _callsToLink;
};

class Q_V4_EXPORT InstructionSelection:
        protected IR::InstructionSelection,
        public EvalInstructionSelection
{
public:
    InstructionSelection(VM::ExecutionEngine *engine, IR::Module *module);
    ~InstructionSelection();

    virtual void run(VM::Function *vmFunction, IR::Function *function);

protected:
    virtual void callBuiltinInvalid(IR::Name *func, IR::ExprList *args, IR::Temp *result);
    virtual void callBuiltinTypeofMember(IR::Temp *base, const QString &name, IR::Temp *result);
    virtual void callBuiltinTypeofSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result);
    virtual void callBuiltinTypeofName(const QString &name, IR::Temp *result);
    virtual void callBuiltinTypeofValue(IR::Temp *value, IR::Temp *result);
    virtual void callBuiltinDeleteMember(IR::Temp *base, const QString &name, IR::Temp *result);
    virtual void callBuiltinDeleteSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result);
    virtual void callBuiltinDeleteName(const QString &name, IR::Temp *result);
    virtual void callBuiltinDeleteValue(IR::Temp *result);
    virtual void callBuiltinPostDecrementMember(IR::Temp *base, const QString &name, IR::Temp *result);
    virtual void callBuiltinPostDecrementSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result);
    virtual void callBuiltinPostDecrementName(const QString &name, IR::Temp *result);
    virtual void callBuiltinPostDecrementValue(IR::Temp *value, IR::Temp *result);
    virtual void callBuiltinPostIncrementMember(IR::Temp *base, const QString &name, IR::Temp *result);
    virtual void callBuiltinPostIncrementSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result);
    virtual void callBuiltinPostIncrementName(const QString &name, IR::Temp *result);
    virtual void callBuiltinPostIncrementValue(IR::Temp *value, IR::Temp *result);
    virtual void callBuiltinThrow(IR::Temp *arg);
    virtual void callBuiltinCreateExceptionHandler(IR::Temp *result, IR::Temp *contextTemp);
    virtual void callBuiltinDeleteExceptionHandler();
    virtual void callBuiltinGetException(IR::Temp *result);
    virtual void callBuiltinForeachIteratorObject(IR::Temp *arg, IR::Temp *result);
    virtual void callBuiltinForeachNextPropertyname(IR::Temp *arg, IR::Temp *result);
    virtual void callBuiltinPushWithScope(IR::Temp *arg);
    virtual void callBuiltinPushCatchScope(const QString &exceptionVarName);
    virtual void callBuiltinPopScope();
    virtual void callBuiltinDeclareVar(bool deletable, const QString &name);
    virtual void callBuiltinDefineGetterSetter(IR::Temp *object, const QString &name, IR::Temp *getter, IR::Temp *setter);
    virtual void callBuiltinDefineProperty(IR::Temp *object, const QString &name, IR::Temp *value);
    virtual void callBuiltinDefineArrayProperty(IR::Temp *object, int index, IR::Temp *value);
    virtual void callProperty(IR::Temp *base, const QString &name, IR::ExprList *args, IR::Temp *result);
    virtual void callSubscript(IR::Temp *base, IR::Temp *index, IR::ExprList *args, IR::Temp *result);
    virtual void callValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result);
    virtual void loadThisObject(IR::Temp *temp);
    virtual void loadConst(IR::Const *sourceConst, IR::Temp *targetTemp);
    virtual void loadString(const QString &str, IR::Temp *targetTemp);
    virtual void loadRegexp(IR::RegExp *sourceRegexp, IR::Temp *targetTemp);
    virtual void getActivationProperty(const QString &name, IR::Temp *temp);
    virtual void setActivationProperty(IR::Temp *source, const QString &targetName);
    virtual void initClosure(IR::Closure *closure, IR::Temp *target);
    virtual void getProperty(IR::Temp *base, const QString &name, IR::Temp *target);
    virtual void setProperty(IR::Temp *source, IR::Temp *targetBase, const QString &targetName);
    virtual void getElement(IR::Temp *base, IR::Temp *index, IR::Temp *target);
    virtual void setElement(IR::Expr *source, IR::Temp *targetBase, IR::Temp *targetIndex);
    virtual void copyValue(IR::Temp *sourceTemp, IR::Temp *targetTemp);
    virtual void unop(IR::AluOp oper, IR::Temp *sourceTemp, IR::Temp *targetTemp);
    virtual void binop(IR::AluOp oper, IR::Expr *leftSource, IR::Expr *rightSource, IR::Temp *target);
    virtual void inplaceNameOp(IR::AluOp oper, IR::Expr *sourceExpr, const QString &targetName);
    virtual void inplaceElementOp(IR::AluOp oper, IR::Expr *sourceExpr, IR::Temp *targetBaseTemp, IR::Temp *targetIndexTemp);
    virtual void inplaceMemberOp(IR::AluOp oper, IR::Expr *source, IR::Temp *targetBase, const QString &targetName);

    typedef Assembler::Address Address;
    typedef Assembler::Pointer Pointer;

    Address addressForArgument(int index) const
    {
        if (index < Assembler::RegisterArgumentCount)
            return Address(_as->registerForArgument(index), 0);

        // StackFrameRegister points to its old value on the stack, and above
        // it we have the return address, hence the need to step over two
        // values before reaching the first argument.
        return Address(Assembler::StackFrameRegister, (index - Assembler::RegisterArgumentCount + 2) * sizeof(void*));
    }

    // Some run-time functions take (Value* args, int argc). This function is for populating
    // the args.
    Pointer argumentAddressForCall(int argument)
    {
        const int index = _function->maxNumberOfArguments - argument;
        return Pointer(Assembler::StackFrameRegister, sizeof(VM::Value) * (-index)
                                                      - sizeof(void*) // size of ebp
                       );
    }
    Pointer baseAddressForCallArguments()
    {
        return argumentAddressForCall(0);
    }

    VM::String *identifier(const QString &s);
    virtual void constructActivationProperty(IR::Name *func, IR::ExprList *args, IR::Temp *result);
    virtual void constructProperty(IR::Temp *base, const QString &name, IR::ExprList *args, IR::Temp *result);
    virtual void constructValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result);

    virtual void visitJump(IR::Jump *);
    virtual void visitCJump(IR::CJump *);
    virtual void visitRet(IR::Ret *);

private:
    #define isel_stringIfyx(s) #s
    #define isel_stringIfy(s) isel_stringIfyx(s)

    #define generateFunctionCall(t, function, ...) \
        _as->generateFunctionCallImp(t, isel_stringIfy(function), function, __VA_ARGS__)

    int prepareVariableArguments(IR::ExprList* args);

    typedef void (*ActivationMethod)(VM::ExecutionContext *, VM::Value *result, VM::String *name, VM::Value *args, int argc);
    void callRuntimeMethodImp(IR::Temp *result, const char* name, ActivationMethod method, IR::Expr *base, IR::ExprList *args);
#define callRuntimeMethod(result, function, ...) \
    callRuntimeMethodImp(result, isel_stringIfy(function), function, __VA_ARGS__)

    uint addLookup(VM::String *name);

    IR::BasicBlock *_block;
    IR::Function* _function;
    VM::Function* _vmFunction;
    QVector<VM::Lookup> _lookups;
    Assembler* _as;
};

class Q_V4_EXPORT ISelFactory: public EvalISelFactory
{
public:
    virtual ~ISelFactory() {}
    virtual EvalInstructionSelection *create(VM::ExecutionEngine *engine, IR::Module *module)
    { return new InstructionSelection(engine, module); }
};

} // end of namespace MASM
} // end of namespace QQmlJS

#endif // QV4ISEL_MASM_P_H
