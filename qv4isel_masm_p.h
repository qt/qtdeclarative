#ifndef QV4ISEL_MASM_P_H
#define QV4ISEL_MASM_P_H

#include "qv4ir_p.h"
#include "qmljs_objects.h"

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
    static const RegisterID Gpr0 = JSC::X86Registers::eax;
    static const RegisterID Gpr1 = JSC::X86Registers::ecx;
    static const RegisterID Gpr2 = JSC::X86Registers::edx;
    static const RegisterID Gpr3 = JSC::X86Registers::edi;
    static const FPRegisterID FPGpr0 = JSC::X86Registers::xmm0;
#else
#error Argh.
#endif

#if CPU(X86) || CPU(X86_64)
    void enterStandardStackFrame()
    {
        push(StackFrameRegister);
        move(StackPointerRegister, StackFrameRegister);
    }
    void leaveStandardStackFrame()
    {
        pop(StackFrameRegister);
    }
#else
#error Argh.
#endif

    Address addressForArgument(int index) const
    {
        // ### CPU specific: on x86/x86_64 we need +2 to jump over ebp and the return address
        // on the stack. Maybe same on arm if we save lr on stack on enter.
        return Address(StackFrameRegister, (index + 2) * sizeof(void*));
    }

    VM::String *identifier(const QString &s);
    Address loadTempAddress(RegisterID reg, IR::Temp *t);
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
    typedef JSC::FunctionPtr FunctionPtr;

    void callAbsolute(FunctionPtr function) {
        CallToLink ctl;
        ctl.call = call();
        ctl.externalFunction = function;
        _callsToLink.append(ctl);
    }

    template <typename Arg1>
    void callHelper(FunctionPtr function, Arg1 arg1) {
        push(arg1);
        callAbsolute(function);
        add32(TrustedImm32(1 * sizeof(void*)), StackPointerRegister);
    }

    template <typename Arg1, typename Arg2>
    void callHelper(FunctionPtr function, Arg1 arg1, Arg2 arg2) {
        push(arg2);
        push(arg1);
        callAbsolute(function);
        add32(TrustedImm32(2 * sizeof(void*)), StackPointerRegister);
    }

    template <typename Arg1, typename Arg2, typename Arg3>
    void callHelper(FunctionPtr function, Arg1 arg1, Arg2 arg2, Arg3 arg3) {
        push(arg3);
        push(arg2);
        push(arg1);
        callAbsolute(function);
        add32(TrustedImm32(3 * sizeof(void*)), StackPointerRegister);
    }

    struct CallToLink {
        Call call;
        FunctionPtr externalFunction;
    };

    VM::ExecutionEngine *_engine;
    IR::Module *_module;
    IR::Function *_function;
    IR::BasicBlock *_block;
    uchar *_buffer;
    uchar *_code;
    uchar *_codePtr;
    QHash<IR::BasicBlock *, QVector<uchar *> > _patches;
    QHash<IR::BasicBlock *, uchar *> _addrs;
    QList<CallToLink> _callsToLink;
};

} // end of namespace MASM
} // end of namespace QQmlJS

#endif // QV4ISEL_MASM_P_H
