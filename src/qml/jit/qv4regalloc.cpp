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

#include "qv4regalloc_p.h"
#include <private/qv4value_inl_p.h>

#include <algorithm>

//#define DEBUG_REGALLOC

namespace {
struct Use {
    enum RegisterFlag { MustHaveRegister = 0, CouldHaveRegister = 1 };
    unsigned flag : 1;
    unsigned pos  : 31;

    Use(): flag(MustHaveRegister), pos(0) {}
    Use(int pos, RegisterFlag flag): flag(flag), pos(pos) {}

    bool mustHaveRegister() const { return flag == MustHaveRegister; }
};
}

QT_BEGIN_NAMESPACE

Q_DECLARE_TYPEINFO(Use, Q_MOVABLE_TYPE);

using namespace QV4::IR;

namespace QV4 {
namespace JIT {

class RegAllocInfo: public IRDecoder
{
    struct Def {
        unsigned defStmt : 30;
        unsigned canHaveReg : 1;
        unsigned isPhiTarget : 1;

        Def(): defStmt(0), canHaveReg(0), isPhiTarget(0) {}
        Def(int defStmt, bool canHaveReg, bool isPhiTarget)
            : defStmt(defStmt), canHaveReg(canHaveReg), isPhiTarget(isPhiTarget)
        {
            Q_ASSERT(defStmt > 0);
            Q_ASSERT(defStmt < (1 << 30));
        }

        bool isValid() const { return defStmt != 0; } // 0 is invalid, as stmt numbers start at 1.
    };

    Stmt *_currentStmt;
    QHash<Temp, Def> _defs;
    QHash<Temp, QList<Use> > _uses;
    QList<int> _calls;
    QHash<Temp, QList<Temp> > _hints;

public:
    RegAllocInfo(): _currentStmt(0) {}

    void collect(IR::Function *function)
    {
        foreach (BasicBlock *bb, function->basicBlocks()) {
            foreach (Stmt *s, bb->statements()) {
                Q_ASSERT(s->id > 0);
                _currentStmt = s;
                s->accept(this);
            }
        }
    }

    QList<Use> uses(const Temp &t) const { return _uses[t]; }
    bool useMustHaveReg(const Temp &t, int position) {
        foreach (const Use &use, uses(t))
            if (use.pos == position)
                return use.mustHaveRegister();
        return false;
    }

    bool isUsedAt(const Temp &t, int position) {
        foreach (const Use &use, uses(t))
            if (use.pos == position)
                return true;
        return false;
    }

    int def(const Temp &t) const {
        Q_ASSERT(_defs[t].isValid());
        return _defs[t].defStmt;
    }
    bool canHaveRegister(const Temp &t) const {
        Q_ASSERT(_defs[t].isValid());
        return _defs[t].canHaveReg;
    }
    bool isPhiTarget(const Temp &t) const {
        Q_ASSERT(_defs[t].isValid());
        return _defs[t].isPhiTarget;
    }

    const QList<int> &calls() const { return _calls; }
    QList<Temp> hints(const Temp &t) const { return _hints[t]; }
    void addHint(const Temp &t, int physicalRegister)
    {
        Temp hint;
        hint.init(Temp::PhysicalRegister, physicalRegister, 0);
        _hints[t].append(hint);
    }

#ifdef DEBUG_REGALLOC
    void dump() const
    {
        QTextStream qout(stdout, QIODevice::WriteOnly);

        qout << "RegAllocInfo:" << endl << "Defs/uses:" << endl;
        QList<Temp> temps = _defs.keys();
        std::sort(temps.begin(), temps.end());
        foreach (const Temp &t, temps) {
            t.dump(qout);
            qout << " def at " << _defs[t].defStmt << " ("
                 << (_defs[t].canHaveReg ? "can" : "can NOT")
                 << " have a register, and "
                 << (isPhiTarget(t) ? "is" : "is NOT")
                 << " defined by a phi node), uses at: ";
            const QList<Use> &uses = _uses[t];
            for (int i = 0; i < uses.size(); ++i) {
                if (i > 0) qout << ", ";
                qout << uses[i].pos;
                if (uses[i].mustHaveRegister()) qout << "(R)"; else qout << "(S)";
            }
            qout << endl;
        }

        qout << "Calls at: ";
        for (int i = 0; i < _calls.size(); ++i) {
            if (i > 0) qout << ", ";
            qout << _calls[i];
        }
        qout << endl;

        qout << "Hints:" << endl;
        QList<Temp> hinted = _hints.keys();
        if (hinted.isEmpty())
            qout << "\t(none)" << endl;
        std::sort(hinted.begin(), hinted.end());
        foreach (const Temp &t, hinted) {
            qout << "\t";
            t.dump(qout);
            qout << ": ";
            QList<Temp> hints = _hints[t];
            for (int i = 0; i < hints.size(); ++i) {
                if (i > 0) qout << ", ";
                hints[i].dump(qout);
            }
            qout << endl;
        }
    }
#endif // DEBUG_REGALLOC

protected: // IRDecoder
    virtual void callBuiltinInvalid(IR::Name *, IR::ExprList *, IR::Temp *) {}
    virtual void callBuiltinTypeofMember(IR::Expr *, const QString &, IR::Temp *) {}
    virtual void callBuiltinTypeofSubscript(IR::Expr *, IR::Expr *, IR::Temp *) {}
    virtual void callBuiltinTypeofName(const QString &, IR::Temp *) {}
    virtual void callBuiltinTypeofValue(IR::Expr *, IR::Temp *) {}
    virtual void callBuiltinDeleteMember(IR::Temp *, const QString &, IR::Temp *) {}
    virtual void callBuiltinDeleteSubscript(IR::Temp *, IR::Expr *, IR::Temp *) {}
    virtual void callBuiltinDeleteName(const QString &, IR::Temp *) {}
    virtual void callBuiltinDeleteValue(IR::Temp *) {}
    virtual void callBuiltinThrow(IR::Expr *) {}
    virtual void callBuiltinReThrow() {}
    virtual void callBuiltinUnwindException(IR::Temp *) {}
    virtual void callBuiltinPushCatchScope(const QString &) {};
    virtual void callBuiltinForeachIteratorObject(IR::Expr *, IR::Temp *) {}
    virtual void callBuiltinForeachNextProperty(IR::Temp *, IR::Temp *) {}
    virtual void callBuiltinForeachNextPropertyname(IR::Temp *, IR::Temp *) {}
    virtual void callBuiltinPushWithScope(IR::Temp *) {}
    virtual void callBuiltinPopScope() {}
    virtual void callBuiltinDeclareVar(bool , const QString &) {}
    virtual void callBuiltinDefineArray(IR::Temp *, IR::ExprList *) {}
    virtual void callBuiltinDefineObjectLiteral(IR::Temp *, int, IR::ExprList *, IR::ExprList *, bool) {}
    virtual void callBuiltinSetupArgumentObject(IR::Temp *) {}
    virtual void callBuiltinConvertThisToObject() {}

