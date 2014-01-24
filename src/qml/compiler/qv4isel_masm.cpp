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

#include <assembler/LinkBuffer.h>
#include <WTFStubs.h>

#include <iostream>
#include <cassert>

#if ENABLE(ASSEMBLER)

#if USE(UDIS86)
#  include <udis86.h>
#endif

using namespace QQmlJS;
using namespace QQmlJS::MASM;
using namespace QV4;

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
                                                           (ReturnedValue (*)(QV4::ExecutionContext *, const uchar *)) codeRefs[i].code().executableAddress(),
                                                           codeSizes[i]);
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

namespace {
inline bool isPregOrConst(V4IR::Expr *e)
{
    if (V4IR::Temp *t = e->asTemp())
        return t->kind == V4IR::Temp::PhysicalRegister;
    return e->asConst() != 0;
}
} // anonymous namespace

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
    // ### FIXME: remove unused registers.
    JSC::ARMRegisters::r12,
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

Assembler::Assembler(InstructionSelection *isel, V4IR::Function* function, QV4::ExecutableAllocator *executableAllocator,
                     int maxArgCountForBuiltins)
    : _stackLayout(function, maxArgCountForBuiltins)
    , _constTable(this)
    , _function(function)
    , _nextBlock(0)
    , _executableAllocator(executableAllocator)
    , _isel(isel)
{
}

void Assembler::registerBlock(V4IR::BasicBlock* block, V4IR::BasicBlock *nextBlock)
{
    _addrs[block] = label();
    catchBlock = block->catchBlock;
    _nextBlock = nextBlock;
}

void Assembler::jumpToBlock(V4IR::BasicBlock* current, V4IR::BasicBlock *target)
{
    Q_UNUSED(current);

    if (target != _nextBlock)
        _patches[target].append(jump());
}

void Assembler::addPatch(V4IR::BasicBlock* targetBlock, Jump targetJump)
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

void Assembler::addPatch(DataLabelPtr patch, V4IR::BasicBlock *target)
{
    _labelPatches[target].append(patch);
}

void Assembler::generateCJumpOnNonZero(RegisterID reg, V4IR::BasicBlock *currentBlock,
                                       V4IR::BasicBlock *trueBlock, V4IR::BasicBlock *falseBlock)
{
    generateCJumpOnCompare(NotEqual, reg, TrustedImm32(0), currentBlock, trueBlock, falseBlock);
}

void Assembler::generateCJumpOnCompare(RelationalCondition cond, RegisterID left,TrustedImm32 right,
                                       V4IR::BasicBlock *currentBlock,  V4IR::BasicBlock *trueBlock,
                                       V4IR::BasicBlock *falseBlock)
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
                                       V4IR::BasicBlock *currentBlock,  V4IR::BasicBlock *trueBlock,
                                       V4IR::BasicBlock *falseBlock)
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

Assembler::Pointer Assembler::loadTempAddress(RegisterID baseReg, V4IR::Temp *t)
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
    case V4IR::Temp::Formal:
    case V4IR::Temp::ScopedFormal: {
        loadPtr(Address(context, qOffsetOf(ExecutionContext, callData)), baseReg);
        offset = sizeof(CallData) + (t->index - 1) * sizeof(SafeValue);
    } break;
    case V4IR::Temp::Local:
    case V4IR::Temp::ScopedLocal: {
        loadPtr(Address(context, qOffsetOf(CallContext, locals)), baseReg);
        offset = t->index * sizeof(SafeValue);
    } break;
    case V4IR::Temp::StackSlot: {
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
    return Pointer(reg, id * sizeof(QV4::SafeString));
}

void Assembler::loadStringRef(RegisterID reg, const QString &string)
{
    loadPtr(Address(Assembler::ContextRegister, qOffsetOf(QV4::ExecutionContext, compilationUnit)), reg);
    loadPtr(Address(reg, qOffsetOf(QV4::CompiledData::CompilationUnit, runtimeStrings)), reg);
    const int id = _isel->registerString(string);
    addPtr(TrustedImmPtr(id * sizeof(QV4::SafeString)), reg);
}

template <typename Result, typename Source>
void Assembler::copyValue(Result result, Source source)
{
#ifdef VALUE_FITS_IN_REGISTER
    // Use ReturnValueRegister as "scratch" register because loadArgument
    // and storeArgument are functions that may need a scratch register themselves.
    loadArgumentInRegister(source, ReturnValueRegister, 0);
    storeReturnValue(result);
#else
    loadDouble(source, FPGpr0);
    storeDouble(FPGpr0, result);
#endif
}

template <typename Result>
void Assembler::copyValue(Result result, V4IR::Expr* source)
{
    if (source->type == V4IR::BoolType) {
        RegisterID reg = toInt32Register(source, ScratchRegister);
        storeBool(reg, result);
    } else if (source->type == V4IR::SInt32Type) {
        RegisterID reg = toInt32Register(source, ScratchRegister);
        storeInt32(reg, result);
    } else if (source->type == V4IR::UInt32Type) {
        RegisterID reg = toUInt32Register(source, ScratchRegister);
        storeUInt32(reg, result);
    } else if (source->type == V4IR::DoubleType) {
        storeDouble(toDoubleRegister(source), result);
    } else if (V4IR::Temp *temp = source->asTemp()) {
#ifdef VALUE_FITS_IN_REGISTER
        Q_UNUSED(temp);

        // Use ReturnValueRegister as "scratch" register because loadArgument
        // and storeArgument are functions that may need a scratch register themselves.
        loadArgumentInRegister(source, ReturnValueRegister, 0);
        storeReturnValue(result);
#else
        loadDouble(temp, FPGpr0);
        storeDouble(FPGpr0, result);
#endif
    } else if (V4IR::Const *c = source->asConst()) {
        QV4::Primitive v = convertToValue(c);
        storeValue(v, result);
    } else {
        Q_UNREACHABLE();
    }
}

void Assembler::storeValue(QV4::Primitive value, V4IR::Temp* destination)
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

const Assembler::BinaryOperationInfo Assembler::binaryOperations[QQmlJS::V4IR::LastAluOp + 1] = {
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

#if OS(LINUX) || OS(MAC_OS_X)
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

JSC::MacroAssemblerCodeRef Assembler::link(int *codeSize)
{
    Label endOfCode = label();

    {
        QHashIterator<V4IR::BasicBlock *, QVector<Jump> > it(_patches);
        while (it.hasNext()) {
            it.next();
            V4IR::BasicBlock *block = it.key();
            Label target = _addrs.value(block);
            assert(target.isSet());
            foreach (Jump jump, it.value())
                jump.linkTo(target, this);
        }
    }

    JSC::JSGlobalData dummy(_executableAllocator);
    JSC::LinkBuffer linkBuffer(dummy, this, 0);

    QHash<void*, const char*> functions;
    foreach (CallToLink ctl, _callsToLink) {
        linkBuffer.link(ctl.call, ctl.externalFunction);
        functions[ctl.externalFunction.value()] = ctl.functionName;
    }

    foreach (const DataLabelPatch &p, _dataLabelPatches)
        linkBuffer.patch(p.dataLabel, linkBuffer.locationOf(p.target));

    // link exception handlers
    foreach(Jump jump, exceptionPropagationJumps)
        linkBuffer.link(jump, linkBuffer.locationOf(exceptionReturnLabel));

    {
        QHashIterator<V4IR::BasicBlock *, QVector<DataLabelPtr> > it(_labelPatches);
        while (it.hasNext()) {
            it.next();
            V4IR::BasicBlock *block = it.key();
            Label target = _addrs.value(block);
            assert(target.isSet());
            foreach (DataLabelPtr label, it.value())
                linkBuffer.patch(label, linkBuffer.locationOf(target));
        }
    }
    _constTable.finalize(linkBuffer, _isel);

    *codeSize = linkBuffer.offsetOf(endOfCode);

    JSC::MacroAssemblerCodeRef codeRef;

    static bool showCode = !qgetenv("QV4_SHOW_ASM").isNull();
    if (showCode) {
#if OS(LINUX) && !defined(Q_OS_ANDROID)
        char* disasmOutput = 0;
        size_t disasmLength = 0;
        FILE* disasmStream = open_memstream(&disasmOutput, &disasmLength);
        WTF::setDataFile(disasmStream);
#elif OS(MAC_OS_X)
        struct MemStream {
            QByteArray buf;
            static int write(void *cookie, const char *buf, int len) {
                MemStream *stream = reinterpret_cast<MemStream *>(cookie);
                stream->buf.append(buf, len);
                return len;
            }
        };
        MemStream memStream;

        FILE* disasmStream = fwopen(&memStream, MemStream::write);
        WTF::setDataFile(disasmStream);
#endif

        QByteArray name = _function->name->toUtf8();
        if (name.isEmpty()) {
            name = QByteArray::number(quintptr(_function), 16);
            name.prepend("IR::Function(0x");
            name.append(")");
        }
        codeRef = linkBuffer.finalizeCodeWithDisassembly("%s", name.data());

        WTF::setDataFile(stderr);
#if (OS(LINUX) && !defined(Q_OS_ANDROID)) || OS(MAC_OS_X)
        fclose(disasmStream);
#  if OS(MAC_OS_X)
        char *disasmOutput = memStream.buf.data();
#  endif
#  if CPU(X86) || CPU(X86_64) || CPU(ARM)
        QHash<void*, String*> idents;
        printDisassembledOutputWithCalls(disasmOutput, functions);
#  endif
#  if OS(LINUX)
        free(disasmOutput);
#  endif
#endif
    } else {
        codeRef = linkBuffer.finalizeCodeWithoutDisassembly();
    }

    return codeRef;
}

InstructionSelection::InstructionSelection(QQmlEnginePrivate *qmlEngine, QV4::ExecutableAllocator *execAllocator, V4IR::Module *module, Compiler::JSUnitGenerator *jsGenerator)
    : EvalInstructionSelection(execAllocator, module, jsGenerator)
    , _block(0)
    , _as(0)
    , qmlEngine(qmlEngine)
{
    compilationUnit = new CompilationUnit;
    compilationUnit->codeRefs.resize(module->functions.size());
    compilationUnit->codeSizes.resize(module->functions.size());
}

InstructionSelection::~InstructionSelection()
{
    delete _as;
}

void InstructionSelection::run(int functionIndex)
{
    V4IR::Function *function = irModule->functions[functionIndex];
    QVector<Lookup> lookups;
    qSwap(_function, function);

    V4IR::Optimizer opt(_function);
    opt.run(qmlEngine);

#if (CPU(X86_64) && (OS(MAC_OS_X) || OS(LINUX))) || (CPU(X86) && OS(LINUX))
    static const bool withRegisterAllocator = qgetenv("QV4_NO_REGALLOC").isEmpty();
    if (opt.isInSSA() && withRegisterAllocator) {
#if CPU(X86) && OS(LINUX) // x86 with linux
        static const QVector<int> intRegisters = QVector<int>()
                << JSC::X86Registers::edx
                << JSC::X86Registers::ebx;
#else // x86_64 with linux or with macos
        static const QVector<int> intRegisters = QVector<int>()
                << JSC::X86Registers::edi
                << JSC::X86Registers::esi
                << JSC::X86Registers::edx
                << JSC::X86Registers::r9
                << JSC::X86Registers::r8
                << JSC::X86Registers::r13
                << JSC::X86Registers::r15;
#endif
        static const QVector<int> fpRegisters = QVector<int>()
                << JSC::X86Registers::xmm2
                << JSC::X86Registers::xmm3
                << JSC::X86Registers::xmm4
                << JSC::X86Registers::xmm5
                << JSC::X86Registers::xmm6
                << JSC::X86Registers::xmm7;
        RegisterAllocator(intRegisters, fpRegisters).run(_function, opt);
    } else
#endif
    {
        if (opt.isInSSA())
            // No register allocator available for this platform, or env. var was set, so:
            opt.convertOutOfSSA();
        ConvertTemps().toStackSlots(_function);
    }
    V4IR::Optimizer::showMeTheCode(_function);
    QSet<V4IR::Jump *> removableJumps = opt.calculateOptionalJumps();
    qSwap(_removableJumps, removableJumps);

    Assembler* oldAssembler = _as;
    _as = new Assembler(this, _function, executableAllocator, 6); // 6 == max argc for calls to built-ins with an argument array

    _as->enterStandardStackFrame();

#ifdef ARGUMENTS_IN_REGISTERS
    _as->move(_as->registerForArgument(0), Assembler::ContextRegister);
#else
    _as->loadPtr(addressForArgument(0), Assembler::ContextRegister);
#endif

    const int locals = _as->stackLayout().calculateJSStackFrameSize();
    _as->loadPtr(Address(Assembler::ContextRegister, qOffsetOf(ExecutionContext, engine)), Assembler::ScratchRegister);
    _as->loadPtr(Address(Assembler::ScratchRegister, qOffsetOf(ExecutionEngine, jsStackTop)), Assembler::LocalsRegister);
    _as->addPtr(Assembler::TrustedImm32(sizeof(QV4::SafeValue)*locals), Assembler::LocalsRegister);
    _as->storePtr(Assembler::LocalsRegister, Address(Assembler::ScratchRegister, qOffsetOf(ExecutionEngine, jsStackTop)));

    int lastLine = -1;
    for (int i = 0, ei = _function->basicBlocks.size(); i != ei; ++i) {
        V4IR::BasicBlock *nextBlock = (i < ei - 1) ? _function->basicBlocks[i + 1] : 0;
        _block = _function->basicBlocks[i];
        _as->registerBlock(_block, nextBlock);

        foreach (V4IR::Stmt *s, _block->statements) {
            if (s->location.isValid()) {
                if (int(s->location.startLine) != lastLine) {
                    Assembler::Address lineAddr(Assembler::ContextRegister, qOffsetOf(QV4::ExecutionContext, lineNumber));
                    _as->store32(Assembler::TrustedImm32(s->location.startLine), lineAddr);
                    lastLine = s->location.startLine;
                }
            }
            s->accept(this);
        }
    }

    if (!_as->exceptionReturnLabel.isSet())
        visitRet(0);

    JSC::MacroAssemblerCodeRef codeRef =_as->link(&compilationUnit->codeSizes[functionIndex]);
    compilationUnit->codeRefs[functionIndex] = codeRef;

    qSwap(_function, function);
    delete _as;
    _as = oldAssembler;
    qSwap(_removableJumps, removableJumps);
}

void *InstructionSelection::addConstantTable(QVector<Primitive> *values)
{
    compilationUnit->constantValues.append(*values);
    values->clear();

    QVector<QV4::Primitive> &finalValues = compilationUnit->constantValues.last();
    finalValues.squeeze();
    return finalValues.data();
}

QV4::CompiledData::CompilationUnit *InstructionSelection::backendCompileStep()
{
    return compilationUnit;
}

void InstructionSelection::callBuiltinInvalid(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result)
{
    prepareCallData(args, 0);

    if (useFastLookups && func->global) {
        uint index = registerGlobalGetterLookup(*func->id);
        generateFunctionCall(result, __qmljs_call_global_lookup,
                             Assembler::ContextRegister,
                             Assembler::TrustedImm32(index),
                             baseAddressForCallData());
    } else {
        generateFunctionCall(result, __qmljs_call_activation_property,
                             Assembler::ContextRegister,
                             Assembler::PointerToString(*func->id),
                             baseAddressForCallData());
    }
}

void InstructionSelection::callBuiltinTypeofMember(V4IR::Expr *base, const QString &name,
                                                   V4IR::Temp *result)
{
    generateFunctionCall(result, __qmljs_builtin_typeof_member, Assembler::ContextRegister,
                         Assembler::PointerToValue(base), Assembler::PointerToString(name));
}

void InstructionSelection::callBuiltinTypeofSubscript(V4IR::Expr *base, V4IR::Expr *index,
                                                      V4IR::Temp *result)
{
    generateFunctionCall(result, __qmljs_builtin_typeof_element,
                         Assembler::ContextRegister,
                         Assembler::PointerToValue(base), Assembler::PointerToValue(index));
}

void InstructionSelection::callBuiltinTypeofName(const QString &name, V4IR::Temp *result)
{
    generateFunctionCall(result, __qmljs_builtin_typeof_name, Assembler::ContextRegister,
                         Assembler::PointerToString(name));
}

void InstructionSelection::callBuiltinTypeofValue(V4IR::Expr *value, V4IR::Temp *result)
{
    generateFunctionCall(result, __qmljs_builtin_typeof, Assembler::ContextRegister,
                         Assembler::PointerToValue(value));
}

void InstructionSelection::callBuiltinDeleteMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result)
{
    generateFunctionCall(result, __qmljs_delete_member, Assembler::ContextRegister,
                         Assembler::Reference(base), Assembler::PointerToString(name));
}

