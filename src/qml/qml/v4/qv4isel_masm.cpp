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
#include "qv4runtime_p.h"
#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include "qv4regexpobject_p.h"
#include "qv4unwindhelper_p.h"
#include "qv4lookup_p.h"
#include "qv4function_p.h"

#include <assembler/LinkBuffer.h>
#include <WTFStubs.h>

#include <iostream>
#include <cassert>

#if USE(UDIS86)
#  include <udis86.h>
#endif

using namespace QQmlJS;
using namespace QQmlJS::MASM;
using namespace QV4;

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
    // Keep these in reverse order and make sure to also edit the unwind program in
    // qv4unwindhelper_p-arm.h when changing this list.
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

Assembler::Assembler(V4IR::Function* function, QV4::Function *vmFunction, QV4::ExecutionEngine *engine)
    : _function(function), _vmFunction(vmFunction), _engine(engine), _nextBlock(0)
{
}

void Assembler::registerBlock(V4IR::BasicBlock* block, V4IR::BasicBlock *nextBlock)
{
    _addrs[block] = label();
    _nextBlock = nextBlock;
}

void Assembler::jumpToBlock(V4IR::BasicBlock* current, V4IR::BasicBlock *target)
{
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

Assembler::Pointer Assembler::loadTempAddress(RegisterID reg, V4IR::Temp *t)
{
    int32_t offset = 0;
    int scope = t->scope;
    QV4::Function *f = _vmFunction;
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
    switch (t->kind) {
    case V4IR::Temp::Formal:
    case V4IR::Temp::ScopedFormal: {
        loadPtr(Address(context, offsetof(CallContext, arguments)), reg);
        offset = t->index * sizeof(Value);
    } break;
    case V4IR::Temp::Local:
    case V4IR::Temp::ScopedLocal: {
        loadPtr(Address(context, offsetof(CallContext, locals)), reg);
        offset = t->index * sizeof(Value);
    } break;
    case V4IR::Temp::VirtualRegister: {
        assert(t->scope == 0);
        const int arg = _function->maxNumberOfArguments + t->index + 1;
        offset = - sizeof(Value) * (arg + 1);
        offset -= sizeof(void*) * calleeSavedRegisterCount;
        reg = LocalsRegister;
    } break;
    default:
        Q_UNIMPLEMENTED();
    }
    return Pointer(reg, offset);
}

template <typename Result, typename Source>
void Assembler::copyValue(Result result, Source source)
{
#ifdef VALUE_FITS_IN_REGISTER
    // Use ReturnValueRegister as "scratch" register because loadArgument
    // and storeArgument are functions that may need a scratch register themselves.
    loadArgumentInRegister(source, ReturnValueRegister);
    storeReturnValue(result);
#else
    loadDouble(source, FPGpr0);
    storeDouble(FPGpr0, result);
#endif
}

template <typename Result>
void Assembler::copyValue(Result result, V4IR::Expr* source)
{
#ifdef VALUE_FITS_IN_REGISTER
    // Use ReturnValueRegister as "scratch" register because loadArgument
    // and storeArgument are functions that may need a scratch register themselves.
    loadArgumentInRegister(source, ReturnValueRegister);
    storeReturnValue(result);
#else
    if (V4IR::Temp *temp = source->asTemp()) {
        loadDouble(temp, FPGpr0);
        storeDouble(FPGpr0, result);
    } else if (V4IR::Const *c = source->asConst()) {
        QV4::Value v = convertToValue(c);
        storeValue(v, result);
    } else {
        assert(! "not implemented");
    }
#endif
}


void Assembler::storeValue(QV4::Value value, V4IR::Temp* destination)
{
    Address addr = loadTempAddress(ScratchRegister, destination);
    storeValue(value, addr);
}

int Assembler::calculateStackFrameSize(int locals)
{
    const int stackSpaceAllocatedOtherwise = StackSpaceAllocatedUponFunctionEntry
                                             + RegisterSize; // saved StackFrameRegister

    // space for the locals and the callee saved registers
    int frameSize = locals * sizeof(QV4::Value) + sizeof(void*) * calleeSavedRegisterCount;

    frameSize = WTF::roundUpToMultipleOf(StackAlignment, frameSize + stackSpaceAllocatedOtherwise);
    frameSize -= stackSpaceAllocatedOtherwise;

    return frameSize;
}

void Assembler::enterStandardStackFrame(int locals)
{
    platformEnterStandardStackFrame();

    // ### FIXME: Handle through calleeSavedRegisters mechanism
    // or eliminate StackFrameRegister altogether.
    push(StackFrameRegister);
    move(StackPointerRegister, StackFrameRegister);

    int frameSize = calculateStackFrameSize(locals);

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

    int frameSize = calculateStackFrameSize(locals);
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

void Assembler::generateBinOp(V4IR::AluOp operation, V4IR::Temp* target, V4IR::Temp *left, V4IR::Temp *right)
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
            typeAddress.offset += offsetof(QV4::Value, tag);
            leftTypeCheck = branch32(NotEqual, typeAddress, TrustedImm32(QV4::Value::_Integer_Type));
        }

        Jump rightTypeCheck;
        if (right->asTemp()) {
            Address typeAddress = loadTempAddress(ScratchRegister, right->asTemp());
            typeAddress.offset += offsetof(QV4::Value, tag);
            rightTypeCheck = branch32(NotEqual, typeAddress, TrustedImm32(QV4::Value::_Integer_Type));
        }

        if (left->asTemp()) {
            Address leftValue = loadTempAddress(ScratchRegister, left->asTemp());
            leftValue.offset += offsetof(QV4::Value, int_32);
            load32(leftValue, IntegerOpRegister);
        } else { // left->asConst()
            move(TrustedImm32(leftConst.integerValue()), IntegerOpRegister);
        }

        Jump overflowCheck;

        if (right->asTemp()) {
            Address rightValue = loadTempAddress(ScratchRegister, right->asTemp());
            rightValue.offset += offsetof(QV4::Value, int_32);

            overflowCheck = (this->*info.inlineMemRegOp)(rightValue, IntegerOpRegister);
        } else { // right->asConst()
            overflowCheck = (this->*info.inlineImmRegOp)(TrustedImm32(rightConst.integerValue()), IntegerOpRegister);
        }

        Address resultAddr = loadTempAddress(ScratchRegister, target);
        Address resultValueAddr = resultAddr;
        resultValueAddr.offset += offsetof(QV4::Value, int_32);
        store32(IntegerOpRegister, resultValueAddr);

        Address resultTypeAddr = resultAddr;
        resultTypeAddr.offset += offsetof(QV4::Value, tag);
        store32(TrustedImm32(QV4::Value::_Integer_Type), resultTypeAddr);

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
#if OS(LINUX) || OS(MAC_OS_X)
static void printDisassembledOutputWithCalls(const char* output, const QHash<void*, const char*>& functions,
                                             const QVector<String*> &identifiers)
{
    QByteArray processedOutput(output);
    for (QHash<void*, const char*>::ConstIterator it = functions.begin(), end = functions.end();
         it != end; ++it) {
        QByteArray ptrString = QByteArray::number(quintptr(it.key()), 16);
        ptrString.prepend("0x");
        processedOutput = processedOutput.replace(ptrString, it.value());
    }
    for (QVector<String*>::ConstIterator it = identifiers.begin(), end = identifiers.end();
         it != end; ++it) {
        QByteArray ptrString = QByteArray::number(quintptr(*it), 16);
        ptrString.prepend("0x");
        QByteArray replacement = "\"" + (*it)->toQString().toUtf8() + "\"";
        processedOutput = processedOutput.replace(ptrString, replacement);
    }
    fprintf(stderr, "%s\n", processedOutput.constData());
}
#endif

void Assembler::recordLineNumber(int lineNumber)
{
    CodeLineNumerMapping mapping;
    mapping.location = label();
    mapping.lineNumber = lineNumber;
    codeLineNumberMappings << mapping;
}


void Assembler::link(QV4::Function *vmFunc)
{
    Label endOfCode = label();
#if defined(Q_PROCESSOR_ARM) && !defined(Q_OS_IOS)
    // Let the ARM exception table follow right after that
    for (int i = 0, nops = UnwindHelper::unwindInfoSize() / 2; i < nops; ++i)
        nop();
#endif

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

    JSC::JSGlobalData dummy(_engine->executableAllocator);
    JSC::LinkBuffer linkBuffer(dummy, this, 0);
    vmFunc->codeSize = linkBuffer.offsetOf(endOfCode);

    vmFunc->lineNumberMappings.resize(codeLineNumberMappings.count());
    for (int i = 0; i < codeLineNumberMappings.count(); ++i) {
        QV4::LineNumberMapping mapping;
        mapping.codeOffset = linkBuffer.offsetOf(codeLineNumberMappings.at(i).location);
        mapping.lineNumber = codeLineNumberMappings.at(i).lineNumber;
        vmFunc->lineNumberMappings[i] = mapping;
    }

    QHash<void*, const char*> functions;
    foreach (CallToLink ctl, _callsToLink) {
        linkBuffer.link(ctl.call, ctl.externalFunction);
        functions[ctl.externalFunction.value()] = ctl.functionName;
    }

    foreach (const DataLabelPatch &p, _dataLabelPatches)
        linkBuffer.patch(p.dataLabel, linkBuffer.locationOf(p.target));

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

#if defined(Q_PROCESSOR_ARM) && !defined(Q_OS_IOS)
    UnwindHelper::writeARMUnwindInfo(linkBuffer.debugAddress(), linkBuffer.offsetOf(endOfCode));
#endif

    static bool showCode = !qgetenv("SHOW_CODE").isNull();
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
        vmFunc->codeRef = linkBuffer.finalizeCodeWithDisassembly("%s", name.data());

        WTF::setDataFile(stderr);
#if (OS(LINUX) && !defined(Q_OS_ANDROID)) || OS(MAC_OS_X)
#  if OS(MAC_OS_X)
        char *disasmOutput = memStream.buf.data();
#  endif
        fclose(disasmStream);
#  if CPU(X86) || CPU(X86_64)
        QHash<void*, String*> idents;
        printDisassembledOutputWithCalls(disasmOutput, functions, _vmFunction->identifiers);
#  endif
#  if OS(LINUX)
        free(disasmOutput);
#  endif
#endif
    } else {
        vmFunc->codeRef = linkBuffer.finalizeCodeWithoutDisassembly();
    }

    vmFunc->code = (Value (*)(QV4::ExecutionContext *, const uchar *)) vmFunc->codeRef.code().executableAddress();
}

