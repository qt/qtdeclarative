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

#include "qv4isel_util_p.h"
#include "qv4isel_moth_p.h"
#include "qv4vme_moth_p.h"
#include "qv4ssa_p.h"
#include <private/qv4debugging_p.h>
#include <private/qv4function_p.h>
#include <private/qv4regexpobject_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qqmlengine_p.h>

#undef USE_TYPE_INFO

using namespace QV4;
using namespace QV4::Moth;

namespace {

inline QV4::Runtime::BinaryOperation aluOpFunction(IR::AluOp op)
{
    switch (op) {
    case IR::OpInvalid:
        return 0;
    case IR::OpIfTrue:
        return 0;
    case IR::OpNot:
        return 0;
    case IR::OpUMinus:
        return 0;
    case IR::OpUPlus:
        return 0;
    case IR::OpCompl:
        return 0;
    case IR::OpBitAnd:
        return QV4::Runtime::bitAnd;
    case IR::OpBitOr:
        return QV4::Runtime::bitOr;
    case IR::OpBitXor:
        return QV4::Runtime::bitXor;
    case IR::OpAdd:
        return 0;
    case IR::OpSub:
        return QV4::Runtime::sub;
    case IR::OpMul:
        return QV4::Runtime::mul;
    case IR::OpDiv:
        return QV4::Runtime::div;
    case IR::OpMod:
        return QV4::Runtime::mod;
    case IR::OpLShift:
        return QV4::Runtime::shl;
    case IR::OpRShift:
        return QV4::Runtime::shr;
    case IR::OpURShift:
        return QV4::Runtime::ushr;
    case IR::OpGt:
        return QV4::Runtime::greaterThan;
    case IR::OpLt:
        return QV4::Runtime::lessThan;
    case IR::OpGe:
        return QV4::Runtime::greaterEqual;
    case IR::OpLe:
        return QV4::Runtime::lessEqual;
    case IR::OpEqual:
        return QV4::Runtime::equal;
    case IR::OpNotEqual:
        return QV4::Runtime::notEqual;
    case IR::OpStrictEqual:
        return QV4::Runtime::strictEqual;
    case IR::OpStrictNotEqual:
        return QV4::Runtime::strictNotEqual;
    case IR::OpInstanceof:
        return 0;
    case IR::OpIn:
        return 0;
    case IR::OpAnd:
        return 0;
    case IR::OpOr:
        return 0;
    default:
        Q_ASSERT(!"Unknown AluOp");
        return 0;
    }
};

inline bool isNumberType(IR::Expr *e)
{
    switch (e->type) {
    case IR::SInt32Type:
    case IR::UInt32Type:
    case IR::DoubleType:
        return true;
    default:
        return false;
    }
}

inline bool isIntegerType(IR::Expr *e)
{
    switch (e->type) {
    case IR::SInt32Type:
    case IR::UInt32Type:
        return true;
    default:
        return false;
    }
}

inline bool isBoolType(IR::Expr *e)
{
    return (e->type == IR::BoolType);
}

/*
 * stack slot allocation:
 *
 * foreach bb do
 *   foreach stmt do
 *     if the current statement is not a phi-node:
 *       purge ranges that end before the current statement
 *       check for life ranges to activate, and if they don't have a stackslot associated then allocate one
 *       renumber temps to stack
 *     for phi nodes: check if all temps (src+dst) are assigned stack slots and marked as allocated
 *     if it's a jump:
 *       foreach phi node in the successor:
 *         allocate slots for each temp (both sources and targets) if they don't have one allocated already
 *         insert moves before the jump
 */
class AllocateStackSlots: protected ConvertTemps
{
    const QVector<IR::LifeTimeInterval> _intervals;
    QVector<const IR::LifeTimeInterval *> _unhandled;
    QVector<const IR::LifeTimeInterval *> _live;
    QBitArray _slotIsInUse;
    IR::Function *_function;

public:
    AllocateStackSlots(const QVector<IR::LifeTimeInterval> &intervals)
        : _intervals(intervals)
        , _slotIsInUse(intervals.size(), false)
        , _function(0)
    {
        _live.reserve(8);
        _unhandled.reserve(_intervals.size());
        for (int i = _intervals.size() - 1; i >= 0; --i)
            _unhandled.append(&_intervals.at(i));
    }

    void forFunction(IR::Function *function)
    {
        IR::Optimizer::showMeTheCode(function);
        _function = function;
        toStackSlots(function);

//        QTextStream os(stdout, QIODevice::WriteOnly);
//        os << "Frame layout:" << endl;
//        foreach (int t, _stackSlotForTemp.keys()) {
//            os << "\t" << t << " -> " << _stackSlotForTemp[t] << endl;
//        }
    }

protected:
    virtual int allocateFreeSlot()
    {
        for (int i = 0, ei = _slotIsInUse.size(); i != ei; ++i) {
            if (!_slotIsInUse[i]) {
                if (_nextUnusedStackSlot <= i) {
                    Q_ASSERT(_nextUnusedStackSlot == i);
                    _nextUnusedStackSlot = i + 1;
                }
                _slotIsInUse[i] = true;
                return i;
            }
        }

        Q_UNREACHABLE();
        return -1;
    }