void InstructionSelection::callBuiltinDeleteSubscript(V4IR::Temp *base, V4IR::Expr *index,
                                                      V4IR::Temp *result)
{
    generateFunctionCall(result, __qmljs_delete_subscript, Assembler::ContextRegister,
                         Assembler::Reference(base), Assembler::PointerToValue(index));
}

void InstructionSelection::callBuiltinDeleteName(const QString &name, V4IR::Temp *result)
{
    generateFunctionCall(result, __qmljs_delete_name, Assembler::ContextRegister,
                         Assembler::PointerToString(name));
}

void InstructionSelection::callBuiltinDeleteValue(V4IR::Temp *result)
{
    _as->storeValue(Primitive::fromBoolean(false), result);
}

void InstructionSelection::callBuiltinThrow(V4IR::Expr *arg)
{
    generateFunctionCall(Assembler::ReturnValueRegister, __qmljs_throw, Assembler::ContextRegister,
                         Assembler::PointerToValue(arg));
}

void InstructionSelection::callBuiltinReThrow()
{
    _as->jumpToExceptionHandler();
}

void InstructionSelection::callBuiltinUnwindException(V4IR::Temp *result)
{
    generateFunctionCall(result, __qmljs_builtin_unwind_exception, Assembler::ContextRegister);

}

void InstructionSelection::callBuiltinPushCatchScope(const QString &exceptionName)
{
    Assembler::Pointer s = _as->loadStringAddress(Assembler::ScratchRegister, exceptionName);
    generateFunctionCall(Assembler::ContextRegister, __qmljs_builtin_push_catch_scope, Assembler::ContextRegister, s);
}

void InstructionSelection::callBuiltinForeachIteratorObject(V4IR::Temp *arg, V4IR::Temp *result)
{
    Q_ASSERT(arg);
    Q_ASSERT(result);

    generateFunctionCall(result, __qmljs_foreach_iterator_object, Assembler::ContextRegister, Assembler::Reference(arg));
}

void InstructionSelection::callBuiltinForeachNextPropertyname(V4IR::Temp *arg, V4IR::Temp *result)
{
    Q_ASSERT(arg);
    Q_ASSERT(result);

    generateFunctionCall(result, __qmljs_foreach_next_property_name, Assembler::Reference(arg));
}

void InstructionSelection::callBuiltinPushWithScope(V4IR::Temp *arg)
{
    Q_ASSERT(arg);

    generateFunctionCall(Assembler::ContextRegister, __qmljs_builtin_push_with_scope, Assembler::Reference(arg), Assembler::ContextRegister);
}

void InstructionSelection::callBuiltinPopScope()
{
    generateFunctionCall(Assembler::ContextRegister, __qmljs_builtin_pop_scope, Assembler::ContextRegister);
}

void InstructionSelection::callBuiltinDeclareVar(bool deletable, const QString &name)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_declare_var, Assembler::ContextRegister,
                         Assembler::TrustedImm32(deletable), Assembler::PointerToString(name));
}

void InstructionSelection::callBuiltinDefineGetterSetter(V4IR::Temp *object, const QString &name, V4IR::Temp *getter, V4IR::Temp *setter)
{
    Q_ASSERT(object);
    Q_ASSERT(getter);
    Q_ASSERT(setter);
    generateFunctionCall(Assembler::Void, __qmljs_builtin_define_getter_setter, Assembler::ContextRegister,
                         Assembler::Reference(object), Assembler::PointerToString(name), Assembler::PointerToValue(getter), Assembler::PointerToValue(setter));
}

void InstructionSelection::callBuiltinDefineProperty(V4IR::Temp *object, const QString &name,
                                                     V4IR::Expr *value)
{
    Q_ASSERT(object);
    Q_ASSERT(value->asTemp() || value->asConst());

    generateFunctionCall(Assembler::Void, __qmljs_builtin_define_property,
                         Assembler::ContextRegister, Assembler::Reference(object),
                         Assembler::PointerToString(name),
                         Assembler::PointerToValue(value));
}

void InstructionSelection::callBuiltinDefineArray(V4IR::Temp *result, V4IR::ExprList *args)
{
    Q_ASSERT(result);

    int length = prepareVariableArguments(args);
    generateFunctionCall(result, __qmljs_builtin_define_array, Assembler::ContextRegister,
                         baseAddressForCallArguments(), Assembler::TrustedImm32(length));
}

void InstructionSelection::callBuiltinDefineObjectLiteral(V4IR::Temp *result, V4IR::ExprList *args)
{
    Q_ASSERT(result);

    int argc = 0;

    const int classId = registerJSClass(args);

    V4IR::ExprList *it = args;
    while (it) {
        it = it->next;

        bool isData = it->expr->asConst()->value;
        it = it->next;

        _as->copyValue(_as->stackLayout().argumentAddressForCall(argc++), it->expr);

        if (!isData) {
            it = it->next;
            _as->copyValue(_as->stackLayout().argumentAddressForCall(argc++), it->expr);
        }

        it = it->next;
    }

    generateFunctionCall(result, __qmljs_builtin_define_object_literal, Assembler::ContextRegister,
                         baseAddressForCallArguments(), Assembler::TrustedImm32(classId));
}

void InstructionSelection::callBuiltinSetupArgumentObject(V4IR::Temp *result)
{
    generateFunctionCall(result, __qmljs_builtin_setup_arguments_object, Assembler::ContextRegister);
}

void InstructionSelection::callBuiltinConvertThisToObject()
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_convert_this_to_object, Assembler::ContextRegister);
}

void InstructionSelection::callValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result)
{
    Q_ASSERT(value);

    prepareCallData(args, 0);
    generateFunctionCall(result, __qmljs_call_value, Assembler::ContextRegister,
                         Assembler::Reference(value),
                         baseAddressForCallData());
}

void InstructionSelection::loadThisObject(V4IR::Temp *temp)
{
    _as->loadPtr(Address(Assembler::ContextRegister, qOffsetOf(ExecutionContext, callData)), Assembler::ScratchRegister);
#if defined(VALUE_FITS_IN_REGISTER)
    _as->load64(Pointer(Assembler::ScratchRegister, qOffsetOf(CallData, thisObject)),
                Assembler::ReturnValueRegister);
    _as->storeReturnValue(temp);
#else
    _as->copyValue(temp, Pointer(Assembler::ScratchRegister, qOffsetOf(CallData, thisObject)));
#endif
}

void InstructionSelection::loadQmlIdArray(V4IR::Temp *temp)
{
    generateFunctionCall(temp, __qmljs_get_id_array, Assembler::ContextRegister);
}

void InstructionSelection::loadQmlImportedScripts(V4IR::Temp *temp)
{
    generateFunctionCall(temp, __qmljs_get_imported_scripts, Assembler::ContextRegister);
}

void InstructionSelection::loadQmlContextObject(V4IR::Temp *temp)
{
    generateFunctionCall(temp, __qmljs_get_context_object, Assembler::ContextRegister);
}

void InstructionSelection::loadQmlScopeObject(V4IR::Temp *temp)
{
    generateFunctionCall(temp, __qmljs_get_scope_object, Assembler::ContextRegister);
}

void InstructionSelection::loadQmlSingleton(const QString &name, V4IR::Temp *temp)
{
    generateFunctionCall(temp, __qmljs_get_qml_singleton, Assembler::ContextRegister, Assembler::PointerToString(name));
}

void InstructionSelection::loadConst(V4IR::Const *sourceConst, V4IR::Temp *targetTemp)
{
    if (targetTemp->kind == V4IR::Temp::PhysicalRegister) {
        if (targetTemp->type == V4IR::DoubleType) {
            Q_ASSERT(sourceConst->type == V4IR::DoubleType);
            _as->toDoubleRegister(sourceConst, (Assembler::FPRegisterID) targetTemp->index);
        } else if (targetTemp->type == V4IR::SInt32Type) {
            Q_ASSERT(sourceConst->type == V4IR::SInt32Type);
            _as->toInt32Register(sourceConst, (Assembler::RegisterID) targetTemp->index);
        } else if (targetTemp->type == V4IR::UInt32Type) {
            Q_ASSERT(sourceConst->type == V4IR::UInt32Type);
            _as->toUInt32Register(sourceConst, (Assembler::RegisterID) targetTemp->index);
        } else if (targetTemp->type == V4IR::BoolType) {
            Q_ASSERT(sourceConst->type == V4IR::BoolType);
            _as->move(Assembler::TrustedImm32(convertToValue(sourceConst).int_32),
                      (Assembler::RegisterID) targetTemp->index);
        } else {
            Q_UNREACHABLE();
        }
    } else {
        _as->storeValue(convertToValue(sourceConst), targetTemp);
    }
}

void InstructionSelection::loadString(const QString &str, V4IR::Temp *targetTemp)
{
    Pointer srcAddr = _as->loadStringAddress(Assembler::ReturnValueRegister, str);
    _as->loadPtr(srcAddr, Assembler::ReturnValueRegister);
    Pointer destAddr = _as->loadTempAddress(Assembler::ScratchRegister, targetTemp);
#if QT_POINTER_SIZE == 8
    _as->store64(Assembler::ReturnValueRegister, destAddr);
#else
    _as->store32(Assembler::ReturnValueRegister, destAddr);
    destAddr.offset += 4;
    _as->store32(Assembler::TrustedImm32(QV4::Value::Managed_Type), destAddr);
#endif
}