InstructionSelection::InstructionSelection(QV4::ExecutionEngine *engine, V4IR::Module *module)
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

void InstructionSelection::run(QV4::Function *vmFunction, V4IR::Function *function)
{
    QVector<Lookup> lookups;
    QSet<V4IR::BasicBlock*> reentryBlocks;
    qSwap(_function, function);
    qSwap(_vmFunction, vmFunction);
    qSwap(_lookups, lookups);
    qSwap(_reentryBlocks, reentryBlocks);
    Assembler* oldAssembler = _as;
    _as = new Assembler(_function, _vmFunction, engine());

    int locals = (_function->tempCount + _function->maxNumberOfArguments) + 1;
    locals = (locals + 1) & ~1;
    qSwap(_locals, locals);
    _as->enterStandardStackFrame(_locals);

    int contextPointer = 0;
#if !defined(RETURN_VALUE_IN_REGISTER)
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

    for (int i = 0, ei = _function->basicBlocks.size(); i != ei; ++i) {
        V4IR::BasicBlock *nextBlock = (i < ei - 1) ? _function->basicBlocks[i + 1] : 0;
        _block = _function->basicBlocks[i];
        _as->registerBlock(_block, nextBlock);

        if (_reentryBlocks.contains(_block)) {
            _as->enterStandardStackFrame(/*locals*/0);
#ifdef ARGUMENTS_IN_REGISTERS
            _as->move(Assembler::registerForArgument(0), Assembler::ContextRegister);
            _as->move(Assembler::registerForArgument(1), Assembler::LocalsRegister);
#else
            _as->loadPtr(addressForArgument(0), Assembler::ContextRegister);
            _as->loadPtr(addressForArgument(1), Assembler::LocalsRegister);
#endif
        }

        foreach (V4IR::Stmt *s, _block->statements) {
            if (s->location.isValid())
                _as->recordLineNumber(s->location.startLine);
            s->accept(this);
        }
    }

    _as->link(_vmFunction);

    if (_lookups.size()) {
        _vmFunction->lookups = new Lookup[_lookups.size()];
        memcpy(_vmFunction->lookups, _lookups.constData(), _lookups.size()*sizeof(Lookup));
    }

    UnwindHelper::registerFunction(_vmFunction);

    qSwap(_vmFunction, vmFunction);
    qSwap(_function, function);
    qSwap(_lookups, lookups);
    qSwap(_reentryBlocks, reentryBlocks);
    qSwap(_locals, locals);
    delete _as;
    _as = oldAssembler;
}