    virtual void callValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result)
    {
        addDef(result);
        addUses(value, Use::CouldHaveRegister);
        addUses(args, Use::CouldHaveRegister);
        addCall();
    }

    virtual void callProperty(IR::Expr *base, const QString &name, IR::ExprList *args,
                              IR::Temp *result)
    {
        Q_UNUSED(name)

        addDef(result);
        addUses(base->asTemp(), Use::CouldHaveRegister);
        addUses(args, Use::CouldHaveRegister);
        addCall();
    }

    virtual void callSubscript(IR::Expr *base, IR::Expr *index, IR::ExprList *args,
                               IR::Temp *result)
    {
        addDef(result);
        addUses(base->asTemp(), Use::CouldHaveRegister);
        addUses(index->asTemp(), Use::CouldHaveRegister);
        addUses(args, Use::CouldHaveRegister);
        addCall();
    }

    virtual void convertType(IR::Temp *source, IR::Temp *target)
    {
        addDef(target);

        bool needsCall = true;
        Use::RegisterFlag sourceReg = Use::CouldHaveRegister;

        switch (target->type) {
        case DoubleType:
            switch (source->type) {
            case UInt32Type:
            case SInt32Type:
            case NullType:
            case UndefinedType:
            case BoolType:
                needsCall = false;
                break;
            default:
                break;
            }
            break;
        case BoolType:
            switch (source->type) {
            case UInt32Type:
                sourceReg = Use::MustHaveRegister;
                needsCall = false;
                break;
            case DoubleType:
            case UndefinedType:
            case NullType:
            case SInt32Type:
                needsCall = false;
                break;
            default:
                break;
            }
            break;
        case SInt32Type:
            switch (source->type) {
            case UInt32Type:
            case NullType:
            case UndefinedType:
            case BoolType:
                needsCall = false;
            default:
                break;
            }
            break;
        case UInt32Type:
            switch (source->type) {
            case SInt32Type:
            case NullType:
            case UndefinedType:
            case BoolType:
                needsCall = false;
            default:
                break;
            }
            break;
        default:
            break;
        }

        addUses(source, sourceReg);

        if (needsCall)
            addCall();
        else
            addHint(target, source);
    }

    virtual void constructActivationProperty(IR::Name *, IR::ExprList *args, IR::Temp *result)
    {
        addDef(result);
        addUses(args, Use::CouldHaveRegister);
        addCall();
    }

    virtual void constructProperty(IR::Temp *base, const QString &, IR::ExprList *args, IR::Temp *result)
    {
        addDef(result);
        addUses(base, Use::CouldHaveRegister);
        addUses(args, Use::CouldHaveRegister);
        addCall();
    }

    virtual void constructValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result)
    {
        addDef(result);
        addUses(value, Use::CouldHaveRegister);
        addUses(args, Use::CouldHaveRegister);
        addCall();
    }

    virtual void loadThisObject(IR::Temp *temp)
    {
        addDef(temp);
    }

    virtual void loadQmlIdArray(IR::Temp *temp)
    {
        addDef(temp);
        addCall();
    }

    virtual void loadQmlImportedScripts(IR::Temp *temp)
    {
        addDef(temp);
        addCall();
    }

    virtual void loadQmlContextObject(Temp *temp)
    {
        addDef(temp);
        addCall();
    }

    virtual void loadQmlScopeObject(Temp *temp)
    {
        Q_UNUSED(temp);

        addDef(temp);
        addCall();
    }

    virtual void loadQmlSingleton(const QString &/*name*/, Temp *temp)
    {
        Q_UNUSED(temp);

        addDef(temp);
        addCall();
    }

    virtual void loadConst(IR::Const *sourceConst, IR::Temp *targetTemp)
    {
        Q_UNUSED(sourceConst);

        addDef(targetTemp);
    }

    virtual void loadString(const QString &str, IR::Temp *targetTemp)
    {
        Q_UNUSED(str);

        addDef(targetTemp);
    }

    virtual void loadRegexp(IR::RegExp *sourceRegexp, IR::Temp *targetTemp)
    {
        Q_UNUSED(sourceRegexp);

        addDef(targetTemp);
        addCall();
    }

    virtual void getActivationProperty(const IR::Name *, IR::Temp *temp)
    {
        addDef(temp);
        addCall();
    }

    virtual void setActivationProperty(IR::Expr *source, const QString &)
    {
        addUses(source->asTemp(), Use::CouldHaveRegister);
        addCall();
    }

    virtual void initClosure(IR::Closure *closure, IR::Temp *target)
    {
        Q_UNUSED(closure);

        addDef(target);
        addCall();
    }

    virtual void getProperty(IR::Expr *base, const QString &, IR::Temp *target)
    {
        addDef(target);
        addUses(base->asTemp(), Use::CouldHaveRegister);
        addCall();
    }

    virtual void setProperty(IR::Expr *source, IR::Expr *targetBase, const QString &)
    {
        addUses(source->asTemp(), Use::CouldHaveRegister);
        addUses(targetBase->asTemp(), Use::CouldHaveRegister);
        addCall();
    }

    virtual void setQObjectProperty(IR::Expr *source, IR::Expr *targetBase, int /*propertyIndex*/)
    {
        addUses(source->asTemp(), Use::CouldHaveRegister);
        addUses(targetBase->asTemp(), Use::CouldHaveRegister);
        addCall();
    }

    virtual void getQObjectProperty(IR::Expr *base, int /*propertyIndex*/, bool /*captureRequired*/, int /*attachedPropertiesId*/, IR::Temp *target)
    {
        addDef(target);
        addUses(base->asTemp(), Use::CouldHaveRegister);
        addCall();
    }

    virtual void getElement(IR::Expr *base, IR::Expr *index, IR::Temp *target)
    {
        addDef(target);
        addUses(base->asTemp(), Use::CouldHaveRegister);
        addUses(index->asTemp(), Use::CouldHaveRegister);
        addCall();
    }

    virtual void setElement(IR::Expr *source, IR::Expr *targetBase, IR::Expr *targetIndex)
    {
        addUses(source->asTemp(), Use::CouldHaveRegister);
        addUses(targetBase->asTemp(), Use::CouldHaveRegister);
        addUses(targetIndex->asTemp(), Use::CouldHaveRegister);
        addCall();
    }

    virtual void copyValue(IR::Temp *sourceTemp, IR::Temp *targetTemp)
    {
        addDef(targetTemp);
        addUses(sourceTemp, Use::CouldHaveRegister);
        addHint(targetTemp, sourceTemp);
    }

    virtual void swapValues(IR::Temp *, IR::Temp *)
    {
        // Inserted by the register allocator, so it cannot occur here.
        Q_UNREACHABLE();
    }