void InstructionSelection::loadRegexp(V4IR::RegExp *sourceRegexp, V4IR::Temp *targetTemp)
{
    int id = registerRegExp(sourceRegexp);
    generateFunctionCall(targetTemp, __qmljs_lookup_runtime_regexp, Assembler::ContextRegister, Assembler::TrustedImm32(id));
}

void InstructionSelection::getActivationProperty(const V4IR::Name *name, V4IR::Temp *temp)
{
    if (useFastLookups && name->global) {
        uint index = registerGlobalGetterLookup(*name->id);
        generateLookupCall(temp, index, qOffsetOf(QV4::Lookup, globalGetter), Assembler::ContextRegister, Assembler::Void);
        return;
    }
    generateFunctionCall(temp, __qmljs_get_activation_property, Assembler::ContextRegister, Assembler::PointerToString(*name->id));
}

void InstructionSelection::setActivationProperty(V4IR::Expr *source, const QString &targetName)
{
    // ### should use a lookup call here
    generateFunctionCall(Assembler::Void, __qmljs_set_activation_property,
                         Assembler::ContextRegister, Assembler::PointerToString(targetName), Assembler::PointerToValue(source));
}

void InstructionSelection::initClosure(V4IR::Closure *closure, V4IR::Temp *target)
{
    int id = closure->value;
    generateFunctionCall(target, __qmljs_init_closure, Assembler::ContextRegister, Assembler::TrustedImm32(id));
}

void InstructionSelection::getProperty(V4IR::Expr *base, const QString &name, V4IR::Temp *target)
{
    if (useFastLookups) {
        uint index = registerGetterLookup(name);
        generateLookupCall(target, index, qOffsetOf(QV4::Lookup, getter), Assembler::PointerToValue(base), Assembler::Void);
    } else {
        generateFunctionCall(target, __qmljs_get_property, Assembler::ContextRegister,
                             Assembler::PointerToValue(base), Assembler::PointerToString(name));
    }
}

void InstructionSelection::getQObjectProperty(V4IR::Expr *base, int propertyIndex, bool captureRequired, int attachedPropertiesId, V4IR::Temp *target)
{
    if (attachedPropertiesId != 0)
        generateFunctionCall(target, __qmljs_get_attached_property, Assembler::ContextRegister, Assembler::TrustedImm32(attachedPropertiesId), Assembler::TrustedImm32(propertyIndex));
    else
        generateFunctionCall(target, __qmljs_get_qobject_property, Assembler::ContextRegister, Assembler::PointerToValue(base), Assembler::TrustedImm32(propertyIndex),
                             Assembler::TrustedImm32(captureRequired));
}

void InstructionSelection::setProperty(V4IR::Expr *source, V4IR::Expr *targetBase,
                                       const QString &targetName)
{
    if (useFastLookups) {
        uint index = registerSetterLookup(targetName);
        generateLookupCall(Assembler::Void, index, qOffsetOf(QV4::Lookup, setter),
                           Assembler::PointerToValue(targetBase),
                           Assembler::PointerToValue(source));
    } else {
        generateFunctionCall(Assembler::Void, __qmljs_set_property, Assembler::ContextRegister,
                             Assembler::PointerToValue(targetBase), Assembler::PointerToString(targetName),
                             Assembler::PointerToValue(source));
    }
}

void InstructionSelection::setQObjectProperty(V4IR::Expr *source, V4IR::Expr *targetBase, int propertyIndex)
{
    generateFunctionCall(Assembler::Void, __qmljs_set_qobject_property, Assembler::ContextRegister, Assembler::PointerToValue(targetBase),
                         Assembler::TrustedImm32(propertyIndex), Assembler::PointerToValue(source));
}

void InstructionSelection::getElement(V4IR::Expr *base, V4IR::Expr *index, V4IR::Temp *target)
{
#if QT_POINTER_SIZE == 8
    V4IR::Temp *tbase = base->asTemp();
    V4IR::Temp *tindex = index->asTemp();
    if (tbase && tindex &&
        tbase->kind != V4IR::Temp::PhysicalRegister) {
        Assembler::Pointer addr = _as->loadTempAddress(Assembler::ReturnValueRegister, tbase);
        _as->load64(addr, Assembler::ScratchRegister);
        _as->move(Assembler::ScratchRegister, Assembler::ReturnValueRegister);
        _as->urshift64(Assembler::TrustedImm32(QV4::Value::IsManaged_Shift), Assembler::ReturnValueRegister);
        Assembler::Jump notManaged = _as->branch64(Assembler::NotEqual, Assembler::ReturnValueRegister, Assembler::TrustedImm64(0));
        // check whether we have an object with a simple array
        Assembler::Address managedType(Assembler::ScratchRegister, qOffsetOf(QV4::Managed, flags));
        _as->load8(managedType, Assembler::ReturnValueRegister);
        _as->and32(Assembler::TrustedImm32(QV4::Managed::SimpleArray), Assembler::ReturnValueRegister);
        Assembler::Jump notSimple = _as->branch32(Assembler::Equal, Assembler::ReturnValueRegister, Assembler::TrustedImm32(0));

        bool needNegativeCheck = false;
        Assembler::Jump fallback, fallback2;
        if (tindex->kind == V4IR::Temp::PhysicalRegister) {
            if (tindex->type == V4IR::SInt32Type) {
                fallback = _as->branch32(Assembler::LessThan, (Assembler::RegisterID)tindex->index, Assembler::TrustedImm32(0));
                _as->move((Assembler::RegisterID) tindex->index, Assembler::ScratchRegister);
                needNegativeCheck = true;
            } else {
                // double, convert and check if it's a int
                fallback2 = _as->branchTruncateDoubleToUint32((Assembler::FPRegisterID) tindex->index, Assembler::ScratchRegister);
                _as->convertInt32ToDouble(Assembler::ScratchRegister, Assembler::FPGpr0);
                fallback = _as->branchDouble(Assembler::DoubleNotEqual, Assembler::FPGpr0, (Assembler::FPRegisterID) tindex->index);
            }
        } else {
            Assembler::Pointer indexAddr = _as->loadTempAddress(Assembler::ReturnValueRegister, tindex);
            _as->load64(indexAddr, Assembler::ScratchRegister);
            _as->move(Assembler::ScratchRegister, Assembler::ReturnValueRegister);
            _as->urshift64(Assembler::TrustedImm32(QV4::Value::IsNumber_Shift), Assembler::ReturnValueRegister);
            Assembler::Jump isInteger = _as->branch64(Assembler::Equal, Assembler::ReturnValueRegister, Assembler::TrustedImm64(1));

            // other type, convert to double and check if it's a int
            // this check is ok to do even if the type is something else than a double, as
            // that would result in a NaN
            _as->move(Assembler::TrustedImm64(QV4::Value::NaNEncodeMask), Assembler::ReturnValueRegister);
            _as->xor64(Assembler::ScratchRegister, Assembler::ReturnValueRegister);
            _as->move64ToDouble(Assembler::ReturnValueRegister, Assembler::FPGpr0);
            fallback2 = _as->branchTruncateDoubleToUint32(Assembler::FPGpr0, Assembler::ScratchRegister);
            _as->convertInt32ToDouble(Assembler::ScratchRegister, Assembler::FPGpr1);
            fallback = _as->branchDouble(Assembler::DoubleNotEqualOrUnordered, Assembler::FPGpr0, Assembler::FPGpr1);

            isInteger.link(_as);
            _as->or32(Assembler::TrustedImm32(0), Assembler::ScratchRegister);
            needNegativeCheck = true;
        }

        // get data, ScratchRegister holds index
        addr = _as->loadTempAddress(Assembler::ReturnValueRegister, tbase);
        _as->load64(addr, Assembler::ReturnValueRegister);
        Address arrayDataLen(Assembler::ReturnValueRegister, qOffsetOf(Object, arrayDataLen));
        Assembler::Jump outOfRange;
        if (needNegativeCheck)
            outOfRange = _as->branch32(Assembler::LessThan, Assembler::ScratchRegister, Assembler::TrustedImm32(0));
        Assembler::Jump outOfRange2 = _as->branch32(Assembler::GreaterThanOrEqual, Assembler::ScratchRegister, arrayDataLen);
        Address arrayData(Assembler::ReturnValueRegister, qOffsetOf(Object, arrayData));
        _as->load64(arrayData, Assembler::ReturnValueRegister);
        Q_ASSERT(sizeof(Property) == (1<<4));
        _as->lshift64(Assembler::TrustedImm32(4), Assembler::ScratchRegister);
        _as->add64(Assembler::ReturnValueRegister, Assembler::ScratchRegister);
        Address value(Assembler::ScratchRegister, qOffsetOf(Property, value));
        _as->load64(value, Assembler::ReturnValueRegister);

        // check that the value is not empty
        _as->move(Assembler::ReturnValueRegister, Assembler::ScratchRegister);
        _as->urshift64(Assembler::TrustedImm32(32), Assembler::ScratchRegister);
        Assembler::Jump emptyValue = _as->branch32(Assembler::Equal, Assembler::TrustedImm32(QV4::Value::Empty_Type), Assembler::ScratchRegister);
        _as->storeReturnValue(target);

        Assembler::Jump done = _as->jump();

        emptyValue.link(_as);
        if (outOfRange.isSet())
            outOfRange.link(_as);
        outOfRange2.link(_as);
        if (fallback.isSet())
            fallback.link(_as);
        if (fallback2.isSet())
            fallback2.link(_as);
        notSimple.link(_as);
        notManaged.link(_as);

        generateFunctionCall(target, __qmljs_get_element, Assembler::ContextRegister,
                             Assembler::PointerToValue(base), Assembler::PointerToValue(index));

        done.link(_as);
        return;
    }
#endif

    generateFunctionCall(target, __qmljs_get_element, Assembler::ContextRegister,
                         Assembler::PointerToValue(base), Assembler::PointerToValue(index));
}

void InstructionSelection::setElement(V4IR::Expr *source, V4IR::Expr *targetBase, V4IR::Expr *targetIndex)
{
    generateFunctionCall(Assembler::Void, __qmljs_set_element, Assembler::ContextRegister,
                         Assembler::PointerToValue(targetBase), Assembler::PointerToValue(targetIndex),
                         Assembler::PointerToValue(source));
}

void InstructionSelection::copyValue(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp)
{
    if (*sourceTemp == *targetTemp)
        return;

    if (sourceTemp->kind == V4IR::Temp::PhysicalRegister) {
        if (targetTemp->kind == V4IR::Temp::PhysicalRegister) {
            if (sourceTemp->type == V4IR::DoubleType)
                _as->moveDouble((Assembler::FPRegisterID) sourceTemp->index,
                                (Assembler::FPRegisterID) targetTemp->index);
            else
                _as->move((Assembler::RegisterID) sourceTemp->index,
                          (Assembler::RegisterID) targetTemp->index);
            return;
        } else {
            switch (sourceTemp->type) {
            case V4IR::DoubleType:
                _as->storeDouble((Assembler::FPRegisterID) sourceTemp->index, targetTemp);
                break;
            case V4IR::SInt32Type:
                _as->storeInt32((Assembler::RegisterID) sourceTemp->index, targetTemp);
                break;
            case V4IR::UInt32Type:
                _as->storeUInt32((Assembler::RegisterID) sourceTemp->index, targetTemp);
                break;
            case V4IR::BoolType:
                _as->storeBool((Assembler::RegisterID) sourceTemp->index, targetTemp);
                break;
            default:
                Q_ASSERT(!"Unreachable");
                break;
            }
            return;
        }
    } else if (targetTemp->kind == V4IR::Temp::PhysicalRegister) {
        switch (targetTemp->type) {
        case V4IR::DoubleType:
            Q_ASSERT(sourceTemp->type == V4IR::DoubleType);
            _as->toDoubleRegister(sourceTemp, (Assembler::FPRegisterID) targetTemp->index);
            return;
        case V4IR::BoolType:
            Q_ASSERT(sourceTemp->type == V4IR::BoolType);
            _as->toInt32Register(sourceTemp, (Assembler::RegisterID) targetTemp->index);
            return;
        case V4IR::SInt32Type:
            Q_ASSERT(sourceTemp->type == V4IR::SInt32Type);
            _as->toInt32Register(sourceTemp, (Assembler::RegisterID) targetTemp->index);
            return;
        case V4IR::UInt32Type:
            Q_ASSERT(sourceTemp->type == V4IR::UInt32Type);
            _as->toUInt32Register(sourceTemp, (Assembler::RegisterID) targetTemp->index);
            return;
        default:
            Q_ASSERT(!"Unreachable");
            break;
        }
    }

    // The target is not a physical register, nor is the source. So we can do a memory-to-memory copy:
    _as->memcopyValue(_as->loadTempAddress(Assembler::ReturnValueRegister, targetTemp), sourceTemp,
                      Assembler::ScratchRegister);
}