void InstructionSelection::callBuiltinInvalid(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result)
{
    int argc = prepareVariableArguments(args);
    QV4::String *s = identifier(*func->id);

    if (useFastLookups && func->global) {
        uint index = addGlobalLookup(s);
        generateFunctionCall(Assembler::Void, __qmljs_call_global_lookup,
                             Assembler::ContextRegister, Assembler::PointerToValue(result),
                             Assembler::TrustedImm32(index),
                             baseAddressForCallArguments(),
                             Assembler::TrustedImm32(argc));
    } else {
        generateFunctionCall(Assembler::Void, __qmljs_call_activation_property,
                             Assembler::ContextRegister, Assembler::PointerToValue(result),
                             s,
                             baseAddressForCallArguments(),
                             Assembler::TrustedImm32(argc));
    }
}

void InstructionSelection::callBuiltinTypeofMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_typeof_member, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::Reference(base), identifier(name));
}

void InstructionSelection::callBuiltinTypeofSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_typeof_element,
            Assembler::ContextRegister, Assembler::PointerToValue(result),
            Assembler::Reference(base), Assembler::Reference(index));
}

void InstructionSelection::callBuiltinTypeofName(const QString &name, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_typeof_name, Assembler::ContextRegister, Assembler::PointerToValue(result), identifier(name));
}

void InstructionSelection::callBuiltinTypeofValue(V4IR::Temp *value, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_typeof, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::Reference(value));
}

void InstructionSelection::callBuiltinDeleteMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_delete_member, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::Reference(base), identifier(name));
}