    virtual void unop(AluOp oper, Temp *sourceTemp, Temp *targetTemp)
    {
        addDef(targetTemp);

        bool needsCall = true;
        if (oper == OpNot && sourceTemp->type == IR::BoolType && targetTemp->type == IR::BoolType)
            needsCall = false;

#if 0 // TODO: change masm to generate code
        switch (oper) {
        case OpIfTrue:
        case OpNot:
        case OpUMinus:
        case OpUPlus:
        case OpCompl:
            needsCall = sourceTemp->type & ~NumberType && sourceTemp->type != BoolType;
            break;

        case OpIncrement:
        case OpDecrement:
        default:
            Q_UNREACHABLE();
        }
#endif

        if (needsCall) {
            addUses(sourceTemp, Use::CouldHaveRegister);
            addCall();
        } else {
            addUses(sourceTemp, Use::MustHaveRegister);
        }
    }

    virtual void binop(AluOp oper, Expr *leftSource, Expr *rightSource, Temp *target)
    {
        bool needsCall = true;

        if (oper == OpStrictEqual || oper == OpStrictNotEqual) {
            bool noCall = leftSource->type == NullType || rightSource->type == NullType
                    || leftSource->type == UndefinedType || rightSource->type == UndefinedType
                    || leftSource->type == BoolType || rightSource->type == BoolType;
            needsCall = !noCall;
        } else if (leftSource->type == DoubleType && rightSource->type == DoubleType) {
            if (oper == OpMul || oper == OpAdd || oper == OpDiv || oper == OpSub
                    || (oper >= OpGt && oper <= OpStrictNotEqual)) {
                needsCall = false;
            }
        } else if (oper == OpBitAnd || oper == OpBitOr || oper == OpBitXor || oper == OpLShift || oper == OpRShift || oper == OpURShift) {
            needsCall = false;
        } else if (oper == OpAdd
                   || oper == OpMul
                   ||
                   oper == OpSub
                   ) {
            if (leftSource->type == SInt32Type && rightSource->type == SInt32Type)
                needsCall = false;
        }

        addDef(target);

        if (needsCall) {
            addUses(leftSource->asTemp(), Use::CouldHaveRegister);
            addUses(rightSource->asTemp(), Use::CouldHaveRegister);
            addCall();
        } else {
            addUses(leftSource->asTemp(), Use::MustHaveRegister);
            addHint(target, leftSource->asTemp());

            addUses(rightSource->asTemp(), Use::MustHaveRegister);
            switch (oper) {
            case OpAdd:
            case OpMul:
                addHint(target, rightSource->asTemp());
                break;
            default:
                break;
            }
        }
    }

    virtual void visitJump(IR::Jump *) {}
    virtual void visitCJump(IR::CJump *s)
    {
        if (Temp *t = s->cond->asTemp()) {
#if 0 // TODO: change masm to generate code
            addUses(t, Use::MustHaveRegister);
#else
            addUses(t, Use::CouldHaveRegister);
            addCall();
#endif
        } else if (Binop *b = s->cond->asBinop()) {
            binop(b->op, b->left, b->right, 0);
        } else if (s->cond->asConst()) {
            // TODO: SSA optimization for constant condition evaluation should remove this.
            // See also visitCJump() in masm.
            addCall();
        } else {
            Q_UNREACHABLE();
        }
    }

    virtual void visitRet(IR::Ret *s)
    { addUses(s->expr->asTemp(), Use::CouldHaveRegister); }

    virtual void visitPhi(IR::Phi *s)
    {
        addDef(s->targetTemp, true);
        foreach (Expr *e, s->d->incoming) {
            if (Temp *t = e->asTemp()) {
                addUses(t, Use::CouldHaveRegister);
                addHint(s->targetTemp, t);
            }
        }
    }

protected:
    virtual void callBuiltin(IR::Call *c, IR::Temp *result)
    {
        addDef(result);
        addUses(c->base->asTemp(), Use::CouldHaveRegister);
        addUses(c->args, Use::CouldHaveRegister);
        addCall();
    }

private:
    void addDef(Temp *t, bool isPhiTarget = false)
    {
        if (!t || t->kind != Temp::VirtualRegister)
            return;
        Q_ASSERT(!_defs.contains(*t));
        bool canHaveReg = true;
        switch (t->type) {
        case QObjectType:
        case VarType:
        case StringType:
        case UndefinedType:
        case NullType:
            canHaveReg = false;
            break;
        default:
            break;
        }

        _defs[*t] = Def(_currentStmt->id, canHaveReg, isPhiTarget);
    }

    void addUses(Temp *t, Use::RegisterFlag flag)
    {
        Q_ASSERT(_currentStmt->id > 0);
        if (t && t->kind == Temp::VirtualRegister)
            _uses[*t].append(Use(_currentStmt->id, flag));
    }

    void addUses(ExprList *l, Use::RegisterFlag flag)
    {
        for (ExprList *it = l; it; it = it->next)
            addUses(it->expr->asTemp(), flag);
    }

    void addCall()
    {
        _calls.append(_currentStmt->id);
    }

    void addHint(Temp *hinted, Temp *hint1, Temp *hint2 = 0)
    {
        if (!hinted || hinted->kind != Temp::VirtualRegister)
            return;
        if (hint1 && hint1->kind == Temp::VirtualRegister && hinted->type == hint1->type)
            _hints[*hinted].append(*hint1);
        if (hint2 && hint2->kind == Temp::VirtualRegister && hinted->type == hint2->type)
            _hints[*hinted].append(*hint2);
    }
};

} // JIT namespace
} // QV4 namespace
QT_END_NAMESPACE

QT_USE_NAMESPACE

using namespace QT_PREPEND_NAMESPACE(QV4::JIT);
using namespace QT_PREPEND_NAMESPACE(QV4::IR);
using namespace QT_PREPEND_NAMESPACE(QV4);

namespace {
class ResolutionPhase: protected StmtVisitor, protected ExprVisitor {
    const QVector<LifeTimeInterval> &_intervals;
    QVector<const LifeTimeInterval *> _unprocessed;
    IR::Function *_function;
#if !defined(QT_NO_DEBUG)
    RegAllocInfo *_info;
#endif
    const QHash<IR::Temp, int> &_assignedSpillSlots;
    QHash<IR::Temp, const LifeTimeInterval *> _intervalForTemp;
    const QVector<int> &_intRegs;
    const QVector<int> &_fpRegs;

    Stmt *_currentStmt;
    QVector<Move *> _loads;
    QVector<Move *> _stores;

    QHash<BasicBlock *, QList<const LifeTimeInterval *> > _liveAtStart;
    QHash<BasicBlock *, QList<const LifeTimeInterval *> > _liveAtEnd;

public:
    ResolutionPhase(const QVector<LifeTimeInterval> &intervals, IR::Function *function, RegAllocInfo *info,
                    const QHash<IR::Temp, int> &assignedSpillSlots,
                    const QVector<int> &intRegs, const QVector<int> &fpRegs)
        : _intervals(intervals)
        , _function(function)
#if !defined(QT_NO_DEBUG)
        , _info(info)
#endif
        , _assignedSpillSlots(assignedSpillSlots)
        , _intRegs(intRegs)
        , _fpRegs(fpRegs)
    {
#if defined(QT_NO_DEBUG)
        Q_UNUSED(info)
#endif

        _unprocessed.resize(_intervals.size());
        for (int i = 0, ei = _intervals.size(); i != ei; ++i)
            _unprocessed[i] = &_intervals[i];

        _liveAtStart.reserve(function->basicBlockCount());
        _liveAtEnd.reserve(function->basicBlockCount());
    }