void InstructionSelection::swapValues(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp)
{
    if (sourceTemp->kind == V4IR::Temp::PhysicalRegister) {
        if (targetTemp->kind == V4IR::Temp::PhysicalRegister) {
            Q_ASSERT(sourceTemp->type == targetTemp->type);

            if (sourceTemp->type == V4IR::DoubleType) {
                _as->moveDouble((Assembler::FPRegisterID) targetTemp->index, Assembler::FPGpr0);
                _as->moveDouble((Assembler::FPRegisterID) sourceTemp->index,
                                (Assembler::FPRegisterID) targetTemp->index);
                _as->moveDouble(Assembler::FPGpr0, (Assembler::FPRegisterID) sourceTemp->index);
            } else {
                _as->swap((Assembler::RegisterID) sourceTemp->index,
                          (Assembler::RegisterID) targetTemp->index);
            }
            return;
        }
    } else if (sourceTemp->kind == V4IR::Temp::StackSlot) {
        if (targetTemp->kind == V4IR::Temp::StackSlot) {
            // Note: a swap for two stack-slots can involve different types.
#if CPU(X86_64)
            _as->load64(_as->stackSlotPointer(targetTemp), Assembler::ReturnValueRegister);
            _as->load64(_as->stackSlotPointer(sourceTemp), Assembler::ScratchRegister);
            _as->store64(Assembler::ScratchRegister, _as->stackSlotPointer(targetTemp));
            _as->store64(Assembler::ReturnValueRegister, _as->stackSlotPointer(sourceTemp));
#else
            Assembler::FPRegisterID tReg = _as->toDoubleRegister(targetTemp);
            Assembler::Pointer sAddr = _as->stackSlotPointer(sourceTemp);
            Assembler::Pointer tAddr = _as->stackSlotPointer(targetTemp);
            _as->load32(sAddr, Assembler::ScratchRegister);
            _as->store32(Assembler::ScratchRegister, tAddr);
            sAddr.offset += 4;
            tAddr.offset += 4;
            _as->load32(sAddr, Assembler::ScratchRegister);
            _as->store32(Assembler::ScratchRegister, tAddr);
            _as->storeDouble(tReg, _as->stackSlotPointer(sourceTemp));
#endif
            return;
        }
    }

    V4IR::Temp *stackTemp = sourceTemp->kind == V4IR::Temp::StackSlot ? sourceTemp : targetTemp;
    V4IR::Temp *registerTemp = sourceTemp->kind == V4IR::Temp::PhysicalRegister ? sourceTemp
                                                                                : targetTemp;
    Assembler::Pointer addr = _as->stackSlotPointer(stackTemp);
    if (registerTemp->type == V4IR::DoubleType) {
        _as->loadDouble(addr, Assembler::FPGpr0);
        _as->storeDouble((Assembler::FPRegisterID) registerTemp->index, addr);
        _as->moveDouble(Assembler::FPGpr0, (Assembler::FPRegisterID) registerTemp->index);
    } else if (registerTemp->type == V4IR::UInt32Type) {
        _as->toUInt32Register(addr, Assembler::ScratchRegister);
        _as->storeUInt32((Assembler::RegisterID) registerTemp->index, addr);
        _as->move(Assembler::ScratchRegister, (Assembler::RegisterID) registerTemp->index);
    } else {
        _as->load32(addr, Assembler::ScratchRegister);
        _as->store32((Assembler::RegisterID) registerTemp->index, addr);
        addr.offset += 4;
        quint32 tag;
        switch (registerTemp->type) {
        case V4IR::BoolType:
            tag = QV4::Value::_Boolean_Type;
            break;
        case V4IR::SInt32Type:
            tag = QV4::Value::_Integer_Type;
            break;
        default:
            tag = QV4::Value::Undefined_Type;
            Q_UNREACHABLE();
        }
        _as->store32(Assembler::TrustedImm32(tag), addr);
        _as->move(Assembler::ScratchRegister, (Assembler::RegisterID) registerTemp->index);
    }
}

#define setOp(op, opName, operation) \
    do { op = operation; opName = isel_stringIfy(operation); } while (0)
#define setOpContext(op, opName, operation) \
    do { opContext = operation; opName = isel_stringIfy(operation); } while (0)

void InstructionSelection::unop(V4IR::AluOp oper, V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp)
{
    UnaryOpName op = 0;
    const char *opName = 0;
    switch (oper) {
    case V4IR::OpIfTrue: assert(!"unreachable"); break;
    case V4IR::OpNot:
        if (sourceTemp->type == V4IR::BoolType && targetTemp->type == V4IR::BoolType) {
            Assembler::RegisterID tReg = Assembler::ScratchRegister;
            if (targetTemp->kind == V4IR::Temp::PhysicalRegister)
                tReg = (Assembler::RegisterID) targetTemp->index;
            _as->xor32(Assembler::TrustedImm32(0x1),
                       _as->toInt32Register(sourceTemp, Assembler::ScratchRegister),
                       tReg);
            if (targetTemp->kind != V4IR::Temp::PhysicalRegister)
                _as->storeBool(tReg, targetTemp);
            return;
        } else {
            setOp(op, opName, __qmljs_not); break;
        }
    case V4IR::OpUMinus: setOp(op, opName, __qmljs_uminus); break;
    case V4IR::OpUPlus: setOp(op, opName, __qmljs_uplus); break;
    case V4IR::OpCompl: setOp(op, opName, __qmljs_compl); break;
    case V4IR::OpIncrement: setOp(op, opName, __qmljs_increment); break;
    case V4IR::OpDecrement: setOp(op, opName, __qmljs_decrement); break;
    default: assert(!"unreachable"); break;
    } // switch

    if (op) {
        _as->generateFunctionCallImp(targetTemp, opName, op,
                                     Assembler::PointerToValue(sourceTemp));
    }
}

static inline Assembler::FPRegisterID getFreeFPReg(V4IR::Expr *shouldNotOverlap, unsigned hint)
{
    if (V4IR::Temp *t = shouldNotOverlap->asTemp())
        if (t->type == V4IR::DoubleType)
            if (t->kind == V4IR::Temp::PhysicalRegister)
                if (t->index == hint)
                    return Assembler::FPRegisterID(hint + 1);
    return Assembler::FPRegisterID(hint);
}

Assembler::Jump InstructionSelection::genInlineBinop(V4IR::AluOp oper, V4IR::Expr *leftSource, V4IR::Expr *rightSource, V4IR::Temp *target)
{
    Assembler::Jump done;

    // Try preventing a call for a few common binary operations. This is used in two cases:
    // - no register allocation was performed (not available for the platform, or the IR was
    //   not transformed into SSA)
    // - type inference found that either or both operands can be of non-number type, and the
    //   register allocator will have prepared for a call (meaning: all registers that do not
    //   hold operands are spilled to the stack, which makes them available here)
    // Note: FPGPr0 can still not be used, because uint32->double conversion uses it as a scratch
    //       register.
    switch (oper) {
    case V4IR::OpAdd: {
        Assembler::FPRegisterID lReg = getFreeFPReg(rightSource, 2);
        Assembler::FPRegisterID rReg = getFreeFPReg(leftSource, 4);
        Assembler::Jump leftIsNoDbl = genTryDoubleConversion(leftSource, lReg);
        Assembler::Jump rightIsNoDbl = genTryDoubleConversion(rightSource, rReg);

        _as->addDouble(rReg, lReg);
        _as->storeDouble(lReg, target);
        done = _as->jump();

        if (leftIsNoDbl.isSet())
            leftIsNoDbl.link(_as);
        if (rightIsNoDbl.isSet())
            rightIsNoDbl.link(_as);
    } break;
    case V4IR::OpMul: {
        Assembler::FPRegisterID lReg = getFreeFPReg(rightSource, 2);
        Assembler::FPRegisterID rReg = getFreeFPReg(leftSource, 4);
        Assembler::Jump leftIsNoDbl = genTryDoubleConversion(leftSource, lReg);
        Assembler::Jump rightIsNoDbl = genTryDoubleConversion(rightSource, rReg);

        _as->mulDouble(rReg, lReg);
        _as->storeDouble(lReg, target);
        done = _as->jump();

        if (leftIsNoDbl.isSet())
            leftIsNoDbl.link(_as);
        if (rightIsNoDbl.isSet())
            rightIsNoDbl.link(_as);
    } break;
    case V4IR::OpSub: {
        Assembler::FPRegisterID lReg = getFreeFPReg(rightSource, 2);
        Assembler::FPRegisterID rReg = getFreeFPReg(leftSource, 4);
        Assembler::Jump leftIsNoDbl = genTryDoubleConversion(leftSource, lReg);
        Assembler::Jump rightIsNoDbl = genTryDoubleConversion(rightSource, rReg);

        _as->subDouble(rReg, lReg);
        _as->storeDouble(lReg, target);
        done = _as->jump();

        if (leftIsNoDbl.isSet())
            leftIsNoDbl.link(_as);
        if (rightIsNoDbl.isSet())
            rightIsNoDbl.link(_as);
    } break;
    case V4IR::OpDiv: {
        Assembler::FPRegisterID lReg = getFreeFPReg(rightSource, 2);
        Assembler::FPRegisterID rReg = getFreeFPReg(leftSource, 4);
        Assembler::Jump leftIsNoDbl = genTryDoubleConversion(leftSource, lReg);
        Assembler::Jump rightIsNoDbl = genTryDoubleConversion(rightSource, rReg);

        _as->divDouble(rReg, lReg);
        _as->storeDouble(lReg, target);
        done = _as->jump();

        if (leftIsNoDbl.isSet())
            leftIsNoDbl.link(_as);
        if (rightIsNoDbl.isSet())
            rightIsNoDbl.link(_as);
    } break;
    default:
        break;
    }

    return done;
}

void InstructionSelection::binop(V4IR::AluOp oper, V4IR::Expr *leftSource, V4IR::Expr *rightSource, V4IR::Temp *target)
{
    if (oper != V4IR::OpMod
            && leftSource->type == V4IR::DoubleType && rightSource->type == V4IR::DoubleType
            && isPregOrConst(leftSource) && isPregOrConst(rightSource)) {
        doubleBinop(oper, leftSource, rightSource, target);
        return;
    }
    if (leftSource->type == V4IR::SInt32Type && rightSource->type == V4IR::SInt32Type) {
        if (int32Binop(oper, leftSource, rightSource, target))
            return;
    }

    Assembler::Jump done;
    if (leftSource->type != V4IR::StringType && rightSource->type != V4IR::StringType)
        done = genInlineBinop(oper, leftSource, rightSource, target);

    // TODO: inline var===null and var!==null
    Assembler::BinaryOperationInfo info = Assembler::binaryOperation(oper);

    if (oper == V4IR::OpAdd &&
            (leftSource->type == V4IR::StringType || rightSource->type == V4IR::StringType)) {
        const Assembler::BinaryOperationInfo stringAdd = OPCONTEXT(__qmljs_add_string);
        info = stringAdd;
    }

    if (info.fallbackImplementation) {
        _as->generateFunctionCallImp(target, info.name, info.fallbackImplementation,
                                     Assembler::PointerToValue(leftSource),
                                     Assembler::PointerToValue(rightSource));
    } else if (info.contextImplementation) {
        _as->generateFunctionCallImp(target, info.name, info.contextImplementation,
                                     Assembler::ContextRegister,
                                     Assembler::PointerToValue(leftSource),
                                     Assembler::PointerToValue(rightSource));
    } else {
        assert(!"unreachable");
    }

    if (done.isSet())
        done.link(_as);
}

void InstructionSelection::callProperty(V4IR::Expr *base, const QString &name, V4IR::ExprList *args,
                                        V4IR::Temp *result)
{
    assert(base != 0);

    prepareCallData(args, base);

    if (useFastLookups) {
        uint index = registerGetterLookup(name);
        generateFunctionCall(result, __qmljs_call_property_lookup,
                             Assembler::ContextRegister,
                             Assembler::TrustedImm32(index),
                             baseAddressForCallData());
    } else
    {
        generateFunctionCall(result, __qmljs_call_property, Assembler::ContextRegister,
                             Assembler::PointerToString(name),
                             baseAddressForCallData());
    }
}

void InstructionSelection::callSubscript(V4IR::Expr *base, V4IR::Expr *index, V4IR::ExprList *args,
                                         V4IR::Temp *result)
{
    assert(base != 0);

    prepareCallData(args, base);
    generateFunctionCall(result, __qmljs_call_element, Assembler::ContextRegister,
                         Assembler::PointerToValue(index),
                         baseAddressForCallData());
}

void InstructionSelection::convertType(V4IR::Temp *source, V4IR::Temp *target)
{
    switch (target->type) {
    case V4IR::DoubleType:
        convertTypeToDouble(source, target);
        break;
    case V4IR::BoolType:
        convertTypeToBool(source, target);
        break;
    case V4IR::SInt32Type:
        convertTypeToSInt32(source, target);
        break;
    case V4IR::UInt32Type:
        convertTypeToUInt32(source, target);
        break;
    default:
        convertTypeSlowPath(source, target);
        break;
    }
}