void InstructionSelection::callBuiltinDeleteSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_delete_subscript, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::Reference(base), Assembler::Reference(index));
}

void InstructionSelection::callBuiltinDeleteName(const QString &name, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_delete_name, Assembler::ContextRegister, Assembler::PointerToValue(result), identifier(name));
}

void InstructionSelection::callBuiltinDeleteValue(V4IR::Temp *result)
{
    _as->storeValue(Value::fromBoolean(false), result);
}

void InstructionSelection::callBuiltinPostIncrementMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_increment_member, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), Assembler::PointerToValue(base), identifier(name));
}

void InstructionSelection::callBuiltinPostIncrementSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_increment_element, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), Assembler::Reference(base), Assembler::PointerToValue(index));
}

void InstructionSelection::callBuiltinPostIncrementName(const QString &name, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_increment_name, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), identifier(name));
}

void InstructionSelection::callBuiltinPostIncrementValue(V4IR::Temp *value, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_increment,
                         Assembler::PointerToValue(result), Assembler::PointerToValue(value));
}

void InstructionSelection::callBuiltinPostDecrementMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_decrement_member, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), Assembler::Reference(base), identifier(name));
}

void InstructionSelection::callBuiltinPostDecrementSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_decrement_element, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), Assembler::Reference(base),
                         Assembler::Reference(index));
}

void InstructionSelection::callBuiltinPostDecrementName(const QString &name, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_decrement_name, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), identifier(name));
}

void InstructionSelection::callBuiltinPostDecrementValue(V4IR::Temp *value, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_post_decrement,
                         Assembler::PointerToValue(result), Assembler::PointerToValue(value));
}

void InstructionSelection::callBuiltinThrow(V4IR::Temp *arg)
{
    generateFunctionCall(Assembler::Void, __qmljs_throw, Assembler::ContextRegister, Assembler::Reference(arg));
}

typedef void *(*MiddleOfFunctionEntryPoint(ExecutionContext *, void *localsPtr));
static void *tryWrapper(ExecutionContext *context, void *localsPtr, MiddleOfFunctionEntryPoint tryBody, MiddleOfFunctionEntryPoint catchBody,
                        QV4::String *exceptionVarName, Value *exceptionVar)
{
    *exceptionVar = Value::undefinedValue();
    void *addressToContinueAt = 0;
    try {
        addressToContinueAt = tryBody(context, localsPtr);
    } catch (Exception& ex) {
        ex.accept(context);
        *exceptionVar = ex.value();
        try {
            ExecutionContext *catchContext = __qmljs_builtin_push_catch_scope(exceptionVarName, ex.value(), context);
            addressToContinueAt = catchBody(catchContext, localsPtr);
            context = __qmljs_builtin_pop_scope(catchContext);
        } catch (Exception& ex) {
            *exceptionVar = ex.value();
            ex.accept(context);
            addressToContinueAt = catchBody(context, localsPtr);
        }
    }
    return addressToContinueAt;
}

void InstructionSelection::visitTry(V4IR::Try *t)
{
    // Call tryWrapper, which is going to re-enter the same function at the address of the try block. At then end
    // of the try function the JIT code will return with the address of the sub-sequent instruction, which tryWrapper
    // returns and to which we jump to.

    _reentryBlocks.insert(t->tryBlock);
    _reentryBlocks.insert(t->catchBlock);

    generateFunctionCall(Assembler::ReturnValueRegister, tryWrapper, Assembler::ContextRegister, Assembler::LocalsRegister,
                         Assembler::ReentryBlock(t->tryBlock), Assembler::ReentryBlock(t->catchBlock),
                         identifier(t->exceptionVarName), Assembler::PointerToValue(t->exceptionVar));
    _as->jump(Assembler::ReturnValueRegister);
}

void InstructionSelection::callBuiltinFinishTry()
{
    // This assumes that we're in code that was called by tryWrapper, so we return to try wrapper
    // with the address that we'd like to continue at, which is right after the ret below.
    Assembler::DataLabelPtr continuation = _as->moveWithPatch(Assembler::TrustedImmPtr(0), Assembler::ReturnValueRegister);
    _as->leaveStandardStackFrame(/*locals*/0);
    _as->ret();
    _as->addPatch(continuation, _as->label());
}

void InstructionSelection::callBuiltinForeachIteratorObject(V4IR::Temp *arg, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_foreach_iterator_object, Assembler::ContextRegister, Assembler::PointerToValue(result), Assembler::Reference(arg));
}

void InstructionSelection::callBuiltinForeachNextPropertyname(V4IR::Temp *arg, V4IR::Temp *result)
{
    generateFunctionCall(Assembler::Void, __qmljs_foreach_next_property_name, Assembler::PointerToValue(result), Assembler::Reference(arg));
}

