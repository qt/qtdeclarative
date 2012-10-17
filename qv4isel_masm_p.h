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

#include "qv4ir_p.h"
#include "qmljs_objects.h"
#include "qmljs_runtime.h"

#include <QtCore/QHash>
#include <config.h>
#include <wtf/Vector.h>
#include <assembler/MacroAssembler.h>

namespace QQmlJS {
namespace MASM {

class InstructionSelection: protected IR::StmtVisitor, public JSC::MacroAssembler
{
public:
    InstructionSelection(VM::ExecutionEngine *engine, IR::Module *module, uchar *code);
    ~InstructionSelection();

    void operator()(IR::Function *function);

protected:
#if CPU(X86)
    static const RegisterID StackFrameRegister = JSC::X86Registers::ebp;
    static const RegisterID StackPointerRegister = JSC::X86Registers::esp;
    static const RegisterID ContextRegister = JSC::X86Registers::esi;
    static const RegisterID ReturnValueRegister = JSC::X86Registers::eax;
    static const RegisterID Gpr0 = JSC::X86Registers::eax;
    static const RegisterID Gpr1 = JSC::X86Registers::ecx;
    static const RegisterID Gpr2 = JSC::X86Registers::edx;
    static const RegisterID Gpr3 = JSC::X86Registers::esi;
    static const RegisterID CalleeSavedFirstRegister = Gpr3;
    static const RegisterID CalleeSavedLastRegister = Gpr3;
    static const FPRegisterID FPGpr0 = JSC::X86Registers::xmm0;
#elif CPU(X86_64)
    static const RegisterID StackFrameRegister = JSC::X86Registers::ebp;
    static const RegisterID StackPointerRegister = JSC::X86Registers::esp;
    static const RegisterID ContextRegister = JSC::X86Registers::r14;
    static const RegisterID ReturnValueRegister = JSC::X86Registers::eax;
    static const RegisterID Gpr0 = JSC::X86Registers::eax;
    static const RegisterID Gpr1 = JSC::X86Registers::ecx;
    static const RegisterID Gpr2 = JSC::X86Registers::edx;
    static const RegisterID Gpr3 = JSC::X86Registers::esi;
    static const FPRegisterID FPGpr0 = JSC::X86Registers::xmm0;

    static const RegisterID RegisterArgument1 = JSC::X86Registers::edi;
    static const RegisterID RegisterArgument2 = JSC::X86Registers::esi;
    static const RegisterID RegisterArgument3 = JSC::X86Registers::edx;
    static const RegisterID RegisterArgument4 = JSC::X86Registers::ecx;
    static const RegisterID RegisterArgument5 = JSC::X86Registers::r8;
    static const RegisterID RegisterArgument6 = JSC::X86Registers::r9;
#elif CPU(ARM)
    static const RegisterID StackFrameRegister = JSC::ARMRegisters::r4;
    static const RegisterID StackPointerRegister = JSC::ARMRegisters::sp;
    static const RegisterID ContextRegister = JSC::ARMRegisters::r5;
    static const RegisterID ReturnValueRegister = JSC::ARMRegisters::r0;
    static const RegisterID Gpr0 = JSC::ARMRegisters::r6;
    static const RegisterID Gpr1 = JSC::ARMRegisters::r7;
    static const RegisterID Gpr2 = JSC::ARMRegisters::r8;
    static const RegisterID Gpr3 = JSC::ARMRegisters::r10;
    static const RegisterID CalleeSavedFirstRegister = JSC::ARMRegisters::r4;
    static const RegisterID CalleeSavedLastRegister = JSC::ARMRegisters::r11;
    static const FPRegisterID FPGpr0 = JSC::ARMRegisters::d0;

    static const RegisterID RegisterArgument1 = JSC::ARMRegisters::r0;
    static const RegisterID RegisterArgument2 = JSC::ARMRegisters::r1;
    static const RegisterID RegisterArgument3 = JSC::ARMRegisters::r2;
    static const RegisterID RegisterArgument4 = JSC::ARMRegisters::r3;
#else
#error Argh.
#endif
    struct VoidType {};
    static const VoidType Void;

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

