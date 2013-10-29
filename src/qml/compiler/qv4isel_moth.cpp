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

#undef USE_TYPE_INFO

using namespace QQmlJS;
using namespace QQmlJS::Moth;

namespace {

inline QV4::BinOp aluOpFunction(V4IR::AluOp op)
{
    switch (op) {
    case V4IR::OpInvalid:
        return 0;
    case V4IR::OpIfTrue:
        return 0;
    case V4IR::OpNot:
        return 0;
    case V4IR::OpUMinus:
        return 0;
    case V4IR::OpUPlus:
        return 0;
    case V4IR::OpCompl:
        return 0;
    case V4IR::OpBitAnd:
        return QV4::__qmljs_bit_and;
    case V4IR::OpBitOr:
        return QV4::__qmljs_bit_or;
    case V4IR::OpBitXor:
        return QV4::__qmljs_bit_xor;
    case V4IR::OpAdd:
        return 0;
    case V4IR::OpSub:
        return QV4::__qmljs_sub;
    case V4IR::OpMul:
        return QV4::__qmljs_mul;
    case V4IR::OpDiv:
        return QV4::__qmljs_div;
    case V4IR::OpMod:
        return QV4::__qmljs_mod;
    case V4IR::OpLShift:
        return QV4::__qmljs_shl;
    case V4IR::OpRShift:
        return QV4::__qmljs_shr;
    case V4IR::OpURShift:
        return QV4::__qmljs_ushr;
    case V4IR::OpGt:
        return QV4::__qmljs_gt;
    case V4IR::OpLt:
        return QV4::__qmljs_lt;
    case V4IR::OpGe:
        return QV4::__qmljs_ge;
    case V4IR::OpLe:
        return QV4::__qmljs_le;
    case V4IR::OpEqual:
        return QV4::__qmljs_eq;
    case V4IR::OpNotEqual:
        return QV4::__qmljs_ne;
    case V4IR::OpStrictEqual:
        return QV4::__qmljs_se;
    case V4IR::OpStrictNotEqual:
        return QV4::__qmljs_sne;
    case V4IR::OpInstanceof:
        return 0;
    case V4IR::OpIn:
        return 0;
    case V4IR::OpAnd:
        return 0;
    case V4IR::OpOr:
        return 0;
    default:
        assert(!"Unknown AluOp");
        return 0;
    }
};

inline bool isNumberType(V4IR::Expr *e)
{
    switch (e->type) {
    case V4IR::SInt32Type:
    case V4IR::UInt32Type:
    case V4IR::DoubleType:
        return true;
    default:
        return false;
    }
}

} // anonymous namespace

// TODO: extend to optimize out temp-to-temp moves, where the lifetime of one temp ends at that statement.
//       To handle that, add a hint when such a move will occur, and add a stmt for the hint.
//       Then when asked for a register, check if the active statement is the terminating statement, and if so, apply the hint.
//       This generalises the hint usage for Phi removal too, when the phi is passed in there as the current statement.
class QQmlJS::Moth::StackSlotAllocator
{
    QHash<V4IR::Temp, int> _slotForTemp;
    QHash<V4IR::Temp, int> _hints;
    QVector<int> _activeSlots;

    QHash<V4IR::Temp, V4IR::LifeTimeInterval> _intervals;

public:
    StackSlotAllocator(const QVector<V4IR::LifeTimeInterval> &ranges, int maxTempCount)
        : _activeSlots(maxTempCount)
    {
        _intervals.reserve(ranges.size());
        foreach (const V4IR::LifeTimeInterval &r, ranges)
            _intervals[r.temp()] = r;
    }

    void addHint(const V4IR::Temp &hintedSlotOfTemp, const V4IR::Temp &newTemp)
    {
        if (hintedSlotOfTemp.kind != V4IR::Temp::VirtualRegister
                || newTemp.kind != V4IR::Temp::VirtualRegister)
            return;

        if (_slotForTemp.contains(newTemp) || _hints.contains(newTemp))
            return;

        int hintedSlot = _slotForTemp.value(hintedSlotOfTemp, -1);
        Q_ASSERT(hintedSlot >= 0);
        _hints[newTemp] = hintedSlot;
    }