void InstructionSelection::callBuiltinPushWithScope(V4IR::Temp *arg)
{
    generateFunctionCall(Assembler::ContextRegister, __qmljs_builtin_push_with_scope, Assembler::Reference(arg), Assembler::ContextRegister);
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

void InstructionSelection::callBuiltinDefineGetterSetter(V4IR::Temp *object, const QString &name, V4IR::Temp *getter, V4IR::Temp *setter)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_define_getter_setter, Assembler::ContextRegister,
                         Assembler::Reference(object), identifier(name), Assembler::PointerToValue(getter), Assembler::PointerToValue(setter));
}

void InstructionSelection::callBuiltinDefineProperty(V4IR::Temp *object, const QString &name, V4IR::Temp *value)
{
    generateFunctionCall(Assembler::Void, __qmljs_builtin_define_property, Assembler::ContextRegister,
                         Assembler::Reference(object), identifier(name), Assembler::PointerToValue(value));
}

void InstructionSelection::callBuiltinDefineArray(V4IR::Temp *result, V4IR::ExprList *args)
{
    int length = prepareVariableArguments(args);
    generateFunctionCall(Assembler::Void, __qmljs_builtin_define_array, Assembler::ContextRegister,
                         Assembler::PointerToValue(result),
                         baseAddressForCallArguments(), Assembler::TrustedImm32(length));
}

void InstructionSelection::callBuiltinDefineObjectLiteral(V4IR::Temp *result, V4IR::ExprList *args)
{
    int argc = 0;

    InternalClass *klass = engine()->emptyClass;
    V4IR::ExprList *it = args;
    while (it) {
        V4IR::Name *name = it->expr->asName();
        it = it->next;

        bool isData = it->expr->asConst()->value;
        it = it->next;
        klass = klass->addMember(identifier(*name->id), isData ? QV4::Attr_Data : QV4::Attr_Accessor);

        _as->copyValue(argumentAddressForCall(argc++), it->expr);

        if (!isData) {
            it = it->next;
            _as->copyValue(argumentAddressForCall(argc++), it->expr);
        }

        it = it->next;
    }

    _as->move(Assembler::TrustedImmPtr(klass), Assembler::ReturnValueRegister);

    generateFunctionCall(Assembler::Void, __qmljs_builtin_define_object_literal, Assembler::ContextRegister,
                         Assembler::PointerToValue(result), baseAddressForCallArguments(),
                         Assembler::ReturnValueRegister);
}

void InstructionSelection::callValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result)
{
    int argc = prepareVariableArguments(args);
    V4IR::Temp* thisObject = 0;
    generateFunctionCall(Assembler::Void, __qmljs_call_value, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::PointerToValue(thisObject),
            Assembler::Reference(value), baseAddressForCallArguments(), Assembler::TrustedImm32(argc));
}

void InstructionSelection::loadThisObject(V4IR::Temp *temp)
{
#if defined(VALUE_FITS_IN_REGISTER)
    _as->load64(Pointer(Assembler::ContextRegister, offsetof(ExecutionContext, thisObject)), Assembler::ReturnValueRegister);
    _as->storeReturnValue(temp);
#else
    _as->copyValue(temp, Pointer(Assembler::ContextRegister, offsetof(ExecutionContext, thisObject)));
#endif
}

void InstructionSelection::loadConst(V4IR::Const *sourceConst, V4IR::Temp *targetTemp)
{
    _as->storeValue(convertToValue(sourceConst), targetTemp);
}

void InstructionSelection::loadString(const QString &str, V4IR::Temp *targetTemp)
{
    Value v = Value::fromString(identifier(str));
    _as->storeValue(v, targetTemp);
}

void InstructionSelection::loadRegexp(V4IR::RegExp *sourceRegexp, V4IR::Temp *targetTemp)
{
    Value v = Value::fromObject(engine()->newRegExpObject(*sourceRegexp->value,
                                                          sourceRegexp->flags));
    _vmFunction->generatedValues.append(v);
    _as->storeValue(v, targetTemp);
}

void InstructionSelection::getActivationProperty(const V4IR::Name *name, V4IR::Temp *temp)
{
    String *propertyName = identifier(*name->id);
    if (useFastLookups && name->global) {
        uint index = addGlobalLookup(propertyName);
        generateLookupCall(index, offsetof(QV4::Lookup, globalGetter), Assembler::ContextRegister, Assembler::PointerToValue(temp));
        return;
    }
    generateFunctionCall(Assembler::Void, __qmljs_get_activation_property, Assembler::ContextRegister, Assembler::PointerToValue(temp), propertyName);
}

void InstructionSelection::setActivationProperty(V4IR::Temp *source, const QString &targetName)
{
    String *propertyName = identifier(targetName);
    generateFunctionCall(Assembler::Void, __qmljs_set_activation_property,
            Assembler::ContextRegister, propertyName, Assembler::Reference(source));
}

void InstructionSelection::initClosure(V4IR::Closure *closure, V4IR::Temp *target)
{
    QV4::Function *vmFunc = vmFunction(closure->value);
    assert(vmFunc);
    generateFunctionCall(Assembler::Void, __qmljs_init_closure, Assembler::ContextRegister, Assembler::PointerToValue(target), Assembler::TrustedImmPtr(vmFunc));
}