    void enterStandardStackFrame(int locals)
    {
#if CPU(ARM)
        push(JSC::ARMRegisters::lr);
#endif
        push(StackFrameRegister);
        move(StackPointerRegister, StackFrameRegister);
        subPtr(TrustedImm32(locals*sizeof(QQmlJS::VM::Value)), StackPointerRegister);
#if CPU(X86) || CPU(ARM)
        for (int saveReg = CalleeSavedFirstRegister; saveReg <= CalleeSavedLastRegister; ++saveReg)
            push(static_cast<RegisterID>(saveReg));
#endif
    }
    void leaveStandardStackFrame(int locals)
    {
#if CPU(X86) || CPU(ARM)
        for (int saveReg = CalleeSavedLastRegister; saveReg >= CalleeSavedFirstRegister; --saveReg)
            pop(static_cast<RegisterID>(saveReg));
#endif
        addPtr(TrustedImm32(locals*sizeof(QQmlJS::VM::Value)), StackPointerRegister);
        pop(StackFrameRegister);
#if CPU(ARM)
        pop(JSC::ARMRegisters::lr);
#endif
    }

    Address stackAddressForArgument(int index) const
    {
        // StackFrameRegister points to its old value on the stack, and above
        // it we have the return address, hence the need to step over two
        // values before reaching the first argument.
        return Address(StackFrameRegister, (index + 2) * sizeof(void*));
    }
#if CPU(X86)
    Address addressForArgument(int index) const
    {
        return stackAddressForArgument(index);
    }
#elif CPU(X86_64)
    Address addressForArgument(int index) const
    {
        static RegisterID args[6] = {
            RegisterArgument1,
            RegisterArgument2,
            RegisterArgument3,
            RegisterArgument4,
            RegisterArgument5,
            RegisterArgument6
        };
        if (index < 6)
            return Address(args[index], 0);
        else
            return stackAddressForArgument(index - 6);
    }
#elif CPU(ARM)
    Address addressForArgument(int index) const
    {
        static RegisterID args[4] = {
            RegisterArgument1,
            RegisterArgument2,
            RegisterArgument3,
            RegisterArgument4,
        };
        if (index < 4)
            return Address(args[index], 0);
        else
            return stackAddressForArgument(index - 4);
    }
#endif

    // Some run-time functions take (Value* args, int argc). This function is for populating
    // the args.
    Pointer argumentAddressForCall(int argument)
    {
        const int index = _function->maxNumberOfArguments - argument;
        return Pointer(StackFrameRegister, sizeof(VM::Value) * (-index)
                                          - sizeof(void*) // size of ebp
                       );
    }
    Pointer baseAddressForCallArguments()
    {
        return argumentAddressForCall(0);
    }

    VM::String *identifier(const QString &s);
    Pointer loadTempAddress(RegisterID reg, IR::Temp *t);
    void callActivationProperty(IR::Call *call, IR::Temp *result);
    void callProperty(IR::Call *call, IR::Temp *result);
    void constructActivationProperty(IR::New *call, IR::Temp *result);
    void constructProperty(IR::New *ctor, IR::Temp *result);
    void callValue(IR::Call *call, IR::Temp *result);
    void constructValue(IR::New *call, IR::Temp *result);
    void checkExceptions();

    virtual void visitExp(IR::Exp *);
    virtual void visitEnter(IR::Enter *);
    virtual void visitLeave(IR::Leave *);
    virtual void visitMove(IR::Move *);
    virtual void visitJump(IR::Jump *);
    virtual void visitCJump(IR::CJump *);
    virtual void visitRet(IR::Ret *);

private:
    void jumpToBlock(IR::BasicBlock *target);

    typedef JSC::FunctionPtr FunctionPtr;

    void callAbsolute(const char* functionName, FunctionPtr function) {
        CallToLink ctl;
        ctl.call = call();
        ctl.externalFunction = function;
        ctl.functionName = functionName;
        _callsToLink.append(ctl);
    }

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

    void loadArgument(IR::Temp* temp, RegisterID dest)
    {
        if (!temp) {
            VM::Value undefined = VM::Value::undefinedValue();
            move(TrustedImmPtr((const void *)undefined.val), dest);
        } else {
            Pointer addr = loadTempAddress(dest, temp);
            loadPtr(addr, dest);
        }
    }

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
            // ### Should use some ScratchRegister here
            Pointer addr = loadTempAddress(Gpr3, temp);
            storePtr(src, addr);
        }
    }

    void storeArgument(RegisterID src, const Pointer &dest)
    {
        storePtr(src, dest);
    }

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
        addPtr(TrustedImm32(ptr.offset), ptr.base, Gpr0);
        push(Gpr0);
    }

    void push(IR::Temp* temp)
    {
        if (temp) {
            Pointer addr = loadTempAddress(Gpr0, temp);
            push(addr);
        } else {
            xorPtr(Gpr0, Gpr0);
            push(Gpr0);
        }
    }

    void push(TrustedImmPtr ptr)
    {
        move(TrustedImmPtr(ptr), Gpr0);
        push(Gpr0);
    }

    void push(VM::String* name)
    {
        push(TrustedImmPtr(name));
    }

    void callFunctionPrologue()
    {
#if CPU(X86)
        // Callee might clobber it :(
        push(ContextRegister);
#endif
    }
    void callFunctionEpilogue()
    {
#if CPU(X86)
        pop(ContextRegister);
#endif
    }

    #define isel_stringIfyx(s) #s
    #define isel_stringIfy(s) isel_stringIfyx(s)

    #define generateFunctionCall(t, function, ...) \
        generateFunctionCallImp(t, isel_stringIfy(function), function, __VA_ARGS__)