void InstructionSelection::convertTypeSlowPath(V4IR::Temp *source, V4IR::Temp *target)
{
    Q_ASSERT(target->type != V4IR::BoolType);

    if (target->type & V4IR::NumberType)
        unop(V4IR::OpUPlus, source, target);
    else
        copyValue(source, target);
}

void InstructionSelection::convertTypeToDouble(V4IR::Temp *source, V4IR::Temp *target)
{
    switch (source->type) {
    case V4IR::SInt32Type:
    case V4IR::BoolType:
    case V4IR::NullType:
        convertIntToDouble(source, target);
        break;
    case V4IR::UInt32Type:
        convertUIntToDouble(source, target);
        break;
    case V4IR::UndefinedType:
        _as->loadDouble(_as->loadTempAddress(Assembler::ScratchRegister, source), Assembler::FPGpr0);
        _as->storeDouble(Assembler::FPGpr0, target);
        break;
    case V4IR::StringType:
    case V4IR::VarType: {
        // load the tag:
        Assembler::Pointer tagAddr = _as->loadTempAddress(Assembler::ScratchRegister, source);
        tagAddr.offset += 4;
        _as->load32(tagAddr, Assembler::ScratchRegister);

        // check if it's an int32:
        Assembler::Jump isNoInt = _as->branch32(Assembler::NotEqual, Assembler::ScratchRegister,
                                                Assembler::TrustedImm32(Value::_Integer_Type));
        convertIntToDouble(source, target);
        Assembler::Jump intDone = _as->jump();

        // not an int, check if it's NOT a double:
        isNoInt.link(_as);
#if QT_POINTER_SIZE == 8
        _as->and32(Assembler::TrustedImm32(Value::IsDouble_Mask), Assembler::ScratchRegister);
        Assembler::Jump isDbl = _as->branch32(Assembler::NotEqual, Assembler::ScratchRegister,
                                              Assembler::TrustedImm32(0));
#else
        _as->and32(Assembler::TrustedImm32(Value::NotDouble_Mask), Assembler::ScratchRegister);
        Assembler::Jump isDbl = _as->branch32(Assembler::NotEqual, Assembler::ScratchRegister,
                                              Assembler::TrustedImm32(Value::NotDouble_Mask));
#endif

        generateFunctionCall(target, __qmljs_value_to_double, Assembler::PointerToValue(source));
        Assembler::Jump noDoubleDone = _as->jump();

        // it is a double:
        isDbl.link(_as);
        Assembler::Pointer addr2 = _as->loadTempAddress(Assembler::ScratchRegister, source);
        if (target->kind == V4IR::Temp::StackSlot) {
#if QT_POINTER_SIZE == 8
            _as->load64(addr2, Assembler::ScratchRegister);
            _as->store64(Assembler::ScratchRegister, _as->stackSlotPointer(target));
#else
            _as->loadDouble(addr2, Assembler::FPGpr0);
            _as->storeDouble(Assembler::FPGpr0, _as->stackSlotPointer(target));
#endif
        } else {
            _as->loadDouble(addr2, (Assembler::FPRegisterID) target->index);
        }

        noDoubleDone.link(_as);
        intDone.link(_as);
    } break;
    default:
        convertTypeSlowPath(source, target);
        break;
    }
}

void InstructionSelection::convertTypeToBool(V4IR::Temp *source, V4IR::Temp *target)
{
    switch (source->type) {
    case V4IR::SInt32Type:
    case V4IR::UInt32Type:
        convertIntToBool(source, target);
        break;
    case V4IR::DoubleType: {
        // The source is in a register if the register allocator is used. If the register
        // allocator was not used, then that means that we can use any register for to
        // load the double into.
        Assembler::FPRegisterID reg;
        if (source->kind == V4IR::Temp::PhysicalRegister)
            reg = (Assembler::FPRegisterID) source->index;
        else
            reg = _as->toDoubleRegister(source, (Assembler::FPRegisterID) 1);
        Assembler::Jump nonZero = _as->branchDoubleNonZero(reg, Assembler::FPGpr0);

        // it's 0, so false:
        _as->storeBool(false, target);
        Assembler::Jump done = _as->jump();

        // it's non-zero, so true:
        nonZero.link(_as);
        _as->storeBool(true, target);

        // done:
        done.link(_as);
    } break;
    case V4IR::UndefinedType:
    case V4IR::NullType:
        _as->storeBool(false, target);
        break;
    case V4IR::StringType:
    case V4IR::VarType:
    default:
        generateFunctionCall(Assembler::ReturnValueRegister, __qmljs_to_boolean,
                             Assembler::PointerToValue(source));
        _as->storeBool(Assembler::ReturnValueRegister, target);
        break;
    }
}

void InstructionSelection::convertTypeToSInt32(V4IR::Temp *source, V4IR::Temp *target)
{
    switch (source->type) {
    case V4IR::VarType: {

#if QT_POINTER_SIZE == 8
        Assembler::Pointer addr = _as->loadTempAddress(Assembler::ScratchRegister, source);
        _as->load64(addr, Assembler::ScratchRegister);
        _as->move(Assembler::ScratchRegister, Assembler::ReturnValueRegister);

        // check if it's a number
        _as->urshift64(Assembler::TrustedImm32(QV4::Value::IsNumber_Shift), Assembler::ScratchRegister);
        Assembler::Jump fallback = _as->branch32(Assembler::Equal, Assembler::ScratchRegister, Assembler::TrustedImm32(0));
        // we have a number
        _as->urshift64(Assembler::TrustedImm32(1), Assembler::ScratchRegister);
        Assembler::Jump isInt = _as->branch32(Assembler::Equal, Assembler::ScratchRegister, Assembler::TrustedImm32(0));

        // it's a double
        _as->move(Assembler::TrustedImm64(QV4::Value::NaNEncodeMask), Assembler::ScratchRegister);
        _as->xor64(Assembler::ScratchRegister, Assembler::ReturnValueRegister);
        _as->move64ToDouble(Assembler::ReturnValueRegister, Assembler::FPGpr0);
        Assembler::Jump success =
                _as->branchTruncateDoubleToInt32(Assembler::FPGpr0, Assembler::ReturnValueRegister,
                                                 Assembler::BranchIfTruncateSuccessful);

        // not an int:
        fallback.link(_as);
        generateFunctionCall(Assembler::ReturnValueRegister, __qmljs_value_to_int32,
                             _as->loadTempAddress(Assembler::ScratchRegister, source));

        isInt.link(_as);
        success.link(_as);
        if (target->kind == V4IR::Temp::StackSlot) {
            Assembler::Pointer targetAddr = _as->stackSlotPointer(target);
            _as->store32(Assembler::ReturnValueRegister, targetAddr);
            targetAddr.offset += 4;
            _as->store32(Assembler::TrustedImm32(Value::_Integer_Type), targetAddr);
        } else {
            _as->storeInt32(Assembler::ReturnValueRegister, target);
        }
#else
        // load the tag:
        Assembler::Pointer addr = _as->loadTempAddress(Assembler::ScratchRegister, source);
        Assembler::Pointer tagAddr = addr;
        tagAddr.offset += 4;
        _as->load32(tagAddr, Assembler::ReturnValueRegister);

        // check if it's an int32:
        Assembler::Jump fallback = _as->branch32(Assembler::NotEqual, Assembler::ReturnValueRegister,
                                                Assembler::TrustedImm32(Value::_Integer_Type));
        if (target->kind == V4IR::Temp::StackSlot) {
            _as->load32(addr, Assembler::ScratchRegister);
            Assembler::Pointer targetAddr = _as->stackSlotPointer(target);
            _as->store32(Assembler::ScratchRegister, targetAddr);
            targetAddr.offset += 4;
            _as->store32(Assembler::TrustedImm32(Value::_Integer_Type), targetAddr);
        } else {
            _as->load32(addr, (Assembler::RegisterID) target->index);
        }
        Assembler::Jump intDone = _as->jump();

        // not an int:
        fallback.link(_as);
        generateFunctionCall(Assembler::ReturnValueRegister, __qmljs_value_to_int32,
                             _as->loadTempAddress(Assembler::ScratchRegister, source));
        _as->storeInt32(Assembler::ReturnValueRegister, target);

        intDone.link(_as);
#endif

    } break;
    case V4IR::DoubleType: {
        Assembler::Jump success =
                _as->branchTruncateDoubleToInt32(_as->toDoubleRegister(source),
                                                 Assembler::ReturnValueRegister,
                                                 Assembler::BranchIfTruncateSuccessful);
        generateFunctionCall(Assembler::ReturnValueRegister, __qmljs_double_to_int32,
                             Assembler::PointerToValue(source));
        success.link(_as);
        _as->storeInt32(Assembler::ReturnValueRegister, target);
    } break;
    case V4IR::UInt32Type:
        _as->storeInt32(_as->toUInt32Register(source, Assembler::ReturnValueRegister), target);
        break;
    case V4IR::NullType:
    case V4IR::UndefinedType:
        _as->move(Assembler::TrustedImm32(0), Assembler::ReturnValueRegister);
        _as->storeInt32(Assembler::ReturnValueRegister, target);
        break;
    case V4IR::BoolType:
        _as->storeInt32(_as->toInt32Register(source, Assembler::ReturnValueRegister), target);
        break;
    case V4IR::StringType:
    default:
        generateFunctionCall(Assembler::ReturnValueRegister, __qmljs_value_to_int32,
                             _as->loadTempAddress(Assembler::ScratchRegister, source));
        _as->storeInt32(Assembler::ReturnValueRegister, target);
        break;
    } // switch (source->type)
}

void InstructionSelection::convertTypeToUInt32(V4IR::Temp *source, V4IR::Temp *target)
{
    switch (source->type) {
    case V4IR::VarType: {
        // load the tag:
        Assembler::Pointer tagAddr = _as->loadTempAddress(Assembler::ScratchRegister, source);
        tagAddr.offset += 4;
        _as->load32(tagAddr, Assembler::ScratchRegister);

        // check if it's an int32:
        Assembler::Jump isNoInt = _as->branch32(Assembler::NotEqual, Assembler::ScratchRegister,
                                                Assembler::TrustedImm32(Value::_Integer_Type));
        Assembler::Pointer addr = _as->loadTempAddress(Assembler::ScratchRegister, source);
        _as->storeUInt32(_as->toInt32Register(addr, Assembler::ScratchRegister), target);
        Assembler::Jump intDone = _as->jump();

        // not an int:
        isNoInt.link(_as);
        generateFunctionCall(Assembler::ReturnValueRegister, __qmljs_value_to_uint32,
                             _as->loadTempAddress(Assembler::ScratchRegister, source));
        _as->storeInt32(Assembler::ReturnValueRegister, target);

        intDone.link(_as);
    } break;
    case V4IR::DoubleType: {
        Assembler::FPRegisterID reg = _as->toDoubleRegister(source);
        Assembler::Jump success =
                _as->branchTruncateDoubleToUint32(reg, Assembler::ReturnValueRegister,
                                                  Assembler::BranchIfTruncateSuccessful);
        generateFunctionCall(Assembler::ReturnValueRegister, __qmljs_double_to_uint32,
                             Assembler::PointerToValue(source));
        success.link(_as);
        _as->storeUInt32(Assembler::ReturnValueRegister, target);
    } break;
    case V4IR::NullType:
    case V4IR::UndefinedType:
        _as->move(Assembler::TrustedImm32(0), Assembler::ReturnValueRegister);
        _as->storeUInt32(Assembler::ReturnValueRegister, target);
        break;
    case V4IR::StringType:
        generateFunctionCall(Assembler::ReturnValueRegister, __qmljs_value_to_uint32,
                             Assembler::PointerToValue(source));
        _as->storeUInt32(Assembler::ReturnValueRegister, target);
        break;
    case V4IR::SInt32Type:
    case V4IR::BoolType:
        _as->storeUInt32(_as->toInt32Register(source, Assembler::ReturnValueRegister), target);
        break;
    default:
        break;
    } // switch (source->type)
}

void InstructionSelection::constructActivationProperty(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result)
{
    assert(func != 0);
    prepareCallData(args, 0);

    if (useFastLookups && func->global) {
        uint index = registerGlobalGetterLookup(*func->id);
        generateFunctionCall(result, __qmljs_construct_global_lookup,
                             Assembler::ContextRegister,
                             Assembler::TrustedImm32(index), baseAddressForCallData());
        return;
    }

    generateFunctionCall(result, __qmljs_construct_activation_property,
                         Assembler::ContextRegister,
                         Assembler::PointerToString(*func->id),
                         baseAddressForCallData());
}


void InstructionSelection::constructProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result)
{
    prepareCallData(args, base);
    if (useFastLookups) {
        uint index = registerGetterLookup(name);
        generateFunctionCall(result, __qmljs_construct_property_lookup,
                             Assembler::ContextRegister,
                             Assembler::TrustedImm32(index),
                             baseAddressForCallData());
        return;
    }

    generateFunctionCall(result, __qmljs_construct_property, Assembler::ContextRegister,
                         Assembler::PointerToString(name),
                         baseAddressForCallData());
}