    void run() {
        renumber();
        Optimizer::showMeTheCode(_function);
        resolve();
    }

private:
    void renumber()
    {
        foreach (BasicBlock *bb, _function->basicBlocks()) {
            QVector<Stmt *> statements = bb->statements();
            QVector<Stmt *> newStatements;
            newStatements.reserve(bb->statements().size() + 7);

            bool seenFirstNonPhiStmt = false;
            for (int i = 0, ei = statements.size(); i != ei; ++i) {
                _currentStmt = statements[i];
                _loads.clear();
                _stores.clear();
                addNewIntervals();
                if (!seenFirstNonPhiStmt && !_currentStmt->asPhi()) {
                    seenFirstNonPhiStmt = true;
                    _liveAtStart[bb] = _intervalForTemp.values();
                }
                _currentStmt->accept(this);
                foreach (Move *load, _loads)
                    newStatements.append(load);
                if (_currentStmt->asPhi())
                    newStatements.prepend(_currentStmt);
                else
                    newStatements.append(_currentStmt);
                foreach (Move *store, _stores)
                    newStatements.append(store);
            }

            cleanOldIntervals();
            _liveAtEnd[bb] = _intervalForTemp.values();

#ifdef DEBUG_REGALLOC
            QTextStream os(stdout, QIODevice::WriteOnly);
            os << "Intervals live at the start of L" << bb->index << ":" << endl;
            if (_liveAtStart[bb].isEmpty())
                os << "\t(none)" << endl;
            foreach (const LifeTimeInterval *i, _liveAtStart[bb]) {
                os << "\t";
                i->dump(os);
                os << endl;
            }
            os << "Intervals live at the end of L" << bb->index << ":" << endl;
            if (_liveAtEnd[bb].isEmpty())
                os << "\t(none)" << endl;
            foreach (const LifeTimeInterval *i, _liveAtEnd[bb]) {
                os << "\t";
                i->dump(os);
                os << endl;
            }
#endif

            bb->setStatements(newStatements);
        }

    }

    void activate(const LifeTimeInterval *i)
    {
        Q_ASSERT(!i->isFixedInterval());
        _intervalForTemp[i->temp()] = i;

        if (i->reg() != LifeTimeInterval::Invalid) {
            // check if we need to generate spill/unspill instructions
            if (i->start() == _currentStmt->id) {
                if (i->isSplitFromInterval()) {
                    int pReg = platformRegister(*i);
                    _loads.append(generateUnspill(i->temp(), pReg));
                } else {
                    int pReg = platformRegister(*i);
                    int spillSlot = _assignedSpillSlots.value(i->temp(), -1);
                    if (spillSlot != -1)
                        _stores.append(generateSpill(spillSlot, i->temp().type, pReg));
                }
            }
        }
    }

    void addNewIntervals()
    {
        if (Phi *phi = _currentStmt->asPhi()) {
            // for phi nodes, only activate the range belonging to that node
            for (int it = 0, eit = _unprocessed.size(); it != eit; ++it) {
                const LifeTimeInterval *i = _unprocessed.at(it);
                if (i->start() > _currentStmt->id)
                    break;
                if (i->temp() == *phi->targetTemp) {
                    activate(i);
                    _unprocessed.remove(it);
                    break;
                }
            }
            return;
        }

        while (!_unprocessed.isEmpty()) {
            const LifeTimeInterval *i = _unprocessed.first();
            if (i->start() > _currentStmt->id)
                break;

            activate(i);

            _unprocessed.removeFirst();
        }
    }

    void cleanOldIntervals()
    {
        const int id = _currentStmt->id;
        QMutableHashIterator<Temp, const LifeTimeInterval *> it(_intervalForTemp);
        while (it.hasNext()) {
            const LifeTimeInterval *i = it.next().value();
            if (i->end() < id || i->isFixedInterval())
                it.remove();
        }
    }

    void resolve()
    {
        foreach (BasicBlock *bb, _function->basicBlocks()) {
            foreach (BasicBlock *bbOut, bb->out)
                resolveEdge(bb, bbOut);
        }
    }

    void resolveEdge(BasicBlock *predecessor, BasicBlock *successor)
    {
#ifdef DEBUG_REGALLOC
        Optimizer::showMeTheCode(_function);
        qDebug() << "Resolving edge" << predecessor->index << "->" << successor->index;
#endif // DEBUG_REGALLOC

        MoveMapping mapping;

        const int predecessorEnd = predecessor->terminator()->id; // the terminator is always last and always has an id set...
        Q_ASSERT(predecessorEnd > 0); // ... but we verify it anyway for good measure.

        int successorStart = -1;
        foreach (Stmt *s, successor->statements()) {
            if (s && s->id > 0) {
                successorStart = s->id;
                break;
            }
        }

        Q_ASSERT(successorStart > 0);

        foreach (const LifeTimeInterval *it, _liveAtStart[successor]) {
            if (it->end() < successorStart)
                continue;

            bool isPhiTarget = false;
            Expr *moveFrom = 0;

            if (it->start() == successorStart) {
                foreach (Stmt *s, successor->statements()) {
                    if (!s || s->id < 1)
                        continue;
                    if (Phi *phi = s->asPhi()) {
                        if (*phi->targetTemp == it->temp()) {
                            isPhiTarget = true;
                            Expr *opd = phi->d->incoming[successor->in.indexOf(predecessor)];
                            if (opd->asConst()) {
                                moveFrom = opd;
                            } else {
                                Temp *t = opd->asTemp();
                                Q_ASSERT(t);

                                foreach (const LifeTimeInterval *it2, _liveAtEnd[predecessor]) {
                                    if (it2->temp() == *t
                                            && it2->reg() != LifeTimeInterval::Invalid
                                            && it2->covers(predecessorEnd)) {
                                        moveFrom = createTemp(Temp::PhysicalRegister,
                                                              platformRegister(*it2), t->type);
                                        break;
                                    }
                                }
                                if (!moveFrom)
                                    moveFrom = createTemp(Temp::StackSlot,
                                                          _assignedSpillSlots.value(*t, -1),
                                                          t->type);
                            }
                        }
                    } else {
                        break;
                    }
                }
            } else {
                foreach (const LifeTimeInterval *predIt, _liveAtEnd[predecessor]) {
                    if (predIt->temp() == it->temp()) {
                        if (predIt->reg() != LifeTimeInterval::Invalid
                                && predIt->covers(predecessorEnd)) {
                            moveFrom = createTemp(Temp::PhysicalRegister, platformRegister(*predIt),
                                                  predIt->temp().type);
                        } else {
                            int spillSlot = _assignedSpillSlots.value(predIt->temp(), -1);
                            if (spillSlot != -1)
                                moveFrom = createTemp(Temp::StackSlot, spillSlot, predIt->temp().type);
                        }
                        break;
                    }
                }
            }
            if (!moveFrom) {
#if !defined(QT_NO_DEBUG)
                bool lifeTimeHole = false;
                if (it->ranges().first().start <= successorStart && it->ranges().last().end >= successorStart)
                    lifeTimeHole = !it->covers(successorStart);

                Q_ASSERT(!_info->isPhiTarget(it->temp()) || it->isSplitFromInterval() || lifeTimeHole);
                if (_info->def(it->temp()) != successorStart && !it->isSplitFromInterval()) {
                    const int successorEnd = successor->terminator()->id;
                    const int idx = successor->in.indexOf(predecessor);
                    foreach (const Use &use, _info->uses(it->temp())) {
                        if (use.pos == static_cast<unsigned>(successorStart)) {
                            // only check the current edge, not all other possible ones. This is
                            // important for phi nodes: they have uses that are only valid when
                            // coming in over a specific edge.
                            foreach (Stmt *s, successor->statements()) {
                                if (Phi *phi = s->asPhi()) {
                                    Q_ASSERT(it->temp().index != phi->targetTemp->index);
                                    Q_ASSERT(phi->d->incoming[idx]->asTemp() == 0
                                             || it->temp().index != phi->d->incoming[idx]->asTemp()->index);
                                } else {
                                    // TODO: check that the first non-phi statement does not use
                                    // the temp.
                                    break;
                                }
                            }
                        } else {
                            Q_ASSERT(use.pos < static_cast<unsigned>(successorStart) ||
                                     use.pos > static_cast<unsigned>(successorEnd));
                        }
                    }
                }
#endif

                continue;
            }

            Temp *moveTo;
            if (it->reg() == LifeTimeInterval::Invalid || !it->covers(successorStart)) {
                if (!isPhiTarget) // if it->temp() is a phi target, skip it.
                    continue;
                const int spillSlot = _assignedSpillSlots.value(it->temp(), -1);
                if (spillSlot == -1)
                    continue; // it has a life-time hole here.
                moveTo = createTemp(Temp::StackSlot, spillSlot, it->temp().type);
            } else {
                moveTo = createTemp(Temp::PhysicalRegister, platformRegister(*it), it->temp().type);
            }

            // add move to mapping
            mapping.add(moveFrom, moveTo);
        }

        mapping.order();
#ifdef DEBUG_REGALLOC
        mapping.dump();
#endif // DEBUG_REGALLOC

        bool insertIntoPredecessor = successor->in.size() > 1;
        mapping.insertMoves(insertIntoPredecessor ? predecessor : successor, _function,
                            insertIntoPredecessor);
    }

