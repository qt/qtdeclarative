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
    static const RegisterID CalleeSavedGpr = Gpr3;
    static const FPRegisterID FPGpr0 = JSC::X86Registers::xmm0;
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

#if CPU(X86) || CPU(X86_64)
    void enterStandardStackFrame(int locals)
    {
        push(StackFrameRegister);
        move(StackPointerRegister, StackFrameRegister);
        sub32(TrustedImm32(locals), StackPointerRegister);
        push(CalleeSavedGpr);
    }
    void leaveStandardStackFrame(int locals)
    {
        pop(CalleeSavedGpr);
        add32(TrustedImm32(locals), StackPointerRegister);
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

    using JSC::MacroAssembler::push;

    void push(const Pointer& ptr)
    {
        add32(TrustedImm32(ptr.offset), ptr.base, Gpr0);
        push(Gpr0);
    }

    void push(IR::Temp* temp)
    {
        if (temp) {
            Pointer addr = loadTempAddress(Gpr0, temp);
            push(addr);
        } else {
            xor32(Gpr0, Gpr0);
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
        // Callee might clobber it :(
        push(ContextRegister);
    }
    void callFunctionEpilogue()
    {
        pop(ContextRegister);
    }

    #define isel_stringIfyx(s) #s
    #define isel_stringIfy(s) isel_stringIfyx(s)

    #define generateFunctionCall(function, ...) \
        generateFunctionCallImp(isel_stringIfy(function), function, __VA_ARGS__)

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

    int prepareVariableArguments(IR::ExprList* args);

    typedef void (*ActivationMethod)(VM::Context *, VM::Value *result, VM::String *name, VM::Value *args, int argc);
    typedef void (*BuiltinMethod)(VM::Context *, VM::Value *result, VM::Value *args, int argc);
    void callRuntimeMethodImp(const char* name, ActivationMethod method, IR::Temp *result, IR::Expr *base, IR::ExprList *args);
    void callRuntimeMethodImp(const char* name, BuiltinMethod method, IR::Temp *result, IR::ExprList *args);
#define callRuntimeMethod(function, ...) \
    callRuntimeMethodImp(isel_stringIfy(function), function, __VA_ARGS__)

    struct CallToLink {
        Call call;
        FunctionPtr externalFunction;
        const char* functionName;
    };

    template <VM::Value::ValueType type>
    void storeValue(TrustedImm32 value, Address destination)
    {
        destination.offset += offsetof(VM::ValueData, tag);
        store32(TrustedImm32(type), destination);
        destination.offset -= offsetof(VM::ValueData, tag);
        destination.offset += VM::ValueOffsetHelper<type>::DataOffset;
        store32(value, destination);
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