void InstructionSelection::constructValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result)
{
    assert(value != 0);

    prepareCallData(args, 0);
    generateFunctionCall(result, __qmljs_construct_value,
                         Assembler::ContextRegister,
                         Assembler::Reference(value),
                         baseAddressForCallData());
}

void InstructionSelection::visitJump(V4IR::Jump *s)
{
    if (!_removableJumps.contains(s))
        _as->jumpToBlock(_block, s->target);
}

void InstructionSelection::visitCJump(V4IR::CJump *s)
{
    if (V4IR::Temp *t = s->cond->asTemp()) {
        Assembler::RegisterID reg;
        if (t->kind == V4IR::Temp::PhysicalRegister) {
            Q_ASSERT(t->type == V4IR::BoolType);
            reg = (Assembler::RegisterID) t->index;
        } else if (t->kind == V4IR::Temp::StackSlot && t->type == V4IR::BoolType) {
            reg = Assembler::ReturnValueRegister;
            _as->toInt32Register(t, reg);
        } else {
            Address temp = _as->loadTempAddress(Assembler::ScratchRegister, t);
            Address tag = temp;
            tag.offset += qOffsetOf(QV4::Value, tag);
            Assembler::Jump booleanConversion = _as->branch32(Assembler::NotEqual, tag, Assembler::TrustedImm32(QV4::Value::Boolean_Type));

            Address data = temp;
            data.offset += qOffsetOf(QV4::Value, int_32);
            _as->load32(data, Assembler::ReturnValueRegister);
            Assembler::Jump testBoolean = _as->jump();

            booleanConversion.link(_as);
            reg = Assembler::ReturnValueRegister;
            generateFunctionCall(reg, __qmljs_to_boolean, Assembler::Reference(t));

            testBoolean.link(_as);
        }

        _as->generateCJumpOnNonZero(reg, _block, s->iftrue, s->iffalse);
        return;
    } else if (V4IR::Const *c = s->cond->asConst()) {
        // TODO: SSA optimization for constant condition evaluation should remove this.
        // See also visitCJump() in RegAllocInfo.
        generateFunctionCall(Assembler::ReturnValueRegister, __qmljs_to_boolean,
                             Assembler::PointerToValue(c));
        _as->generateCJumpOnNonZero(Assembler::ReturnValueRegister, _block, s->iftrue, s->iffalse);
        return;
    } else if (V4IR::Binop *b = s->cond->asBinop()) {
        if (b->left->type == V4IR::DoubleType && b->right->type == V4IR::DoubleType
                && visitCJumpDouble(b->op, b->left, b->right, s->iftrue, s->iffalse))
            return;

        if (b->op == V4IR::OpStrictEqual || b->op == V4IR::OpStrictNotEqual) {
            visitCJumpStrict(b, s->iftrue, s->iffalse);
            return;
        }
        if (b->op == V4IR::OpEqual || b->op == V4IR::OpNotEqual) {
            visitCJumpEqual(b, s->iftrue, s->iffalse);
            return;
        }

        CmpOp op = 0;
        CmpOpContext opContext = 0;
        const char *opName = 0;
        switch (b->op) {
        default: Q_UNREACHABLE(); assert(!"todo"); break;
        case V4IR::OpGt: setOp(op, opName, __qmljs_cmp_gt); break;
        case V4IR::OpLt: setOp(op, opName, __qmljs_cmp_lt); break;
        case V4IR::OpGe: setOp(op, opName, __qmljs_cmp_ge); break;
        case V4IR::OpLe: setOp(op, opName, __qmljs_cmp_le); break;
        case V4IR::OpEqual: setOp(op, opName, __qmljs_cmp_eq); break;
        case V4IR::OpNotEqual: setOp(op, opName, __qmljs_cmp_ne); break;
        case V4IR::OpStrictEqual: setOp(op, opName, __qmljs_cmp_se); break;
        case V4IR::OpStrictNotEqual: setOp(op, opName, __qmljs_cmp_sne); break;
        case V4IR::OpInstanceof: setOpContext(op, opName, __qmljs_cmp_instanceof); break;
        case V4IR::OpIn: setOpContext(op, opName, __qmljs_cmp_in); break;
        } // switch

        // TODO: in SSA optimization, do constant expression evaluation.
        // The case here is, for example:
        //   if (true === true) .....
        // Of course, after folding the CJUMP to a JUMP, dead-code (dead-basic-block)
        // elimination (which isn't there either) would remove the whole else block.
        if (opContext)
            _as->generateFunctionCallImp(Assembler::ReturnValueRegister, opName, opContext,
                                         Assembler::ContextRegister,
                                         Assembler::PointerToValue(b->left),
                                         Assembler::PointerToValue(b->right));
        else
            _as->generateFunctionCallImp(Assembler::ReturnValueRegister, opName, op,
                                         Assembler::PointerToValue(b->left),
                                         Assembler::PointerToValue(b->right));

        _as->generateCJumpOnNonZero(Assembler::ReturnValueRegister, _block, s->iftrue, s->iffalse);
        return;
    }
    Q_UNREACHABLE();
}

void InstructionSelection::visitRet(V4IR::Ret *s)
{
    if (!s) {
        // this only happens if the method doesn't have a return statement and can
        // only exit through an exception
    } else if (V4IR::Temp *t = s->expr->asTemp()) {
#if CPU(X86) || CPU(ARM)

#  if CPU(X86)
        Assembler::RegisterID lowReg = JSC::X86Registers::eax;
        Assembler::RegisterID highReg = JSC::X86Registers::edx;
#  else // CPU(ARM)
        Assembler::RegisterID lowReg = JSC::ARMRegisters::r0;
        Assembler::RegisterID highReg = JSC::ARMRegisters::r1;
#  endif

        if (t->kind == V4IR::Temp::PhysicalRegister) {
            switch (t->type) {
            case V4IR::DoubleType:
                _as->moveDoubleToInts((Assembler::FPRegisterID) t->index, lowReg, highReg);
                break;
            case V4IR::UInt32Type: {
                Assembler::RegisterID srcReg = (Assembler::RegisterID) t->index;
                Assembler::Jump intRange = _as->branch32(Assembler::GreaterThanOrEqual, srcReg, Assembler::TrustedImm32(0));
                _as->convertUInt32ToDouble(srcReg, Assembler::FPGpr0, Assembler::ReturnValueRegister);
                _as->moveDoubleToInts(Assembler::FPGpr0, lowReg, highReg);
                Assembler::Jump done = _as->jump();
                intRange.link(_as);
                _as->move(srcReg, lowReg);
                _as->move(Assembler::TrustedImm32(QV4::Value::_Integer_Type), highReg);
                done.link(_as);
            } break;
            case V4IR::SInt32Type:
                _as->move((Assembler::RegisterID) t->index, lowReg);
                _as->move(Assembler::TrustedImm32(QV4::Value::_Integer_Type), highReg);
                break;
            case V4IR::BoolType:
                _as->move((Assembler::RegisterID) t->index, lowReg);
                _as->move(Assembler::TrustedImm32(QV4::Value::_Boolean_Type), highReg);
                break;
            default:
                Q_UNREACHABLE();
            }
        } else {
            Pointer addr = _as->loadTempAddress(Assembler::ScratchRegister, t);
            _as->load32(addr, lowReg);
            addr.offset += 4;
            _as->load32(addr, highReg);
        }
#else
        if (t->kind == V4IR::Temp::PhysicalRegister) {
            if (t->type == V4IR::DoubleType) {
                _as->moveDoubleTo64((Assembler::FPRegisterID) t->index,
                                    Assembler::ReturnValueRegister);
                _as->move(Assembler::TrustedImm64(QV4::Value::NaNEncodeMask),
                          Assembler::ScratchRegister);
                _as->xor64(Assembler::ScratchRegister, Assembler::ReturnValueRegister);
            } else if (t->type == V4IR::UInt32Type) {
                Assembler::RegisterID srcReg = (Assembler::RegisterID) t->index;
                Assembler::Jump intRange = _as->branch32(Assembler::GreaterThanOrEqual, srcReg, Assembler::TrustedImm32(0));
                _as->convertUInt32ToDouble(srcReg, Assembler::FPGpr0, Assembler::ReturnValueRegister);
                _as->moveDoubleTo64(Assembler::FPGpr0, Assembler::ReturnValueRegister);
                _as->move(Assembler::TrustedImm64(QV4::Value::NaNEncodeMask), Assembler::ScratchRegister);
                _as->xor64(Assembler::ScratchRegister, Assembler::ReturnValueRegister);
                Assembler::Jump done = _as->jump();
                intRange.link(_as);
                _as->zeroExtend32ToPtr(srcReg, Assembler::ReturnValueRegister);
                quint64 tag = QV4::Value::_Integer_Type;
                _as->or64(Assembler::TrustedImm64(tag << 32),
                          Assembler::ReturnValueRegister);
                done.link(_as);
            } else {
                _as->zeroExtend32ToPtr((Assembler::RegisterID) t->index, Assembler::ReturnValueRegister);
                quint64 tag;
                switch (t->type) {
                case V4IR::SInt32Type:
                    tag = QV4::Value::_Integer_Type;
                    break;
                case V4IR::BoolType:
                    tag = QV4::Value::_Boolean_Type;
                    break;
                default:
                    tag = QV4::Value::Undefined_Type;
                    Q_UNREACHABLE();
                }
                _as->or64(Assembler::TrustedImm64(tag << 32),
                          Assembler::ReturnValueRegister);
            }
        } else {
            _as->copyValue(Assembler::ReturnValueRegister, t);
        }
#endif
    } else if (V4IR::Const *c = s->expr->asConst()) {
        QV4::Primitive retVal = convertToValue(c);
#if CPU(X86)
        _as->move(Assembler::TrustedImm32(retVal.int_32), JSC::X86Registers::eax);
        _as->move(Assembler::TrustedImm32(retVal.tag), JSC::X86Registers::edx);
#elif CPU(ARM)
        _as->move(Assembler::TrustedImm32(retVal.int_32), JSC::ARMRegisters::r0);
        _as->move(Assembler::TrustedImm32(retVal.tag), JSC::ARMRegisters::r1);
#else
        _as->move(Assembler::TrustedImm64(retVal.val), Assembler::ReturnValueRegister);
#endif
    } else {
        Q_UNREACHABLE();
        Q_UNUSED(s);
    }

    _as->exceptionReturnLabel = _as->label();

    const int locals = _as->stackLayout().calculateJSStackFrameSize();
    _as->subPtr(Assembler::TrustedImm32(sizeof(QV4::SafeValue)*locals), Assembler::LocalsRegister);
    _as->loadPtr(Address(Assembler::ContextRegister, qOffsetOf(ExecutionContext, engine)), Assembler::ScratchRegister);
    _as->storePtr(Assembler::LocalsRegister, Address(Assembler::ScratchRegister, qOffsetOf(ExecutionEngine, jsStackTop)));

    _as->leaveStandardStackFrame();
    _as->ret();
}

int InstructionSelection::prepareVariableArguments(V4IR::ExprList* args)
{
    int argc = 0;
    for (V4IR::ExprList *it = args; it; it = it->next) {
        ++argc;
    }

    int i = 0;
    for (V4IR::ExprList *it = args; it; it = it->next, ++i) {
        V4IR::Expr *arg = it->expr;
        Q_ASSERT(arg != 0);
        Pointer dst(_as->stackLayout().argumentAddressForCall(i));
        if (arg->asTemp() && arg->asTemp()->kind != V4IR::Temp::PhysicalRegister)
            _as->memcopyValue(dst, arg->asTemp(), Assembler::ScratchRegister);
        else
            _as->copyValue(dst, arg);
    }

    return argc;
}

int InstructionSelection::prepareCallData(V4IR::ExprList* args, V4IR::Expr *thisObject)
{
    int argc = 0;
    for (V4IR::ExprList *it = args; it; it = it->next) {
        ++argc;
    }

    Pointer p = _as->stackLayout().callDataAddress(qOffsetOf(CallData, tag));
    _as->store32(Assembler::TrustedImm32(QV4::Value::_Integer_Type), p);
    p = _as->stackLayout().callDataAddress(qOffsetOf(CallData, argc));
    _as->store32(Assembler::TrustedImm32(argc), p);
    p = _as->stackLayout().callDataAddress(qOffsetOf(CallData, thisObject));
    if (!thisObject)
        _as->storeValue(QV4::Primitive::undefinedValue(), p);
    else
        _as->copyValue(p, thisObject);

    int i = 0;
    for (V4IR::ExprList *it = args; it; it = it->next, ++i) {
        V4IR::Expr *arg = it->expr;
        Q_ASSERT(arg != 0);
        Pointer dst(_as->stackLayout().argumentAddressForCall(i));
        if (arg->asTemp() && arg->asTemp()->kind != V4IR::Temp::PhysicalRegister)
            _as->memcopyValue(dst, arg->asTemp(), Assembler::ScratchRegister);
        else
            _as->copyValue(dst, arg);
    }
    return argc;
}