#if CPU(X86)
    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
    void generateFunctionCallImp(const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
    {
        callFunctionPrologue();
        // Reverse order
        push(arg6);
        push(arg5);
        push(arg4);
        push(arg3);
        push(arg2);
        push(arg1);
        callAbsolute(functionName, function);
        add32(TrustedImm32(6 * sizeof(void*)), StackPointerRegister);
        callFunctionEpilogue();
    }
    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    void generateFunctionCallImp(const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
    {
        callFunctionPrologue();
        // Reverse order
        push(arg5);
        push(arg4);
        push(arg3);
        push(arg2);
        push(arg1);
        callAbsolute(functionName, function);
        add32(TrustedImm32(5 * sizeof(void*)), StackPointerRegister);
        callFunctionEpilogue();
    }


    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    void generateFunctionCallImp(const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
    {
        callFunctionPrologue();
        // Reverse order
        push(arg4);
        push(arg3);
        push(arg2);
        push(arg1);
        callAbsolute(functionName, function);
        add32(TrustedImm32(4 * sizeof(void*)), StackPointerRegister);
        callFunctionEpilogue();
    }


    template <typename Arg1, typename Arg2, typename Arg3>
    void generateFunctionCallImp(const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3)
    {
        callFunctionPrologue();
        // Reverse order
        push(arg3);
        push(arg2);
        push(arg1);
        callAbsolute(functionName, function);
        add32(TrustedImm32(3 * sizeof(void*)), StackPointerRegister);
        callFunctionEpilogue();
    }

    template <typename Arg1, typename Arg2>
    void generateFunctionCallImp(const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2)
    {
        callFunctionPrologue();
        // Reverse order
        push(arg2);
        push(arg1);
        callAbsolute(functionName, function);
        add32(TrustedImm32(2 * sizeof(void*)), StackPointerRegister);
        callFunctionEpilogue();
    }
#elif CPU(X86_64)
    template <typename ArgRet, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    void generateFunctionCallImp(ArgRet r, const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
    {
        callFunctionPrologue();
        loadArgument(arg1, RegisterArgument1);
        loadArgument(arg2, RegisterArgument2);
        loadArgument(arg3, RegisterArgument3);
        loadArgument(arg4, RegisterArgument4);
        callAbsolute(functionName, function);
        storeArgument(ReturnValueRegister, r);
        callFunctionEpilogue();
    }

    template <typename ArgRet, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    void generateFunctionCallImp(ArgRet r, const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
    {
        callFunctionPrologue();
        loadArgument(arg1, RegisterArgument1);
        loadArgument(arg2, RegisterArgument2);
        loadArgument(arg3, RegisterArgument3);
        loadArgument(arg4, RegisterArgument4);
        loadArgument(arg5, RegisterArgument5);
        callAbsolute(functionName, function);
        storeArgument(ReturnValueRegister, r);
        callFunctionEpilogue();
    }

    template <typename ArgRet, typename Arg1, typename Arg2, typename Arg3>
    void generateFunctionCallImp(ArgRet r, const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3)
    {
        callFunctionPrologue();
        loadArgument(arg1, RegisterArgument1);
        loadArgument(arg2, RegisterArgument2);
        loadArgument(arg3, RegisterArgument3);
        callAbsolute(functionName, function);
        storeArgument(ReturnValueRegister, r);
        callFunctionEpilogue();
    }

    template <typename ArgRet, typename Arg1, typename Arg2>
    void generateFunctionCallImp(ArgRet r, const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2)
    {
        callFunctionPrologue();
        loadArgument(arg1, RegisterArgument1);
        loadArgument(arg2, RegisterArgument2);
        callAbsolute(functionName, function);
        storeArgument(ReturnValueRegister, r);
        callFunctionEpilogue();
    }

    template <typename ArgRet, typename Arg1>
    void generateFunctionCallImp(ArgRet r, const char* functionName, FunctionPtr function, Arg1 arg1)
    {
        callFunctionPrologue();
        loadArgument(arg1, RegisterArgument1);
        callAbsolute(functionName, function);
        storeArgument(ReturnValueRegister, r);
        callFunctionEpilogue();
    }

#elif CPU(ARM)

    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
    void generateFunctionCallImp(const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
    {
        callFunctionPrologue();
        loadArgument(arg1, RegisterArgument1);
        loadArgument(arg2, RegisterArgument2);
        loadArgument(arg3, RegisterArgument3);
        loadArgument(arg4, RegisterArgument4);
        push(arg5);
        push(arg6);
        callAbsolute(functionName, function);
        add32(TrustedImm32(2 * sizeof(void*)), StackPointerRegister);
        callFunctionEpilogue();
    }
    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    void generateFunctionCallImp(const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
    {
        callFunctionPrologue();
        loadArgument(arg1, RegisterArgument1);
        loadArgument(arg2, RegisterArgument2);
        loadArgument(arg3, RegisterArgument3);
        loadArgument(arg4, RegisterArgument4);
        push(arg5);
        callAbsolute(functionName, function);
        add32(TrustedImm32(1 * sizeof(void*)), StackPointerRegister);
        callFunctionEpilogue();
    }


    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    void generateFunctionCallImp(const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
    {
        callFunctionPrologue();
        loadArgument(arg1, RegisterArgument1);
        loadArgument(arg2, RegisterArgument2);
        loadArgument(arg3, RegisterArgument3);
        loadArgument(arg4, RegisterArgument4);
        callAbsolute(functionName, function);
        callFunctionEpilogue();
    }


    template <typename Arg1, typename Arg2, typename Arg3>
    void generateFunctionCallImp(const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3)
    {
        callFunctionPrologue();
        loadArgument(arg1, RegisterArgument1);
        loadArgument(arg2, RegisterArgument2);
        loadArgument(arg3, RegisterArgument3);
        callAbsolute(functionName, function);
        callFunctionEpilogue();
    }

    template <typename Arg1, typename Arg2>
    void generateFunctionCallImp(const char* functionName, FunctionPtr function, Arg1 arg1, Arg2 arg2)
    {
        callFunctionPrologue();
        loadArgument(arg1, RegisterArgument1);
        loadArgument(arg2, RegisterArgument2);
        callAbsolute(functionName, function);
        callFunctionEpilogue();
    }

#endif

    int prepareVariableArguments(IR::ExprList* args);

    typedef VM::Value (*ActivationMethod)(VM::Context *, VM::String *name, VM::Value *args, int argc);
    typedef VM::Value (*BuiltinMethod)(VM::Context *, VM::Value *args, int argc);
    void callRuntimeMethodImp(IR::Temp *result, const char* name, ActivationMethod method, IR::Expr *base, IR::ExprList *args);
    void callRuntimeMethodImp(IR::Temp *result, const char* name, BuiltinMethod method, IR::ExprList *args);
#define callRuntimeMethod(result, function, ...) \
    callRuntimeMethodImp(result, isel_stringIfy(function), function, __VA_ARGS__)

    using JSC::MacroAssembler::loadDouble;
    void loadDouble(IR::Temp* temp, FPRegisterID dest)
    {
        Pointer ptr = loadTempAddress(Gpr0, temp);
        loadDouble(ptr, dest);
    }

    using JSC::MacroAssembler::storeDouble;
    void storeDouble(FPRegisterID source, IR::Temp* temp)
    {
        Pointer ptr = loadTempAddress(Gpr0, temp);
        storeDouble(source, ptr);
    }

    template <typename Result, typename Source>
    void copyValue(Result result, Source source);

    struct CallToLink {
        Call call;
        FunctionPtr externalFunction;
        const char* functionName;
    };

    void storeValue(VM::Value value, Address destination)
    {
#if CPU(X86_64)
        storePtr(TrustedImmPtr((void *)value.val), destination);
#elif CPU(X86)
        destination.offset += offsetof(VM::ValueData, tag);
        store32(value.tag, destination);
        destination.offset -= offsetof(VM::ValueData, tag);
        destination.offset += offsetof(VM::ValueData, int_32);
        store32(value.int_32, destination);
#else
#error "Missing implementation"
#endif
    }


    VM::ExecutionEngine *_engine;
    IR::Module *_module;
    IR::Function *_function;
    IR::BasicBlock *_block;
    uchar *_buffer;
    uchar *_code;
    uchar *_codePtr;
    QHash<IR::BasicBlock *, QVector<Jump> > _patches;
    QHash<IR::BasicBlock *, Label> _addrs;
    QList<CallToLink> _callsToLink;
};

} // end of namespace MASM
} // end of namespace QQmlJS

#endif // QV4ISEL_MASM_P_H