void InstructionSelection::getProperty(V4IR::Temp *base, const QString &name, V4IR::Temp *target)
{
    if (useFastLookups) {
        QV4::String *s = identifier(name);
        uint index = addLookup(s);
        generateLookupCall(index, offsetof(QV4::Lookup, getter), Assembler::PointerToValue(target),
                           Assembler::Reference(base));
    } else {
        generateFunctionCall(Assembler::Void, __qmljs_get_property, Assembler::ContextRegister, Assembler::PointerToValue(target),
                             Assembler::Reference(base), identifier(name));
    }
}

void InstructionSelection::setProperty(V4IR::Temp *source, V4IR::Temp *targetBase, const QString &targetName)
{
    if (useFastLookups) {
        QV4::String *s = identifier(targetName);
        uint index = addSetterLookup(s);
        generateLookupCall(index, offsetof(QV4::Lookup, setter), Assembler::Reference(targetBase), Assembler::Reference(source));
    } else {
        generateFunctionCall(Assembler::Void, __qmljs_set_property, Assembler::ContextRegister,
                Assembler::Reference(targetBase),
                identifier(targetName), Assembler::Reference(source));
    }
}

void InstructionSelection::getElement(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *target)
{
    generateFunctionCall(Assembler::Void, __qmljs_get_element, Assembler::ContextRegister,
                         Assembler::PointerToValue(target), Assembler::Reference(base),
                         Assembler::Reference(index));
}

void InstructionSelection::setElement(V4IR::Temp *source, V4IR::Temp *targetBase, V4IR::Temp *targetIndex)
{
    generateFunctionCall(Assembler::Void, __qmljs_set_element, Assembler::ContextRegister,
                         Assembler::Reference(targetBase), Assembler::Reference(targetIndex),
                         Assembler::Reference(source));
}

void InstructionSelection::copyValue(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp)
{
    _as->copyValue(targetTemp, sourceTemp);
}

#define setOp(op, opName, operation) \
    do { op = operation; opName = isel_stringIfy(operation); } while (0)

void InstructionSelection::unop(V4IR::AluOp oper, V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp)
{
    UnaryOpName op = 0;
    const char *opName = 0;
    switch (oper) {
    case V4IR::OpIfTrue: assert(!"unreachable"); break;
    case V4IR::OpNot: setOp(op, opName, __qmljs_not); break;
    case V4IR::OpUMinus: setOp(op, opName, __qmljs_uminus); break;
    case V4IR::OpUPlus: setOp(op, opName, __qmljs_uplus); break;
    case V4IR::OpCompl: setOp(op, opName, __qmljs_compl); break;
    case V4IR::OpIncrement: setOp(op, opName, __qmljs_increment); break;
    case V4IR::OpDecrement: setOp(op, opName, __qmljs_decrement); break;
    default: assert(!"unreachable"); break;
    } // switch

    if (op)
        _as->generateFunctionCallImp(Assembler::Void, opName, op, Assembler::PointerToValue(targetTemp),
                                     Assembler::Reference(sourceTemp));
}

void InstructionSelection::binop(V4IR::AluOp oper, V4IR::Expr *leftSource, V4IR::Expr *rightSource, V4IR::Temp *target)
{
    Q_ASSERT(leftSource->asTemp() && rightSource->asTemp());
    _as->generateBinOp(oper, target, leftSource->asTemp(), rightSource->asTemp());
}

void InstructionSelection::inplaceNameOp(V4IR::AluOp oper, V4IR::Temp *rightSource, const QString &targetName)
{
    InplaceBinOpName op = 0;
    const char *opName = 0;
    switch (oper) {
    case V4IR::OpBitAnd: setOp(op, opName, __qmljs_inplace_bit_and_name); break;
    case V4IR::OpBitOr: setOp(op, opName, __qmljs_inplace_bit_or_name); break;
    case V4IR::OpBitXor: setOp(op, opName, __qmljs_inplace_bit_xor_name); break;
    case V4IR::OpAdd: setOp(op, opName, __qmljs_inplace_add_name); break;
    case V4IR::OpSub: setOp(op, opName, __qmljs_inplace_sub_name); break;
    case V4IR::OpMul: setOp(op, opName, __qmljs_inplace_mul_name); break;
    case V4IR::OpDiv: setOp(op, opName, __qmljs_inplace_div_name); break;
    case V4IR::OpMod: setOp(op, opName, __qmljs_inplace_mod_name); break;
    case V4IR::OpLShift: setOp(op, opName, __qmljs_inplace_shl_name); break;
    case V4IR::OpRShift: setOp(op, opName, __qmljs_inplace_shr_name); break;
    case V4IR::OpURShift: setOp(op, opName, __qmljs_inplace_ushr_name); break;
    default:
        Q_UNREACHABLE();
        break;
    }
    if (op) {
        _as->generateFunctionCallImp(Assembler::Void, opName, op, Assembler::ContextRegister,
                identifier(targetName), Assembler::Reference(rightSource));
    }
}