QT_BEGIN_NAMESPACE
namespace QV4 {
bool operator==(const Primitive &v1, const Primitive &v2)
{
    return v1.rawValue() == v2.rawValue();
}
} // QV4 namespace
QT_END_NAMESPACE

int Assembler::ConstantTable::add(const Primitive &v)
{
    int idx = _values.indexOf(v);
    if (idx == -1) {
        idx = _values.size();
        _values.append(v);
    }
    return idx;
}

Assembler::ImplicitAddress Assembler::ConstantTable::loadValueAddress(V4IR::Const *c,
                                                                      RegisterID baseReg)
{
    return loadValueAddress(convertToValue(c), baseReg);
}

Assembler::ImplicitAddress Assembler::ConstantTable::loadValueAddress(const Primitive &v,
                                                                      RegisterID baseReg)
{
    _toPatch.append(_as->moveWithPatch(TrustedImmPtr(0), baseReg));
    ImplicitAddress addr(baseReg);
    addr.offset = add(v) * sizeof(QV4::Primitive);
    Q_ASSERT(addr.offset >= 0);
    return addr;
}

void Assembler::ConstantTable::finalize(JSC::LinkBuffer &linkBuffer, InstructionSelection *isel)
{
    void *tablePtr = isel->addConstantTable(&_values);

    foreach (DataLabelPtr label, _toPatch)
        linkBuffer.patch(label, tablePtr);
}

// Try to load the source expression into the destination FP register. This assumes that two
// general purpose (integer) registers are available: the ScratchRegister and the
// ReturnValueRegister. It returns a Jump if no conversion can be performed.
Assembler::Jump InstructionSelection::genTryDoubleConversion(V4IR::Expr *src,
                                                             Assembler::FPRegisterID dest)
{
    switch (src->type) {
    case V4IR::DoubleType:
        _as->moveDouble(_as->toDoubleRegister(src, dest), dest);
        return Assembler::Jump();
    case V4IR::SInt32Type:
        _as->convertInt32ToDouble(_as->toInt32Register(src, Assembler::ScratchRegister),
                                  dest);
        return Assembler::Jump();
    case V4IR::UInt32Type:
        _as->convertUInt32ToDouble(_as->toUInt32Register(src, Assembler::ScratchRegister),
                                   dest, Assembler::ReturnValueRegister);
        return Assembler::Jump();
    case V4IR::BoolType:
        // TODO?
        return _as->jump();
    default:
        break;
    }

    V4IR::Temp *sourceTemp = src->asTemp();
    Q_ASSERT(sourceTemp);

    // It's not a number type, so it cannot be in a register.
    Q_ASSERT(sourceTemp->kind != V4IR::Temp::PhysicalRegister || sourceTemp->type == V4IR::BoolType);

    Assembler::Pointer tagAddr = _as->loadTempAddress(Assembler::ScratchRegister, sourceTemp);
    tagAddr.offset += 4;
    _as->load32(tagAddr, Assembler::ScratchRegister);

    // check if it's an int32:
    Assembler::Jump isNoInt = _as->branch32(Assembler::NotEqual, Assembler::ScratchRegister,
                                            Assembler::TrustedImm32(Value::_Integer_Type));
    _as->convertInt32ToDouble(_as->toInt32Register(src, Assembler::ScratchRegister), dest);
    Assembler::Jump intDone = _as->jump();

    // not an int, check if it's a double:
    isNoInt.link(_as);
#if QT_POINTER_SIZE == 8
    _as->and32(Assembler::TrustedImm32(Value::IsDouble_Mask), Assembler::ScratchRegister);
    Assembler::Jump isNoDbl = _as->branch32(Assembler::Equal, Assembler::ScratchRegister,
                                            Assembler::TrustedImm32(0));
#else
    _as->and32(Assembler::TrustedImm32(Value::NotDouble_Mask), Assembler::ScratchRegister);
    Assembler::Jump isNoDbl = _as->branch32(Assembler::Equal, Assembler::ScratchRegister,
                                            Assembler::TrustedImm32(Value::NotDouble_Mask));
#endif
    _as->toDoubleRegister(src, dest);
    intDone.link(_as);

    return isNoDbl;
}

void InstructionSelection::doubleBinop(V4IR::AluOp oper, V4IR::Expr *leftSource,
                                       V4IR::Expr *rightSource, V4IR::Temp *target)
{
    Q_ASSERT(leftSource->asConst() == 0 || rightSource->asConst() == 0);
    Q_ASSERT(isPregOrConst(leftSource));
    Q_ASSERT(isPregOrConst(rightSource));
    Assembler::FPRegisterID targetReg;
    if (target->kind == V4IR::Temp::PhysicalRegister)
        targetReg = (Assembler::FPRegisterID) target->index;
    else
        targetReg = Assembler::FPGpr0;

    switch (oper) {
    case V4IR::OpAdd:
        _as->addDouble(_as->toDoubleRegister(leftSource), _as->toDoubleRegister(rightSource),
                       targetReg);
        break;
    case V4IR::OpMul:
        _as->mulDouble(_as->toDoubleRegister(leftSource), _as->toDoubleRegister(rightSource),
                       targetReg);
        break;
    case V4IR::OpSub:
#if CPU(X86) || CPU(X86_64)
        if (V4IR::Temp *rightTemp = rightSource->asTemp()) {
            if (rightTemp->kind == V4IR::Temp::PhysicalRegister && rightTemp->index == targetReg) {
                _as->moveDouble(targetReg, Assembler::FPGpr0);
                _as->moveDouble(_as->toDoubleRegister(leftSource, targetReg), targetReg);
                _as->subDouble(Assembler::FPGpr0, targetReg);
                break;
            }
        } else if (rightSource->asConst() && targetReg == Assembler::FPGpr0) {
            Q_ASSERT(leftSource->asTemp());
            Q_ASSERT(leftSource->asTemp()->kind == V4IR::Temp::PhysicalRegister);
            _as->moveDouble(_as->toDoubleRegister(leftSource, targetReg), targetReg);
            Assembler::FPRegisterID reg = (Assembler::FPRegisterID) leftSource->asTemp()->index;
            _as->moveDouble(_as->toDoubleRegister(rightSource, reg), reg);
            _as->subDouble(reg, targetReg);
            break;
        }
#endif

        _as->subDouble(_as->toDoubleRegister(leftSource), _as->toDoubleRegister(rightSource),
                       targetReg);
        break;
    case V4IR::OpDiv:
#if CPU(X86) || CPU(X86_64)
        if (V4IR::Temp *rightTemp = rightSource->asTemp()) {
            if (rightTemp->kind == V4IR::Temp::PhysicalRegister && rightTemp->index == targetReg) {
                _as->moveDouble(targetReg, Assembler::FPGpr0);
                _as->moveDouble(_as->toDoubleRegister(leftSource, targetReg), targetReg);
                _as->divDouble(Assembler::FPGpr0, targetReg);
                break;
            }
        } else if (rightSource->asConst() && targetReg == Assembler::FPGpr0) {
            Q_ASSERT(leftSource->asTemp());
            Q_ASSERT(leftSource->asTemp()->kind == V4IR::Temp::PhysicalRegister);
            _as->moveDouble(_as->toDoubleRegister(leftSource, targetReg), targetReg);
            Assembler::FPRegisterID reg = (Assembler::FPRegisterID) leftSource->asTemp()->index;
            _as->moveDouble(_as->toDoubleRegister(rightSource, reg), reg);
            _as->divDouble(reg, targetReg);
            break;
        }
#endif
        _as->divDouble(_as->toDoubleRegister(leftSource), _as->toDoubleRegister(rightSource),
                       targetReg);
        break;
    default: {
        Q_ASSERT(target->type == V4IR::BoolType);
        Assembler::Jump trueCase = branchDouble(false, oper, leftSource, rightSource);
        _as->storeBool(false, target);
        Assembler::Jump done = _as->jump();
        trueCase.link(_as);
        _as->storeBool(true, target);
        done.link(_as);
    } return;
    }

    if (target->kind != V4IR::Temp::PhysicalRegister)
        _as->storeDouble(Assembler::FPGpr0, target);
}

Assembler::Jump InstructionSelection::branchDouble(bool invertCondition, V4IR::AluOp op,
                                                   V4IR::Expr *left, V4IR::Expr *right)
{
    Q_ASSERT(isPregOrConst(left));
    Q_ASSERT(isPregOrConst(right));
    Q_ASSERT(left->asConst() == 0 || right->asConst() == 0);

    Assembler::DoubleCondition cond;
    switch (op) {
    case V4IR::OpGt: cond = Assembler::DoubleGreaterThan; break;
    case V4IR::OpLt: cond = Assembler::DoubleLessThan; break;
    case V4IR::OpGe: cond = Assembler::DoubleGreaterThanOrEqual; break;
    case V4IR::OpLe: cond = Assembler::DoubleLessThanOrEqual; break;
    case V4IR::OpEqual:
    case V4IR::OpStrictEqual: cond = Assembler::DoubleEqual; break;
    case V4IR::OpNotEqual:
    case V4IR::OpStrictNotEqual: cond = Assembler::DoubleNotEqualOrUnordered; break; // No, the inversion of DoubleEqual is NOT DoubleNotEqual.
    default:
        Q_UNREACHABLE();
    }
    if (invertCondition)
        cond = JSC::MacroAssembler::invert(cond);

    return _as->branchDouble(cond, _as->toDoubleRegister(left), _as->toDoubleRegister(right));
}

bool InstructionSelection::visitCJumpDouble(V4IR::AluOp op, V4IR::Expr *left, V4IR::Expr *right,
                                            V4IR::BasicBlock *iftrue, V4IR::BasicBlock *iffalse)
{
    if (!isPregOrConst(left) || !isPregOrConst(right))
        return false;

    if (_as->nextBlock() == iftrue) {
        Assembler::Jump target = branchDouble(true, op, left, right);
        _as->addPatch(iffalse, target);
    } else {
        Assembler::Jump target = branchDouble(false, op, left, right);
        _as->addPatch(iftrue, target);
        _as->jumpToBlock(_block, iffalse);
    }
    return true;
}

void InstructionSelection::visitCJumpStrict(V4IR::Binop *binop, V4IR::BasicBlock *trueBlock,
                                            V4IR::BasicBlock *falseBlock)
{
    Q_ASSERT(binop->op == V4IR::OpStrictEqual || binop->op == V4IR::OpStrictNotEqual);

    if (visitCJumpStrictNullUndefined(V4IR::NullType, binop, trueBlock, falseBlock))
        return;
    if (visitCJumpStrictNullUndefined(V4IR::UndefinedType, binop, trueBlock, falseBlock))
        return;
    if (visitCJumpStrictBool(binop, trueBlock, falseBlock))
        return;

    V4IR::Expr *left = binop->left;
    V4IR::Expr *right = binop->right;

    _as->generateFunctionCallImp(Assembler::ReturnValueRegister, "__qmljs_cmp_se", __qmljs_cmp_se,
                                 Assembler::PointerToValue(left), Assembler::PointerToValue(right));
    _as->generateCJumpOnCompare(binop->op == V4IR::OpStrictEqual ? Assembler::NotEqual : Assembler::Equal,
                                Assembler::ReturnValueRegister, Assembler::TrustedImm32(0),
                                _block, trueBlock, falseBlock);
}

// Only load the non-null temp.
bool InstructionSelection::visitCJumpStrictNullUndefined(V4IR::Type nullOrUndef, V4IR::Binop *binop,
                                                         V4IR::BasicBlock *trueBlock,
                                                         V4IR::BasicBlock *falseBlock)
{
    Q_ASSERT(nullOrUndef == V4IR::NullType || nullOrUndef == V4IR::UndefinedType);

    V4IR::Expr *varSrc = 0;
    if (binop->left->type == V4IR::VarType && binop->right->type == nullOrUndef)
        varSrc = binop->left;
    else if (binop->left->type == nullOrUndef && binop->right->type == V4IR::VarType)
        varSrc = binop->right;
    if (!varSrc)
        return false;

    if (varSrc->asTemp() && varSrc->asTemp()->kind == V4IR::Temp::PhysicalRegister) {
        _as->jumpToBlock(_block, falseBlock);
        return true;
    }

    if (V4IR::Const *c = varSrc->asConst()) {
        if (c->type == nullOrUndef)
            _as->jumpToBlock(_block, trueBlock);
        else
            _as->jumpToBlock(_block, falseBlock);
        return true;
    }

    V4IR::Temp *t = varSrc->asTemp();
    Q_ASSERT(t);

    Assembler::Pointer tagAddr = _as->loadTempAddress(Assembler::ScratchRegister, t);
    tagAddr.offset += 4;
    const Assembler::RegisterID tagReg = Assembler::ScratchRegister;
    _as->load32(tagAddr, tagReg);

    Assembler::RelationalCondition cond = binop->op == V4IR::OpStrictEqual ? Assembler::Equal
                                                                           : Assembler::NotEqual;
    const Assembler::TrustedImm32 tag(nullOrUndef == V4IR::NullType ? int(QV4::Value::_Null_Type)
                                                                    : int(QV4::Value::Undefined_Type));
    _as->generateCJumpOnCompare(cond, tagReg, tag, _block, trueBlock, falseBlock);
    return true;
}