    Temp *createTemp(Temp::Kind kind, int index, Type type) const
    {
        Q_ASSERT(index >= 0);
        Temp *t = _function->New<Temp>();
        t->init(kind, index, 0);
        t->type = type;
        return t;
    }

    int platformRegister(const LifeTimeInterval &i) const
    {
        if (i.isFP())
            return _fpRegs.value(i.reg(), -1);
        else
            return _intRegs.value(i.reg(), -1);
    }

    Move *generateSpill(int spillSlot, Type type, int pReg) const
    {
        Q_ASSERT(spillSlot >= 0);

        Move *store = _function->New<Move>();
        store->init(createTemp(Temp::StackSlot, spillSlot, type),
                    createTemp(Temp::PhysicalRegister, pReg, type));
        return store;
    }

    Move *generateUnspill(const Temp &t, int pReg) const
    {
        Q_ASSERT(pReg >= 0);
        int spillSlot = _assignedSpillSlots.value(t, -1);
        Q_ASSERT(spillSlot != -1);
        Move *load = _function->New<Move>();
        load->init(createTemp(Temp::PhysicalRegister, pReg, t.type),
                   createTemp(Temp::StackSlot, spillSlot, t.type));
        return load;
    }

protected:
    virtual void visitTemp(Temp *t)
    {
        if (t->kind != Temp::VirtualRegister)
            return;

        const LifeTimeInterval *i = _intervalForTemp[*t];
        Q_ASSERT(i->isValid());
        if (i->reg() != LifeTimeInterval::Invalid && i->covers(_currentStmt->id)) {
            int pReg = platformRegister(*i);
            t->kind = Temp::PhysicalRegister;
            t->index = pReg;
        } else {
            int stackSlot = _assignedSpillSlots.value(*t, -1);
            Q_ASSERT(stackSlot >= 0);
            t->kind = Temp::StackSlot;
            t->index = stackSlot;
        }
    }

    virtual void visitConst(Const *) {}
    virtual void visitString(IR::String *) {}
    virtual void visitRegExp(IR::RegExp *) {}
    virtual void visitName(Name *) {}
    virtual void visitClosure(Closure *) {}
    virtual void visitConvert(Convert *e) { e->expr->accept(this); }
    virtual void visitUnop(Unop *e) { e->expr->accept(this); }
    virtual void visitBinop(Binop *e) { e->left->accept(this); e->right->accept(this); }
    virtual void visitSubscript(Subscript *e) { e->base->accept(this); e->index->accept(this); }
    virtual void visitMember(Member *e) { e->base->accept(this); }