    virtual void process(IR::Stmt *s)
    {
        Q_ASSERT(s->id > 0);

//        qDebug("L%d statement %d:", _currentBasicBlock->index, s->id);

        if (IR::Phi *phi = s->asPhi()) {
            visitPhi(phi);
        } else {
            // purge ranges no longer alive:
            for (int i = 0; i < _live.size(); ) {
                const IR::LifeTimeInterval *lti = _live.at(i);
                if (lti->end() < s->id) {
//                    qDebug() << "\t - moving temp" << lti->temp().index << "to handled, freeing slot" << _stackSlotForTemp[lti->temp().index];
                    _live.remove(i);
                    Q_ASSERT(_slotIsInUse[_stackSlotForTemp[lti->temp().index]]);
                    _slotIsInUse[_stackSlotForTemp[lti->temp().index]] = false;
                    continue;
                } else {
                    ++i;
                }
            }

            // active new ranges:
            while (!_unhandled.isEmpty()) {
                const IR::LifeTimeInterval *lti = _unhandled.last();
                if (lti->start() > s->id)
                    break; // we're done
                Q_ASSERT(!_stackSlotForTemp.contains(lti->temp().index));
                _stackSlotForTemp[lti->temp().index] = allocateFreeSlot();
//                qDebug() << "\t - activating temp" << lti->temp().index << "on slot" << _stackSlotForTemp[lti->temp().index];
                _live.append(lti);
                _unhandled.removeLast();
            }

            s->accept(this);
        }

        if (IR::Jump *jump = s->asJump()) {
            IR::MoveMapping moves;
            foreach (IR::Stmt *succStmt, jump->target->statements()) {
                if (IR::Phi *phi = succStmt->asPhi()) {
                    forceActivation(*phi->targetTemp);
                    for (int i = 0, ei = phi->d->incoming.size(); i != ei; ++i) {
                        IR::Expr *e = phi->d->incoming[i];
                        if (IR::Temp *t = e->asTemp()) {
                            forceActivation(*t);
                        }
                        if (jump->target->in[i] == _currentBasicBlock)
                            moves.add(phi->d->incoming[i], phi->targetTemp);
                    }
                } else {
                    break;
                }
            }
            moves.order();
            QList<IR::Move *> newMoves = moves.insertMoves(_currentBasicBlock, _function, true);
            foreach (IR::Move *move, newMoves)
                move->accept(this);
        }
    }

    void forceActivation(const IR::Temp &t)
    {
        if (_stackSlotForTemp.contains(t.index))
            return;

        int i = _unhandled.size() - 1;
        for (; i >= 0; --i) {
            const IR::LifeTimeInterval *lti = _unhandled[i];
            if (lti->temp() == t) {
                _live.append(lti);
                _unhandled.remove(i);
                break;
            }
        }
        Q_ASSERT(i >= 0); // check that we always found the entry

        _stackSlotForTemp[t.index] = allocateFreeSlot();
//        qDebug() << "\t - force activating temp" << t.index << "on slot" << _stackSlotForTemp[t.index];
    }

    virtual void visitPhi(IR::Phi *phi)
    {
        Q_UNUSED(phi);
#if !defined(QT_NO_DEBUG)
        Q_ASSERT(_stackSlotForTemp.contains(phi->targetTemp->index));
        Q_ASSERT(_slotIsInUse[_stackSlotForTemp[phi->targetTemp->index]]);
        foreach (IR::Expr *e, phi->d->incoming) {
            if (IR::Temp *t = e->asTemp())
                Q_ASSERT(_stackSlotForTemp.contains(t->index));
        }
#endif // defined(QT_NO_DEBUG)
    }
};
} // anonymous namespace

InstructionSelection::InstructionSelection(QQmlEnginePrivate *qmlEngine, QV4::ExecutableAllocator *execAllocator, IR::Module *module, QV4::Compiler::JSUnitGenerator *jsGenerator)
    : EvalInstructionSelection(execAllocator, module, jsGenerator)
    , qmlEngine(qmlEngine)
    , _block(0)
    , _codeStart(0)
    , _codeNext(0)
    , _codeEnd(0)
    , _currentStatement(0)
{
    compilationUnit = new CompilationUnit;
}

InstructionSelection::~InstructionSelection()
{
}

