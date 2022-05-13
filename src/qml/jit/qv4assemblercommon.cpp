// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QBuffer>
#include <QFile>
#include <QLoggingCategory>

#include "qv4engine_p.h"
#include "qv4assemblercommon_p.h"
#include <private/qv4function_p.h>
#include <private/qv4functiontable_p.h>
#include <private/qv4runtime_p.h>

#include <assembler/MacroAssemblerCodeRef.h>
#include <assembler/LinkBuffer.h>
#include <WTFStubs.h>

#if QT_CONFIG(qml_jit)

#undef ENABLE_ALL_ASSEMBLERS_FOR_REFACTORING_PURPOSES

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace JIT {

Q_LOGGING_CATEGORY(lcAsm, "qt.qml.v4.asm")

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

    void vprintf(const char* format, va_list argList) override WTF_ATTRIBUTE_PRINTF(2, 0)
    {
        const int written = qvsnprintf(buf.data(), buf.size(), format, argList);
        if (written > 0)
            dest->write(buf.constData(), written);
        memset(buf.data(), 0, qMin(written, buf.size()));
    }

    void flush() override
    {}

private:
    QIODevice *dest;
    QByteArray buf;
};
} // anonymous namespace

static void printDisassembledOutputWithCalls(QByteArray processedOutput,
                                             const QHash<const void*, const char*>& functions)
{
    const auto symbols = Runtime::symbolTable();
    const QByteArray padding("                          ; ");
    for (auto it = functions.begin(), end = functions.end(); it != end; ++it) {
        const QByteArray ptrString = "0x" + QByteArray::number(quintptr(it.key()), 16);
        int idx = 0;
        while (idx >= 0) {
            idx = processedOutput.indexOf(ptrString, idx);
            if (idx < 0)
                break;
            idx = processedOutput.indexOf('\n', idx);
            if (idx < 0)
                break;
            const char *functionName = it.value();
            processedOutput = processedOutput.insert(
                    idx, QByteArray(padding + QByteArray(
                                        functionName ? functionName : symbols[it.key()])));
        }
    }

    auto lines = processedOutput.split('\n');
    for (const auto &line : lines)
        qCDebug(lcAsm, "%s", line.constData());
}

JIT::PlatformAssemblerCommon::~PlatformAssemblerCommon()
{}

void PlatformAssemblerCommon::link(Function *function, const char *jitKind)
{
    for (const auto &jumpTarget : jumpsToLink)
        jumpTarget.jump.linkTo(labelForOffset[jumpTarget.offset], this);

    JSC::JSGlobalData dummy(function->internalClass->engine->executableAllocator);
    JSC::LinkBuffer<MacroAssembler> linkBuffer(dummy, this, nullptr);

    for (const auto &ehTarget : ehTargets) {
        auto targetLabel = labelForOffset.value(ehTarget.offset);
        linkBuffer.patch(ehTarget.label, linkBuffer.locationOf(targetLabel));
    }

    JSC::MacroAssemblerCodeRef codeRef;

    static const bool showCode = lcAsm().isDebugEnabled();
    if (showCode) {
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        WTF::setDataFile(new QIODevicePrintStream(&buf));

        // We use debugAddress here because it's actually for debugging and hidden behind an
        // environment variable.
        const QByteArray name = Function::prettyName(function, linkBuffer.debugAddress()).toUtf8();
        codeRef = linkBuffer.finalizeCodeWithDisassembly(jitKind, name.constData());

        WTF::setDataFile(stderr);
        printDisassembledOutputWithCalls(buf.data(), functions);
    } else {
        codeRef = linkBuffer.finalizeCodeWithoutDisassembly();
    }

    function->codeRef = new JSC::MacroAssemblerCodeRef(codeRef);
    function->jittedCode = reinterpret_cast<Function::JittedCode>(function->codeRef->code().executableAddress());

    generateFunctionTable(function, &codeRef);

    if (Q_UNLIKELY(!linkBuffer.makeExecutable()))
        function->jittedCode = nullptr; // The function is not executable, but the coderef exists.
}

void PlatformAssemblerCommon::prepareCallWithArgCount(int argc)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(remainingArgcForCall == NoCall);
    remainingArgcForCall = argc;
#endif

    if (argc > ArgInRegCount) {
        argcOnStackForCall = int(WTF::roundUpToMultipleOf(16, size_t(argc - ArgInRegCount) * PointerSize));
        subPtr(TrustedImm32(argcOnStackForCall), StackPointerRegister);
    }
}

void PlatformAssemblerCommon::storeInstructionPointer(int instructionOffset)
{
    Address addr(CppStackFrameRegister, offsetof(QV4::JSTypesStackFrame, instructionPointer));
    store32(TrustedImm32(instructionOffset), addr);
}

PlatformAssemblerCommon::Address PlatformAssemblerCommon::argStackAddress(int arg)
{
    int offset = arg - ArgInRegCount;
    Q_ASSERT(offset >= 0);
    return Address(StackPointerRegister, offset * PointerSize);
}

void PlatformAssemblerCommon::passAccumulatorAsArg(int arg)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(arg < remainingArgcForCall);
    --remainingArgcForCall;
#endif

    passAccumulatorAsArg_internal(arg, false);
}

void JIT::PlatformAssemblerCommon::pushAccumulatorAsArg(int arg)
{
    passAccumulatorAsArg_internal(arg, true);
}

