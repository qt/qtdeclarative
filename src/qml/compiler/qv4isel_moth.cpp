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
    StackSlotAllocator(const QList<V4IR::LifeTimeInterval> &ranges, int maxTempCount)
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
        int idx = _slotForTemp.value(*t, -1);
        if (idx == -1)
            idx = allocateSlot(t, currentStmt);
        Q_ASSERT(idx >= 0);
        return idx;
    }

private:
    int allocateSlot(V4IR::Temp *t, V4IR::Stmt *currentStmt) {
        const V4IR::LifeTimeInterval &interval = _intervals[*t];
        int idx = _hints.value(*t, -1);
        if (idx != -1 && _activeSlots[idx] <= currentStmt->id) {
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

InstructionSelection::InstructionSelection(QV4::ExecutionEngine *engine, V4IR::Module *module)
    : EvalInstructionSelection(engine, module)
    , _function(0)
    , _vmFunction(0)
    , _block(0)
    , _codeStart(0)
    , _codeNext(0)
    , _codeEnd(0)
    , _stackSlotAllocator(0)
    , _currentStatement(0)
{
}

InstructionSelection::~InstructionSelection()
{
}

void InstructionSelection::run(QV4::Function *vmFunction, V4IR::Function *function)
{
    V4IR::BasicBlock *block = 0, *nextBlock = 0;

    QHash<V4IR::BasicBlock *, QVector<ptrdiff_t> > patches;
    QHash<V4IR::BasicBlock *, ptrdiff_t> addrs;

    int codeSize = 4096;
    uchar *codeStart = new uchar[codeSize];
    memset(codeStart, 0, codeSize);
    uchar *codeNext = codeStart;
    uchar *codeEnd = codeStart + codeSize;

    qSwap(_function, function);
    qSwap(_vmFunction, vmFunction);
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
    if (opt.isInSSA())
        stackSlotAllocator = new StackSlotAllocator(opt.lifeRanges(), _function->tempCount);
    qSwap(_stackSlotAllocator, stackSlotAllocator);
    V4IR::Stmt *cs = 0;
    qSwap(_currentStatement, cs);

    int locals = frameSize();
    assert(locals >= 0);

    Instruction::Push push;
    push.value = quint32(locals);
    addInstruction(push);

    for (int i = 0, ei = _function->basicBlocks.size(); i != ei; ++i) {
        _block = _function->basicBlocks[i];
        _nextBlock = (i < ei - 1) ? _function->basicBlocks[i + 1] : 0;
        _addrs.insert(_block, _codeNext - _codeStart);

        foreach (V4IR::Stmt *s, _block->statements) {
            if (s->location.isValid()) {
                QV4::LineNumberMapping mapping;
                mapping.codeOffset = _codeNext - _codeStart;
                mapping.lineNumber = s->location.startLine;
                _vmFunction->lineNumberMappings.append(mapping);
            }

            if (opt.isInSSA() && s->asTerminator()) {
                foreach (const V4IR::Optimizer::SSADeconstructionMove &move,
                         opt.ssaDeconstructionMoves(_block)) {
                    Q_ASSERT(move.source->asTemp()); // FIXME: support Const exprs in Phi nodes.
                    if (move.needsConversion())
                        convertType(move.source->asTemp(), move.target);
                    else
                        copyValue(move.source->asTemp(), move.target);
                }
            }

            _currentStatement = s;
            s->accept(this);
        }
    }

    // TODO: patch stack size (the push instruction)
    patchJumpAddresses();

    _vmFunction->code = VME::exec;
    _vmFunction->codeData = squeezeCode();

    if (QV4::Debugging::Debugger *debugger = engine()->debugger)
        debugger->setPendingBreakpoints(_vmFunction);

    qSwap(_currentStatement, cs);
    qSwap(_stackSlotAllocator, stackSlotAllocator);
    delete stackSlotAllocator;
    qSwap(_function, function);
    qSwap(_vmFunction, vmFunction);
    qSwap(block, _block);
    qSwap(nextBlock, _nextBlock);
    qSwap(patches, _patches);
    qSwap(addrs, _addrs);
    qSwap(codeStart, _codeStart);
    qSwap(codeNext, _codeNext);
    qSwap(codeEnd, _codeEnd);

    delete[] codeStart;
}

void InstructionSelection::callValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result)
{
    Instruction::CallValue call;
    prepareCallArgs(args, call.argc, call.args);
    call.dest = getParam(value);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result)
{
    // call the property on the loaded base
    Instruction::CallProperty call;
    call.base = getParam(base);
    call.name = identifier(name);
    prepareCallArgs(args, call.argc, call.args);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::ExprList *args, V4IR::Temp *result)
{
    // call the property on the loaded base
    Instruction::CallElement call;
    call.base = getParam(base);
    call.index = getParam(index);
    prepareCallArgs(args, call.argc, call.args);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::convertType(V4IR::Temp *source, V4IR::Temp *target)
{
    if (_stackSlotAllocator)
        _stackSlotAllocator->addHint(*source, *target);

    // FIXME: do something more useful with this info
    copyValue(source, target);
}

void InstructionSelection::constructActivationProperty(V4IR::Name *func,
                                                       V4IR::ExprList *args,
                                                       V4IR::Temp *result)
{
    Instruction::CreateActivationProperty create;
    create.name = identifier(*func->id);
    prepareCallArgs(args, create.argc, create.args);
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::constructProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result)
{
    Instruction::CreateProperty create;
    create.base = getParam(base);
    create.name = identifier(name);
    prepareCallArgs(args, create.argc, create.args);
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::constructValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result)
{
    Instruction::CreateValue create;
    create.func = getParam(value);
    prepareCallArgs(args, create.argc, create.args);
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
    Instruction::LoadValue load;
    load.value = Param::createValue(QV4::Value::fromString(identifier(str)));
    load.result = getResultParam(targetTemp);
    addInstruction(load);
}

void InstructionSelection::loadRegexp(V4IR::RegExp *sourceRegexp, V4IR::Temp *targetTemp)
{
    QV4::Value v = QV4::Value::fromObject(engine()->newRegExpObject(
                                            *sourceRegexp->value,
                                            sourceRegexp->flags));
    _vmFunction->generatedValues.append(v);

    Instruction::LoadValue load;
    load.value = Param::createValue(v);
    load.result = getResultParam(targetTemp);
    addInstruction(load);
}

void InstructionSelection::getActivationProperty(const V4IR::Name *name, V4IR::Temp *temp)
{
    Instruction::LoadName load;
    load.name = identifier(*name->id);
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::setActivationProperty(V4IR::Temp *source, const QString &targetName)
{
    Instruction::StoreName store;
    store.source = getParam(source);
    store.name = identifier(targetName);
    addInstruction(store);
}

void InstructionSelection::initClosure(V4IR::Closure *closure, V4IR::Temp *target)
{
    QV4::Function *vmFunc = vmFunction(closure->value);
    assert(vmFunc);
    Instruction::LoadClosure load;
    load.value = vmFunc;
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::getProperty(V4IR::Temp *base, const QString &name, V4IR::Temp *target)
{
    Instruction::LoadProperty load;
    load.base = getParam(base);
    load.name = identifier(name);
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::setProperty(V4IR::Temp *source, V4IR::Temp *targetBase, const QString &targetName)
{
    Instruction::StoreProperty store;
    store.base = getParam(targetBase);
    store.name = identifier(targetName);
    store.source = getParam(source);
    addInstruction(store);
}

void InstructionSelection::getElement(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *target)
{
    Instruction::LoadElement load;
    load.base = getParam(base);
    load.index = getParam(index);
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::setElement(V4IR::Temp *source, V4IR::Temp *targetBase, V4IR::Temp *targetIndex)
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

void InstructionSelection::unop(V4IR::AluOp oper, V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp)
{
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
#ifdef USE_TYPE_INFO
    if (leftSource->type & V4IR::NumberType && rightSource->type & V4IR::NumberType) {
        // TODO: add Temp+Const variation on the topic.
        switch (oper) {
        case V4IR::OpAdd: {
            Instruction::AddNumberParams instr;
            instr.lhs = getParam(leftSource);
            instr.rhs = getParam(rightSource);
            instr.result = getResultParam(target);
            addInstruction(instr);
        } return instr.result;
        case V4IR::OpMul: {
            Instruction::MulNumberParams instr;
            instr.lhs = getParam(leftSource);
            instr.rhs = getParam(rightSource);
            instr.result = getResultParam(target);
            addInstruction(instr);
        } return instr.result;
        case V4IR::OpSub: {
            Instruction::SubNumberParams instr;
            instr.lhs = getParam(leftSource);
            instr.rhs = getParam(rightSource);
            instr.result = getResultParam(target);
            addInstruction(instr);
        } return instr.result;
        default:
            break;
        }
    }
#else // !USE_TYPE_INFO
    Q_ASSERT(leftSource->asTemp() && rightSource->asTemp());
#endif // USE_TYPE_INFO

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

void InstructionSelection::inplaceNameOp(V4IR::AluOp oper, V4IR::Temp *rightSource, const QString &targetName)
{
    QV4::InplaceBinOpName op = 0;
    switch (oper) {
    case V4IR::OpBitAnd: op = QV4::__qmljs_inplace_bit_and_name; break;
    case V4IR::OpBitOr: op = QV4::__qmljs_inplace_bit_or_name; break;
    case V4IR::OpBitXor: op = QV4::__qmljs_inplace_bit_xor_name; break;
    case V4IR::OpAdd: op = QV4::__qmljs_inplace_add_name; break;
    case V4IR::OpSub: op = QV4::__qmljs_inplace_sub_name; break;
    case V4IR::OpMul: op = QV4::__qmljs_inplace_mul_name; break;
    case V4IR::OpDiv: op = QV4::__qmljs_inplace_div_name; break;
    case V4IR::OpMod: op = QV4::__qmljs_inplace_mod_name; break;
    case V4IR::OpLShift: op = QV4::__qmljs_inplace_shl_name; break;
    case V4IR::OpRShift: op = QV4::__qmljs_inplace_shr_name; break;
    case V4IR::OpURShift: op = QV4::__qmljs_inplace_ushr_name; break;
    default: break;
    }

    if (op) {
        Instruction::InplaceNameOp ieo;
        ieo.alu = op;
        ieo.name = identifier(targetName);
        ieo.source = getParam(rightSource);
        addInstruction(ieo);
    }
}

void InstructionSelection::inplaceElementOp(V4IR::AluOp oper, V4IR::Temp *source, V4IR::Temp *targetBaseTemp, V4IR::Temp *targetIndexTemp)
{
    QV4::InplaceBinOpElement op = 0;
    switch (oper) {
    case V4IR::OpBitAnd: op = QV4::__qmljs_inplace_bit_and_element; break;
    case V4IR::OpBitOr: op = QV4::__qmljs_inplace_bit_or_element; break;
    case V4IR::OpBitXor: op = QV4::__qmljs_inplace_bit_xor_element; break;
    case V4IR::OpAdd: op = QV4::__qmljs_inplace_add_element; break;
    case V4IR::OpSub: op = QV4::__qmljs_inplace_sub_element; break;
    case V4IR::OpMul: op = QV4::__qmljs_inplace_mul_element; break;
    case V4IR::OpDiv: op = QV4::__qmljs_inplace_div_element; break;
    case V4IR::OpMod: op = QV4::__qmljs_inplace_mod_element; break;
    case V4IR::OpLShift: op = QV4::__qmljs_inplace_shl_element; break;
    case V4IR::OpRShift: op = QV4::__qmljs_inplace_shr_element; break;
    case V4IR::OpURShift: op = QV4::__qmljs_inplace_ushr_element; break;
    default: break;
    }

    Instruction::InplaceElementOp ieo;
    ieo.alu = op;
    ieo.base = getParam(targetBaseTemp);
    ieo.index = getParam(targetIndexTemp);
    ieo.source = getParam(source);
    addInstruction(ieo);
}

void InstructionSelection::inplaceMemberOp(V4IR::AluOp oper, V4IR::Temp *source, V4IR::Temp *targetBase, const QString &targetName)
{
    QV4::InplaceBinOpMember op = 0;
    switch (oper) {
    case V4IR::OpBitAnd: op = QV4::__qmljs_inplace_bit_and_member; break;
    case V4IR::OpBitOr: op = QV4::__qmljs_inplace_bit_or_member; break;
    case V4IR::OpBitXor: op = QV4::__qmljs_inplace_bit_xor_member; break;
    case V4IR::OpAdd: op = QV4::__qmljs_inplace_add_member; break;
    case V4IR::OpSub: op = QV4::__qmljs_inplace_sub_member; break;
    case V4IR::OpMul: op = QV4::__qmljs_inplace_mul_member; break;
    case V4IR::OpDiv: op = QV4::__qmljs_inplace_div_member; break;
    case V4IR::OpMod: op = QV4::__qmljs_inplace_mod_member; break;
    case V4IR::OpLShift: op = QV4::__qmljs_inplace_shl_member; break;
    case V4IR::OpRShift: op = QV4::__qmljs_inplace_shr_member; break;
    case V4IR::OpURShift: op = QV4::__qmljs_inplace_ushr_member; break;
    default: break;
    }

    Instruction::InplaceMemberOp imo;
    imo.alu = op;
    imo.base = getParam(targetBase);
    imo.member = identifier(targetName);
    imo.source = getParam(source);
    addInstruction(imo);
}

void InstructionSelection::prepareCallArgs(V4IR::ExprList *e, quint32 &argc, quint32 &args)
{
    bool singleArgIsTemp = false;
    if (e && e->next == 0 && e->expr->asTemp()) {
        singleArgIsTemp = e->expr->asTemp()->kind == V4IR::Temp::VirtualRegister;
    }

    if (singleArgIsTemp) {
        // We pass single arguments as references to the stack, but only if it's not a local or an argument.
        argc = 1;
        args = getParam(e->expr).index;
    } else if (e) {
        // We need to move all the temps into the function arg array
        int argLocation = outgoingArgumentTempStart();
        assert(argLocation >= 0);
        argc = 0;
        args = argLocation;
        while (e) {
            Instruction::MoveTemp move;
            move.source = getParam(e->expr);
            move.result = Param::createTemp(argLocation);
            addInstruction(move);
            ++argLocation;
            ++argc;
            e = e->next;
        }
    } else {
        argc = 0;
        args = 0;
    }
}

void InstructionSelection::visitJump(V4IR::Jump *s)
{
    if (s->target == _nextBlock)
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
    ptrdiff_t trueLoc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));
    _patches[s->iftrue].append(trueLoc);

    if (s->iffalse != _nextBlock) {
        Instruction::Jump jump;
        jump.offset = 0;
        ptrdiff_t falseLoc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));
        _patches[s->iffalse].append(falseLoc);
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
    enterTry.exceptionVarName = identifier(*t->exceptionVarName);
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
    call.name = identifier(*func->id);
    prepareCallArgs(args, call.argc, call.args);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result)
{
    Instruction::CallBuiltinTypeofMember call;
    call.base = getParam(base);
    call.member = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result)
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
    call.name = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofValue(V4IR::Temp *value, V4IR::Temp *result)
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
    call.member = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result)
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
    call.name = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteValue(V4IR::Temp *result)
{
    Instruction::LoadValue load;
    load.value = Param::createValue(QV4::Value::fromBoolean(false));
    load.result = getResultParam(result);
    addInstruction(load);
}

void InstructionSelection::callBuiltinPostDecrementMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result)
{
    Instruction::CallBuiltinPostDecMember call;
    call.base = getParam(base);
    call.member = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostDecrementSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result)
{
    Instruction::CallBuiltinPostDecSubscript call;
    call.base = getParam(base);
    call.index = getParam(index);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostDecrementName(const QString &name, V4IR::Temp *result)
{
    Instruction::CallBuiltinPostDecName call;
    call.name = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostDecrementValue(V4IR::Temp *value, V4IR::Temp *result)
{
    Instruction::CallBuiltinPostDecValue call;
    call.value = getParam(value);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostIncrementMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result)
{
    Instruction::CallBuiltinPostIncMember call;
    call.base = getParam(base);
    call.member = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostIncrementSubscript(V4IR::Temp *base, V4IR::Temp *index, V4IR::Temp *result)
{
    Instruction::CallBuiltinPostIncSubscript call;
    call.base = getParam(base);
    call.index = getParam(index);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostIncrementName(const QString &name, V4IR::Temp *result)
{
    Instruction::CallBuiltinPostIncName call;
    call.name = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostIncrementValue(V4IR::Temp *value, V4IR::Temp *result)
{
    Instruction::CallBuiltinPostIncValue call;
    call.value = getParam(value);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinThrow(V4IR::Temp *arg)
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
    call.varName = identifier(name);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineGetterSetter(V4IR::Temp *object, const QString &name, V4IR::Temp *getter, V4IR::Temp *setter)
{
    Instruction::CallBuiltinDefineGetterSetter call;
    call.object = getParam(object);
    call.name = identifier(name);
    call.getter = getParam(getter);
    call.setter = getParam(setter);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineProperty(V4IR::Temp *object, const QString &name, V4IR::Temp *value)
{
    Instruction::CallBuiltinDefineProperty call;
    call.object = getParam(object);
    call.name = identifier(name);
    call.value = getParam(value);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineArray(V4IR::Temp *result, V4IR::ExprList *args)
{
    Instruction::CallBuiltinDefineArray call;
    prepareCallArgs(args, call.argc, call.args);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineObjectLiteral(V4IR::Temp *result, V4IR::ExprList *args)
{
    int argLocation = outgoingArgumentTempStart();

    QV4::InternalClass *klass = engine()->emptyClass;
    V4IR::ExprList *it = args;
    while (it) {
        V4IR::Name *name = it->expr->asName();
        it = it->next;

        bool isData = it->expr->asConst()->value;
        it = it->next;
        klass = klass->addMember(identifier(*name->id), isData ? QV4::Attr_Data : QV4::Attr_Accessor);

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
    call.internalClass = klass;
    call.args = outgoingArgumentTempStart();
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

uchar *InstructionSelection::squeezeCode() const
{
    int codeSize = _codeNext - _codeStart;
    uchar *squeezed = new uchar[codeSize];
    ::memcpy(squeezed, _codeStart, codeSize);
    return squeezed;
}

QV4::String *InstructionSelection::identifier(const QString &s)
{
    QV4::String *str = engine()->newIdentifier(s);
    _vmFunction->identifiers.append(str);
    return str;
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