void InstructionSelection::run(int functionIndex)
{
    IR::Function *function = irModule->functions[functionIndex];
    IR::BasicBlock *block = 0, *nextBlock = 0;

    QHash<IR::BasicBlock *, QVector<ptrdiff_t> > patches;
    QHash<IR::BasicBlock *, ptrdiff_t> addrs;

    int codeSize = 4096;
    uchar *codeStart = new uchar[codeSize];
    memset(codeStart, 0, codeSize);
    uchar *codeNext = codeStart;
    uchar *codeEnd = codeStart + codeSize;

    qSwap(_function, function);
    qSwap(block, _block);
    qSwap(nextBlock, _nextBlock);
    qSwap(patches, _patches);
    qSwap(addrs, _addrs);
    qSwap(codeStart, _codeStart);
    qSwap(codeNext, _codeNext);
    qSwap(codeEnd, _codeEnd);

    IR::Optimizer opt(_function);
    opt.run(qmlEngine);
    if (opt.isInSSA()) {
        static const bool doStackSlotAllocation =
                qgetenv("QV4_NO_INTERPRETER_STACK_SLOT_ALLOCATION").isEmpty();

        if (doStackSlotAllocation) {
            AllocateStackSlots(opt.lifeTimeIntervals()).forFunction(_function);
        } else {
            opt.convertOutOfSSA();
            ConvertTemps().toStackSlots(_function);
        }
        opt.showMeTheCode(_function);
    } else {
        ConvertTemps().toStackSlots(_function);
    }

    QSet<IR::Jump *> removableJumps = opt.calculateOptionalJumps();
    qSwap(_removableJumps, removableJumps);

    IR::Stmt *cs = 0;
    qSwap(_currentStatement, cs);

    int locals = frameSize();
    Q_ASSERT(locals >= 0);

    IR::BasicBlock *exceptionHandler = 0;

    Instruction::Push push;
    push.value = quint32(locals);
    addInstruction(push);

    currentLine = 0;
    QVector<IR::BasicBlock *> basicBlocks = _function->basicBlocks();
    for (int i = 0, ei = basicBlocks.size(); i != ei; ++i) {
        blockNeedsDebugInstruction = irModule->debugMode;
        _block = basicBlocks[i];
        _nextBlock = (i < ei - 1) ? basicBlocks[i + 1] : 0;
        _addrs.insert(_block, _codeNext - _codeStart);

        if (_block->catchBlock != exceptionHandler) {
            Instruction::SetExceptionHandler set;
            set.offset = 0;
            if (_block->catchBlock) {
                ptrdiff_t loc = addInstruction(set) + (((const char *)&set.offset) - ((const char *)&set));
                _patches[_block->catchBlock].append(loc);
            } else {
                addInstruction(set);
            }
            exceptionHandler = _block->catchBlock;
        }

        foreach (IR::Stmt *s, _block->statements()) {
            _currentStatement = s;

            if (s->location.isValid()) {
                if (s->location.startLine != currentLine) {
                    blockNeedsDebugInstruction = false;
                    currentLine = s->location.startLine;
                    if (irModule->debugMode) {
                        Instruction::Debug debug;
                        debug.lineNumber = currentLine;
                        addInstruction(debug);
                    } else {
                        Instruction::Line line;
                        line.lineNumber = currentLine;
                        addInstruction(line);
                    }
                }
            }

            s->accept(this);
        }
    }

    // TODO: patch stack size (the push instruction)
    patchJumpAddresses();

    codeRefs.insert(_function, squeezeCode());

    qSwap(_currentStatement, cs);
    qSwap(_removableJumps, removableJumps);
    qSwap(_function, function);
    qSwap(block, _block);
    qSwap(nextBlock, _nextBlock);
    qSwap(patches, _patches);
    qSwap(addrs, _addrs);
    qSwap(codeStart, _codeStart);
    qSwap(codeNext, _codeNext);
    qSwap(codeEnd, _codeEnd);

    delete[] codeStart;
}

QV4::CompiledData::CompilationUnit *InstructionSelection::backendCompileStep()
{
    compilationUnit->codeRefs.resize(irModule->functions.size());
    int i = 0;
    foreach (IR::Function *irFunction, irModule->functions)
        compilationUnit->codeRefs[i++] = codeRefs[irFunction];
    return compilationUnit;
}