    int stackSlotFor(V4IR::Temp *t, V4IR::Stmt *currentStmt) {
        Q_ASSERT(t->kind == V4IR::Temp::VirtualRegister);
        Q_ASSERT(t->scope == 0);
        int idx = _slotForTemp.value(*t, -1);
        if (idx == -1)
            idx = allocateSlot(t, currentStmt);
        Q_ASSERT(idx >= 0);
        return idx;
    }

private:
    int allocateSlot(V4IR::Temp *t, V4IR::Stmt *currentStmt) {
        Q_ASSERT(currentStmt->id > 0);

        const V4IR::LifeTimeInterval &interval = _intervals[*t];
        int idx = _hints.value(*t, -1);
        if (idx != -1 && _activeSlots[idx] == currentStmt->id) {
            _slotForTemp[*t] = idx;
            _activeSlots[idx] = interval.end();
            return idx;
        }

        for (int i = 0, ei = _activeSlots.size(); i != ei; ++i) {
            if (_activeSlots[i] < currentStmt->id) {
                _slotForTemp[*t] = i;
                _activeSlots[i] = interval.end();
                return i;
            }
        }

        return -1;
    }
};

InstructionSelection::InstructionSelection(QV4::ExecutableAllocator *execAllocator, V4IR::Module *module, QV4::Compiler::JSUnitGenerator *jsGenerator)
    : EvalInstructionSelection(execAllocator, module, jsGenerator)
    , _function(0)
    , _block(0)
    , _codeStart(0)
    , _codeNext(0)
    , _codeEnd(0)
    , _stackSlotAllocator(0)
    , _currentStatement(0)
{
    compilationUnit = new CompilationUnit;
}

InstructionSelection::~InstructionSelection()
{
}

void InstructionSelection::run(int functionIndex)
{
    V4IR::Function *function = irModule->functions[functionIndex];
    V4IR::BasicBlock *block = 0, *nextBlock = 0;

    QHash<V4IR::BasicBlock *, QVector<ptrdiff_t> > patches;
    QHash<V4IR::BasicBlock *, ptrdiff_t> addrs;

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

    V4IR::Optimizer opt(_function);
    opt.run();
    StackSlotAllocator *stackSlotAllocator = 0;
    if (opt.isInSSA()) {
        //stackSlotAllocator = new StackSlotAllocator(opt.lifeRanges(), _function->tempCount);
        opt.convertOutOfSSA();
    }

    qSwap(_stackSlotAllocator, stackSlotAllocator);
    QSet<V4IR::Jump *> removableJumps = opt.calculateOptionalJumps();
    qSwap(_removableJumps, removableJumps);

    V4IR::Stmt *cs = 0;
    qSwap(_currentStatement, cs);

    int locals = frameSize();
    assert(locals >= 0);

    Instruction::Push push;
    push.value = quint32(locals);
    addInstruction(push);

    QVector<uint> lineNumberMappings;
    lineNumberMappings.reserve(_function->basicBlocks.size() * 2);

    for (int i = 0, ei = _function->basicBlocks.size(); i != ei; ++i) {
        _block = _function->basicBlocks[i];
        _nextBlock = (i < ei - 1) ? _function->basicBlocks[i + 1] : 0;
        _addrs.insert(_block, _codeNext - _codeStart);

        foreach (V4IR::Stmt *s, _block->statements) {
            _currentStatement = s;

            if (s->location.isValid())
                lineNumberMappings << _codeNext - _codeStart << s->location.startLine;

            s->accept(this);
        }
    }

    jsGenerator->registerLineNumberMapping(_function, lineNumberMappings);

    // TODO: patch stack size (the push instruction)
    patchJumpAddresses();

    codeRefs.insert(_function, squeezeCode());

    qSwap(_currentStatement, cs);
    qSwap(_removableJumps, removableJumps);
    qSwap(_stackSlotAllocator, stackSlotAllocator);
    delete stackSlotAllocator;
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
    foreach (V4IR::Function *irFunction, irModule->functions)
        compilationUnit->codeRefs[i++] = codeRefs[irFunction];
    return compilationUnit;
}