    virtual void visitCall(Call *e) {
        e->base->accept(this);
        for (ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }

    virtual void visitNew(New *e) {
        e->base->accept(this);
        for (ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }

    virtual void visitExp(Exp *s) { s->expr->accept(this); }
    virtual void visitMove(Move *s) { s->source->accept(this); s->target->accept(this); }
    virtual void visitJump(Jump *) {}
    virtual void visitCJump(CJump *s) { s->cond->accept(this); }
    virtual void visitRet(Ret *s) { s->expr->accept(this); }
    virtual void visitPhi(Phi *) {}
};
} // anonymous namespace

RegisterAllocator::RegisterAllocator(const QVector<int> &normalRegisters, const QVector<int> &fpRegisters)
    : _normalRegisters(normalRegisters)
    , _fpRegisters(fpRegisters)
{
    Q_ASSERT(normalRegisters.size() >= 2);
    Q_ASSERT(fpRegisters.size() >= 2);
    _active.reserve((normalRegisters.size() + fpRegisters.size()) * 2);
    _inactive.reserve(_active.size());
}

RegisterAllocator::~RegisterAllocator()
{
}

void RegisterAllocator::run(IR::Function *function, const Optimizer &opt)
{
    _lastAssignedRegister.reserve(function->tempCount);
    _assignedSpillSlots.reserve(function->tempCount);
    _activeSpillSlots.resize(function->tempCount);

#ifdef DEBUG_REGALLOC
    qDebug() << "*** Running regalloc for function" << (function->name ? qPrintable(*function->name) : "NO NAME") << "***";
#endif // DEBUG_REGALLOC

    _unhandled = opt.lifeTimeIntervals();
    _handled.reserve(_unhandled.size());

    _info.reset(new RegAllocInfo);
    _info->collect(function);

#ifdef DEBUG_REGALLOC
    {
        QTextStream qout(stdout, QIODevice::WriteOnly);
        qout << "Ranges:" << endl;
        QVector<LifeTimeInterval> intervals = _unhandled;
        std::sort(intervals.begin(), intervals.end(), LifeTimeInterval::lessThanForTemp);
        foreach (const LifeTimeInterval &r, intervals) {
            r.dump(qout);
            qout << endl;
        }
    }
    _info->dump();
#endif // DEBUG_REGALLOC

    prepareRanges();

    Optimizer::showMeTheCode(function);

    linearScan();

#ifdef DEBUG_REGALLOC
    dump();
#endif // DEBUG_REGALLOC

    std::sort(_handled.begin(), _handled.end(), LifeTimeInterval::lessThan);
    ResolutionPhase(_handled, function, _info.data(), _assignedSpillSlots, _normalRegisters, _fpRegisters).run();

    function->tempCount = QSet<int>::fromList(_assignedSpillSlots.values()).size();

    Optimizer::showMeTheCode(function);

#ifdef DEBUG_REGALLOC
    qDebug() << "*** Finished regalloc for function" << (function->name ? qPrintable(*function->name) : "NO NAME") << "***";
#endif // DEBUG_REGALLOC
}

static inline LifeTimeInterval createFixedInterval(int rangeCount)
{
    LifeTimeInterval i(rangeCount);
    i.setReg(0);

    Temp t;
    t.init(Temp::PhysicalRegister, 0, 0);
    t.type = IR::SInt32Type;
    i.setTemp(t);

    return i;
}

static inline LifeTimeInterval cloneFixedInterval(int reg, bool isFP, LifeTimeInterval lti)
{
    lti.setReg(reg);
    lti.setFixedInterval(true);

    Temp t;
    t.init(Temp::PhysicalRegister, reg, 0);
    t.type = isFP ? IR::DoubleType : IR::SInt32Type;
    lti.setTemp(t);

    return lti;
}

void RegisterAllocator::prepareRanges()
{
    LifeTimeInterval ltiWithCalls = createFixedInterval(_info->calls().size());
    foreach (int callPosition, _info->calls())
        ltiWithCalls.addRange(callPosition, callPosition);

    const int regCount = _normalRegisters.size();
    _fixedRegisterRanges.reserve(regCount);
    for (int reg = 0; reg < regCount; ++reg) {
        LifeTimeInterval lti = cloneFixedInterval(reg, false, ltiWithCalls);
        _fixedRegisterRanges.append(lti);
        if (lti.isValid())
            _active.append(lti);
    }

    const int fpRegCount = _fpRegisters.size();
    _fixedFPRegisterRanges.reserve(fpRegCount);
    for (int fpReg = 0; fpReg < fpRegCount; ++fpReg) {
        LifeTimeInterval lti = cloneFixedInterval(fpReg, true, ltiWithCalls);
        _fixedFPRegisterRanges.append(lti);
        if (lti.isValid())
            _active.append(lti);
    }
}

void RegisterAllocator::linearScan()
{
    while (!_unhandled.isEmpty()) {
        LifeTimeInterval current = _unhandled.first();
        _unhandled.removeFirst();
        int position = current.start();

        // check for intervals in active that are handled or inactive
        for (int i = 0; i < _active.size(); ) {
            const LifeTimeInterval &it = _active.at(i);
            if (it.end() < position) {
                if (!it.isFixedInterval())
                    _handled += it;
                _active.remove(i);
            } else if (!it.covers(position)) {
                _inactive += it;
                _active.remove(i);
            } else {
                ++i;
            }
        }

        // check for intervals in inactive that are handled or active
        for (int i = 0; i < _inactive.size(); ) {
            const LifeTimeInterval &it = _inactive.at(i);
            if (it.end() < position) {
                if (!it.isFixedInterval())
                    _handled += it;
                _inactive.remove(i);
            } else if (it.covers(position)) {
                if (it.reg() != LifeTimeInterval::Invalid) {
                    _active += it;
                    _inactive.remove(i);
                } else {
                    // although this interval is now active, it has no register allocated (always
                    // spilled), so leave it in inactive.
                    ++i;
                }
            } else {
                ++i;
            }
        }

        Q_ASSERT(!current.isFixedInterval());

#ifdef DEBUG_REGALLOC
        qDebug() << "** Position" << position;
#endif // DEBUG_REGALLOC

        if (_info->canHaveRegister(current.temp())) {
            tryAllocateFreeReg(current, position);
            if (current.reg() == LifeTimeInterval::Invalid)
                allocateBlockedReg(current, position);
            if (current.reg() != LifeTimeInterval::Invalid)
                _active += current;
        } else {
            assignSpillSlot(current.temp(), current.start(), current.end());
            _inactive += current;
#ifdef DEBUG_REGALLOC
            qDebug() << "*** allocating stack slot" << _assignedSpillSlots[current.temp()]
                     << "for %" << current.temp().index << "as it cannot be loaded in a register";
#endif // DEBUG_REGALLOC
        }
    }

    foreach (const LifeTimeInterval &r, _active)
        if (!r.isFixedInterval())
            _handled.append(r);
    _active.clear();
    foreach (const LifeTimeInterval &r, _inactive)
        if (!r.isFixedInterval())
            _handled.append(r);
    _inactive.clear();
}

static inline int indexOfRangeCoveringPosition(const LifeTimeInterval::Ranges &ranges, int position)
{
    for (int i = 0, ei = ranges.size(); i != ei; ++i) {
        if (position <= ranges[i].end)
            return i;
    }
    return -1;
}

static inline int intersectionPosition(const LifeTimeInterval::Range &one, const LifeTimeInterval::Range &two)
{
    if (one.covers(two.start))
        return two.start;
    if (two.covers(one.start))
        return one.start;
    return -1;
}

static inline bool isFP(const Temp &t)
{ return t.type == DoubleType; }

void RegisterAllocator::tryAllocateFreeReg(LifeTimeInterval &current, const int position)
{
    Q_ASSERT(!current.isFixedInterval());
    Q_ASSERT(current.reg() == LifeTimeInterval::Invalid);

    const bool needsFPReg = isFP(current.temp());
    QVector<int> freeUntilPos(needsFPReg ? _fpRegisters.size() : _normalRegisters.size(), INT_MAX);
    Q_ASSERT(freeUntilPos.size() > 0);

    const bool isPhiTarget = _info->isPhiTarget(current.temp());
    foreach (const LifeTimeInterval &it, _active) {
        if (it.isFP() == needsFPReg) {
            if (!isPhiTarget && it.isFixedInterval() && !current.isSplitFromInterval()) {
                const int idx = indexOfRangeCoveringPosition(it.ranges(), position);
                if (it.ranges().at(idx).end == current.start()) {
                    if (it.ranges().size() > idx + 1)
                        freeUntilPos[it.reg()] = it.ranges().at(idx + 1).start;
                    continue;
                }
            }

            if (isPhiTarget || it.end() >= current.firstPossibleUsePosition(isPhiTarget))
                freeUntilPos[it.reg()] = 0; // mark register as unavailable
        }
    }

    foreach (const LifeTimeInterval &it, _inactive) {
        if (current.isSplitFromInterval() || it.isFixedInterval()) {
            if (it.isFP() == needsFPReg && it.reg() != LifeTimeInterval::Invalid) {
                const int intersectionPos = nextIntersection(current, it, position);
                if (!isPhiTarget && it.isFixedInterval() && current.end() == intersectionPos)
                    freeUntilPos[it.reg()] = qMin(freeUntilPos[it.reg()], intersectionPos + 1);
                else if (intersectionPos != -1)
                    freeUntilPos[it.reg()] = qMin(freeUntilPos[it.reg()], intersectionPos);
            }
        }
    }

    int reg = LifeTimeInterval::Invalid;
    int freeUntilPos_reg = 0;

    foreach (const Temp &hint, _info->hints(current.temp())) {
        int candidate;
        if (hint.kind == Temp::PhysicalRegister)
            candidate = hint.index;
        else
            candidate = _lastAssignedRegister.value(hint, LifeTimeInterval::Invalid);

        const int end = current.end();
        if (candidate != LifeTimeInterval::Invalid) {
            if (current.isFP() == (hint.type == DoubleType)) {
                int fp = freeUntilPos[candidate];
                if ((freeUntilPos_reg < end && fp > freeUntilPos_reg)
                        || (freeUntilPos_reg >= end && fp >= end && freeUntilPos_reg > fp)) {
                    reg = candidate;
                    freeUntilPos_reg = fp;
                }
            }
        }
    }

    if (reg == LifeTimeInterval::Invalid)
        longestAvailableReg(freeUntilPos, reg, freeUntilPos_reg, current.end());

    if (freeUntilPos_reg == 0) {
        // no register available without spilling
#ifdef DEBUG_REGALLOC
        qDebug() << "*** no register available for %" << current.temp().index;
#endif // DEBUG_REGALLOC
        return;
    } else if (current.end() < freeUntilPos_reg) {
        // register available for the whole interval
#ifdef DEBUG_REGALLOC
        qDebug() << "*** allocating register" << reg << "for the whole interval of %" << current.temp().index;
#endif // DEBUG_REGALLOC
        current.setReg(reg);
        _lastAssignedRegister.insert(current.temp(), reg);
    } else {
        // register available for the first part of the interval
        current.setReg(reg);
        _lastAssignedRegister.insert(current.temp(), reg);
#ifdef DEBUG_REGALLOC
        qDebug() << "*** allocating register" << reg << "for the first part of interval of %" << current.temp().index;
#endif // DEBUG_REGALLOC
        split(current, freeUntilPos_reg, true);
    }
}

void RegisterAllocator::allocateBlockedReg(LifeTimeInterval &current, const int position)
{
    Q_ASSERT(!current.isFixedInterval());
    Q_ASSERT(current.reg() == LifeTimeInterval::Invalid);

    const bool isPhiTarget = _info->isPhiTarget(current.temp());
    if (isPhiTarget && !current.isSplitFromInterval()) {
        split(current, position + 1, true);
        _inactive.append(current);
        return;
    }

    const bool needsFPReg = isFP(current.temp());
    QVector<int> nextUsePos(needsFPReg ? _fpRegisters.size() : _normalRegisters.size(), INT_MAX);
    QVector<LifeTimeInterval *> nextUseRangeForReg(nextUsePos.size(), 0);
    Q_ASSERT(nextUsePos.size() > 0);

    const bool definedAtCurrentPosition = !current.isSplitFromInterval() && current.start() == position;

    for (int i = 0, ei = _active.size(); i != ei; ++i) {
        LifeTimeInterval &it = _active[i];
        if (it.isFP() == needsFPReg) {
            int nu = it.isFixedInterval() ? 0 : nextUse(it.temp(), current.firstPossibleUsePosition(isPhiTarget));
            if (nu == position && !definedAtCurrentPosition) {
                nextUsePos[it.reg()] = 0;
            } else if (nu != -1 && nu < nextUsePos[it.reg()]) {
                nextUsePos[it.reg()] = nu;
                nextUseRangeForReg[it.reg()] = &it;
            } else if (nu == -1 && nextUsePos[it.reg()] == INT_MAX) {
                // in a loop, the range can be active, but only used before the current position (e.g. in a loop header or phi node)
                nextUseRangeForReg[it.reg()] = &it;
            }
        }
    }

    for (int i = 0, ei = _inactive.size(); i != ei; ++i) {
        LifeTimeInterval &it = _inactive[i];
        if (current.isSplitFromInterval() || it.isFixedInterval()) {
            if (it.isFP() == needsFPReg && it.reg() != LifeTimeInterval::Invalid) {
                if (nextIntersection(current, it, position) != -1) {
                    int nu = nextUse(it.temp(), current.firstPossibleUsePosition(isPhiTarget));
                    if (nu != -1 && nu < nextUsePos[it.reg()]) {
                        nextUsePos[it.reg()] = nu;
                        nextUseRangeForReg[it.reg()] = &it;
                    }
                }
            }
        }
    }

    int reg, nextUsePos_reg;
    longestAvailableReg(nextUsePos, reg, nextUsePos_reg, current.end());

    if (current.start() > nextUsePos_reg) {
        // all other intervals are used before current, so it is best to spill current itself
#ifdef DEBUG_REGALLOC
        QTextStream out(stderr, QIODevice::WriteOnly);
        out << "*** splitting current for range ";current.dump(out);out<<endl;
#endif // DEBUG_REGALLOC
        Q_ASSERT(!_info->useMustHaveReg(current.temp(), position));
        split(current, position + 1, true);
        _inactive.append(current);
    } else {
        // spill intervals that currently block reg
#ifdef DEBUG_REGALLOC
        QTextStream out(stderr, QIODevice::WriteOnly);
        out << "*** spilling intervals that block reg "<<reg<<" for interval ";current.dump(out);out<<endl;
#endif // DEBUG_REGALLOC
        current.setReg(reg);
        _lastAssignedRegister.insert(current.temp(), reg);
        LifeTimeInterval *nextUse = nextUseRangeForReg[reg];
        Q_ASSERT(nextUse);
        Q_ASSERT(!nextUse->isFixedInterval());

        if (_info->isUsedAt(nextUse->temp(), position)) {
            Q_ASSERT(!_info->isUsedAt(current.temp(), position));
            // the register is used (as an incoming parameter) at the current position, so split
            // the interval immediately after the (use at the) current position
            split(*nextUse, position + 1);
        } else {
            // the register was used before the current position
            split(*nextUse, position);
        }

        splitInactiveAtEndOfLifetimeHole(reg, needsFPReg, position);

        // make sure that current does not intersect with the fixed interval for reg
        const LifeTimeInterval &fixedRegRange = needsFPReg ? _fixedFPRegisterRanges.at(reg)
                                                           : _fixedRegisterRanges.at(reg);
        int ni = nextIntersection(current, fixedRegRange, position);
        if (ni != -1) {
#ifdef DEBUG_REGALLOC
            out << "***-- current range intersects with a fixed reg use at "<<ni<<", so splitting it."<<endl;
#endif // DEBUG_REGALLOC
            split(current, ni, true);
        }
    }
}

void RegisterAllocator::longestAvailableReg(const QVector<int> &nextUses, int &reg,
                                            int &freeUntilPos_reg, int lastUse) const
{
    reg = LifeTimeInterval::Invalid;
    freeUntilPos_reg = 0;

    for (int candidate = 0, candidateEnd = nextUses.size(); candidate != candidateEnd; ++candidate) {
        int fp = nextUses[candidate];
        if ((freeUntilPos_reg < lastUse && fp > freeUntilPos_reg)
                || (freeUntilPos_reg >= lastUse && fp >= lastUse && freeUntilPos_reg > fp)) {
            reg = candidate;
            freeUntilPos_reg = fp;
        }
    }
}

int RegisterAllocator::nextIntersection(const LifeTimeInterval &current,
                                        const LifeTimeInterval &another, const int position) const
{
    LifeTimeInterval::Ranges currentRanges = current.ranges();
    int currentIt = indexOfRangeCoveringPosition(currentRanges, position);
    if (currentIt == -1)
        return -1;

    LifeTimeInterval::Ranges anotherRanges = another.ranges();
    const int anotherItStart = indexOfRangeCoveringPosition(anotherRanges, position);
    if (anotherItStart == -1)
        return -1;

    for (int currentEnd = currentRanges.size(); currentIt < currentEnd; ++currentIt) {
        const LifeTimeInterval::Range currentRange = currentRanges.at(currentIt);
        for (int anotherIt = anotherItStart, anotherEnd = anotherRanges.size(); anotherIt < anotherEnd; ++anotherIt) {
            const LifeTimeInterval::Range anotherRange = anotherRanges.at(anotherIt);
            if (anotherRange.start > currentRange.end)
                break;
            int intersectPos = intersectionPosition(currentRange, anotherRange);
            if (intersectPos != -1)
                return intersectPos;
        }
    }

    return -1;
}

int RegisterAllocator::nextUse(const Temp &t, int startPosition) const
{
    QList<Use> usePositions = _info->uses(t);
    for (int i = 0, ei = usePositions.size(); i != ei; ++i) {
        int usePos = usePositions[i].pos;
        if (usePos >= startPosition)
            return usePos;
    }

    return -1;
}

static inline void insertSorted(QVector<LifeTimeInterval> &intervals, const LifeTimeInterval &newInterval)
{
    newInterval.validate();
    for (int i = 0, ei = intervals.size(); i != ei; ++i) {
        if (LifeTimeInterval::lessThan(newInterval, intervals.at(i))) {
            intervals.insert(i, newInterval);
            return;
        }
    }
    intervals.append(newInterval);
}

void RegisterAllocator::split(LifeTimeInterval &current, int beforePosition,
                              bool skipOptionalRegisterUses)
{ // TODO: check if we can always skip the optional register uses
    Q_ASSERT(!current.isFixedInterval());

#ifdef DEBUG_REGALLOC
    QTextStream out(stderr, QIODevice::WriteOnly);
    out << "***** split request for range ";current.dump(out);out<<" before position "<<beforePosition<<" and skipOptionalRegisterUses = "<<skipOptionalRegisterUses<<endl;
#endif // DEBUG_REGALLOC

    assignSpillSlot(current.temp(), current.start(), current.end());

    const int defPosition = _info->def(current.temp());
    if (beforePosition < defPosition) {
#ifdef DEBUG_REGALLOC
        out << "***** split before position is before or at definition, so not splitting."<<endl;
#endif // DEBUG_REGALLOC
        return;
    }

    int lastUse = -1;
    if (defPosition < beforePosition)
        lastUse = defPosition;
    int nextUse = -1;
    QList<Use> usePositions = _info->uses(current.temp());
    for (int i = 0, ei = usePositions.size(); i != ei; ++i) {
        const Use &usePosition = usePositions[i];
        const int usePos = usePosition.pos;
        if (lastUse < usePos && usePos < beforePosition) {
            lastUse = usePos;
        } else if (usePos >= beforePosition) {
            if (!skipOptionalRegisterUses || usePosition.mustHaveRegister()) {
                nextUse = usePos;
                break;
            }
        }
    }
    if (lastUse == -1)
        lastUse = beforePosition - 1;

    Q_ASSERT(lastUse < beforePosition);

#ifdef DEBUG_REGALLOC
    out << "***** last use = "<<lastUse<<", nextUse = " << nextUse<<endl;
#endif // DEBUG_REGALLOC
    LifeTimeInterval newInterval = current.split(lastUse, nextUse);
#ifdef DEBUG_REGALLOC
    out << "***** new interval: "; newInterval.dump(out); out << endl;
    out << "***** preceding interval: "; current.dump(out); out << endl;
#endif // DEBUG_REGALLOC
    if (newInterval.isValid()) {
        if (current.reg() != LifeTimeInterval::Invalid)
            _info->addHint(current.temp(), current.reg());
        newInterval.setReg(LifeTimeInterval::Invalid);
        insertSorted(_unhandled, newInterval);
    }
}

void RegisterAllocator::splitInactiveAtEndOfLifetimeHole(int reg, bool isFPReg, int position)
{
    for (int i = 0, ei = _inactive.size(); i != ei; ++i) {
        LifeTimeInterval &interval = _inactive[i];
        if (interval.isFixedInterval())
            continue;
        if (isFPReg == interval.isFP() && interval.reg() == reg) {
            LifeTimeInterval::Ranges ranges = interval.ranges();
            int endOfLifetimeHole = -1;
            for (int j = 0, ej = ranges.size(); j != ej; ++j) {
                if (position < ranges[j].start)
                    endOfLifetimeHole = ranges[j].start;
            }
            if (endOfLifetimeHole != -1)
                split(interval, endOfLifetimeHole);
        }
    }
}

void RegisterAllocator::assignSpillSlot(const Temp &t, int startPos, int endPos)
{
    if (_assignedSpillSlots.contains(t))
        return;

    for (int i = 0, ei = _activeSpillSlots.size(); i != ei; ++i) {
        if (_activeSpillSlots.at(i) < startPos) {
            _activeSpillSlots[i] = endPos;
            _assignedSpillSlots.insert(t, i);
            return;
        }
    }

    Q_UNREACHABLE();
}

void RegisterAllocator::dump() const
{
#ifdef DEBUG_REGALLOC
    QTextStream qout(stdout, QIODevice::WriteOnly);

    {
        qout << "Ranges:" << endl;
        QVector<LifeTimeInterval> handled = _handled;
        std::sort(handled.begin(), handled.end(), LifeTimeInterval::lessThanForTemp);
        foreach (const LifeTimeInterval &r, handled) {
            r.dump(qout);
            qout << endl;
        }
    }

    {
        qout << "Spill slots:" << endl;
        QList<Temp> temps = _assignedSpillSlots.keys();
        if (temps.isEmpty())
            qout << "\t(none)" << endl;
        std::sort(temps.begin(), temps.end());
        foreach (const Temp &t, temps) {
            qout << "\t";
            t.dump(qout);
            qout << " -> " << _assignedSpillSlots[t] << endl;
        }
    }
#endif // DEBUG_REGALLOC
}