bool InstructionSelection::visitCJumpStrictBool(V4IR::Binop *binop, V4IR::BasicBlock *trueBlock,
                                                V4IR::BasicBlock *falseBlock)
{
    V4IR::Expr *boolSrc = 0, *otherSrc = 0;
    if (binop->left->type == V4IR::BoolType) {
        boolSrc = binop->left;
        otherSrc = binop->right;
    } else if (binop->right->type == V4IR::BoolType) {
        boolSrc = binop->right;
        otherSrc = binop->left;
    } else {
        // neither operands are statically typed as bool, so bail out.
        return false;
    }

    Assembler::RelationalCondition cond = binop->op == V4IR::OpStrictEqual ? Assembler::Equal
                                                                           : Assembler::NotEqual;

    if (otherSrc->type == V4IR::BoolType) { // both are boolean
        Assembler::RegisterID one = _as->toBoolRegister(boolSrc, Assembler::ReturnValueRegister);
        Assembler::RegisterID two = _as->toBoolRegister(otherSrc, Assembler::ScratchRegister);
        _as->generateCJumpOnCompare(cond, one, two, _block, trueBlock, falseBlock);
        return true;
    }

    if (otherSrc->type != V4IR::VarType) {
        _as->jumpToBlock(_block, falseBlock);
        return true;
    }

    V4IR::Temp *otherTemp = otherSrc->asTemp();
    Q_ASSERT(otherTemp); // constants cannot have "var" type
    Q_ASSERT(otherTemp->kind != V4IR::Temp::PhysicalRegister);

    Assembler::Pointer otherAddr = _as->loadTempAddress(Assembler::ReturnValueRegister, otherTemp);
    otherAddr.offset += 4; // tag address

    // check if the tag of the var operand is indicates 'boolean'
    _as->load32(otherAddr, Assembler::ScratchRegister);
    Assembler::Jump noBool = _as->branch32(Assembler::NotEqual, Assembler::ScratchRegister,
                                           Assembler::TrustedImm32(QV4::Value::_Boolean_Type));
    if (binop->op == V4IR::OpStrictEqual)
        _as->addPatch(falseBlock, noBool);
    else
        _as->addPatch(trueBlock, noBool);

    // ok, both are boolean, so let's load them and compare them.
    otherAddr.offset -= 4; // int_32 address
    _as->load32(otherAddr, Assembler::ReturnValueRegister);
    Assembler::RegisterID boolReg = _as->toBoolRegister(boolSrc, Assembler::ScratchRegister);
    _as->generateCJumpOnCompare(cond, boolReg, Assembler::ReturnValueRegister, _block, trueBlock,
                                falseBlock);
    return true;
}

bool InstructionSelection::visitCJumpNullUndefined(V4IR::Type nullOrUndef, V4IR::Binop *binop,
                                                         V4IR::BasicBlock *trueBlock,
                                                         V4IR::BasicBlock *falseBlock)
{
    Q_ASSERT(nullOrUndef == V4IR::NullType || nullOrUndef == V4IR::UndefinedType);

    V4IR::Expr *varSrc = 0;
    if (binop->left->type == V4IR::VarType && binop->right->type == nullOrUndef)
        varSrc = binop->left;
    else if (binop->left->type == nullOrUndef && binop->right->type == V4IR::VarType)
        varSrc = binop->right;
    if (!varSrc)
        return false;

    if (varSrc->asTemp() && varSrc->asTemp()->kind == V4IR::Temp::PhysicalRegister) {
        _as->jumpToBlock(_block, falseBlock);
        return true;
    }

    if (V4IR::Const *c = varSrc->asConst()) {
        if (c->type == nullOrUndef)
            _as->jumpToBlock(_block, trueBlock);
        else
            _as->jumpToBlock(_block, falseBlock);
        return true;
    }

    V4IR::Temp *t = varSrc->asTemp();
    Q_ASSERT(t);

    Assembler::Pointer tagAddr = _as->loadTempAddress(Assembler::ScratchRegister, t);
    tagAddr.offset += 4;
    const Assembler::RegisterID tagReg = Assembler::ScratchRegister;
    _as->load32(tagAddr, tagReg);

    if (binop->op == V4IR::OpNotEqual)
        qSwap(trueBlock, falseBlock);
    Assembler::Jump isNull = _as->branch32(Assembler::Equal, tagReg, Assembler::TrustedImm32(int(QV4::Value::_Null_Type)));
    Assembler::Jump isUndefined = _as->branch32(Assembler::Equal, tagReg, Assembler::TrustedImm32(int(QV4::Value::Undefined_Type)));
    _as->addPatch(trueBlock, isNull);
    _as->addPatch(trueBlock, isUndefined);
    _as->jumpToBlock(_block, falseBlock);

    return true;
}


void InstructionSelection::visitCJumpEqual(V4IR::Binop *binop, V4IR::BasicBlock *trueBlock,
                                            V4IR::BasicBlock *falseBlock)
{
    Q_ASSERT(binop->op == V4IR::OpEqual || binop->op == V4IR::OpNotEqual);

    if (visitCJumpNullUndefined(V4IR::NullType, binop, trueBlock, falseBlock))
        return;

    V4IR::Expr *left = binop->left;
    V4IR::Expr *right = binop->right;

    _as->generateFunctionCallImp(Assembler::ReturnValueRegister, "__qmljs_cmp_eq", __qmljs_cmp_eq,
                                 Assembler::PointerToValue(left), Assembler::PointerToValue(right));
    _as->generateCJumpOnCompare(binop->op == V4IR::OpEqual ? Assembler::NotEqual : Assembler::Equal,
                                Assembler::ReturnValueRegister, Assembler::TrustedImm32(0),
                                _block, trueBlock, falseBlock);
}


bool InstructionSelection::int32Binop(V4IR::AluOp oper, V4IR::Expr *leftSource,
                                      V4IR::Expr *rightSource, V4IR::Temp *target)
{
    Q_ASSERT(leftSource->type == V4IR::SInt32Type);

    switch (oper) {
    case V4IR::OpBitAnd: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);
        if (rightSource->asTemp() && rightSource->asTemp()->kind == V4IR::Temp::PhysicalRegister
                && target->kind == V4IR::Temp::PhysicalRegister
                && target->index == rightSource->asTemp()->index) {
            _as->and32(_as->toInt32Register(leftSource, Assembler::ScratchRegister),
                       (Assembler::RegisterID) target->index);
            return true;
        }
        Assembler::RegisterID targetReg;
        if (target->kind == V4IR::Temp::PhysicalRegister)
            targetReg = (Assembler::RegisterID) target->index;
        else
            targetReg = Assembler::ReturnValueRegister;

        _as->and32(_as->toInt32Register(leftSource, targetReg),
                   _as->toInt32Register(rightSource, Assembler::ScratchRegister),
                   targetReg);
        _as->storeInt32(targetReg, target);
    } return true;
    case V4IR::OpBitOr: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);
        if (rightSource->asTemp() && rightSource->asTemp()->kind == V4IR::Temp::PhysicalRegister
                && target->kind == V4IR::Temp::PhysicalRegister
                && target->index == rightSource->asTemp()->index) {
            _as->or32(_as->toInt32Register(leftSource, Assembler::ScratchRegister),
                       (Assembler::RegisterID) target->index);
            return true;
        }
        Assembler::RegisterID targetReg;
        if (target->kind == V4IR::Temp::PhysicalRegister)
            targetReg = (Assembler::RegisterID) target->index;
        else
            targetReg = Assembler::ReturnValueRegister;

        _as->or32(_as->toInt32Register(leftSource, targetReg),
                  _as->toInt32Register(rightSource, Assembler::ScratchRegister),
                  targetReg);
        _as->storeInt32(targetReg, target);
    } return true;
    case V4IR::OpBitXor: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);
        if (rightSource->asTemp() && rightSource->asTemp()->kind == V4IR::Temp::PhysicalRegister
                && target->kind == V4IR::Temp::PhysicalRegister
                && target->index == rightSource->asTemp()->index) {
            _as->xor32(_as->toInt32Register(leftSource, Assembler::ScratchRegister),
                       (Assembler::RegisterID) target->index);
            return true;
        }
        Assembler::RegisterID targetReg;
        if (target->kind == V4IR::Temp::PhysicalRegister)
            targetReg = (Assembler::RegisterID) target->index;
        else
            targetReg = Assembler::ReturnValueRegister;

        _as->xor32(_as->toInt32Register(leftSource, targetReg),
                   _as->toInt32Register(rightSource, Assembler::ScratchRegister),
                   targetReg);
        _as->storeInt32(targetReg, target);
    } return true;
    case V4IR::OpLShift: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);
        Assembler::RegisterID targetReg;
        if (target->kind == V4IR::Temp::PhysicalRegister)
            targetReg = (Assembler::RegisterID) target->index;
        else
            targetReg = Assembler::ReturnValueRegister;

        _as->move(_as->toInt32Register(rightSource, Assembler::ScratchRegister),
                  Assembler::ScratchRegister);
        _as->and32(Assembler::TrustedImm32(0x1f), Assembler::ScratchRegister); // TODO: for constants, do this in the IR
        _as->lshift32(_as->toInt32Register(leftSource, targetReg), Assembler::ScratchRegister,
                      Assembler::ReturnValueRegister);
        _as->storeInt32(Assembler::ReturnValueRegister, target);
    } return true;
    case V4IR::OpRShift: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);
        Assembler::RegisterID targetReg;
        if (target->kind == V4IR::Temp::PhysicalRegister)
            targetReg = (Assembler::RegisterID) target->index;
        else
            targetReg = Assembler::ReturnValueRegister;

        _as->move(_as->toInt32Register(rightSource, Assembler::ScratchRegister),
                  Assembler::ScratchRegister);
        _as->and32(Assembler::TrustedImm32(0x1f), Assembler::ScratchRegister); // TODO: for constants, do this in the IR
        _as->rshift32(_as->toInt32Register(leftSource, targetReg), Assembler::ScratchRegister,
                      Assembler::ReturnValueRegister);
        _as->storeInt32(Assembler::ReturnValueRegister, target);
    } return true;
    case V4IR::OpURShift:
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);
        _as->move(_as->toInt32Register(leftSource, Assembler::ReturnValueRegister),
                  Assembler::ReturnValueRegister);
        _as->move(_as->toInt32Register(rightSource, Assembler::ScratchRegister),
                  Assembler::ScratchRegister);
        _as->and32(Assembler::TrustedImm32(0x1f), Assembler::ScratchRegister); // TODO: for constants, do this in the IR
        _as->urshift32(Assembler::ScratchRegister, Assembler::ReturnValueRegister);
        _as->storeUInt32(Assembler::ReturnValueRegister, target);
        return true;
    case V4IR::OpAdd: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);

        Assembler::RegisterID targetReg;
        if (target->kind == V4IR::Temp::PhysicalRegister)
            targetReg = (Assembler::RegisterID) target->index;
        else
            targetReg = Assembler::ReturnValueRegister;

        _as->add32(_as->toInt32Register(leftSource, targetReg),
                   _as->toInt32Register(rightSource, Assembler::ScratchRegister),
                   targetReg);
        _as->storeInt32(targetReg, target);
    } return true;
    case V4IR::OpSub: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);

        if (rightSource->asTemp() && rightSource->asTemp()->kind == V4IR::Temp::PhysicalRegister
                && target->kind == V4IR::Temp::PhysicalRegister
                && target->index == rightSource->asTemp()->index) {
            Assembler::RegisterID targetReg = (Assembler::RegisterID) target->index;
            _as->move(targetReg, Assembler::ScratchRegister);
            _as->move(_as->toInt32Register(leftSource, targetReg), targetReg);
            _as->sub32(Assembler::ScratchRegister, targetReg);
            _as->storeInt32(targetReg, target);
            return true;
        }

        Assembler::RegisterID targetReg;
        if (target->kind == V4IR::Temp::PhysicalRegister)
            targetReg = (Assembler::RegisterID) target->index;
        else
            targetReg = Assembler::ReturnValueRegister;

        _as->move(_as->toInt32Register(leftSource, targetReg), targetReg);
        _as->sub32(_as->toInt32Register(rightSource, Assembler::ScratchRegister), targetReg);
        _as->storeInt32(targetReg, target);
    } return true;
    case V4IR::OpMul: {
        Q_ASSERT(rightSource->type == V4IR::SInt32Type);

        Assembler::RegisterID targetReg;
        if (target->kind == V4IR::Temp::PhysicalRegister)
            targetReg = (Assembler::RegisterID) target->index;
        else
            targetReg = Assembler::ReturnValueRegister;

        _as->mul32(_as->toInt32Register(leftSource, targetReg),
                   _as->toInt32Register(rightSource, Assembler::ScratchRegister),
                   targetReg);
        _as->storeInt32(targetReg, target);
    } return true;
    default:
        return false;
    }
}

#endif // ENABLE(ASSEMBLER)