void InstructionSelection::callValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result)
{
    Instruction::CallValue call;
    prepareCallArgs(args, call.argc);
    call.callData = callDataStart();
    call.dest = getParam(value);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callProperty(V4IR::Expr *base, const QString &name, V4IR::ExprList *args,
                                        V4IR::Temp *result)
{
    // call the property on the loaded base
    Instruction::CallProperty call;
    call.base = getParam(base);
    call.name = registerString(name);
    prepareCallArgs(args, call.argc);
    call.callData = callDataStart();
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callSubscript(V4IR::Expr *base, V4IR::Expr *index, V4IR::ExprList *args,
                                         V4IR::Temp *result)
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

void InstructionSelection::convertType(V4IR::Temp *source, V4IR::Temp *target)
{
    if (_stackSlotAllocator)
        _stackSlotAllocator->addHint(*source, *target);

    // FIXME: do something more useful with this info
    if (target->type & V4IR::NumberType)
        unop(V4IR::OpUPlus, source, target);
    else
        copyValue(source, target);
}

void InstructionSelection::constructActivationProperty(V4IR::Name *func,
                                                       V4IR::ExprList *args,
                                                       V4IR::Temp *result)
{
    Instruction::CreateActivationProperty create;
    create.name = registerString(*func->id);
    prepareCallArgs(args, create.argc);
    create.callData = callDataStart();
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::constructProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result)
{
    Instruction::CreateProperty create;
    create.base = getParam(base);
    create.name = registerString(name);
    prepareCallArgs(args, create.argc);
    create.callData = callDataStart();
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::constructValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result)
{
    Instruction::CreateValue create;
    create.func = getParam(value);
    prepareCallArgs(args, create.argc);
    create.callData = callDataStart();
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::loadThisObject(V4IR::Temp *temp)
{
    Instruction::LoadThis load;
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::loadConst(V4IR::Const *sourceConst, V4IR::Temp *targetTemp)
{
    assert(sourceConst);

    Instruction::LoadValue load;
    load.value = getParam(sourceConst);
    load.result = getResultParam(targetTemp);
    addInstruction(load);
}

void InstructionSelection::loadString(const QString &str, V4IR::Temp *targetTemp)
{
    Instruction::LoadRuntimeString load;
    load.stringId = registerString(str);
    load.result = getResultParam(targetTemp);
    addInstruction(load);
}

void InstructionSelection::loadRegexp(V4IR::RegExp *sourceRegexp, V4IR::Temp *targetTemp)
{
    Instruction::LoadRegExp load;
    load.regExpId = registerRegExp(sourceRegexp);
    load.result = getResultParam(targetTemp);
    addInstruction(load);
}

void InstructionSelection::getActivationProperty(const V4IR::Name *name, V4IR::Temp *temp)
{
    Instruction::LoadName load;
    load.name = registerString(*name->id);
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::setActivationProperty(V4IR::Expr *source, const QString &targetName)
{
    Instruction::StoreName store;
    store.source = getParam(source);
    store.name = registerString(targetName);
    addInstruction(store);
}

void InstructionSelection::initClosure(V4IR::Closure *closure, V4IR::Temp *target)
{
    int id = closure->value;
    Instruction::LoadClosure load;
    load.value = id;
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::getProperty(V4IR::Expr *base, const QString &name, V4IR::Temp *target)
{
    Instruction::LoadProperty load;
    load.base = getParam(base);
    load.name = registerString(name);
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::setProperty(V4IR::Expr *source, V4IR::Expr *targetBase,
                                       const QString &targetName)
{
    Instruction::StoreProperty store;
    store.base = getParam(targetBase);
    store.name = registerString(targetName);
    store.source = getParam(source);
    addInstruction(store);
}

void InstructionSelection::getElement(V4IR::Expr *base, V4IR::Expr *index, V4IR::Temp *target)
{
    Instruction::LoadElement load;
    load.base = getParam(base);
    load.index = getParam(index);
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::setElement(V4IR::Expr *source, V4IR::Expr *targetBase,
                                      V4IR::Expr *targetIndex)
{
    Instruction::StoreElement store;
    store.base = getParam(targetBase);
    store.index = getParam(targetIndex);
    store.source = getParam(source);
    addInstruction(store);
}

void InstructionSelection::copyValue(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp)
{
    if (_stackSlotAllocator)
        _stackSlotAllocator->addHint(*sourceTemp, *targetTemp);

    Instruction::MoveTemp move;
    move.source = getParam(sourceTemp);
    move.result = getResultParam(targetTemp);
    if (move.source != move.result)
        addInstruction(move);
}

void InstructionSelection::swapValues(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp)
{
    Instruction::SwapTemps swap;
    swap.left = getParam(sourceTemp);
    swap.right = getParam(targetTemp);
    addInstruction(swap);
}

void InstructionSelection::unop(V4IR::AluOp oper, V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp)
{
    if (_stackSlotAllocator)
        _stackSlotAllocator->addHint(*sourceTemp, *targetTemp);

    QV4::UnaryOpName op = 0;
    switch (oper) {
    case V4IR::OpIfTrue: assert(!"unreachable"); break;
    case V4IR::OpNot: op = QV4::__qmljs_not; break;
    case V4IR::OpUMinus: op = QV4::__qmljs_uminus; break;
    case V4IR::OpUPlus: op = QV4::__qmljs_uplus; break;
    case V4IR::OpCompl: op = QV4::__qmljs_compl; break;
    case V4IR::OpIncrement: op = QV4::__qmljs_increment; break;
    case V4IR::OpDecrement: op = QV4::__qmljs_decrement; break;
    default: assert(!"unreachable"); break;
    } // switch

    if (op) {
        Instruction::Unop unop;
        unop.alu = op;
        unop.source = getParam(sourceTemp);
        unop.result = getResultParam(targetTemp);
        addInstruction(unop);
    } else {
        qWarning("  UNOP1");
    }
}

void InstructionSelection::binop(V4IR::AluOp oper, V4IR::Expr *leftSource, V4IR::Expr *rightSource, V4IR::Temp *target)
{
    binopHelper(oper, leftSource, rightSource, target);
}

Param InstructionSelection::binopHelper(V4IR::AluOp oper, V4IR::Expr *leftSource, V4IR::Expr *rightSource, V4IR::Temp *target)
{
    if (isNumberType(leftSource) && isNumberType(rightSource)) {
        switch (oper) {
        case V4IR::OpAdd: {
            Instruction::AddNumberParams instr;
            instr.lhs = getParam(leftSource);
            instr.rhs = getParam(rightSource);
            instr.result = getResultParam(target);
            addInstruction(instr);
            return instr.result;
        }
        case V4IR::OpMul: {
            Instruction::MulNumberParams instr;
            instr.lhs = getParam(leftSource);
            instr.rhs = getParam(rightSource);
            instr.result = getResultParam(target);
            addInstruction(instr);
            return instr.result;
        }
        case V4IR::OpSub: {
            Instruction::SubNumberParams instr;
            instr.lhs = getParam(leftSource);
            instr.rhs = getParam(rightSource);
            instr.result = getResultParam(target);
            addInstruction(instr);
            return instr.result;
        }
        default:
            break;
        }
    }

    if (_stackSlotAllocator && target && leftSource->asTemp())
        _stackSlotAllocator->addHint(*leftSource->asTemp(), *target);

    if (oper == V4IR::OpInstanceof || oper == V4IR::OpIn || oper == V4IR::OpAdd) {
        Instruction::BinopContext binop;
        if (oper == V4IR::OpInstanceof)
            binop.alu = QV4::__qmljs_instanceof;
        else if (oper == V4IR::OpIn)
            binop.alu = QV4::__qmljs_in;
        else
            binop.alu = QV4::__qmljs_add;
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

void InstructionSelection::prepareCallArgs(V4IR::ExprList *e, quint32 &argc, quint32 *args)
{
    int argLocation = outgoingArgumentTempStart();
    argc = 0;
    if (args)
        *args = argLocation;
    if (e) {
        // We need to move all the temps into the function arg array
        assert(argLocation >= 0);
        while (e) {
            Instruction::MoveTemp move;
            move.source = getParam(e->expr);
            move.result = Param::createTemp(argLocation);
            addInstruction(move);
            ++argLocation;
            ++argc;
            e = e->next;
        }
    }
}

void InstructionSelection::visitJump(V4IR::Jump *s)
{
    if (s->target == _nextBlock)
        return;
    if (_removableJumps.contains(s))
        return;

    Instruction::Jump jump;
    jump.offset = 0;
    ptrdiff_t loc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));

    _patches[s->target].append(loc);
}

void InstructionSelection::visitCJump(V4IR::CJump *s)
{
    Param condition;
    if (V4IR::Temp *t = s->cond->asTemp()) {
        condition = getResultParam(t);
    } else if (V4IR::Binop *b = s->cond->asBinop()) {
        condition = binopHelper(b->op, b->left, b->right, /*target*/0);
    } else {
        Q_UNIMPLEMENTED();
    }

    Instruction::CJump jump;
    jump.offset = 0;
    jump.condition = condition;

    if (s->iftrue == _nextBlock) {
        jump.invert = true;
        ptrdiff_t falseLoc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));
        _patches[s->iffalse].append(falseLoc);
    } else {
        jump.invert = false;
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

void InstructionSelection::visitRet(V4IR::Ret *s)
{
    Instruction::Ret ret;
    ret.result = getParam(s->expr);
    addInstruction(ret);
}

void InstructionSelection::visitTry(V4IR::Try *t)
{
    Instruction::EnterTry enterTry;
    enterTry.tryOffset = 0;
    enterTry.catchOffset = 0;
    enterTry.exceptionVarName = registerString(*t->exceptionVarName);
    enterTry.exceptionVar = getParam(t->exceptionVar);
    ptrdiff_t enterTryLoc = addInstruction(enterTry);

    ptrdiff_t tryLoc = enterTryLoc + (((const char *)&enterTry.tryOffset) - ((const char *)&enterTry));
    _patches[t->tryBlock].append(tryLoc);

    ptrdiff_t catchLoc = enterTryLoc + (((const char *)&enterTry.catchOffset) - ((const char *)&enterTry));
    _patches[t->catchBlock].append(catchLoc);
}

void InstructionSelection::callBuiltinInvalid(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result)
{
    Instruction::CallActivationProperty call;
    call.name = registerString(*func->id);
    prepareCallArgs(args, call.argc);
    call.callData = callDataStart();
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofMember(V4IR::Expr *base, const QString &name,
                                                   V4IR::Temp *result)
{
    Instruction::CallBuiltinTypeofMember call;
    call.base = getParam(base);
    call.member = registerString(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofSubscript(V4IR::Expr *base, V4IR::Expr *index,
                                                      V4IR::Temp *result)
{
    Instruction::CallBuiltinTypeofSubscript call;
    call.base = getParam(base);
    call.index = getParam(index);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofName(const QString &name, V4IR::Temp *result)
{
    Instruction::CallBuiltinTypeofName call;
    call.name = registerString(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofValue(V4IR::Expr *value, V4IR::Temp *result)
{
    Instruction::CallBuiltinTypeofValue call;
    call.value = getParam(value);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result)
{
    Instruction::CallBuiltinDeleteMember call;
    call.base = getParam(base);
    call.member = registerString(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteSubscript(V4IR::Temp *base, V4IR::Expr *index,
                                                      V4IR::Temp *result)
{
    Instruction::CallBuiltinDeleteSubscript call;
    call.base = getParam(base);
    call.index = getParam(index);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteName(const QString &name, V4IR::Temp *result)
{
    Instruction::CallBuiltinDeleteName call;
    call.name = registerString(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteValue(V4IR::Temp *result)
{
    Instruction::LoadValue load;
    load.value = Param::createValue(QV4::Primitive::fromBoolean(false));
    load.result = getResultParam(result);
    addInstruction(load);
}

void InstructionSelection::callBuiltinThrow(V4IR::Expr *arg)
{
    Instruction::CallBuiltinThrow call;
    call.arg = getParam(arg);
    addInstruction(call);
}

void InstructionSelection::callBuiltinFinishTry()
{
    Instruction::CallBuiltinFinishTry call;
    addInstruction(call);
}

void InstructionSelection::callBuiltinForeachIteratorObject(V4IR::Temp *arg, V4IR::Temp *result)
{
    Instruction::CallBuiltinForeachIteratorObject call;
    call.arg = getParam(arg);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinForeachNextPropertyname(V4IR::Temp *arg, V4IR::Temp *result)
{
    Instruction::CallBuiltinForeachNextPropertyName call;
    call.arg = getParam(arg);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPushWithScope(V4IR::Temp *arg)
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

void InstructionSelection::callBuiltinDefineGetterSetter(V4IR::Temp *object, const QString &name, V4IR::Temp *getter, V4IR::Temp *setter)
{
    Instruction::CallBuiltinDefineGetterSetter call;
    call.object = getParam(object);
    call.name = registerString(name);
    call.getter = getParam(getter);
    call.setter = getParam(setter);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineProperty(V4IR::Temp *object, const QString &name,
                                                     V4IR::Expr *value)
{
    Instruction::CallBuiltinDefineProperty call;
    call.object = getParam(object);
    call.name = registerString(name);
    call.value = getParam(value);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineArray(V4IR::Temp *result, V4IR::ExprList *args)
{
    Instruction::CallBuiltinDefineArray call;
    prepareCallArgs(args, call.argc, &call.args);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineObjectLiteral(V4IR::Temp *result, V4IR::ExprList *args)
{
    int argLocation = outgoingArgumentTempStart();

    const int classId = registerJSClass(args);
    V4IR::ExprList *it = args;
    while (it) {
        it = it->next;

        bool isData = it->expr->asConst()->value;
        it = it->next;

        Instruction::MoveTemp move;
        move.source = getParam(it->expr);
        move.result = Param::createTemp(argLocation);
        addInstruction(move);
        ++argLocation;

        if (!isData) {
            it = it->next;

            Instruction::MoveTemp move;
            move.source = getParam(it->expr);
            move.result = Param::createTemp(argLocation);
            addInstruction(move);
            ++argLocation;
        }

        it = it->next;
    }

    Instruction::CallBuiltinDefineObjectLiteral call;
    call.internalClassId = classId;
    call.args = outgoingArgumentTempStart();
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinSetupArgumentObject(V4IR::Temp *result)
{
    Instruction::CallBuiltinSetupArgumentsObject call;
    call.result = getResultParam(result);
    addInstruction(call);
}

ptrdiff_t InstructionSelection::addInstructionHelper(Instr::Type type, Instr &instr)
{
#ifdef MOTH_THREADED_INTERPRETER
    instr.common.code = VME::instructionJumpTable()[static_cast<int>(type)];
#else
    instr.common.instructionType = type;
#endif
    instr.common.breakPoint = 0;

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
    typedef QHash<V4IR::BasicBlock *, QVector<ptrdiff_t> >::ConstIterator PatchIt;
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

Param InstructionSelection::getParam(V4IR::Expr *e) {
    typedef Param Param;
    assert(e);

    if (V4IR::Const *c = e->asConst()) {
        return Param::createValue(convertToValue(c));
    } else if (V4IR::Temp *t = e->asTemp()) {
        switch (t->kind) {
        case V4IR::Temp::Formal:
        case V4IR::Temp::ScopedFormal: return Param::createArgument(t->index, t->scope);
        case V4IR::Temp::Local: return Param::createLocal(t->index);
        case V4IR::Temp::ScopedLocal: return Param::createScopedLocal(t->index, t->scope);
        case V4IR::Temp::VirtualRegister:
            return Param::createTemp(_stackSlotAllocator ?
                        _stackSlotAllocator->stackSlotFor(t, _currentStatement) : t->index);
        default:
            Q_UNIMPLEMENTED();
            return Param();
        }
    } else {
        Q_UNIMPLEMENTED();
        return Param();
    }
}


CompilationUnit::~CompilationUnit()
{
    foreach (QV4::Function *f, runtimeFunctions)
        engine->allFunctions.remove(reinterpret_cast<quintptr>(f->codeData));
}

void CompilationUnit::linkBackendToEngine(QV4::ExecutionEngine *engine)
{
    runtimeFunctions.resize(data->functionTableSize);
    runtimeFunctions.fill(0);
    for (int i = 0 ;i < runtimeFunctions.size(); ++i) {
        const QV4::CompiledData::Function *compiledFunction = data->functionAt(i);

        QV4::Function *runtimeFunction = new QV4::Function(engine, this, compiledFunction,
                                                           &VME::exec, /*size - doesn't matter for moth*/0);
        runtimeFunction->codeData = reinterpret_cast<const uchar *>(codeRefs.at(i).constData());
        runtimeFunctions[i] = runtimeFunction;

        if (QV4::Debugging::Debugger *debugger = engine->debugger)
            debugger->setPendingBreakpoints(runtimeFunction);
    }

    foreach (QV4::Function *f, runtimeFunctions)
        engine->allFunctions.insert(reinterpret_cast<quintptr>(f->codeData), f);
}