void InstructionSelection::inplaceElementOp(V4IR::AluOp oper, V4IR::Temp *source, V4IR::Temp *targetBaseTemp, V4IR::Temp *targetIndexTemp)
{
    InplaceBinOpElement op = 0;
    const char *opName = 0;
    switch (oper) {
    case V4IR::OpBitAnd: setOp(op, opName, __qmljs_inplace_bit_and_element); break;
    case V4IR::OpBitOr: setOp(op, opName, __qmljs_inplace_bit_or_element); break;
    case V4IR::OpBitXor: setOp(op, opName, __qmljs_inplace_bit_xor_element); break;
    case V4IR::OpAdd: setOp(op, opName, __qmljs_inplace_add_element); break;
    case V4IR::OpSub: setOp(op, opName, __qmljs_inplace_sub_element); break;
    case V4IR::OpMul: setOp(op, opName, __qmljs_inplace_mul_element); break;
    case V4IR::OpDiv: setOp(op, opName, __qmljs_inplace_div_element); break;
    case V4IR::OpMod: setOp(op, opName, __qmljs_inplace_mod_element); break;
    case V4IR::OpLShift: setOp(op, opName, __qmljs_inplace_shl_element); break;
    case V4IR::OpRShift: setOp(op, opName, __qmljs_inplace_shr_element); break;
    case V4IR::OpURShift: setOp(op, opName, __qmljs_inplace_ushr_element); break;
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

void InstructionSelection::inplaceMemberOp(V4IR::AluOp oper, V4IR::Temp *source, V4IR::Temp *targetBase, const QString &targetName)
{
    InplaceBinOpMember op = 0;
    const char *opName = 0;
    switch (oper) {
    case V4IR::OpBitAnd: setOp(op, opName, __qmljs_inplace_bit_and_member); break;
    case V4IR::OpBitOr: setOp(op, opName, __qmljs_inplace_bit_or_member); break;
    case V4IR::OpBitXor: setOp(op, opName, __qmljs_inplace_bit_xor_member); break;
    case V4IR::OpAdd: setOp(op, opName, __qmljs_inplace_add_member); break;
    case V4IR::OpSub: setOp(op, opName, __qmljs_inplace_sub_member); break;
    case V4IR::OpMul: setOp(op, opName, __qmljs_inplace_mul_member); break;
    case V4IR::OpDiv: setOp(op, opName, __qmljs_inplace_div_member); break;
    case V4IR::OpMod: setOp(op, opName, __qmljs_inplace_mod_member); break;
    case V4IR::OpLShift: setOp(op, opName, __qmljs_inplace_shl_member); break;
    case V4IR::OpRShift: setOp(op, opName, __qmljs_inplace_shr_member); break;
    case V4IR::OpURShift: setOp(op, opName, __qmljs_inplace_ushr_member); break;
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

void InstructionSelection::callProperty(V4IR::Temp *base, const QString &name,
                                        V4IR::ExprList *args, V4IR::Temp *result)
{
    assert(base != 0);

    int argc = prepareVariableArguments(args);
    QV4::String *s = identifier(name);

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

void InstructionSelection::callSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::ExprList *args, V4IR::Temp *result)
{
    assert(base != 0);

    int argc = prepareVariableArguments(args);
    generateFunctionCall(Assembler::Void, __qmljs_call_element,
                         Assembler::ContextRegister, Assembler::PointerToValue(result),
                         Assembler::Reference(base), Assembler::Reference(index),
                         baseAddressForCallArguments(),
                         Assembler::TrustedImm32(argc));
}

void InstructionSelection::convertType(V4IR::Temp *source, V4IR::Temp *target)
{
    // FIXME: do something more useful with this info
    copyValue(source, target);
}

String *InstructionSelection::identifier(const QString &s)
{
    String *str = engine()->newIdentifier(s);
    _vmFunction->identifiers.append(str);
    return str;
}

void InstructionSelection::constructActivationProperty(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result)
{
    assert(func != 0);

    if (useFastLookups && func->global) {
        int argc = prepareVariableArguments(args);
        QV4::String *s = identifier(*func->id);

        uint index = addGlobalLookup(s);
        generateFunctionCall(Assembler::Void, __qmljs_construct_global_lookup,
                             Assembler::ContextRegister, Assembler::PointerToValue(result),
                             Assembler::TrustedImm32(index),
                             baseAddressForCallArguments(),
                             Assembler::TrustedImm32(argc));
        return;
    }

    callRuntimeMethod(result, __qmljs_construct_activation_property, func, args);
}

void InstructionSelection::constructProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result)
{
    int argc = prepareVariableArguments(args);
    generateFunctionCall(Assembler::Void, __qmljs_construct_property, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::Reference(base), identifier(name), baseAddressForCallArguments(), Assembler::TrustedImm32(argc));
}

void InstructionSelection::constructValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result)
{
    assert(value != 0);

    int argc = prepareVariableArguments(args);
    generateFunctionCall(Assembler::Void, __qmljs_construct_value, Assembler::ContextRegister,
            Assembler::PointerToValue(result), Assembler::Reference(value), baseAddressForCallArguments(), Assembler::TrustedImm32(argc));
}

void InstructionSelection::visitJump(V4IR::Jump *s)
{
    _as->jumpToBlock(_block, s->target);
}

void InstructionSelection::visitCJump(V4IR::CJump *s)
{
    if (V4IR::Temp *t = s->cond->asTemp()) {
        Address temp = _as->loadTempAddress(Assembler::ScratchRegister, t);
        Address tag = temp;
        tag.offset += offsetof(QV4::Value, tag);
        Assembler::Jump booleanConversion = _as->branch32(Assembler::NotEqual, tag, Assembler::TrustedImm32(QV4::Value::Boolean_Type));

        Address data = temp;
        data.offset += offsetof(QV4::Value, int_32);
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
    } else if (V4IR::Binop *b = s->cond->asBinop()) {
        if (b->left->asTemp() && b->right->asTemp()) {
            CmpOp op = 0;
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
            case V4IR::OpInstanceof: setOp(op, opName, __qmljs_cmp_instanceof); break;
            case V4IR::OpIn: setOp(op, opName, __qmljs_cmp_in); break;
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

void InstructionSelection::visitRet(V4IR::Ret *s)
{
    if (V4IR::Temp *t = s->expr->asTemp()) {
#if defined(RETURN_VALUE_IN_REGISTER)
#if CPU(X86)
       Address addr = _as->loadTempAddress(Assembler::ScratchRegister, t);
       _as->load32(addr, JSC::X86Registers::eax);
       addr.offset += 4;
       _as->load32(addr, JSC::X86Registers::edx);
#else
        _as->copyValue(Assembler::ReturnValueRegister, t);
#endif
#else
        _as->loadPtr(addressForArgument(0), Assembler::ReturnValueRegister);
        _as->copyValue(Address(Assembler::ReturnValueRegister, 0), t);
#endif
    } else if (V4IR::Const *c = s->expr->asConst()) {
        _as->copyValue(Assembler::ReturnValueRegister, c);
    } else {
        Q_UNIMPLEMENTED();
        Q_UNREACHABLE();
        Q_UNUSED(s);
    }

    _as->leaveStandardStackFrame(_locals);
#if !defined(ARGUMENTS_IN_REGISTERS) && !defined(RETURN_VALUE_IN_REGISTER)
    // Emulate ret(n) instruction
    // Pop off return address into scratch register ...
    _as->pop(Assembler::ScratchRegister);
    // ... and overwrite the invisible argument with
    // the return address.
    _as->poke(Assembler::ScratchRegister);
#endif
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
//        V4IR::Temp *arg = it->expr->asTemp();
//        assert(arg != 0);
        _as->copyValue(argumentAddressForCall(i), it->expr);
    }

    return argc;
}

void InstructionSelection::callRuntimeMethodImp(V4IR::Temp *result, const char* name, ActivationMethod method, V4IR::Expr *base, V4IR::ExprList *args)
{
    V4IR::Name *baseName = base->asName();
    assert(baseName != 0);

    int argc = prepareVariableArguments(args);
    _as->generateFunctionCallImp(Assembler::Void, name, method, Assembler::ContextRegister, Assembler::PointerToValue(result),
                                 identifier(*baseName->id), baseAddressForCallArguments(),
                                 Assembler::TrustedImm32(argc));
}


uint InstructionSelection::addLookup(QV4::String *name)
{
    uint index = (uint)_lookups.size();
    QV4::Lookup l;
    l.getter = Lookup::getterGeneric;
    for (int i = 0; i < Lookup::Size; ++i)
        l.classList[i] = 0;
    l.level = -1;
    l.index = UINT_MAX;
    l.name = name;
    _lookups.append(l);
    return index;
}

uint InstructionSelection::addSetterLookup(QV4::String *name)
{
    uint index = (uint)_lookups.size();
    QV4::Lookup l;
    l.setter = Lookup::setterGeneric;
    for (int i = 0; i < Lookup::Size; ++i)
        l.classList[i] = 0;
    l.level = -1;
    l.index = UINT_MAX;
    l.name = name;
    _lookups.append(l);
    return index;
}

uint InstructionSelection::addGlobalLookup(QV4::String *name)
{
    uint index = (uint)_lookups.size();
    QV4::Lookup l;
    l.globalGetter = Lookup::globalGetterGeneric;
    for (int i = 0; i < Lookup::Size; ++i)
        l.classList[i] = 0;
    l.level = -1;
    l.index = UINT_MAX;
    l.name = name;
    _lookups.append(l);
    return index;
}
