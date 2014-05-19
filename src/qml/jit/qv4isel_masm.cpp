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
#include "qv4unop_p.h"
#include "qv4binop_p.h"

#include <QtCore/QBuffer>

#include <assembler/LinkBuffer.h>
#include <WTFStubs.h>

#include <iostream>

#if ENABLE(ASSEMBLER)

#if USE(UDIS86)
#  include <udis86.h>
#endif

using namespace QV4;
using namespace QV4::JIT;


namespace {
inline bool isPregOrConst(IR::Expr *e)
{
    if (IR::Temp *t = e->asTemp())
        return t->kind == IR::Temp::PhysicalRegister;
    return e->asConst() != 0;
}

class QIODevicePrintStream: public PrintStream
{
    Q_DISABLE_COPY(QIODevicePrintStream)

public:
    explicit QIODevicePrintStream(QIODevice *dest)
        : dest(dest)
        , buf(4096, '0')
    {
        Q_ASSERT(dest);
    }

    ~QIODevicePrintStream()
    {}

    void vprintf(const char* format, va_list argList) WTF_ATTRIBUTE_PRINTF(2, 0)
    {
        const int written = vsnprintf(buf.data(), buf.size(), format, argList);
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

static void printDisassembledOutputWithCalls(QByteArray processedOutput, const QHash<void*, const char*>& functions)
{
    for (QHash<void*, const char*>::ConstIterator it = functions.begin(), end = functions.end();
         it != end; ++it) {
        QByteArray ptrString = QByteArray::number(quintptr(it.key()), 16);
        ptrString.prepend("0x");
        int idx = processedOutput.indexOf(ptrString);
        if (idx < 0)
            continue;
        idx = processedOutput.lastIndexOf('\n', idx);
        if (idx < 0)
            continue;
        processedOutput = processedOutput.insert(idx, QByteArrayLiteral("                          ; call ") + it.value());
    }
    fprintf(stderr, "%s\n", processedOutput.constData());
    fflush(stderr);
}

JSC::MacroAssemblerCodeRef Assembler::link(int *codeSize)
{
    Label endOfCode = label();

    {
        QHashIterator<IR::BasicBlock *, QVector<Jump> > it(_patches);
        while (it.hasNext()) {
            it.next();
            IR::BasicBlock *block = it.key();
            Label target = _addrs.value(block);
            Q_ASSERT(target.isSet());
            foreach (Jump jump, it.value())
                jump.linkTo(target, this);
        }
    }

    JSC::JSGlobalData dummy(_executableAllocator);
    JSC::LinkBuffer linkBuffer(dummy, this, 0);

    QHash<void*, const char*> functions;
    foreach (CallToLink ctl, _callsToLink) {
        linkBuffer.link(ctl.call, ctl.externalFunction);
        functions[linkBuffer.locationOf(ctl.label).dataLocation()] = ctl.functionName;
    }

    foreach (const DataLabelPatch &p, _dataLabelPatches)
        linkBuffer.patch(p.dataLabel, linkBuffer.locationOf(p.target));

    // link exception handlers
    foreach(Jump jump, exceptionPropagationJumps)
        linkBuffer.link(jump, linkBuffer.locationOf(exceptionReturnLabel));

    {
        QHashIterator<IR::BasicBlock *, QVector<DataLabelPtr> > it(_labelPatches);
        while (it.hasNext()) {
            it.next();
            IR::BasicBlock *block = it.key();
            Label target = _addrs.value(block);
            Q_ASSERT(target.isSet());
            foreach (DataLabelPtr label, it.value())
                linkBuffer.patch(label, linkBuffer.locationOf(target));
        }
    }
    _constTable.finalize(linkBuffer, _isel);

    *codeSize = linkBuffer.offsetOf(endOfCode);

    JSC::MacroAssemblerCodeRef codeRef;

    static bool showCode = !qgetenv("QV4_SHOW_ASM").isNull();
    if (showCode) {
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        WTF::setDataFile(new QIODevicePrintStream(&buf));

        QByteArray name = _function->name->toUtf8();
        if (name.isEmpty()) {
            name = QByteArray::number(quintptr(_function), 16);
            name.prepend("IR::Function(0x");
            name.append(")");
        }
        codeRef = linkBuffer.finalizeCodeWithDisassembly("%s", name.data());

        WTF::setDataFile(stderr);
        printDisassembledOutputWithCalls(buf.data(), functions);
    } else {
        codeRef = linkBuffer.finalizeCodeWithoutDisassembly();
    }

    return codeRef;
}

InstructionSelection::InstructionSelection(QQmlEnginePrivate *qmlEngine, QV4::ExecutableAllocator *execAllocator, IR::Module *module, Compiler::JSUnitGenerator *jsGenerator)
    : EvalInstructionSelection(execAllocator, module, jsGenerator)
    , _block(0)
    , _as(0)
    , qmlEngine(qmlEngine)
{
    compilationUnit = new CompilationUnit;
    compilationUnit->codeRefs.resize(module->functions.size());
}

InstructionSelection::~InstructionSelection()
{
    delete _as;
}

#if (CPU(X86_64) && (OS(MAC_OS_X) || OS(LINUX))) || (CPU(X86) && OS(LINUX))
#  define REGALLOC_IS_SUPPORTED
static QVector<int> getIntRegisters()
{
#  if CPU(X86) && OS(LINUX) // x86 with linux
    static const QVector<int> intRegisters = QVector<int>()
            << JSC::X86Registers::edx
            << JSC::X86Registers::ebx;
#  else // x86_64 with linux or with macos
    static const QVector<int> intRegisters = QVector<int>()
            << JSC::X86Registers::ebx
            << JSC::X86Registers::edi
            << JSC::X86Registers::esi
            << JSC::X86Registers::edx
            << JSC::X86Registers::r9
            << JSC::X86Registers::r8
            << JSC::X86Registers::r13
            << JSC::X86Registers::r15;
#  endif
    return intRegisters;
}

static QVector<int> getFpRegisters()
{
// linux/x86_64, linux/x86, and macos/x86_64:
    static const QVector<int> fpRegisters = QVector<int>()
            << JSC::X86Registers::xmm2
            << JSC::X86Registers::xmm3
            << JSC::X86Registers::xmm4
            << JSC::X86Registers::xmm5
            << JSC::X86Registers::xmm6
            << JSC::X86Registers::xmm7;
    return fpRegisters;
}

#elif CPU(ARM) && OS(LINUX)
    // Note: this is not generic for all ARM platforms. Specifically, r9 is platform dependent
    // (e.g. iOS reserves it). See the ARM GNU Linux abi for details.
#  define REGALLOC_IS_SUPPORTED
static QVector<int> getIntRegisters()
{
    static const QVector<int> intRegisters = QVector<int>()
            << JSC::ARMRegisters::r1
            << JSC::ARMRegisters::r2
            << JSC::ARMRegisters::r3
            << JSC::ARMRegisters::r4
            << JSC::ARMRegisters::r8
            << JSC::ARMRegisters::r9;
    return intRegisters;
}

static QVector<int> getFpRegisters()
{
    static const QVector<int> fpRegisters = QVector<int>()
            << JSC::ARMRegisters::d2
            << JSC::ARMRegisters::d3
            << JSC::ARMRegisters::d4
            << JSC::ARMRegisters::d5
            << JSC::ARMRegisters::d6;
    return fpRegisters;
}
#elif CPU(X86) && OS(WINDOWS)
#  define REGALLOC_IS_SUPPORTED
static QVector<int> getIntRegisters()
{
    static const QVector<int> intRegisters = QVector<int>()
            << JSC::X86Registers::edx
            << JSC::X86Registers::ebx;
    return intRegisters;
}

static QVector<int> getFpRegisters()
{
    static const QVector<int> fpRegisters = QVector<int>()
            << JSC::X86Registers::xmm2
            << JSC::X86Registers::xmm3
            << JSC::X86Registers::xmm4
            << JSC::X86Registers::xmm5
            << JSC::X86Registers::xmm6
            << JSC::X86Registers::xmm7;
    return fpRegisters;
}
#endif

void InstructionSelection::run(int functionIndex)
{
    IR::Function *function = irModule->functions[functionIndex];
    QVector<Lookup> lookups;
    qSwap(_function, function);

    IR::Optimizer opt(_function);
    opt.run(qmlEngine);

#ifdef REGALLOC_IS_SUPPORTED
    static const bool withRegisterAllocator = qgetenv("QV4_NO_REGALLOC").isEmpty();
    if (opt.isInSSA() && withRegisterAllocator) {
        RegisterAllocator(getIntRegisters(), getFpRegisters()).run(_function, opt);
    } else
#endif // REGALLOC_IS_SUPPORTED
    {
        if (opt.isInSSA())
            // No register allocator available for this platform, or env. var was set, so:
            opt.convertOutOfSSA();
        ConvertTemps().toStackSlots(_function);
    }
    IR::Optimizer::showMeTheCode(_function);
    QSet<IR::Jump *> removableJumps = opt.calculateOptionalJumps();
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
    _as->addPtr(Assembler::TrustedImm32(sizeof(QV4::Value)*locals), Assembler::LocalsRegister);
    _as->storePtr(Assembler::LocalsRegister, Address(Assembler::ScratchRegister, qOffsetOf(ExecutionEngine, jsStackTop)));

    int lastLine = 0;
    for (int i = 0, ei = _function->basicBlockCount(); i != ei; ++i) {
        IR::BasicBlock *nextBlock = (i < ei - 1) ? _function->basicBlock(i + 1) : 0;
        _block = _function->basicBlock(i);
        if (_block->isRemoved())
            continue;
        _as->registerBlock(_block, nextBlock);

        foreach (IR::Stmt *s, _block->statements()) {
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

    int dummySize;
    JSC::MacroAssemblerCodeRef codeRef =_as->link(&dummySize);
    compilationUnit->codeRefs[functionIndex] = codeRef;

    qSwap(_function, function);
    delete _as;
    _as = oldAssembler;
    qSwap(_removableJumps, removableJumps);
}

const void *InstructionSelection::addConstantTable(QVector<Primitive> *values)
{
    compilationUnit->constantValues.append(*values);
    values->clear();

    QVector<QV4::Primitive> &finalValues = compilationUnit->constantValues.last();
    finalValues.squeeze();
    return finalValues.constData();
}

QV4::CompiledData::CompilationUnit *InstructionSelection::backendCompileStep()
{
    return compilationUnit;
}

void InstructionSelection::callBuiltinInvalid(IR::Name *func, IR::ExprList *args, IR::Temp *result)
{
    prepareCallData(args, 0);

    if (useFastLookups && func->global) {
        uint index = registerGlobalGetterLookup(*func->id);
        generateFunctionCall(result, Runtime::callGlobalLookup,
                             Assembler::ContextRegister,
                             Assembler::TrustedImm32(index),
                             baseAddressForCallData());
    } else {
        generateFunctionCall(result, Runtime::callActivationProperty,
                             Assembler::ContextRegister,
                             Assembler::PointerToString(*func->id),
                             baseAddressForCallData());
    }
}

void InstructionSelection::callBuiltinTypeofMember(IR::Expr *base, const QString &name,
                                                   IR::Temp *result)
{
    generateFunctionCall(result, Runtime::typeofMember, Assembler::ContextRegister,
                         Assembler::PointerToValue(base), Assembler::PointerToString(name));
}

void InstructionSelection::callBuiltinTypeofSubscript(IR::Expr *base, IR::Expr *index,
                                                      IR::Temp *result)
{
    generateFunctionCall(result, Runtime::typeofElement,
                         Assembler::ContextRegister,
                         Assembler::PointerToValue(base), Assembler::PointerToValue(index));
}

void InstructionSelection::callBuiltinTypeofName(const QString &name, IR::Temp *result)
{
    generateFunctionCall(result, Runtime::typeofName, Assembler::ContextRegister,
                         Assembler::PointerToString(name));
}

void InstructionSelection::callBuiltinTypeofValue(IR::Expr *value, IR::Temp *result)
{
    generateFunctionCall(result, Runtime::typeofValue, Assembler::ContextRegister,
                         Assembler::PointerToValue(value));
}

void InstructionSelection::callBuiltinDeleteMember(IR::Temp *base, const QString &name, IR::Temp *result)
{
    generateFunctionCall(result, Runtime::deleteMember, Assembler::ContextRegister,
                         Assembler::Reference(base), Assembler::PointerToString(name));
}

void InstructionSelection::callBuiltinDeleteSubscript(IR::Temp *base, IR::Expr *index,
                                                      IR::Temp *result)
{
    generateFunctionCall(result, Runtime::deleteElement, Assembler::ContextRegister,
                         Assembler::Reference(base), Assembler::PointerToValue(index));
}

void InstructionSelection::callBuiltinDeleteName(const QString &name, IR::Temp *result)
{
    generateFunctionCall(result, Runtime::deleteName, Assembler::ContextRegister,
                         Assembler::PointerToString(name));
}

void InstructionSelection::callBuiltinDeleteValue(IR::Temp *result)
{
    _as->storeValue(Primitive::fromBoolean(false), result);
}

void InstructionSelection::callBuiltinThrow(IR::Expr *arg)
{
    generateFunctionCall(Assembler::ReturnValueRegister, Runtime::throwException, Assembler::ContextRegister,
                         Assembler::PointerToValue(arg));
}

void InstructionSelection::callBuiltinReThrow()
{
    _as->jumpToExceptionHandler();
}

void InstructionSelection::callBuiltinUnwindException(IR::Temp *result)
{
    generateFunctionCall(result, Runtime::unwindException, Assembler::ContextRegister);

}

void InstructionSelection::callBuiltinPushCatchScope(const QString &exceptionName)
{
    Assembler::Pointer s = _as->loadStringAddress(Assembler::ScratchRegister, exceptionName);
    generateFunctionCall(Assembler::ContextRegister, Runtime::pushCatchScope, Assembler::ContextRegister, s);
}

void InstructionSelection::callBuiltinForeachIteratorObject(IR::Expr *arg, IR::Temp *result)
{
    Q_ASSERT(arg);
    Q_ASSERT(result);

    generateFunctionCall(result, Runtime::foreachIterator, Assembler::ContextRegister, Assembler::PointerToValue(arg));
}

void InstructionSelection::callBuiltinForeachNextPropertyname(IR::Temp *arg, IR::Temp *result)
{
    Q_ASSERT(arg);
    Q_ASSERT(result);

    generateFunctionCall(result, Runtime::foreachNextPropertyName, Assembler::Reference(arg));
}

void InstructionSelection::callBuiltinPushWithScope(IR::Temp *arg)
{
    Q_ASSERT(arg);

    generateFunctionCall(Assembler::ContextRegister, Runtime::pushWithScope, Assembler::Reference(arg), Assembler::ContextRegister);
}

void InstructionSelection::callBuiltinPopScope()
{
    generateFunctionCall(Assembler::ContextRegister, Runtime::popScope, Assembler::ContextRegister);
}

void InstructionSelection::callBuiltinDeclareVar(bool deletable, const QString &name)
{
    generateFunctionCall(Assembler::Void, Runtime::declareVar, Assembler::ContextRegister,
                         Assembler::TrustedImm32(deletable), Assembler::PointerToString(name));
}

void InstructionSelection::callBuiltinDefineArray(IR::Temp *result, IR::ExprList *args)
{
    Q_ASSERT(result);

    int length = prepareVariableArguments(args);
    generateFunctionCall(result, Runtime::arrayLiteral, Assembler::ContextRegister,
                         baseAddressForCallArguments(), Assembler::TrustedImm32(length));
}

void InstructionSelection::callBuiltinDefineObjectLiteral(IR::Temp *result, int keyValuePairCount, IR::ExprList *keyValuePairs, IR::ExprList *arrayEntries, bool needSparseArray)
{
    Q_ASSERT(result);

    int argc = 0;

    const int classId = registerJSClass(keyValuePairCount, keyValuePairs);

    IR::ExprList *it = keyValuePairs;
    for (int i = 0; i < keyValuePairCount; ++i, it = it->next) {
        it = it->next;

        bool isData = it->expr->asConst()->value;
        it = it->next;

        _as->copyValue(_as->stackLayout().argumentAddressForCall(argc++), it->expr);

        if (!isData) {
            it = it->next;
            _as->copyValue(_as->stackLayout().argumentAddressForCall(argc++), it->expr);
        }
    }

    it = arrayEntries;
    uint arrayValueCount = 0;
    while (it) {
        uint index = it->expr->asConst()->value;
        it = it->next;

        bool isData = it->expr->asConst()->value;
        it = it->next;

        if (!isData) {
            it = it->next; // getter
            it = it->next; // setter
            continue;
        }

        ++arrayValueCount;

        // Index
        _as->storeValue(QV4::Primitive::fromUInt32(index), _as->stackLayout().argumentAddressForCall(argc++));

        // Value
        _as->copyValue(_as->stackLayout().argumentAddressForCall(argc++), it->expr);
        it = it->next;
    }

    it = arrayEntries;
    uint arrayGetterSetterCount = 0;
    while (it) {
        uint index = it->expr->asConst()->value;
        it = it->next;

        bool isData = it->expr->asConst()->value;
        it = it->next;

        if (isData) {
            it = it->next; // value
            continue;
        }

        ++arrayGetterSetterCount;

        // Index
        _as->storeValue(QV4::Primitive::fromUInt32(index), _as->stackLayout().argumentAddressForCall(argc++));

        // Getter
        _as->copyValue(_as->stackLayout().argumentAddressForCall(argc++), it->expr);
        it = it->next;

        // Setter
        _as->copyValue(_as->stackLayout().argumentAddressForCall(argc++), it->expr);
        it = it->next;
    }

    generateFunctionCall(result, Runtime::objectLiteral, Assembler::ContextRegister,
                         baseAddressForCallArguments(), Assembler::TrustedImm32(classId),
                         Assembler::TrustedImm32(arrayValueCount), Assembler::TrustedImm32(arrayGetterSetterCount | (needSparseArray << 30)));
}

void InstructionSelection::callBuiltinSetupArgumentObject(IR::Temp *result)
{
    generateFunctionCall(result, Runtime::setupArgumentsObject, Assembler::ContextRegister);
}

void InstructionSelection::callBuiltinConvertThisToObject()
{
    generateFunctionCall(Assembler::Void, Runtime::convertThisToObject, Assembler::ContextRegister);
}

void InstructionSelection::callValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result)
{
    Q_ASSERT(value);

    prepareCallData(args, 0);
    generateFunctionCall(result, Runtime::callValue, Assembler::ContextRegister,
                         Assembler::Reference(value),
                         baseAddressForCallData());
}

void InstructionSelection::loadThisObject(IR::Temp *temp)
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

void InstructionSelection::loadQmlIdArray(IR::Temp *temp)
{
    generateFunctionCall(temp, Runtime::getQmlIdArray, Assembler::ContextRegister);
}

void InstructionSelection::loadQmlImportedScripts(IR::Temp *temp)
{
    generateFunctionCall(temp, Runtime::getQmlImportedScripts, Assembler::ContextRegister);
}

void InstructionSelection::loadQmlContextObject(IR::Temp *temp)
{
    generateFunctionCall(temp, Runtime::getQmlContextObject, Assembler::ContextRegister);
}

void InstructionSelection::loadQmlScopeObject(IR::Temp *temp)
{
    generateFunctionCall(temp, Runtime::getQmlScopeObject, Assembler::ContextRegister);
}

void InstructionSelection::loadQmlSingleton(const QString &name, IR::Temp *temp)
{
    generateFunctionCall(temp, Runtime::getQmlSingleton, Assembler::ContextRegister, Assembler::PointerToString(name));
}

void InstructionSelection::loadConst(IR::Const *sourceConst, IR::Temp *targetTemp)
{
    if (targetTemp->kind == IR::Temp::PhysicalRegister) {
        if (targetTemp->type == IR::DoubleType) {
            Q_ASSERT(sourceConst->type == IR::DoubleType);
            _as->toDoubleRegister(sourceConst, (Assembler::FPRegisterID) targetTemp->index);
        } else if (targetTemp->type == IR::SInt32Type) {
            Q_ASSERT(sourceConst->type == IR::SInt32Type);
            _as->toInt32Register(sourceConst, (Assembler::RegisterID) targetTemp->index);
        } else if (targetTemp->type == IR::UInt32Type) {
            Q_ASSERT(sourceConst->type == IR::UInt32Type);
            _as->toUInt32Register(sourceConst, (Assembler::RegisterID) targetTemp->index);
        } else if (targetTemp->type == IR::BoolType) {
            Q_ASSERT(sourceConst->type == IR::BoolType);
            _as->move(Assembler::TrustedImm32(convertToValue(sourceConst).int_32),
                      (Assembler::RegisterID) targetTemp->index);
        } else {
            Q_UNREACHABLE();
        }
    } else {
        _as->storeValue(convertToValue(sourceConst), targetTemp);
    }
}

void InstructionSelection::loadString(const QString &str, IR::Temp *targetTemp)
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

void InstructionSelection::loadRegexp(IR::RegExp *sourceRegexp, IR::Temp *targetTemp)
{
    int id = registerRegExp(sourceRegexp);
    generateFunctionCall(targetTemp, Runtime::regexpLiteral, Assembler::ContextRegister, Assembler::TrustedImm32(id));
}

void InstructionSelection::getActivationProperty(const IR::Name *name, IR::Temp *temp)
{
    if (useFastLookups && name->global) {
        uint index = registerGlobalGetterLookup(*name->id);
        generateLookupCall(temp, index, qOffsetOf(QV4::Lookup, globalGetter), Assembler::ContextRegister, Assembler::Void);
        return;
    }
    generateFunctionCall(temp, Runtime::getActivationProperty, Assembler::ContextRegister, Assembler::PointerToString(*name->id));
}

void InstructionSelection::setActivationProperty(IR::Expr *source, const QString &targetName)
{
    // ### should use a lookup call here
    generateFunctionCall(Assembler::Void, Runtime::setActivationProperty,
                         Assembler::ContextRegister, Assembler::PointerToString(targetName), Assembler::PointerToValue(source));
}

void InstructionSelection::initClosure(IR::Closure *closure, IR::Temp *target)
{
    int id = closure->value;
    generateFunctionCall(target, Runtime::closure, Assembler::ContextRegister, Assembler::TrustedImm32(id));
}

void InstructionSelection::getProperty(IR::Expr *base, const QString &name, IR::Temp *target)
{
    if (useFastLookups) {
        uint index = registerGetterLookup(name);
        generateLookupCall(target, index, qOffsetOf(QV4::Lookup, getter), Assembler::PointerToValue(base), Assembler::Void);
    } else {
        generateFunctionCall(target, Runtime::getProperty, Assembler::ContextRegister,
                             Assembler::PointerToValue(base), Assembler::PointerToString(name));
    }
}

void InstructionSelection::getQObjectProperty(IR::Expr *base, int propertyIndex, bool captureRequired, int attachedPropertiesId, IR::Temp *target)
{
    if (attachedPropertiesId != 0)
        generateFunctionCall(target, Runtime::getQmlAttachedProperty, Assembler::ContextRegister, Assembler::TrustedImm32(attachedPropertiesId), Assembler::TrustedImm32(propertyIndex));
    else
        generateFunctionCall(target, Runtime::getQmlQObjectProperty, Assembler::ContextRegister, Assembler::PointerToValue(base), Assembler::TrustedImm32(propertyIndex),
                             Assembler::TrustedImm32(captureRequired));
}

void InstructionSelection::setProperty(IR::Expr *source, IR::Expr *targetBase,
                                       const QString &targetName)
{
    if (useFastLookups) {
        uint index = registerSetterLookup(targetName);
        generateLookupCall(Assembler::Void, index, qOffsetOf(QV4::Lookup, setter),
                           Assembler::PointerToValue(targetBase),
                           Assembler::PointerToValue(source));
    } else {
        generateFunctionCall(Assembler::Void, Runtime::setProperty, Assembler::ContextRegister,
                             Assembler::PointerToValue(targetBase), Assembler::PointerToString(targetName),
                             Assembler::PointerToValue(source));
    }
}

void InstructionSelection::setQObjectProperty(IR::Expr *source, IR::Expr *targetBase, int propertyIndex)
{
    generateFunctionCall(Assembler::Void, Runtime::setQmlQObjectProperty, Assembler::ContextRegister, Assembler::PointerToValue(targetBase),
                         Assembler::TrustedImm32(propertyIndex), Assembler::PointerToValue(source));
}

void InstructionSelection::getElement(IR::Expr *base, IR::Expr *index, IR::Temp *target)
{
    if (useFastLookups) {
        uint lookup = registerIndexedGetterLookup();
        generateLookupCall(target, lookup, qOffsetOf(QV4::Lookup, indexedGetter),
                           Assembler::PointerToValue(base),
                           Assembler::PointerToValue(index));
        return;
    }

    generateFunctionCall(target, Runtime::getElement, Assembler::ContextRegister,
                         Assembler::PointerToValue(base), Assembler::PointerToValue(index));
}

void InstructionSelection::setElement(IR::Expr *source, IR::Expr *targetBase, IR::Expr *targetIndex)
{
    if (useFastLookups) {
        uint lookup = registerIndexedSetterLookup();
        generateLookupCall(Assembler::Void, lookup, qOffsetOf(QV4::Lookup, indexedSetter),
                           Assembler::PointerToValue(targetBase), Assembler::PointerToValue(targetIndex),
                           Assembler::PointerToValue(source));
        return;
    }
    generateFunctionCall(Assembler::Void, Runtime::setElement, Assembler::ContextRegister,
                         Assembler::PointerToValue(targetBase), Assembler::PointerToValue(targetIndex),
                         Assembler::PointerToValue(source));
}

void InstructionSelection::copyValue(IR::Temp *sourceTemp, IR::Temp *targetTemp)
{
    if (*sourceTemp == *targetTemp)
        return;

    if (sourceTemp->kind == IR::Temp::PhysicalRegister) {
        if (targetTemp->kind == IR::Temp::PhysicalRegister) {
            if (sourceTemp->type == IR::DoubleType)
                _as->moveDouble((Assembler::FPRegisterID) sourceTemp->index,
                                (Assembler::FPRegisterID) targetTemp->index);
            else
                _as->move((Assembler::RegisterID) sourceTemp->index,
                          (Assembler::RegisterID) targetTemp->index);
            return;
        } else {
            switch (sourceTemp->type) {
            case IR::DoubleType:
                _as->storeDouble((Assembler::FPRegisterID) sourceTemp->index, targetTemp);
                break;
            case IR::SInt32Type:
                _as->storeInt32((Assembler::RegisterID) sourceTemp->index, targetTemp);
                break;
            case IR::UInt32Type:
                _as->storeUInt32((Assembler::RegisterID) sourceTemp->index, targetTemp);
                break;
            case IR::BoolType:
                _as->storeBool((Assembler::RegisterID) sourceTemp->index, targetTemp);
                break;
            default:
                Q_ASSERT(!"Unreachable");
                break;
            }
            return;
        }
    } else if (targetTemp->kind == IR::Temp::PhysicalRegister) {
        switch (targetTemp->type) {
        case IR::DoubleType:
            Q_ASSERT(sourceTemp->type == IR::DoubleType);
            _as->toDoubleRegister(sourceTemp, (Assembler::FPRegisterID) targetTemp->index);
            return;
        case IR::BoolType:
            Q_ASSERT(sourceTemp->type == IR::BoolType);
            _as->toInt32Register(sourceTemp, (Assembler::RegisterID) targetTemp->index);
            return;
        case IR::SInt32Type:
            Q_ASSERT(sourceTemp->type == IR::SInt32Type);
            _as->toInt32Register(sourceTemp, (Assembler::RegisterID) targetTemp->index);
            return;
        case IR::UInt32Type:
            Q_ASSERT(sourceTemp->type == IR::UInt32Type);
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

void InstructionSelection::swapValues(IR::Temp *sourceTemp, IR::Temp *targetTemp)
{
    if (sourceTemp->kind == IR::Temp::PhysicalRegister) {
        if (targetTemp->kind == IR::Temp::PhysicalRegister) {
            Q_ASSERT(sourceTemp->type == targetTemp->type);

            if (sourceTemp->type == IR::DoubleType) {
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
    } else if (sourceTemp->kind == IR::Temp::StackSlot) {
        if (targetTemp->kind == IR::Temp::StackSlot) {
            // Note: a swap for two stack-slots can involve different types.
            Assembler::Pointer sAddr = _as->stackSlotPointer(sourceTemp);
            Assembler::Pointer tAddr = _as->stackSlotPointer(targetTemp);
            // use the implementation in JSC::MacroAssembler, as it doesn't do bit swizzling
            _as->JSC::MacroAssembler::loadDouble(sAddr, Assembler::FPGpr0);
            _as->JSC::MacroAssembler::loadDouble(tAddr, Assembler::FPGpr1);
            _as->JSC::MacroAssembler::storeDouble(Assembler::FPGpr1, sAddr);
            _as->JSC::MacroAssembler::storeDouble(Assembler::FPGpr0, tAddr);
            return;
        }
    }

    IR::Temp *stackTemp = sourceTemp->kind == IR::Temp::StackSlot ? sourceTemp : targetTemp;
    IR::Temp *registerTemp = sourceTemp->kind == IR::Temp::PhysicalRegister ? sourceTemp
                                                                                : targetTemp;
    Assembler::Pointer addr = _as->stackSlotPointer(stackTemp);
    if (registerTemp->type == IR::DoubleType) {
        _as->loadDouble(addr, Assembler::FPGpr0);
        _as->storeDouble((Assembler::FPRegisterID) registerTemp->index, addr);
        _as->moveDouble(Assembler::FPGpr0, (Assembler::FPRegisterID) registerTemp->index);
    } else if (registerTemp->type == IR::UInt32Type) {
        _as->toUInt32Register(addr, Assembler::ScratchRegister);
        _as->storeUInt32((Assembler::RegisterID) registerTemp->index, addr);
        _as->move(Assembler::ScratchRegister, (Assembler::RegisterID) registerTemp->index);
    } else {
        _as->load32(addr, Assembler::ScratchRegister);
        _as->store32((Assembler::RegisterID) registerTemp->index, addr);
        if (registerTemp->type != stackTemp->type) {
            addr.offset += 4;
            quint32 tag;
            switch (registerTemp->type) {
            case IR::BoolType:
                tag = QV4::Value::_Boolean_Type;
                break;
            case IR::SInt32Type:
                tag = QV4::Value::_Integer_Type;
                break;
            default:
                tag = QV4::Value::Undefined_Type;
                Q_UNREACHABLE();
            }
            _as->store32(Assembler::TrustedImm32(tag), addr);
        }
        _as->move(Assembler::ScratchRegister, (Assembler::RegisterID) registerTemp->index);
    }
}

#define setOp(op, opName, operation) \
    do { op = operation; opName = isel_stringIfy(operation); } while (0)
#define setOpContext(op, opName, operation) \
    do { opContext = operation; opName = isel_stringIfy(operation); } while (0)

void InstructionSelection::unop(IR::AluOp oper, IR::Temp *sourceTemp, IR::Temp *targetTemp)
{
    QV4::JIT::Unop unop(_as, oper);
    unop.generate(sourceTemp, targetTemp);
}


void InstructionSelection::binop(IR::AluOp oper, IR::Expr *leftSource, IR::Expr *rightSource, IR::Temp *target)
{
    QV4::JIT::Binop binop(_as, oper);
    binop.generate(leftSource, rightSource, target);
}

void InstructionSelection::callProperty(IR::Expr *base, const QString &name, IR::ExprList *args,
                                        IR::Temp *result)
{
    Q_ASSERT(base != 0);

    prepareCallData(args, base);

    if (useFastLookups) {
        uint index = registerGetterLookup(name);
        generateFunctionCall(result, Runtime::callPropertyLookup,
                             Assembler::ContextRegister,
                             Assembler::TrustedImm32(index),
                             baseAddressForCallData());
    } else {
        generateFunctionCall(result, Runtime::callProperty, Assembler::ContextRegister,
                             Assembler::PointerToString(name),
                             baseAddressForCallData());
    }
}

void InstructionSelection::callSubscript(IR::Expr *base, IR::Expr *index, IR::ExprList *args,
                                         IR::Temp *result)
{
    Q_ASSERT(base != 0);

    prepareCallData(args, base);
    generateFunctionCall(result, Runtime::callElement, Assembler::ContextRegister,
                         Assembler::PointerToValue(index),
                         baseAddressForCallData());
}

void InstructionSelection::convertType(IR::Temp *source, IR::Temp *target)
{
    switch (target->type) {
    case IR::DoubleType:
        convertTypeToDouble(source, target);
        break;
    case IR::BoolType:
        convertTypeToBool(source, target);
        break;
    case IR::SInt32Type:
        convertTypeToSInt32(source, target);
        break;
    case IR::UInt32Type:
        convertTypeToUInt32(source, target);
        break;
    default:
        convertTypeSlowPath(source, target);
        break;
    }
}

void InstructionSelection::convertTypeSlowPath(IR::Temp *source, IR::Temp *target)
{
    Q_ASSERT(target->type != IR::BoolType);

    if (target->type & IR::NumberType)
        unop(IR::OpUPlus, source, target);
    else
        copyValue(source, target);
}

void InstructionSelection::convertTypeToDouble(IR::Temp *source, IR::Temp *target)
{
    switch (source->type) {
    case IR::SInt32Type:
    case IR::BoolType:
    case IR::NullType:
        convertIntToDouble(source, target);
        break;
    case IR::UInt32Type:
        convertUIntToDouble(source, target);
        break;
    case IR::UndefinedType:
        _as->loadDouble(_as->loadTempAddress(Assembler::ScratchRegister, source), Assembler::FPGpr0);
        _as->storeDouble(Assembler::FPGpr0, target);
        break;
    case IR::StringType:
    case IR::VarType: {
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

        generateFunctionCall(target, Runtime::toDouble, Assembler::PointerToValue(source));
        Assembler::Jump noDoubleDone = _as->jump();

        // it is a double:
        isDbl.link(_as);
        Assembler::Pointer addr2 = _as->loadTempAddress(Assembler::ScratchRegister, source);
        if (target->kind == IR::Temp::StackSlot) {
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

void InstructionSelection::convertTypeToBool(IR::Temp *source, IR::Temp *target)
{
    switch (source->type) {
    case IR::SInt32Type:
    case IR::UInt32Type:
        convertIntToBool(source, target);
        break;
    case IR::DoubleType: {
        // The source is in a register if the register allocator is used. If the register
        // allocator was not used, then that means that we can use any register for to
        // load the double into.
        Assembler::FPRegisterID reg;
        if (source->kind == IR::Temp::PhysicalRegister)
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
    case IR::UndefinedType:
    case IR::NullType:
        _as->storeBool(false, target);
        break;
    case IR::StringType:
    case IR::VarType:
    default:
        generateFunctionCall(Assembler::ReturnValueRegister, Runtime::toBoolean,
                             Assembler::PointerToValue(source));
        _as->storeBool(Assembler::ReturnValueRegister, target);
        break;
    }
}

void InstructionSelection::convertTypeToSInt32(IR::Temp *source, IR::Temp *target)
{
    switch (source->type) {
    case IR::VarType: {

#if QT_POINTER_SIZE == 8
        Assembler::Pointer addr = _as->loadTempAddress(Assembler::ScratchRegister, source);
        _as->load64(addr, Assembler::ScratchRegister);
        _as->move(Assembler::ScratchRegister, Assembler::ReturnValueRegister);

        // check if it's a number
        _as->urshift64(Assembler::TrustedImm32(QV4::Value::IsNumber_Shift), Assembler::ScratchRegister);
        Assembler::Jump isInt = _as->branch32(Assembler::Equal, Assembler::ScratchRegister, Assembler::TrustedImm32(1));
        Assembler::Jump fallback = _as->branch32(Assembler::Equal, Assembler::ScratchRegister, Assembler::TrustedImm32(0));

        // it's a double
        _as->move(Assembler::TrustedImm64(QV4::Value::NaNEncodeMask), Assembler::ScratchRegister);
        _as->xor64(Assembler::ScratchRegister, Assembler::ReturnValueRegister);
        _as->move64ToDouble(Assembler::ReturnValueRegister, Assembler::FPGpr0);
        Assembler::Jump success =
                _as->branchTruncateDoubleToInt32(Assembler::FPGpr0, Assembler::ReturnValueRegister,
                                                 Assembler::BranchIfTruncateSuccessful);

        // not an int:
        fallback.link(_as);
        generateFunctionCall(Assembler::ReturnValueRegister, Runtime::toInt,
                             _as->loadTempAddress(Assembler::ScratchRegister, source));

        isInt.link(_as);
        success.link(_as);
        if (target->kind == IR::Temp::StackSlot) {
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
        if (target->kind == IR::Temp::StackSlot) {
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
        generateFunctionCall(Assembler::ReturnValueRegister, Runtime::toInt,
                             _as->loadTempAddress(Assembler::ScratchRegister, source));
        _as->storeInt32(Assembler::ReturnValueRegister, target);

        intDone.link(_as);
#endif

    } break;
    case IR::DoubleType: {
        Assembler::Jump success =
                _as->branchTruncateDoubleToInt32(_as->toDoubleRegister(source),
                                                 Assembler::ReturnValueRegister,
                                                 Assembler::BranchIfTruncateSuccessful);
        generateFunctionCall(Assembler::ReturnValueRegister, Runtime::doubleToInt,
                             Assembler::PointerToValue(source));
        success.link(_as);
        _as->storeInt32(Assembler::ReturnValueRegister, target);
    } break;
    case IR::UInt32Type:
        _as->storeInt32(_as->toUInt32Register(source, Assembler::ReturnValueRegister), target);
        break;
    case IR::NullType:
    case IR::UndefinedType:
        _as->move(Assembler::TrustedImm32(0), Assembler::ReturnValueRegister);
        _as->storeInt32(Assembler::ReturnValueRegister, target);
        break;
    case IR::BoolType:
        _as->storeInt32(_as->toInt32Register(source, Assembler::ReturnValueRegister), target);
        break;
    case IR::StringType:
    default:
        generateFunctionCall(Assembler::ReturnValueRegister, Runtime::toInt,
                             _as->loadTempAddress(Assembler::ScratchRegister, source));
        _as->storeInt32(Assembler::ReturnValueRegister, target);
        break;
    } // switch (source->type)
}

void InstructionSelection::convertTypeToUInt32(IR::Temp *source, IR::Temp *target)
{
    switch (source->type) {
    case IR::VarType: {
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
        generateFunctionCall(Assembler::ReturnValueRegister, Runtime::toUInt,
                             _as->loadTempAddress(Assembler::ScratchRegister, source));
        _as->storeInt32(Assembler::ReturnValueRegister, target);

        intDone.link(_as);
    } break;
    case IR::DoubleType: {
        Assembler::FPRegisterID reg = _as->toDoubleRegister(source);
        Assembler::Jump success =
                _as->branchTruncateDoubleToUint32(reg, Assembler::ReturnValueRegister,
                                                  Assembler::BranchIfTruncateSuccessful);
        generateFunctionCall(Assembler::ReturnValueRegister, Runtime::doubleToUInt,
                             Assembler::PointerToValue(source));
        success.link(_as);
        _as->storeUInt32(Assembler::ReturnValueRegister, target);
    } break;
    case IR::NullType:
    case IR::UndefinedType:
        _as->move(Assembler::TrustedImm32(0), Assembler::ReturnValueRegister);
        _as->storeUInt32(Assembler::ReturnValueRegister, target);
        break;
    case IR::StringType:
        generateFunctionCall(Assembler::ReturnValueRegister, Runtime::toUInt,
                             Assembler::PointerToValue(source));
        _as->storeUInt32(Assembler::ReturnValueRegister, target);
        break;
    case IR::SInt32Type:
    case IR::BoolType:
        _as->storeUInt32(_as->toInt32Register(source, Assembler::ReturnValueRegister), target);
        break;
    default:
        break;
    } // switch (source->type)
}

void InstructionSelection::constructActivationProperty(IR::Name *func, IR::ExprList *args, IR::Temp *result)
{
    Q_ASSERT(func != 0);
    prepareCallData(args, 0);

    if (useFastLookups && func->global) {
        uint index = registerGlobalGetterLookup(*func->id);
        generateFunctionCall(result, Runtime::constructGlobalLookup,
                             Assembler::ContextRegister,
                             Assembler::TrustedImm32(index), baseAddressForCallData());
        return;
    }

    generateFunctionCall(result, Runtime::constructActivationProperty,
                         Assembler::ContextRegister,
                         Assembler::PointerToString(*func->id),
                         baseAddressForCallData());
}


void InstructionSelection::constructProperty(IR::Temp *base, const QString &name, IR::ExprList *args, IR::Temp *result)
{
    prepareCallData(args, base);
    if (useFastLookups) {
        uint index = registerGetterLookup(name);
        generateFunctionCall(result, Runtime::constructPropertyLookup,
                             Assembler::ContextRegister,
                             Assembler::TrustedImm32(index),
                             baseAddressForCallData());
        return;
    }

    generateFunctionCall(result, Runtime::constructProperty, Assembler::ContextRegister,
                         Assembler::PointerToString(name),
                         baseAddressForCallData());
}

void InstructionSelection::constructValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result)
{
    Q_ASSERT(value != 0);

    prepareCallData(args, 0);
    generateFunctionCall(result, Runtime::constructValue,
                         Assembler::ContextRegister,
                         Assembler::Reference(value),
                         baseAddressForCallData());
}

void InstructionSelection::visitJump(IR::Jump *s)
{
    if (!_removableJumps.contains(s))
        _as->jumpToBlock(_block, s->target);
}

void InstructionSelection::visitCJump(IR::CJump *s)
{
    if (IR::Temp *t = s->cond->asTemp()) {
        Assembler::RegisterID reg;
        if (t->kind == IR::Temp::PhysicalRegister) {
            Q_ASSERT(t->type == IR::BoolType);
            reg = (Assembler::RegisterID) t->index;
        } else if (t->kind == IR::Temp::StackSlot && t->type == IR::BoolType) {
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
            generateFunctionCall(reg, Runtime::toBoolean, Assembler::Reference(t));

            testBoolean.link(_as);
        }

        _as->generateCJumpOnNonZero(reg, _block, s->iftrue, s->iffalse);
        return;
    } else if (IR::Const *c = s->cond->asConst()) {
        // TODO: SSA optimization for constant condition evaluation should remove this.
        // See also visitCJump() in RegAllocInfo.
        generateFunctionCall(Assembler::ReturnValueRegister, Runtime::toBoolean,
                             Assembler::PointerToValue(c));
        _as->generateCJumpOnNonZero(Assembler::ReturnValueRegister, _block, s->iftrue, s->iffalse);
        return;
    } else if (IR::Binop *b = s->cond->asBinop()) {
        if (b->left->type == IR::DoubleType && b->right->type == IR::DoubleType
                && visitCJumpDouble(b->op, b->left, b->right, s->iftrue, s->iffalse))
            return;

        if (b->op == IR::OpStrictEqual || b->op == IR::OpStrictNotEqual) {
            visitCJumpStrict(b, s->iftrue, s->iffalse);
            return;
        }
        if (b->op == IR::OpEqual || b->op == IR::OpNotEqual) {
            visitCJumpEqual(b, s->iftrue, s->iffalse);
            return;
        }

        Runtime::CompareOperation op = 0;
        Runtime::CompareOperationContext opContext = 0;
        const char *opName = 0;
        switch (b->op) {
        default: Q_UNREACHABLE(); Q_ASSERT(!"todo"); break;
        case IR::OpGt: setOp(op, opName, Runtime::compareGreaterThan); break;
        case IR::OpLt: setOp(op, opName, Runtime::compareLessThan); break;
        case IR::OpGe: setOp(op, opName, Runtime::compareGreaterEqual); break;
        case IR::OpLe: setOp(op, opName, Runtime::compareLessEqual); break;
        case IR::OpEqual: setOp(op, opName, Runtime::compareEqual); break;
        case IR::OpNotEqual: setOp(op, opName, Runtime::compareNotEqual); break;
        case IR::OpStrictEqual: setOp(op, opName, Runtime::compareStrictEqual); break;
        case IR::OpStrictNotEqual: setOp(op, opName, Runtime::compareStrictNotEqual); break;
        case IR::OpInstanceof: setOpContext(op, opName, Runtime::compareInstanceof); break;
        case IR::OpIn: setOpContext(op, opName, Runtime::compareIn); break;
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

void InstructionSelection::visitRet(IR::Ret *s)
{
    if (!s) {
        // this only happens if the method doesn't have a return statement and can
        // only exit through an exception
    } else if (IR::Temp *t = s->expr->asTemp()) {
#if CPU(X86) || CPU(ARM)

#  if CPU(X86)
        Assembler::RegisterID lowReg = JSC::X86Registers::eax;
        Assembler::RegisterID highReg = JSC::X86Registers::edx;
#  else // CPU(ARM)
        Assembler::RegisterID lowReg = JSC::ARMRegisters::r0;
        Assembler::RegisterID highReg = JSC::ARMRegisters::r1;
#  endif

        if (t->kind == IR::Temp::PhysicalRegister) {
            switch (t->type) {
            case IR::DoubleType:
                _as->moveDoubleToInts((Assembler::FPRegisterID) t->index, lowReg, highReg);
                break;
            case IR::UInt32Type: {
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
            case IR::SInt32Type:
                _as->move((Assembler::RegisterID) t->index, lowReg);
                _as->move(Assembler::TrustedImm32(QV4::Value::_Integer_Type), highReg);
                break;
            case IR::BoolType:
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
        if (t->kind == IR::Temp::PhysicalRegister) {
            if (t->type == IR::DoubleType) {
                _as->moveDoubleTo64((Assembler::FPRegisterID) t->index,
                                    Assembler::ReturnValueRegister);
                _as->move(Assembler::TrustedImm64(QV4::Value::NaNEncodeMask),
                          Assembler::ScratchRegister);
                _as->xor64(Assembler::ScratchRegister, Assembler::ReturnValueRegister);
            } else if (t->type == IR::UInt32Type) {
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
                case IR::SInt32Type:
                    tag = QV4::Value::_Integer_Type;
                    break;
                case IR::BoolType:
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
    } else if (IR::Const *c = s->expr->asConst()) {
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
    _as->subPtr(Assembler::TrustedImm32(sizeof(QV4::Value)*locals), Assembler::LocalsRegister);
    _as->loadPtr(Address(Assembler::ContextRegister, qOffsetOf(ExecutionContext, engine)), Assembler::ScratchRegister);
    _as->storePtr(Assembler::LocalsRegister, Address(Assembler::ScratchRegister, qOffsetOf(ExecutionEngine, jsStackTop)));

    _as->leaveStandardStackFrame();
    _as->ret();
}

int InstructionSelection::prepareVariableArguments(IR::ExprList* args)
{
    int argc = 0;
    for (IR::ExprList *it = args; it; it = it->next) {
        ++argc;
    }

    int i = 0;
    for (IR::ExprList *it = args; it; it = it->next, ++i) {
        IR::Expr *arg = it->expr;
        Q_ASSERT(arg != 0);
        Pointer dst(_as->stackLayout().argumentAddressForCall(i));
        if (arg->asTemp() && arg->asTemp()->kind != IR::Temp::PhysicalRegister)
            _as->memcopyValue(dst, arg->asTemp(), Assembler::ScratchRegister);
        else
            _as->copyValue(dst, arg);
    }

    return argc;
}

int InstructionSelection::prepareCallData(IR::ExprList* args, IR::Expr *thisObject)
{
    int argc = 0;
    for (IR::ExprList *it = args; it; it = it->next) {
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
    for (IR::ExprList *it = args; it; it = it->next, ++i) {
        IR::Expr *arg = it->expr;
        Q_ASSERT(arg != 0);
        Pointer dst(_as->stackLayout().argumentAddressForCall(i));
        if (arg->asTemp() && arg->asTemp()->kind != IR::Temp::PhysicalRegister)
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

Assembler::ImplicitAddress Assembler::ConstantTable::loadValueAddress(IR::Const *c,
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
    const void *tablePtr = isel->addConstantTable(&_values);

    foreach (DataLabelPtr label, _toPatch)
        linkBuffer.patch(label, const_cast<void *>(tablePtr));
}

bool InstructionSelection::visitCJumpDouble(IR::AluOp op, IR::Expr *left, IR::Expr *right,
                                            IR::BasicBlock *iftrue, IR::BasicBlock *iffalse)
{
    if (!isPregOrConst(left) || !isPregOrConst(right))
        return false;

    if (_as->nextBlock() == iftrue) {
        Assembler::Jump target = _as->branchDouble(true, op, left, right);
        _as->addPatch(iffalse, target);
    } else {
        Assembler::Jump target = _as->branchDouble(false, op, left, right);
        _as->addPatch(iftrue, target);
        _as->jumpToBlock(_block, iffalse);
    }
    return true;
}

void InstructionSelection::visitCJumpStrict(IR::Binop *binop, IR::BasicBlock *trueBlock,
                                            IR::BasicBlock *falseBlock)
{
    Q_ASSERT(binop->op == IR::OpStrictEqual || binop->op == IR::OpStrictNotEqual);

    if (visitCJumpStrictNullUndefined(IR::NullType, binop, trueBlock, falseBlock))
        return;
    if (visitCJumpStrictNullUndefined(IR::UndefinedType, binop, trueBlock, falseBlock))
        return;
    if (visitCJumpStrictBool(binop, trueBlock, falseBlock))
        return;

    IR::Expr *left = binop->left;
    IR::Expr *right = binop->right;

    _as->generateFunctionCallImp(Assembler::ReturnValueRegister, "Runtime::compareStrictEqual", Runtime::compareStrictEqual,
                                 Assembler::PointerToValue(left), Assembler::PointerToValue(right));
    _as->generateCJumpOnCompare(binop->op == IR::OpStrictEqual ? Assembler::NotEqual : Assembler::Equal,
                                Assembler::ReturnValueRegister, Assembler::TrustedImm32(0),
                                _block, trueBlock, falseBlock);
}

// Only load the non-null temp.
bool InstructionSelection::visitCJumpStrictNullUndefined(IR::Type nullOrUndef, IR::Binop *binop,
                                                         IR::BasicBlock *trueBlock,
                                                         IR::BasicBlock *falseBlock)
{
    Q_ASSERT(nullOrUndef == IR::NullType || nullOrUndef == IR::UndefinedType);

    IR::Expr *varSrc = 0;
    if (binop->left->type == IR::VarType && binop->right->type == nullOrUndef)
        varSrc = binop->left;
    else if (binop->left->type == nullOrUndef && binop->right->type == IR::VarType)
        varSrc = binop->right;
    if (!varSrc)
        return false;

    if (varSrc->asTemp() && varSrc->asTemp()->kind == IR::Temp::PhysicalRegister) {
        _as->jumpToBlock(_block, falseBlock);
        return true;
    }

    if (IR::Const *c = varSrc->asConst()) {
        if (c->type == nullOrUndef)
            _as->jumpToBlock(_block, trueBlock);
        else
            _as->jumpToBlock(_block, falseBlock);
        return true;
    }

    IR::Temp *t = varSrc->asTemp();
    Q_ASSERT(t);

    Assembler::Pointer tagAddr = _as->loadTempAddress(Assembler::ScratchRegister, t);
    tagAddr.offset += 4;
    const Assembler::RegisterID tagReg = Assembler::ScratchRegister;
    _as->load32(tagAddr, tagReg);

    Assembler::RelationalCondition cond = binop->op == IR::OpStrictEqual ? Assembler::Equal
                                                                           : Assembler::NotEqual;
    const Assembler::TrustedImm32 tag(nullOrUndef == IR::NullType ? int(QV4::Value::_Null_Type)
                                                                    : int(QV4::Value::Undefined_Type));
    _as->generateCJumpOnCompare(cond, tagReg, tag, _block, trueBlock, falseBlock);
    return true;
}

bool InstructionSelection::visitCJumpStrictBool(IR::Binop *binop, IR::BasicBlock *trueBlock,
                                                IR::BasicBlock *falseBlock)
{
    IR::Expr *boolSrc = 0, *otherSrc = 0;
    if (binop->left->type == IR::BoolType) {
        boolSrc = binop->left;
        otherSrc = binop->right;
    } else if (binop->right->type == IR::BoolType) {
        boolSrc = binop->right;
        otherSrc = binop->left;
    } else {
        // neither operands are statically typed as bool, so bail out.
        return false;
    }

    Assembler::RelationalCondition cond = binop->op == IR::OpStrictEqual ? Assembler::Equal
                                                                           : Assembler::NotEqual;

    if (otherSrc->type == IR::BoolType) { // both are boolean
        Assembler::RegisterID one = _as->toBoolRegister(boolSrc, Assembler::ReturnValueRegister);
        Assembler::RegisterID two = _as->toBoolRegister(otherSrc, Assembler::ScratchRegister);
        _as->generateCJumpOnCompare(cond, one, two, _block, trueBlock, falseBlock);
        return true;
    }

    if (otherSrc->type != IR::VarType) {
        _as->jumpToBlock(_block, falseBlock);
        return true;
    }

    IR::Temp *otherTemp = otherSrc->asTemp();
    Q_ASSERT(otherTemp); // constants cannot have "var" type
    Q_ASSERT(otherTemp->kind != IR::Temp::PhysicalRegister);

    Assembler::Pointer otherAddr = _as->loadTempAddress(Assembler::ReturnValueRegister, otherTemp);
    otherAddr.offset += 4; // tag address

    // check if the tag of the var operand is indicates 'boolean'
    _as->load32(otherAddr, Assembler::ScratchRegister);
    Assembler::Jump noBool = _as->branch32(Assembler::NotEqual, Assembler::ScratchRegister,
                                           Assembler::TrustedImm32(QV4::Value::_Boolean_Type));
    if (binop->op == IR::OpStrictEqual)
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

bool InstructionSelection::visitCJumpNullUndefined(IR::Type nullOrUndef, IR::Binop *binop,
                                                         IR::BasicBlock *trueBlock,
                                                         IR::BasicBlock *falseBlock)
{
    Q_ASSERT(nullOrUndef == IR::NullType || nullOrUndef == IR::UndefinedType);

    IR::Expr *varSrc = 0;
    if (binop->left->type == IR::VarType && binop->right->type == nullOrUndef)
        varSrc = binop->left;
    else if (binop->left->type == nullOrUndef && binop->right->type == IR::VarType)
        varSrc = binop->right;
    if (!varSrc)
        return false;

    if (varSrc->asTemp() && varSrc->asTemp()->kind == IR::Temp::PhysicalRegister) {
        _as->jumpToBlock(_block, falseBlock);
        return true;
    }

    if (IR::Const *c = varSrc->asConst()) {
        if (c->type == nullOrUndef)
            _as->jumpToBlock(_block, trueBlock);
        else
            _as->jumpToBlock(_block, falseBlock);
        return true;
    }

    IR::Temp *t = varSrc->asTemp();
    Q_ASSERT(t);

    Assembler::Pointer tagAddr = _as->loadTempAddress(Assembler::ScratchRegister, t);
    tagAddr.offset += 4;
    const Assembler::RegisterID tagReg = Assembler::ScratchRegister;
    _as->load32(tagAddr, tagReg);

    if (binop->op == IR::OpNotEqual)
        qSwap(trueBlock, falseBlock);
    Assembler::Jump isNull = _as->branch32(Assembler::Equal, tagReg, Assembler::TrustedImm32(int(QV4::Value::_Null_Type)));
    Assembler::Jump isUndefined = _as->branch32(Assembler::Equal, tagReg, Assembler::TrustedImm32(int(QV4::Value::Undefined_Type)));
    _as->addPatch(trueBlock, isNull);
    _as->addPatch(trueBlock, isUndefined);
    _as->jumpToBlock(_block, falseBlock);

    return true;
}


void InstructionSelection::visitCJumpEqual(IR::Binop *binop, IR::BasicBlock *trueBlock,
                                            IR::BasicBlock *falseBlock)
{
    Q_ASSERT(binop->op == IR::OpEqual || binop->op == IR::OpNotEqual);

    if (visitCJumpNullUndefined(IR::NullType, binop, trueBlock, falseBlock))
        return;

    IR::Expr *left = binop->left;
    IR::Expr *right = binop->right;

    _as->generateFunctionCallImp(Assembler::ReturnValueRegister, "Runtime::compareEqual", Runtime::compareEqual,
                                 Assembler::PointerToValue(left), Assembler::PointerToValue(right));
    _as->generateCJumpOnCompare(binop->op == IR::OpEqual ? Assembler::NotEqual : Assembler::Equal,
                                Assembler::ReturnValueRegister, Assembler::TrustedImm32(0),
                                _block, trueBlock, falseBlock);
}


#endif // ENABLE(ASSEMBLER)