void PlatformAssemblerCommon::passAccumulatorAsArg_internal(int arg, bool doPush)
{
    if (arg < ArgInRegCount) {
        addPtr(TrustedImm32(offsetof(CallData, accumulator)), JSStackFrameRegister, registerForArg(arg));
    } else {
        addPtr(TrustedImm32(offsetof(CallData, accumulator)), JSStackFrameRegister, ScratchRegister);
        if (doPush)
            push(ScratchRegister);
        else
            storePtr(ScratchRegister, argStackAddress(arg));
    }
}

void PlatformAssemblerCommon::passFunctionAsArg(int arg)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(arg < remainingArgcForCall);
    --remainingArgcForCall;
#endif

    if (arg < ArgInRegCount) {
        loadFunctionPtr(registerForArg(arg));
    } else {
        loadFunctionPtr(ScratchRegister);
        storePtr(ScratchRegister, argStackAddress(arg));
    }
}

void PlatformAssemblerCommon::passEngineAsArg(int arg)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(arg < remainingArgcForCall);
    --remainingArgcForCall;
#endif

    if (arg < ArgInRegCount) {
        move(EngineRegister, registerForArg(arg));
    } else {
        storePtr(EngineRegister, argStackAddress(arg));
    }
}

void PlatformAssemblerCommon::passJSSlotAsArg(int reg, int arg)
{
    Address addr(JSStackFrameRegister, reg * int(sizeof(QV4::Value)));
    passAddressAsArg(addr, arg);
}

void JIT::PlatformAssemblerCommon::passAddressAsArg(Address addr, int arg)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(arg < remainingArgcForCall);
    --remainingArgcForCall;
#endif

    if (arg < ArgInRegCount) {
        addPtr(TrustedImm32(addr.offset), addr.base, registerForArg(arg));
    } else {
        addPtr(TrustedImm32(addr.offset), addr.base, ScratchRegister);
        storePtr(ScratchRegister, argStackAddress(arg));
    }
}

void PlatformAssemblerCommon::passCppFrameAsArg(int arg)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(arg < remainingArgcForCall);
    --remainingArgcForCall;
#endif

    if (arg < ArgInRegCount)
        move(CppStackFrameRegister, registerForArg(arg));
    else
        store32(CppStackFrameRegister, argStackAddress(arg));
}

void PlatformAssemblerCommon::passInt32AsArg(int value, int arg)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(arg < remainingArgcForCall);
    --remainingArgcForCall;
#endif

    if (arg < ArgInRegCount)
        move(TrustedImm32(value), registerForArg(arg));
    else
        store32(TrustedImm32(value), argStackAddress(arg));
}

void JIT::PlatformAssemblerCommon::passPointerAsArg(void *ptr, int arg)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(arg < remainingArgcForCall);
    --remainingArgcForCall;
#endif

    if (arg < ArgInRegCount)
        move(TrustedImmPtr(ptr), registerForArg(arg));
    else
        storePtr(TrustedImmPtr(ptr), argStackAddress(arg));
}

void PlatformAssemblerCommon::callRuntime(const void *funcPtr, const char *functionName)
{
#ifndef QT_NO_DEBUG
    Q_ASSERT(remainingArgcForCall == 0);
    remainingArgcForCall = NoCall;
#endif
    callRuntimeUnchecked(funcPtr, functionName);
    if (argcOnStackForCall > 0) {
        addPtr(TrustedImm32(argcOnStackForCall), StackPointerRegister);
        argcOnStackForCall = 0;
    }
}

void PlatformAssemblerCommon::callRuntimeUnchecked(const void *funcPtr, const char *functionName)
{
    Q_ASSERT(functionName || Runtime::symbolTable().contains(funcPtr));
    functions.insert(funcPtr, functionName);
    callAbsolute(funcPtr);
}

void PlatformAssemblerCommon::tailCallRuntime(const void *funcPtr, const char *functionName)
{
    Q_ASSERT(functionName || Runtime::symbolTable().contains(funcPtr));
    functions.insert(funcPtr, functionName);
    setTailCallArg(EngineRegister, 1);
    setTailCallArg(CppStackFrameRegister, 0);
    freeStackSpace();
    generatePlatformFunctionExit(/*tailCall =*/ true);
    jumpAbsolute(funcPtr);
}

void PlatformAssemblerCommon::setTailCallArg(RegisterID src, int arg)
{
    if (arg < ArgInRegCount) {
        move(src, registerForArg(arg));
    } else {
        // We never write to the incoming arguments space on the stack, and the tail call runtime
        // method has the same signature as the jitted function, so it is safe for us to just reuse
        // the arguments that we got in.
    }
}

JSC::MacroAssemblerBase::Address PlatformAssemblerCommon::jsAlloca(int slotCount)
{
    Address jsStackTopAddr(EngineRegister, offsetof(EngineBase, jsStackTop));
    RegisterID jsStackTop = AccumulatorRegisterValue;
    loadPtr(jsStackTopAddr, jsStackTop);
    addPtr(TrustedImm32(sizeof(Value) * slotCount), jsStackTop);
    storePtr(jsStackTop, jsStackTopAddr);
    return Address(jsStackTop, 0);
}

void PlatformAssemblerCommon::storeInt32AsValue(int srcInt, Address destAddr)
{
    store32(TrustedImm32(srcInt),
            Address(destAddr.base, destAddr.offset + QV4::Value::valueOffset()));
    store32(TrustedImm32(int(QV4::Value::ValueTypeInternal::Integer)),
            Address(destAddr.base, destAddr.offset + QV4::Value::tagOffset()));
}

} // JIT namespace
} // QV4 namepsace

QT_END_NAMESPACE

#endif // QT_CONFIG(qml_jit)