void InstructionSelection::callValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result)
{
    Instruction::CallValue call;
    prepareCallArgs(args, call.argc);
    call.callData = callDataStart();
    call.dest = getParam(value);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callProperty(IR::Expr *base, const QString &name, IR::ExprList *args,
                                        IR::Temp *result)
{
    if (useFastLookups) {
        Instruction::CallPropertyLookup call;
        call.base = getParam(base);
        call.lookupIndex = registerGetterLookup(name);
        prepareCallArgs(args, call.argc);
        call.callData = callDataStart();
        call.result = getResultParam(result);
        addInstruction(call);
    } else {
        // call the property on the loaded base
        Instruction::CallProperty call;
        call.base = getParam(base);
        call.name = registerString(name);
        prepareCallArgs(args, call.argc);
        call.callData = callDataStart();
        call.result = getResultParam(result);
        addInstruction(call);
    }
}

void InstructionSelection::callSubscript(IR::Expr *base, IR::Expr *index, IR::ExprList *args,
                                         IR::Temp *result)
{
    // call the property on the loaded base
    Instruction::CallElement call;
    call.base = getParam(base);
    call.index = getParam(index);
    prepareCallArgs(args, call.argc);
    call.callData = callDataStart();
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::convertType(IR::Temp *source, IR::Temp *target)
{
    // FIXME: do something more useful with this info
    if (target->type & IR::NumberType && !(source->type & IR::NumberType))
        unop(IR::OpUPlus, source, target);
    else
        copyValue(source, target);
}

void InstructionSelection::constructActivationProperty(IR::Name *func,
                                                       IR::ExprList *args,
                                                       IR::Temp *result)
{
    if (useFastLookups && func->global) {
        Instruction::ConstructGlobalLookup call;
        call.index = registerGlobalGetterLookup(*func->id);
        prepareCallArgs(args, call.argc);
        call.callData = callDataStart();
        call.result = getResultParam(result);
        addInstruction(call);
        return;
    }
    Instruction::CreateActivationProperty create;
    create.name = registerString(*func->id);
    prepareCallArgs(args, create.argc);
    create.callData = callDataStart();
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::constructProperty(IR::Temp *base, const QString &name, IR::ExprList *args, IR::Temp *result)
{
    if (useFastLookups) {
        Instruction::ConstructPropertyLookup call;
        call.base = getParam(base);
        call.index = registerGetterLookup(name);
        prepareCallArgs(args, call.argc);
        call.callData = callDataStart();
        call.result = getResultParam(result);
        addInstruction(call);
        return;
    }
    Instruction::CreateProperty create;
    create.base = getParam(base);
    create.name = registerString(name);
    prepareCallArgs(args, create.argc);
    create.callData = callDataStart();
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::constructValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result)
{
    Instruction::CreateValue create;
    create.func = getParam(value);
    prepareCallArgs(args, create.argc);
    create.callData = callDataStart();
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::loadThisObject(IR::Temp *temp)
{
    Instruction::LoadThis load;
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::loadQmlIdArray(IR::Temp *temp)
{
    Instruction::LoadQmlIdArray load;
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::loadQmlImportedScripts(IR::Temp *temp)
{
    Instruction::LoadQmlImportedScripts load;
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::loadQmlContextObject(IR::Temp *temp)
{
    Instruction::LoadQmlContextObject load;
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::loadQmlScopeObject(IR::Temp *temp)
{
    Instruction::LoadQmlScopeObject load;
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::loadQmlSingleton(const QString &name, IR::Temp *temp)
{
    Instruction::LoadQmlSingleton load;
    load.result = getResultParam(temp);
    load.name = registerString(name);
    addInstruction(load);
}

void InstructionSelection::loadConst(IR::Const *sourceConst, IR::Temp *targetTemp)
{
    Q_ASSERT(sourceConst);

    Instruction::MoveConst move;
    move.source = convertToValue(sourceConst).asReturnedValue();
    move.result = getResultParam(targetTemp);
    addInstruction(move);
}

void InstructionSelection::loadString(const QString &str, IR::Temp *targetTemp)
{
    Instruction::LoadRuntimeString load;
    load.stringId = registerString(str);
    load.result = getResultParam(targetTemp);
    addInstruction(load);
}

void InstructionSelection::loadRegexp(IR::RegExp *sourceRegexp, IR::Temp *targetTemp)
{
    Instruction::LoadRegExp load;
    load.regExpId = registerRegExp(sourceRegexp);
    load.result = getResultParam(targetTemp);
    addInstruction(load);
}

void InstructionSelection::getActivationProperty(const IR::Name *name, IR::Temp *temp)
{
    if (useFastLookups && name->global) {
        Instruction::GetGlobalLookup load;
        load.index = registerGlobalGetterLookup(*name->id);
        load.result = getResultParam(temp);
        addInstruction(load);
        return;
    }
    Instruction::LoadName load;
    load.name = registerString(*name->id);
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::setActivationProperty(IR::Expr *source, const QString &targetName)
{
    Instruction::StoreName store;
    store.source = getParam(source);
    store.name = registerString(targetName);
    addInstruction(store);
}

void InstructionSelection::initClosure(IR::Closure *closure, IR::Temp *target)
{
    int id = closure->value;
    Instruction::LoadClosure load;
    load.value = id;
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::getProperty(IR::Expr *base, const QString &name, IR::Temp *target)
{
    if (useFastLookups) {
        Instruction::GetLookup load;
        load.base = getParam(base);
        load.index = registerGetterLookup(name);
        load.result = getResultParam(target);
        addInstruction(load);
        return;
    }
    Instruction::LoadProperty load;
    load.base = getParam(base);
    load.name = registerString(name);
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::setProperty(IR::Expr *source, IR::Expr *targetBase,
                                       const QString &targetName)
{
    if (useFastLookups) {
        Instruction::SetLookup store;
        store.base = getParam(targetBase);
        store.index = registerSetterLookup(targetName);
        store.source = getParam(source);
        addInstruction(store);
        return;
    }
    Instruction::StoreProperty store;
    store.base = getParam(targetBase);
    store.name = registerString(targetName);
    store.source = getParam(source);
    addInstruction(store);
}

void InstructionSelection::setQObjectProperty(IR::Expr *source, IR::Expr *targetBase, int propertyIndex)
{
    Instruction::StoreQObjectProperty store;
    store.base = getParam(targetBase);
    store.propertyIndex = propertyIndex;
    store.source = getParam(source);
    addInstruction(store);
}

void InstructionSelection::getQObjectProperty(IR::Expr *base, int propertyIndex, bool captureRequired, int attachedPropertiesId, IR::Temp *target)
{
    if (attachedPropertiesId != 0) {
        Instruction::LoadAttachedQObjectProperty load;
        load.propertyIndex = propertyIndex;
        load.result = getResultParam(target);
        load.attachedPropertiesId = attachedPropertiesId;
        addInstruction(load);
    } else {
        Instruction::LoadQObjectProperty load;
        load.base = getParam(base);
        load.propertyIndex = propertyIndex;
        load.result = getResultParam(target);
        load.captureRequired = captureRequired;
        addInstruction(load);
    }
}

void InstructionSelection::getElement(IR::Expr *base, IR::Expr *index, IR::Temp *target)
{
    if (useFastLookups) {
        Instruction::LoadElementLookup load;
        load.lookup = registerIndexedGetterLookup();
        load.base = getParam(base);
        load.index = getParam(index);
        load.result = getResultParam(target);
        addInstruction(load);
        return;
    }
    Instruction::LoadElement load;
    load.base = getParam(base);
    load.index = getParam(index);
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::setElement(IR::Expr *source, IR::Expr *targetBase,
                                      IR::Expr *targetIndex)
{
    if (useFastLookups) {
        Instruction::StoreElementLookup store;
        store.lookup = registerIndexedSetterLookup();
        store.base = getParam(targetBase);
        store.index = getParam(targetIndex);
        store.source = getParam(source);
        addInstruction(store);
        return;
    }
    Instruction::StoreElement store;
    store.base = getParam(targetBase);
    store.index = getParam(targetIndex);
    store.source = getParam(source);
    addInstruction(store);
}

void InstructionSelection::copyValue(IR::Temp *sourceTemp, IR::Temp *targetTemp)
{
    Instruction::Move move;
    move.source = getParam(sourceTemp);
    move.result = getResultParam(targetTemp);
    if (move.source != move.result)
        addInstruction(move);
}

void InstructionSelection::swapValues(IR::Temp *sourceTemp, IR::Temp *targetTemp)
{
    Instruction::SwapTemps swap;
    swap.left = getParam(sourceTemp);
    swap.right = getParam(targetTemp);
    addInstruction(swap);
}

void InstructionSelection::unop(IR::AluOp oper, IR::Temp *sourceTemp, IR::Temp *targetTemp)
{
    switch (oper) {
    case IR::OpIfTrue:
        Q_ASSERT(!"unreachable"); break;
    case IR::OpNot: {
        // ### enabling this fails in some cases, where apparently the value is not a bool at runtime
        if (0 && isBoolType(sourceTemp)) {
            Instruction::UNotBool unot;
            unot.source = getParam(sourceTemp);
            unot.result = getResultParam(targetTemp);
            addInstruction(unot);
            return;
        }
        Instruction::UNot unot;
        unot.source = getParam(sourceTemp);
        unot.result = getResultParam(targetTemp);
        addInstruction(unot);
        return;
    }
    case IR::OpUMinus: {
        Instruction::UMinus uminus;
        uminus.source = getParam(sourceTemp);
        uminus.result = getResultParam(targetTemp);
        addInstruction(uminus);
        return;
    }
    case IR::OpUPlus: {
        if (isNumberType(sourceTemp)) {
            // use a move
            Instruction::Move move;
            move.source = getParam(sourceTemp);
            move.result = getResultParam(targetTemp);
            if (move.source != move.result)
                addInstruction(move);
            return;
        }
        Instruction::UPlus uplus;
        uplus.source = getParam(sourceTemp);
        uplus.result = getResultParam(targetTemp);
        addInstruction(uplus);
        return;
    }
    case IR::OpCompl: {
        // ### enabling this fails in some cases, where apparently the value is not a int at runtime
        if (0 && isIntegerType(sourceTemp)) {
            Instruction::UComplInt unot;
            unot.source = getParam(sourceTemp);
            unot.result = getResultParam(targetTemp);
            addInstruction(unot);
            return;
        }
        Instruction::UCompl ucompl;
        ucompl.source = getParam(sourceTemp);
        ucompl.result = getResultParam(targetTemp);
        addInstruction(ucompl);
        return;
    }
    case IR::OpIncrement: {
        Instruction::Increment inc;
        inc.source = getParam(sourceTemp);
        inc.result = getResultParam(targetTemp);
        addInstruction(inc);
        return;
    }
    case IR::OpDecrement: {
        Instruction::Decrement dec;
        dec.source = getParam(sourceTemp);
        dec.result = getResultParam(targetTemp);
        addInstruction(dec);
        return;
    }
    default:  break;
    } // switch

    Q_ASSERT(!"unreachable");
}

void InstructionSelection::binop(IR::AluOp oper, IR::Expr *leftSource, IR::Expr *rightSource, IR::Temp *target)
{
    binopHelper(oper, leftSource, rightSource, target);
}

Param InstructionSelection::binopHelper(IR::AluOp oper, IR::Expr *leftSource, IR::Expr *rightSource, IR::Temp *target)
{
    if (oper == IR::OpAdd) {
        Instruction::Add add;
        add.lhs = getParam(leftSource);
        add.rhs = getParam(rightSource);
        add.result = getResultParam(target);
        addInstruction(add);
        return add.result;
    }
    if (oper == IR::OpSub) {
        Instruction::Sub sub;
        sub.lhs = getParam(leftSource);
        sub.rhs = getParam(rightSource);
        sub.result = getResultParam(target);
        addInstruction(sub);
        return sub.result;
    }
    if (oper == IR::OpMul) {
        Instruction::Mul mul;
        mul.lhs = getParam(leftSource);
        mul.rhs = getParam(rightSource);
        mul.result = getResultParam(target);
        addInstruction(mul);
        return mul.result;
    }
    if (oper == IR::OpBitAnd) {
        if (leftSource->asConst())
            qSwap(leftSource, rightSource);
        if (IR::Const *c = rightSource->asConst()) {
            Instruction::BitAndConst bitAnd;
            bitAnd.lhs = getParam(leftSource);
            bitAnd.rhs = convertToValue(c).Value::toInt32();
            bitAnd.result = getResultParam(target);
            addInstruction(bitAnd);
            return bitAnd.result;
        }
        Instruction::BitAnd bitAnd;
        bitAnd.lhs = getParam(leftSource);
        bitAnd.rhs = getParam(rightSource);
        bitAnd.result = getResultParam(target);
        addInstruction(bitAnd);
        return bitAnd.result;
    }
    if (oper == IR::OpBitOr) {
        if (leftSource->asConst())
            qSwap(leftSource, rightSource);
        if (IR::Const *c = rightSource->asConst()) {
            Instruction::BitOrConst bitOr;
            bitOr.lhs = getParam(leftSource);
            bitOr.rhs = convertToValue(c).Value::toInt32();
            bitOr.result = getResultParam(target);
            addInstruction(bitOr);
            return bitOr.result;
        }
        Instruction::BitOr bitOr;
        bitOr.lhs = getParam(leftSource);
        bitOr.rhs = getParam(rightSource);
        bitOr.result = getResultParam(target);
        addInstruction(bitOr);
        return bitOr.result;
    }
    if (oper == IR::OpBitXor) {
        if (leftSource->asConst())
            qSwap(leftSource, rightSource);
        if (IR::Const *c = rightSource->asConst()) {
            Instruction::BitXorConst bitXor;
            bitXor.lhs = getParam(leftSource);
            bitXor.rhs = convertToValue(c).Value::toInt32();
            bitXor.result = getResultParam(target);
            addInstruction(bitXor);
            return bitXor.result;
        }
        Instruction::BitXor bitXor;
        bitXor.lhs = getParam(leftSource);
        bitXor.rhs = getParam(rightSource);
        bitXor.result = getResultParam(target);
        addInstruction(bitXor);
        return bitXor.result;
    }
    if (oper == IR::OpRShift) {
        if (IR::Const *c = rightSource->asConst()) {
            Instruction::ShrConst shr;
            shr.lhs = getParam(leftSource);
            shr.rhs = convertToValue(c).Value::toInt32() & 0x1f;
            shr.result = getResultParam(target);
            addInstruction(shr);
            return shr.result;
        }
        Instruction::Shr shr;
        shr.lhs = getParam(leftSource);
        shr.rhs = getParam(rightSource);
        shr.result = getResultParam(target);
        addInstruction(shr);
        return shr.result;
    }
    if (oper == IR::OpLShift) {
        if (IR::Const *c = rightSource->asConst()) {
            Instruction::ShlConst shl;
            shl.lhs = getParam(leftSource);
            shl.rhs = convertToValue(c).Value::toInt32() & 0x1f;
            shl.result = getResultParam(target);
            addInstruction(shl);
            return shl.result;
        }
        Instruction::Shl shl;
        shl.lhs = getParam(leftSource);
        shl.rhs = getParam(rightSource);
        shl.result = getResultParam(target);
        addInstruction(shl);
        return shl.result;
    }

    if (oper == IR::OpInstanceof || oper == IR::OpIn || oper == IR::OpAdd) {
        Instruction::BinopContext binop;
        if (oper == IR::OpInstanceof)
            binop.alu = QV4::Runtime::instanceof;
        else if (oper == IR::OpIn)
            binop.alu = QV4::Runtime::in;
        else
            binop.alu = QV4::Runtime::add;
        binop.lhs = getParam(leftSource);
        binop.rhs = getParam(rightSource);
        binop.result = getResultParam(target);
        Q_ASSERT(binop.alu);
        addInstruction(binop);
        return binop.result;
    } else {
        Instruction::Binop binop;
        binop.alu = aluOpFunction(oper);
        binop.lhs = getParam(leftSource);
        binop.rhs = getParam(rightSource);
        binop.result = getResultParam(target);
        Q_ASSERT(binop.alu);
        addInstruction(binop);
        return binop.result;
    }
}

void InstructionSelection::prepareCallArgs(IR::ExprList *e, quint32 &argc, quint32 *args)
{
    int argLocation = outgoingArgumentTempStart();
    argc = 0;
    if (args)
        *args = argLocation;
    if (e) {
        // We need to move all the temps into the function arg array
        Q_ASSERT(argLocation >= 0);
        while (e) {
            if (IR::Const *c = e->expr->asConst()) {
                Instruction::MoveConst move;
                move.source = convertToValue(c).asReturnedValue();
                move.result = Param::createTemp(argLocation);
                addInstruction(move);
            } else {
                Instruction::Move move;
                move.source = getParam(e->expr);
                move.result = Param::createTemp(argLocation);
                addInstruction(move);
            }
            ++argLocation;
            ++argc;
            e = e->next;
        }
    }
}

void InstructionSelection::visitJump(IR::Jump *s)
{
    if (s->target == _nextBlock)
        return;
    if (_removableJumps.contains(s))
        return;

    if (blockNeedsDebugInstruction) {
        Instruction::Debug debug;
        debug.lineNumber = -int(currentLine);
        addInstruction(debug);
    }

    Instruction::Jump jump;
    jump.offset = 0;
    ptrdiff_t loc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));

    _patches[s->target].append(loc);
}

void InstructionSelection::visitCJump(IR::CJump *s)
{
    if (blockNeedsDebugInstruction) {
        Instruction::Debug debug;
        debug.lineNumber = -int(currentLine);
        addInstruction(debug);
    }

    Param condition;
    if (IR::Temp *t = s->cond->asTemp()) {
        condition = getResultParam(t);
    } else if (IR::Binop *b = s->cond->asBinop()) {
        condition = binopHelper(b->op, b->left, b->right, /*target*/0);
    } else {
        Q_UNIMPLEMENTED();
    }

    if (s->iftrue == _nextBlock) {
        Instruction::JumpNe jump;
        jump.offset = 0;
        jump.condition = condition;
        ptrdiff_t falseLoc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));
        _patches[s->iffalse].append(falseLoc);
    } else {
        Instruction::JumpEq jump;
        jump.offset = 0;
        jump.condition = condition;
        ptrdiff_t trueLoc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));
        _patches[s->iftrue].append(trueLoc);

        if (s->iffalse != _nextBlock) {
            Instruction::Jump jump;
            jump.offset = 0;
            ptrdiff_t falseLoc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));
            _patches[s->iffalse].append(falseLoc);
        }
    }
}

void InstructionSelection::visitRet(IR::Ret *s)
{
    if (blockNeedsDebugInstruction) {
        // this is required so stepOut will always be guaranteed to stop in every stack frame
        Instruction::Debug debug;
        debug.lineNumber = -int(currentLine);
        addInstruction(debug);
    }

    Instruction::Ret ret;
    ret.result = getParam(s->expr);
    addInstruction(ret);
}

void InstructionSelection::callBuiltinInvalid(IR::Name *func, IR::ExprList *args, IR::Temp *result)
{
    if (useFastLookups && func->global) {
        Instruction::CallGlobalLookup call;
        call.index = registerGlobalGetterLookup(*func->id);
        prepareCallArgs(args, call.argc);
        call.callData = callDataStart();
        call.result = getResultParam(result);
        addInstruction(call);
        return;
    }
    Instruction::CallActivationProperty call;
    call.name = registerString(*func->id);
    prepareCallArgs(args, call.argc);
    call.callData = callDataStart();
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofMember(IR::Expr *base, const QString &name,
                                                   IR::Temp *result)
{
    Instruction::CallBuiltinTypeofMember call;
    call.base = getParam(base);
    call.member = registerString(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofSubscript(IR::Expr *base, IR::Expr *index,
                                                      IR::Temp *result)
{
    Instruction::CallBuiltinTypeofSubscript call;
    call.base = getParam(base);
    call.index = getParam(index);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofName(const QString &name, IR::Temp *result)
{
    Instruction::CallBuiltinTypeofName call;
    call.name = registerString(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofValue(IR::Expr *value, IR::Temp *result)
{
    Instruction::CallBuiltinTypeofValue call;
    call.value = getParam(value);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteMember(IR::Temp *base, const QString &name, IR::Temp *result)
{
    Instruction::CallBuiltinDeleteMember call;
    call.base = getParam(base);
    call.member = registerString(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteSubscript(IR::Temp *base, IR::Expr *index,
                                                      IR::Temp *result)
{
    Instruction::CallBuiltinDeleteSubscript call;
    call.base = getParam(base);
    call.index = getParam(index);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteName(const QString &name, IR::Temp *result)
{
    Instruction::CallBuiltinDeleteName call;
    call.name = registerString(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteValue(IR::Temp *result)
{
    Instruction::MoveConst move;
    move.source = QV4::Encode(false);
    move.result = getResultParam(result);
    addInstruction(move);
}

void InstructionSelection::callBuiltinThrow(IR::Expr *arg)
{
    Instruction::CallBuiltinThrow call;
    call.arg = getParam(arg);
    addInstruction(call);
}

void InstructionSelection::callBuiltinReThrow()
{
    if (_block->catchBlock) {
        // jump to exception handler
        Instruction::Jump jump;
        jump.offset = 0;
        ptrdiff_t loc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));

        _patches[_block->catchBlock].append(loc);
    } else {
        Instruction::Ret ret;
        int idx = jsUnitGenerator()->registerConstant(QV4::Encode::undefined());
        ret.result = Param::createConstant(idx);
        addInstruction(ret);
    }
}

void InstructionSelection::callBuiltinUnwindException(IR::Temp *result)
{
    Instruction::CallBuiltinUnwindException call;
    call.result = getResultParam(result);
    addInstruction(call);
}


void InstructionSelection::callBuiltinPushCatchScope(const QString &exceptionName)
{
    Instruction::CallBuiltinPushCatchScope call;
    call.name = registerString(exceptionName);
    addInstruction(call);
}

void InstructionSelection::callBuiltinForeachIteratorObject(IR::Expr *arg, IR::Temp *result)
{
    Instruction::CallBuiltinForeachIteratorObject call;
    call.arg = getParam(arg);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinForeachNextPropertyname(IR::Temp *arg, IR::Temp *result)
{
    Instruction::CallBuiltinForeachNextPropertyName call;
    call.arg = getParam(arg);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPushWithScope(IR::Temp *arg)
{
    Instruction::CallBuiltinPushScope call;
    call.arg = getParam(arg);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPopScope()
{
    Instruction::CallBuiltinPopScope call;
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeclareVar(bool deletable, const QString &name)
{
    Instruction::CallBuiltinDeclareVar call;
    call.isDeletable = deletable;
    call.varName = registerString(name);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineArray(IR::Temp *result, IR::ExprList *args)
{
    Instruction::CallBuiltinDefineArray call;
    prepareCallArgs(args, call.argc, &call.args);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineObjectLiteral(IR::Temp *result, int keyValuePairCount, IR::ExprList *keyValuePairs, IR::ExprList *arrayEntries, bool needSparseArray)
{
    int argLocation = outgoingArgumentTempStart();

    const int classId = registerJSClass(keyValuePairCount, keyValuePairs);

    // Process key/value pairs first
    IR::ExprList *it = keyValuePairs;
    for (int i = 0; i < keyValuePairCount; ++i, it = it->next) {
        // Skip name
        it = it->next;

        bool isData = it->expr->asConst()->value;
        it = it->next;

        if (IR::Const *c = it->expr->asConst()) {
            Instruction::MoveConst move;
            move.source = convertToValue(c).asReturnedValue();
            move.result = Param::createTemp(argLocation);
            addInstruction(move);
        } else {
            Instruction::Move move;
            move.source = getParam(it->expr);
            move.result = Param::createTemp(argLocation);
            addInstruction(move);
        }
        ++argLocation;

        if (!isData) {
            it = it->next;

            Instruction::Move move;
            move.source = getParam(it->expr);
            move.result = Param::createTemp(argLocation);
            addInstruction(move);
            ++argLocation;
        }
    }

    // Process array values
    uint arrayValueCount = 0;
    it = arrayEntries;
    while (it) {
        IR::Const *index = it->expr->asConst();
        it = it->next;

        bool isData = it->expr->asConst()->value;
        it = it->next;

        if (!isData) {
            it = it->next; // getter
            it = it->next; // setter
            continue;
        }

        ++arrayValueCount;

        Instruction::MoveConst indexMove;
        indexMove.source = convertToValue(index).asReturnedValue();
        indexMove.result = Param::createTemp(argLocation);
        addInstruction(indexMove);
        ++argLocation;

        Instruction::Move move;
        move.source = getParam(it->expr);
        move.result = Param::createTemp(argLocation);
        addInstruction(move);
        ++argLocation;
        it = it->next;
    }

    // Process array getter/setter pairs
    uint arrayGetterSetterCount = 0;
    it = arrayEntries;
    while (it) {
        IR::Const *index = it->expr->asConst();
        it = it->next;

        bool isData = it->expr->asConst()->value;
        it = it->next;

        if (isData) {
            it = it->next; // value
            continue;
        }

        ++arrayGetterSetterCount;

        Instruction::MoveConst indexMove;
        indexMove.source = convertToValue(index).asReturnedValue();
        indexMove.result = Param::createTemp(argLocation);
        addInstruction(indexMove);
        ++argLocation;

        // getter
        Instruction::Move moveGetter;
        moveGetter.source = getParam(it->expr);
        moveGetter.result = Param::createTemp(argLocation);
        addInstruction(moveGetter);
        ++argLocation;
        it = it->next;

        // setter
        Instruction::Move moveSetter;
        moveSetter.source = getParam(it->expr);
        moveSetter.result = Param::createTemp(argLocation);
        addInstruction(moveSetter);
        ++argLocation;
        it = it->next;
    }

    Instruction::CallBuiltinDefineObjectLiteral call;
    call.internalClassId = classId;
    call.arrayValueCount = arrayValueCount;
    call.arrayGetterSetterCountAndFlags = arrayGetterSetterCount | (needSparseArray << 30);
    call.args = outgoingArgumentTempStart();
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinSetupArgumentObject(IR::Temp *result)
{
    Instruction::CallBuiltinSetupArgumentsObject call;
    call.result = getResultParam(result);
    addInstruction(call);
}


void QV4::Moth::InstructionSelection::callBuiltinConvertThisToObject()
{
    Instruction::CallBuiltinConvertThisToObject call;
    addInstruction(call);
}

ptrdiff_t InstructionSelection::addInstructionHelper(Instr::Type type, Instr &instr)
{

#ifdef MOTH_THREADED_INTERPRETER
    instr.common.code = VME::instructionJumpTable()[static_cast<int>(type)];
#else
    instr.common.instructionType = type;
#endif

    int instructionSize = Instr::size(type);
    if (_codeEnd - _codeNext < instructionSize) {
        int currSize = _codeEnd - _codeStart;
        uchar *newCode = new uchar[currSize * 2];
        ::memset(newCode + currSize, 0, currSize);
        ::memcpy(newCode, _codeStart, currSize);
        _codeNext = _codeNext - _codeStart + newCode;
        delete[] _codeStart;
        _codeStart = newCode;
        _codeEnd = _codeStart + currSize * 2;
    }

    ::memcpy(_codeNext, reinterpret_cast<const char *>(&instr), instructionSize);
    ptrdiff_t ptrOffset = _codeNext - _codeStart;
    _codeNext += instructionSize;

    return ptrOffset;
}

void InstructionSelection::patchJumpAddresses()
{
    typedef QHash<IR::BasicBlock *, QVector<ptrdiff_t> >::ConstIterator PatchIt;
    for (PatchIt i = _patches.begin(), ei = _patches.end(); i != ei; ++i) {
        Q_ASSERT(_addrs.contains(i.key()));
        ptrdiff_t target = _addrs.value(i.key());

        const QVector<ptrdiff_t> &patchList = i.value();
        for (int ii = 0, eii = patchList.count(); ii < eii; ++ii) {
            ptrdiff_t patch = patchList.at(ii);

            *((ptrdiff_t *)(_codeStart + patch)) = target - patch;
        }
    }

    _patches.clear();
    _addrs.clear();
}

QByteArray InstructionSelection::squeezeCode() const
{
    int codeSize = _codeNext - _codeStart;
    QByteArray squeezed;
    squeezed.resize(codeSize);
    ::memcpy(squeezed.data(), _codeStart, codeSize);
    return squeezed;
}

Param InstructionSelection::getParam(IR::Expr *e) {
    Q_ASSERT(e);

    if (IR::Const *c = e->asConst()) {
        int idx = jsUnitGenerator()->registerConstant(convertToValue(c).asReturnedValue());
        return Param::createConstant(idx);
    } else if (IR::Temp *t = e->asTemp()) {
        switch (t->kind) {
        case IR::Temp::Formal:
        case IR::Temp::ScopedFormal: return Param::createArgument(t->index, t->scope);
        case IR::Temp::Local: return Param::createLocal(t->index);
        case IR::Temp::ScopedLocal: return Param::createScopedLocal(t->index, t->scope);
        case IR::Temp::StackSlot:
            return Param::createTemp(t->index);
        default:
            Q_UNREACHABLE();
            return Param();
        }
    } else {
        Q_UNIMPLEMENTED();
        return Param();
    }
}


CompilationUnit::~CompilationUnit()
{
}

void CompilationUnit::linkBackendToEngine(QV4::ExecutionEngine *engine)
{
    runtimeFunctions.resize(data->functionTableSize);
    runtimeFunctions.fill(0);
    for (int i = 0 ;i < runtimeFunctions.size(); ++i) {
        const QV4::CompiledData::Function *compiledFunction = data->functionAt(i);

        QV4::Function *runtimeFunction = new QV4::Function(engine, this, compiledFunction, &VME::exec);
        runtimeFunction->codeData = reinterpret_cast<const uchar *>(codeRefs.at(i).constData());
        runtimeFunctions[i] = runtimeFunction;
    }
}
