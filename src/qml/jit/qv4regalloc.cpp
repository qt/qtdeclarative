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

namespace {
enum { DebugRegAlloc = 0 };

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

namespace {
class IRPrinterWithPositions: public IRPrinter
{
    LifeTimeIntervals::Ptr intervals;
    const int positionSize;

public:
    IRPrinterWithPositions(QTextStream *out, const LifeTimeIntervals::Ptr &intervals)
        : IRPrinter(out)
        , intervals(intervals)
        , positionSize(QString::number(intervals->lastPosition()).size())
    {}

protected:
    void addStmtNr(Stmt *s)
    {
        QString posStr;
        int pos = intervals->positionForStatement(s);
        if (pos != Stmt::InvalidId)
            posStr = QString::number(pos);
        *out << posStr.rightJustified(positionSize);
        if (pos == Stmt::InvalidId)
            *out << "  ";
        else
            *out << ": ";
    }
};
}

class RegAllocInfo: public IRDecoder
{
    struct Def {
        unsigned valid : 1;
        unsigned canHaveReg : 1;
        unsigned isPhiTarget : 1;

        Def(): valid(0), canHaveReg(0), isPhiTarget(0) {}
        Def(bool canHaveReg, bool isPhiTarget)
            : valid(1), canHaveReg(canHaveReg), isPhiTarget(isPhiTarget)
        {
        }

        bool isValid() const { return valid != 0; }
    };

    IR::LifeTimeIntervals::Ptr _lifeTimeIntervals;
    BasicBlock *_currentBB;
    Stmt *_currentStmt;
    std::vector<Def> _defs;
    std::vector<QList<Use> > _uses;
    std::vector<int> _calls;
    std::vector<QList<Temp> > _hints;

    int defPosition(Stmt *s) const
    {
        return usePosition(s) + 1;
    }

    int usePosition(Stmt *s) const
    {
        return _lifeTimeIntervals->positionForStatement(s);
    }

public:
    RegAllocInfo(): _currentBB(0), _currentStmt(0) {}

    void collect(IR::Function *function, const IR::LifeTimeIntervals::Ptr &lifeTimeIntervals)
    {
        _lifeTimeIntervals = lifeTimeIntervals;
        _defs.resize(function->tempCount);
        _uses.resize(function->tempCount);
        _calls.reserve(function->statementCount() / 3);
        _hints.resize(function->tempCount);

        foreach (BasicBlock *bb, function->basicBlocks()) {
            _currentBB = bb;
            foreach (Stmt *s, bb->statements()) {
                _currentStmt = s;
                s->accept(this);
            }
        }
    }

    QList<Use> uses(const Temp &t) const
    {
        return _uses[t.index];
    }

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

    bool canHaveRegister(const Temp &t) const {
        Q_ASSERT(_defs[t.index].isValid());
        return _defs[t.index].canHaveReg;
    }
    bool isPhiTarget(const Temp &t) const {
        Q_ASSERT(_defs[t.index].isValid());
        return _defs[t.index].isPhiTarget;
    }

    const std::vector<int> &calls() const { return _calls; }
    const QList<Temp> &hints(const Temp &t) const { return _hints[t.index]; }
    void addHint(const Temp &t, int physicalRegister)
    {
        Temp hint;
        hint.init(Temp::PhysicalRegister, physicalRegister);
        _hints[t.index].append(hint);
    }

    void dump() const
    {
        if (!DebugRegAlloc)
            return;

        QTextStream qout(stdout, QIODevice::WriteOnly);
        IRPrinterWithPositions printer(&qout, _lifeTimeIntervals);

        qout << "RegAllocInfo:" << endl << "Defs/uses:" << endl;
        for (unsigned t = 0; t < _defs.size(); ++t) {
            qout << "%" << t <<": "
                 << " ("
                 << (_defs[t].canHaveReg ? "can" : "can NOT")
                 << " have a register, and "
                 << (_defs[t].isPhiTarget ? "is" : "is NOT")
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
        for (unsigned i = 0; i < _calls.size(); ++i) {
            if (i > 0) qout << ", ";
            qout << _calls[i];
        }
        qout << endl;

        qout << "Hints:" << endl;
        for (unsigned t = 0; t < _hints.size(); ++t) {
            qout << "\t%" << t << ": ";
            QList<Temp> hints = _hints[t];
            for (int i = 0; i < hints.size(); ++i) {
                if (i > 0) qout << ", ";
                printer.print(hints[i]);
            }
            qout << endl;
        }
    }

protected: // IRDecoder
    virtual void callBuiltinInvalid(IR::Name *, IR::ExprList *, IR::Expr *) {}
    virtual void callBuiltinTypeofMember(IR::Expr *, const QString &, IR::Expr *) {}
    virtual void callBuiltinTypeofSubscript(IR::Expr *, IR::Expr *, IR::Expr *) {}
    virtual void callBuiltinTypeofName(const QString &, IR::Expr *) {}
    virtual void callBuiltinTypeofValue(IR::Expr *, IR::Expr *) {}
    virtual void callBuiltinDeleteMember(IR::Expr *, const QString &, IR::Expr *) {}
    virtual void callBuiltinDeleteSubscript(IR::Expr *, IR::Expr *, IR::Expr *) {}
    virtual void callBuiltinDeleteName(const QString &, IR::Expr *) {}
    virtual void callBuiltinDeleteValue(IR::Expr *) {}
    virtual void callBuiltinThrow(IR::Expr *) {}
    virtual void callBuiltinReThrow() {}
    virtual void callBuiltinUnwindException(IR::Expr *) {}
    virtual void callBuiltinPushCatchScope(const QString &) {};
    virtual void callBuiltinForeachIteratorObject(IR::Expr *, IR::Expr *) {}
    virtual void callBuiltinForeachNextProperty(IR::Temp *, IR::Temp *) {}
    virtual void callBuiltinForeachNextPropertyname(IR::Expr *, IR::Expr *) {}
    virtual void callBuiltinPushWithScope(IR::Expr *) {}
    virtual void callBuiltinPopScope() {}
    virtual void callBuiltinDeclareVar(bool , const QString &) {}
    virtual void callBuiltinDefineArray(IR::Expr *, IR::ExprList *) {}
    virtual void callBuiltinDefineObjectLiteral(IR::Expr *, int, IR::ExprList *, IR::ExprList *, bool) {}
    virtual void callBuiltinSetupArgumentObject(IR::Expr *) {}
    virtual void callBuiltinConvertThisToObject() {}

    virtual void callValue(IR::Expr *value, IR::ExprList *args, IR::Expr *result)
    {
        addDef(result);
        if (IR::Temp *tempValue = value->asTemp())
            addUses(tempValue, Use::CouldHaveRegister);
        addUses(args, Use::CouldHaveRegister);
        addCall();
    }

    virtual void callProperty(IR::Expr *base, const QString &name, IR::ExprList *args,
                              IR::Expr *result)
    {
        Q_UNUSED(name)

        addDef(result);
        addUses(base->asTemp(), Use::CouldHaveRegister);
        addUses(args, Use::CouldHaveRegister);
        addCall();
    }

    virtual void callSubscript(IR::Expr *base, IR::Expr *index, IR::ExprList *args,
                               IR::Expr *result)
    {
        addDef(result);
        addUses(base->asTemp(), Use::CouldHaveRegister);
        addUses(index->asTemp(), Use::CouldHaveRegister);
        addUses(args, Use::CouldHaveRegister);
        addCall();
    }

    virtual void convertType(IR::Expr *source, IR::Expr *target)
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

        Temp *sourceTemp = source->asTemp();
        if (sourceTemp)
            addUses(sourceTemp, sourceReg);

        if (needsCall)
            addCall();
        else if (target->asTemp())
            addHint(target->asTemp(), sourceTemp);
    }

    virtual void constructActivationProperty(IR::Name *, IR::ExprList *args, IR::Expr *result)
    {
        addDef(result);
        addUses(args, Use::CouldHaveRegister);
        addCall();
    }

    virtual void constructProperty(IR::Expr *base, const QString &, IR::ExprList *args, IR::Expr *result)
    {
        addDef(result);
        addUses(base, Use::CouldHaveRegister);
        addUses(args, Use::CouldHaveRegister);
        addCall();
    }

    virtual void constructValue(IR::Expr *value, IR::ExprList *args, IR::Expr *result)
    {
        addDef(result);
        addUses(value, Use::CouldHaveRegister);
        addUses(args, Use::CouldHaveRegister);
        addCall();
    }

    virtual void loadThisObject(IR::Expr *temp)
    {
        addDef(temp);
    }

    virtual void loadQmlIdArray(IR::Expr *temp)
    {
        addDef(temp);
        addCall();
    }

    virtual void loadQmlImportedScripts(IR::Expr *temp)
    {
        addDef(temp);
        addCall();
    }

    virtual void loadQmlContextObject(Expr *temp)
    {
        addDef(temp);
        addCall();
    }

    virtual void loadQmlScopeObject(Expr *temp)
    {
        Q_UNUSED(temp);

        addDef(temp);
        addCall();
    }

    virtual void loadQmlSingleton(const QString &/*name*/, Expr *temp)
    {
        Q_UNUSED(temp);

        addDef(temp);
        addCall();
    }

    virtual void loadConst(IR::Const *sourceConst, Expr *targetTemp)
    {
        Q_UNUSED(sourceConst);

        addDef(targetTemp);
    }

    virtual void loadString(const QString &str, Expr *targetTemp)
    {
        Q_UNUSED(str);

        addDef(targetTemp);
    }

    virtual void loadRegexp(IR::RegExp *sourceRegexp, Expr *targetTemp)
    {
        Q_UNUSED(sourceRegexp);

        addDef(targetTemp);
        addCall();
    }

    virtual void getActivationProperty(const IR::Name *, Expr *temp)
    {
        addDef(temp);
        addCall();
    }

    virtual void setActivationProperty(IR::Expr *source, const QString &)
    {
        addUses(source->asTemp(), Use::CouldHaveRegister);
        addCall();
    }

    virtual void initClosure(IR::Closure *closure, Expr *target)
    {
        Q_UNUSED(closure);

        addDef(target);
        addCall();
    }

    virtual void getProperty(IR::Expr *base, const QString &, Expr *target)
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

    virtual void getQObjectProperty(IR::Expr *base, int /*propertyIndex*/, bool /*captureRequired*/, int /*attachedPropertiesId*/, IR::Expr *target)
    {
        addDef(target);
        addUses(base->asTemp(), Use::CouldHaveRegister);
        addCall();
    }

    virtual void getElement(IR::Expr *base, IR::Expr *index, Expr *target)
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

    virtual void copyValue(Expr *source, Expr *target)
    {
        addDef(target);
        Temp *sourceTemp = source->asTemp();
        if (!sourceTemp)
            return;
        addUses(sourceTemp, Use::CouldHaveRegister);
        Temp *targetTemp = target->asTemp();
        if (targetTemp)
            addHint(targetTemp, sourceTemp);
    }

    virtual void swapValues(Expr *, Expr *)
    {
        // Inserted by the register allocator, so it cannot occur here.
        Q_UNREACHABLE();
    }

    virtual void unop(AluOp oper, Expr *source, Expr *target)
    {
        addDef(target);

        bool needsCall = true;
        if (oper == OpNot && source->type == IR::BoolType && target->type == IR::BoolType)
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

        IR::Temp *sourceTemp = source->asTemp();
        if (needsCall) {
            if (sourceTemp)
                addUses(sourceTemp, Use::CouldHaveRegister);
            addCall();
        } else {
            if (sourceTemp)
                addUses(sourceTemp, Use::MustHaveRegister);
        }
    }

    virtual void binop(AluOp oper, Expr *leftSource, Expr *rightSource, Expr *target)
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
    virtual void callBuiltin(IR::Call *c, IR::Expr *result)
    {
        addDef(result);
        addUses(c->base, Use::CouldHaveRegister);
        addUses(c->args, Use::CouldHaveRegister);
        addCall();
    }

private:
    void addDef(Expr *e, bool isPhiTarget = false)
    {
        if (!e)
            return;
        Temp *t = e->asTemp();
        if (!t)
            return;
        if (!t || t->kind != Temp::VirtualRegister)
            return;
        Q_ASSERT(!_defs[t->index].isValid());
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

        _defs[t->index] = Def(canHaveReg, isPhiTarget);
    }

    void addUses(Expr *e, Use::RegisterFlag flag)
    {
        int usePos = usePosition(_currentStmt);
        if (usePos == Stmt::InvalidId)
            usePos = _lifeTimeIntervals->startPosition(_currentBB);
        Q_ASSERT(usePos > 0);
        if (!e)
            return;
        Temp *t = e->asTemp();
        if (!t)
            return;
        if (t && t->kind == Temp::VirtualRegister)
            _uses[t->index].append(Use(usePosition(_currentStmt), flag));
    }

    void addUses(ExprList *l, Use::RegisterFlag flag)
    {
        for (ExprList *it = l; it; it = it->next)
            addUses(it->expr, flag);
    }

    void addCall()
    {
        _calls.push_back(usePosition(_currentStmt));
    }

    void addHint(Expr *hinted, Temp *hint1, Temp *hint2 = 0)
    {
        if (hinted)
            if (Temp *hintedTemp = hinted->asTemp())
                addHint(hintedTemp, hint1, hint2);
    }

    void addHint(Temp *hinted, Temp *hint1, Temp *hint2 = 0)
    {
        if (!hinted || hinted->kind != Temp::VirtualRegister)
            return;
        if (hint1 && hint1->kind == Temp::VirtualRegister && hinted->type == hint1->type)
            _hints[hinted->index].append(*hint1);
        if (hint2 && hint2->kind == Temp::VirtualRegister && hinted->type == hint2->type)
            _hints[hinted->index].append(*hint2);
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
    Q_DISABLE_COPY(ResolutionPhase)

    LifeTimeIntervals::Ptr _intervals;
    QVector<LifeTimeInterval *> _unprocessed;
    IR::Function *_function;
    const std::vector<int> &_assignedSpillSlots;
    QHash<IR::Temp, const LifeTimeInterval *> _intervalForTemp;
    const QVector<int> &_intRegs;
    const QVector<int> &_fpRegs;

    Stmt *_currentStmt;
    QVector<Move *> _loads;
    QVector<Move *> _stores;

    QHash<BasicBlock *, QList<const LifeTimeInterval *> > _liveAtStart;
    QHash<BasicBlock *, QList<const LifeTimeInterval *> > _liveAtEnd;

public:
    ResolutionPhase(const QVector<LifeTimeInterval *> &unprocessed, const LifeTimeIntervals::Ptr &intervals, IR::Function *function,
                    const std::vector<int> &assignedSpillSlots,
                    const QVector<int> &intRegs, const QVector<int> &fpRegs)
        : _intervals(intervals)
        , _function(function)
        , _assignedSpillSlots(assignedSpillSlots)
        , _intRegs(intRegs)
        , _fpRegs(fpRegs)
    {
        _unprocessed = unprocessed;
        _liveAtStart.reserve(function->basicBlockCount());
        _liveAtEnd.reserve(function->basicBlockCount());
    }

    void run() {
        renumber();
        if (DebugRegAlloc) {
            QTextStream qout(stdout, QIODevice::WriteOnly);
            IRPrinterWithPositions(&qout, _intervals).print(_function);
        }
        resolve();
    }

private:
    int defPosition(Stmt *s) const
    {
        return usePosition(s) + 1;
    }

    int usePosition(Stmt *s) const
    {
        return _intervals->positionForStatement(s);
    }

    void renumber()
    {
        foreach (BasicBlock *bb, _function->basicBlocks()) {
            _currentStmt = 0;

            QVector<Stmt *> statements = bb->statements();
            QVector<Stmt *> newStatements;
            newStatements.reserve(bb->statements().size() + 7);

            cleanOldIntervals(_intervals->startPosition(bb));
            addNewIntervals(_intervals->startPosition(bb));
            _liveAtStart[bb] = _intervalForTemp.values();

            for (int i = 0, ei = statements.size(); i != ei; ++i) {
                _currentStmt = statements.at(i);
                _loads.clear();
                _stores.clear();
                if (_currentStmt->asTerminator())
                    addNewIntervals(usePosition(_currentStmt));
                else
                    addNewIntervals(defPosition(_currentStmt));
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

            cleanOldIntervals(_intervals->endPosition(bb));
            _liveAtEnd[bb] = _intervalForTemp.values();

            if (DebugRegAlloc) {
                QTextStream os(stdout, QIODevice::WriteOnly);
                os << "Intervals live at the start of L" << bb->index() << ":" << endl;
                if (_liveAtStart[bb].isEmpty())
                    os << "\t(none)" << endl;
                foreach (const LifeTimeInterval *i, _liveAtStart[bb]) {
                    os << "\t";
                    i->dump(os);
                    os << endl;
                }
                os << "Intervals live at the end of L" << bb->index() << ":" << endl;
                if (_liveAtEnd[bb].isEmpty())
                    os << "\t(none)" << endl;
                foreach (const LifeTimeInterval *i, _liveAtEnd[bb]) {
                    os << "\t";
                    i->dump(os);
                    os << endl;
                }
            }

            bb->setStatements(newStatements);
        }

    }

    void maybeGenerateSpill(Temp *t)
    {
        const LifeTimeInterval *i = _intervalForTemp[*t];
        if (i->reg() == LifeTimeInterval::InvalidRegister)
            return;

        int pReg = platformRegister(*i);
        int spillSlot = _assignedSpillSlots[i->temp().index];
        if (spillSlot != RegisterAllocator::InvalidSpillSlot)
            _stores.append(generateSpill(spillSlot, i->temp().type, pReg));
    }

    void addNewIntervals(int position)
    {
        if (position == Stmt::InvalidId)
            return;

        while (!_unprocessed.isEmpty()) {
            const LifeTimeInterval *i = _unprocessed.first();
            if (i->start() > position)
                break;

            Q_ASSERT(!i->isFixedInterval());
            _intervalForTemp[i->temp()] = i;
            qDebug()<<"-- Activating interval for temp"<<i->temp().index;

            _unprocessed.removeFirst();
        }
    }

    void cleanOldIntervals(int position)
    {
        QMutableHashIterator<Temp, const LifeTimeInterval *> it(_intervalForTemp);
        while (it.hasNext()) {
            const LifeTimeInterval *i = it.next().value();
            if (i->end() < position || i->isFixedInterval())
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

    Phi *findDefPhi(const Temp &t, BasicBlock *bb) const
    {
        foreach (Stmt *s, bb->statements()) {
            Phi *phi = s->asPhi();
            if (!phi)
                return 0;

            if (*phi->targetTemp == t)
                return phi;
        }

        Q_UNREACHABLE();
    }

    void resolveEdge(BasicBlock *predecessor, BasicBlock *successor)
    {
        if (DebugRegAlloc) {
            qDebug() << "Resolving edge" << predecessor->index() << "->" << successor->index();
            QTextStream qout(stdout, QIODevice::WriteOnly);
            IRPrinterWithPositions printer(&qout, _intervals);
            printer.print(predecessor);
            printer.print(successor);
            qout.flush();
        }

        MoveMapping mapping;

        const int predecessorEnd = _intervals->endPosition(predecessor);
        Q_ASSERT(predecessorEnd > 0);

        int successorStart = _intervals->startPosition(successor);
        Q_ASSERT(successorStart > 0);

        foreach (const LifeTimeInterval *it, _liveAtStart[successor]) {
            bool isPhiTarget = false;
            Expr *moveFrom = 0;

            if (it->start() == successorStart) {
                if (Phi *phi = findDefPhi(it->temp(), successor)) {
                    isPhiTarget = true;
                    Expr *opd = phi->d->incoming[successor->in.indexOf(predecessor)];
                    if (opd->asConst()) {
                        moveFrom = opd;
                    } else {
                        Temp *t = opd->asTemp();
                        Q_ASSERT(t);

                        foreach (const LifeTimeInterval *it2, _liveAtEnd[predecessor]) {
                            if (it2->temp() == *t
                                    && it2->reg() != LifeTimeInterval::InvalidRegister
                                    && it2->covers(predecessorEnd)) {
                                moveFrom = createTemp(Temp::PhysicalRegister,
                                                      platformRegister(*it2), t->type);
                                break;
                            }
                        }
                        if (!moveFrom)
                            moveFrom = createTemp(Temp::StackSlot,
                                                  _assignedSpillSlots[t->index],
                                    t->type);
                    }
                }
            } else {
                foreach (const LifeTimeInterval *predIt, _liveAtEnd[predecessor]) {
                    if (predIt->temp() == it->temp()) {
                        if (predIt->reg() != LifeTimeInterval::InvalidRegister
                                && predIt->covers(predecessorEnd)) {
                            moveFrom = createTemp(Temp::PhysicalRegister, platformRegister(*predIt),
                                                  predIt->temp().type);
                        } else {
                            int spillSlot = _assignedSpillSlots[predIt->temp().index];
                            if (spillSlot != -1)
                                moveFrom = createTemp(Temp::StackSlot, spillSlot, predIt->temp().type);
                        }
                        break;
                    }
                }
            }
            if (!moveFrom) {
#if !defined(QT_NO_DEBUG) && 0
                bool lifeTimeHole = false;
                if (it->ranges().first().start <= successorStart && it->ranges().last().end >= successorStart)
                    lifeTimeHole = !it->covers(successorStart);

                Q_ASSERT(!_info->isPhiTarget(it->temp()) || it->isSplitFromInterval() || lifeTimeHole);
                if (_info->def(it->temp()) != successorStart && !it->isSplitFromInterval()) {
                    const int successorEnd = successor->terminator()->id();
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
            if (it->reg() == LifeTimeInterval::InvalidRegister || !it->covers(successorStart)) {
                if (!isPhiTarget) // if it->temp() is a phi target, skip it.
                    continue;
                const int spillSlot = _assignedSpillSlots[it->temp().index];
                if (spillSlot == RegisterAllocator::InvalidSpillSlot)
                    continue; // it has a life-time hole here.
                moveTo = createTemp(Temp::StackSlot, spillSlot, it->temp().type);
            } else {
                moveTo = createTemp(Temp::PhysicalRegister, platformRegister(*it), it->temp().type);
            }

            // add move to mapping
            mapping.add(moveFrom, moveTo);
        }

        mapping.order();
        if (DebugRegAlloc)
            mapping.dump();

        bool insertIntoPredecessor = successor->in.size() > 1;
        mapping.insertMoves(insertIntoPredecessor ? predecessor : successor, _function,
                            insertIntoPredecessor);

        if (DebugRegAlloc) {
            qDebug() << ".. done, result:";
            QTextStream qout(stdout, QIODevice::WriteOnly);
            IRPrinterWithPositions printer(&qout, _intervals);
            printer.print(predecessor);
            printer.print(successor);
            qout.flush();
        }
    }

    Temp *createTemp(Temp::Kind kind, int index, Type type) const
    {
        Q_ASSERT(index >= 0);
        Temp *t = _function->New<Temp>();
        t->init(kind, index);
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

        Move *store = _function->NewStmt<Move>();
        store->init(createTemp(Temp::StackSlot, spillSlot, type),
                    createTemp(Temp::PhysicalRegister, pReg, type));
        return store;
    }

    Move *generateUnspill(const Temp &t, int pReg) const
    {
        Q_ASSERT(pReg >= 0);
        int spillSlot = _assignedSpillSlots[t.index];
        Q_ASSERT(spillSlot != -1);
        Move *load = _function->NewStmt<Move>();
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

        if (_currentStmt != 0 && i->start() == usePosition(_currentStmt)) {
            Q_ASSERT(i->isSplitFromInterval());
            int pReg = platformRegister(*i);
            _loads.append(generateUnspill(i->temp(), pReg));
        }

        if (i->reg() != LifeTimeInterval::InvalidRegister &&
                (i->covers(defPosition(_currentStmt)) ||
                 i->covers(usePosition(_currentStmt)))) {
            int pReg = platformRegister(*i);
            t->kind = Temp::PhysicalRegister;
            t->index = pReg;
        } else {
            int stackSlot = _assignedSpillSlots[t->index];
            Q_ASSERT(stackSlot >= 0);
            t->kind = Temp::StackSlot;
            t->index = stackSlot;
        }
    }

    virtual void visitArgLocal(ArgLocal *) {}
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

    virtual void visitMove(Move *s)
    {
        if (Temp *t = s->target->asTemp())
            maybeGenerateSpill(t);

        s->source->accept(this);
        s->target->accept(this);
    }

    virtual void visitJump(Jump *) {}
    virtual void visitCJump(CJump *s) { s->cond->accept(this); }
    virtual void visitRet(Ret *s) { s->expr->accept(this); }
    virtual void visitPhi(Phi *s)
    {
        maybeGenerateSpill(s->targetTemp);
    }
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
    _lastAssignedRegister.assign(function->tempCount, LifeTimeInterval::InvalidRegister);
    _assignedSpillSlots.assign(function->tempCount, InvalidSpillSlot);
    _activeSpillSlots.resize(function->tempCount);

    if (DebugRegAlloc)
        qDebug() << "*** Running regalloc for function" << (function->name ? qPrintable(*function->name) : "NO NAME") << "***";

    _lifeTimeIntervals = opt.lifeTimeIntervals();

    _unhandled = _lifeTimeIntervals->intervals();
    _handled.reserve(_unhandled.size());

    _info.reset(new RegAllocInfo);
    _info->collect(function, _lifeTimeIntervals);

    if (DebugRegAlloc) {
        QTextStream qout(stdout, QIODevice::WriteOnly);
        qout << "Ranges:" << endl;
        QVector<LifeTimeInterval *> intervals = _unhandled;
        std::reverse(intervals.begin(), intervals.end());
        foreach (const LifeTimeInterval *r, intervals) {
            r->dump(qout);
            qout << endl;
        }
        _info->dump();
    }

    prepareRanges();

    linearScan();

    if (DebugRegAlloc)
        dump(function);

    std::sort(_handled.begin(), _handled.end(), LifeTimeInterval::lessThan);
    ResolutionPhase(_handled, _lifeTimeIntervals, function, _assignedSpillSlots, _normalRegisters, _fpRegisters).run();

    function->tempCount = *std::max_element(_assignedSpillSlots.begin(), _assignedSpillSlots.end()) + 1;

    if (DebugRegAlloc) {
        qDebug() << "*** Finished regalloc for function" << (function->name ? qPrintable(*function->name) : "NO NAME") << "***";
        qDebug() << "*** Result:";
        QTextStream qout(stdout, QIODevice::WriteOnly);
        IRPrinterWithPositions(&qout, _lifeTimeIntervals).print(function);
    }
}

static inline LifeTimeInterval createFixedInterval(int rangeCount)
{
    LifeTimeInterval i(rangeCount);
    i.setReg(0);

    Temp t;
    t.init(Temp::PhysicalRegister, 0);
    t.type = IR::SInt32Type;
    i.setTemp(t);

    return i;
}

LifeTimeInterval *RegisterAllocator::cloneFixedInterval(int reg, bool isFP, const LifeTimeInterval &original)
{
    LifeTimeInterval *lti = new LifeTimeInterval(original);
    _lifeTimeIntervals->add(lti);
    lti->setReg(reg);
    lti->setFixedInterval(true);

    Temp t;
    t.init(Temp::PhysicalRegister, reg);
    t.type = isFP ? IR::DoubleType : IR::SInt32Type;
    lti->setTemp(t);

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
        LifeTimeInterval *lti = cloneFixedInterval(reg, false, ltiWithCalls);
        _fixedRegisterRanges.append(lti);
        if (lti->isValid())
            _active.append(lti);
    }

    const int fpRegCount = _fpRegisters.size();
    _fixedFPRegisterRanges.reserve(fpRegCount);
    for (int fpReg = 0; fpReg < fpRegCount; ++fpReg) {
        LifeTimeInterval *lti = cloneFixedInterval(fpReg, true, ltiWithCalls);
        _fixedFPRegisterRanges.append(lti);
        if (lti->isValid())
            _active.append(lti);
    }
}

void RegisterAllocator::linearScan()
{
    while (!_unhandled.isEmpty()) {
        LifeTimeInterval *current = _unhandled.back();
        _unhandled.pop_back();
        int position = current->start();

        // check for intervals in active that are handled or inactive
        for (int i = 0; i < _active.size(); ) {
            LifeTimeInterval *it = _active.at(i);
            if (it->end() < position) {
                if (!it->isFixedInterval())
                    _handled += it;
                _active.remove(i);
            } else if (!it->covers(position)) {
                _inactive += it;
                _active.remove(i);
            } else {
                ++i;
            }
        }

        // check for intervals in inactive that are handled or active
        for (int i = 0; i < _inactive.size(); ) {
            LifeTimeInterval *it = _inactive.at(i);
            if (it->end() < position) {
                if (!it->isFixedInterval())
                    _handled += it;
                _inactive.remove(i);
            } else if (it->covers(position)) {
                if (it->reg() != LifeTimeInterval::InvalidRegister) {
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

        Q_ASSERT(!current->isFixedInterval());

#ifdef DEBUG_REGALLOC
        qDebug() << "** Position" << position;
#endif // DEBUG_REGALLOC

        if (_info->canHaveRegister(current->temp())) {
            tryAllocateFreeReg(*current, position);
            if (current->reg() == LifeTimeInterval::InvalidRegister)
                allocateBlockedReg(*current, position);
            if (current->reg() != LifeTimeInterval::InvalidRegister)
                _active += current;
        } else {
            assignSpillSlot(current->temp(), current->start(), current->end());
            _inactive += current;
            if (DebugRegAlloc)
                qDebug() << "*** allocating stack slot" << _assignedSpillSlots[current->temp().index]
                         << "for %" << current->temp().index << "as it cannot be loaded in a register";
        }
    }

    foreach (LifeTimeInterval *r, _active)
        if (!r->isFixedInterval())
            _handled.append(r);
    _active.clear();
    foreach (LifeTimeInterval *r, _inactive)
        if (!r->isFixedInterval())
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

static void longestAvailableReg(int *nextUses, int nextUseCount, int &reg, int &freeUntilPos_reg, int lastUse)
{
    reg = LifeTimeInterval::InvalidRegister;
    freeUntilPos_reg = 0;

    for (int candidate = 0, candidateEnd = nextUseCount; candidate != candidateEnd; ++candidate) {
        int fp = nextUses[candidate];
        if ((freeUntilPos_reg < lastUse && fp > freeUntilPos_reg)
                || (freeUntilPos_reg >= lastUse && fp >= lastUse && freeUntilPos_reg > fp)) {
            reg = candidate;
            freeUntilPos_reg = fp;
        }
    }
}

#define ALLOC_INTS_ON_STACK(ty, ptr, sz, val) \
    Q_ASSERT(sz > 0); \
    ty *ptr = reinterpret_cast<ty *>(alloca(sizeof(ty) * (sz))); \
    for (ty *it = ptr, *eit = ptr + (sz); it != eit; ++it) \
        *it = val;


void RegisterAllocator::tryAllocateFreeReg(LifeTimeInterval &current, const int position)
{
    Q_ASSERT(!current.isFixedInterval());
    Q_ASSERT(current.reg() == LifeTimeInterval::InvalidRegister);

    const bool needsFPReg = isFP(current.temp());
    const int freeUntilPosCount = needsFPReg ? _fpRegisters.size() : _normalRegisters.size();
    ALLOC_INTS_ON_STACK(int, freeUntilPos, freeUntilPosCount, INT_MAX);

    const bool isPhiTarget = _info->isPhiTarget(current.temp());
    for (Intervals::const_iterator i = _active.constBegin(), ei = _active.constEnd(); i != ei; ++i) {
        const LifeTimeInterval *it = *i;
        if (it->isFP() == needsFPReg) {
            if (!isPhiTarget && it->isFixedInterval() && !current.isSplitFromInterval()) {
                const int idx = indexOfRangeCoveringPosition(it->ranges(), position);
                if (it->ranges().at(idx).end == current.start()) {
                    if (it->ranges().size() > idx + 1)
                        freeUntilPos[it->reg()] = it->ranges().at(idx + 1).start;
                    continue;
                }
            }

            if (isPhiTarget || it->end() >= current.firstPossibleUsePosition(isPhiTarget))
                freeUntilPos[it->reg()] = 0; // mark register as unavailable
        }
    }

    for (Intervals::const_iterator i = _inactive.constBegin(), ei = _inactive.constEnd(); i != ei; ++i) {
        const LifeTimeInterval *it = *i;
        if (current.isSplitFromInterval() || it->isFixedInterval()) {
            if (it->isFP() == needsFPReg && it->reg() != LifeTimeInterval::InvalidRegister) {
                const int intersectionPos = nextIntersection(current, *it, position);
                if (!isPhiTarget && it->isFixedInterval() && current.end() == intersectionPos)
                    freeUntilPos[it->reg()] = qMin(freeUntilPos[it->reg()], intersectionPos + 1);
                else if (intersectionPos != -1)
                    freeUntilPos[it->reg()] = qMin(freeUntilPos[it->reg()], intersectionPos);
            }
        }
    }

    int reg = LifeTimeInterval::InvalidRegister;
    int freeUntilPos_reg = 0;

    foreach (const Temp &hint, _info->hints(current.temp())) {
        int candidate;
        if (hint.kind == Temp::PhysicalRegister)
            candidate = hint.index;
        else
            candidate = _lastAssignedRegister[hint.index];

        const int end = current.end();
        if (candidate != LifeTimeInterval::InvalidRegister) {
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

    if (reg == LifeTimeInterval::InvalidRegister)
        longestAvailableReg(freeUntilPos, freeUntilPosCount, reg, freeUntilPos_reg, current.end());

    if (freeUntilPos_reg == 0) {
        // no register available without spilling
        if (DebugRegAlloc)
            qDebug() << "*** no register available for %" << current.temp().index;
        return;
    } else if (current.end() < freeUntilPos_reg) {
        // register available for the whole interval
        if (DebugRegAlloc)
            qDebug() << "*** allocating register" << reg << "for the whole interval of %" << current.temp().index;
        current.setReg(reg);
        _lastAssignedRegister[current.temp().index] = reg;
    } else {
        // register available for the first part of the interval
        current.setReg(reg);
        _lastAssignedRegister[current.temp().index] = reg;
        if (DebugRegAlloc)
            qDebug() << "*** allocating register" << reg << "for the first part of interval of %" << current.temp().index;
        split(current, freeUntilPos_reg, true);
    }
}

void RegisterAllocator::allocateBlockedReg(LifeTimeInterval &current, const int position)
{
    Q_ASSERT(!current.isFixedInterval());
    Q_ASSERT(current.reg() == LifeTimeInterval::InvalidRegister);

    const bool isPhiTarget = _info->isPhiTarget(current.temp());
    if (isPhiTarget && !current.isSplitFromInterval()) {
        split(current, position + 1, true);
        _inactive.append(&current);
        return;
    }

    const bool needsFPReg = isFP(current.temp());
    const int nextUsePosCount = needsFPReg ? _fpRegisters.size() : _normalRegisters.size();
    ALLOC_INTS_ON_STACK(int, nextUsePos, nextUsePosCount, INT_MAX);
    QVector<LifeTimeInterval *> nextUseRangeForReg(nextUsePosCount, 0);

    const bool definedAtCurrentPosition = !current.isSplitFromInterval() && current.start() == position;

    for (Intervals::const_iterator i = _active.constBegin(), ei = _active.constEnd(); i != ei; ++i) {
        LifeTimeInterval &it = **i;
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

    for (Intervals::const_iterator i = _inactive.constBegin(), ei = _inactive.constEnd(); i != ei; ++i) {
        LifeTimeInterval &it = **i;
        if (current.isSplitFromInterval() || it.isFixedInterval()) {
            if (it.isFP() == needsFPReg && it.reg() != LifeTimeInterval::InvalidRegister) {
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
    longestAvailableReg(nextUsePos, nextUsePosCount, reg, nextUsePos_reg, current.end());

    if (current.start() > nextUsePos_reg) {
        // all other intervals are used before current, so it is best to spill current itself
        if (DebugRegAlloc) {
            QTextStream out(stderr, QIODevice::WriteOnly);
            out << "*** splitting current for range ";current.dump(out);out<<endl;
        }
        Q_ASSERT(!_info->useMustHaveReg(current.temp(), position));
        split(current, position + 1, true);
        _inactive.append(&current);
    } else {
        // spill intervals that currently block reg
        if (DebugRegAlloc) {
            QTextStream out(stderr, QIODevice::WriteOnly);
            out << "*** spilling intervals that block reg "<<reg<<" for interval ";current.dump(out);out<<endl;
        }
        current.setReg(reg);
        _lastAssignedRegister[current.temp().index] = reg;
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
        const LifeTimeInterval &fixedRegRange = needsFPReg ? *_fixedFPRegisterRanges.at(reg)
                                                           : *_fixedRegisterRanges.at(reg);
        int ni = nextIntersection(current, fixedRegRange, position);
        if (ni != -1) {
            if (DebugRegAlloc) {
                QTextStream out(stderr, QIODevice::WriteOnly);
                out << "***-- current range intersects with a fixed reg use at "<<ni<<", so splitting it."<<endl;
            }
            split(current, ni, true);
        }
    }
}

int RegisterAllocator::nextIntersection(const LifeTimeInterval &current,
                                        const LifeTimeInterval &another, const int position) const
{
    const LifeTimeInterval::Ranges &currentRanges = current.ranges();
    int currentIt = indexOfRangeCoveringPosition(currentRanges, position);
    if (currentIt == -1)
        return -1;

    const LifeTimeInterval::Ranges &anotherRanges = another.ranges();
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

static inline void insertReverseSorted(QVector<LifeTimeInterval *> &intervals, LifeTimeInterval *newInterval)
{
    newInterval->validate();
    for (int i = intervals.size(); i > 0;) {
        if (LifeTimeInterval::lessThan(newInterval, intervals.at(--i))) {
            intervals.insert(i + 1, newInterval);
            return;
        }
    }
    intervals.insert(0, newInterval);
}

void RegisterAllocator::split(LifeTimeInterval &current, int beforePosition,
                              bool skipOptionalRegisterUses)
{ // TODO: check if we can always skip the optional register uses
    Q_ASSERT(!current.isFixedInterval());

    if (DebugRegAlloc) {
        QTextStream out(stderr, QIODevice::WriteOnly);
        out << "***** split request for range ";current.dump(out);out<<" before position "<<beforePosition<<" and skipOptionalRegisterUses = "<<skipOptionalRegisterUses<<endl;
    }

    assignSpillSlot(current.temp(), current.start(), current.end());

    const int firstPosition = current.start();
    Q_ASSERT(beforePosition > firstPosition && "split before start");

    int lastUse = firstPosition;
    int nextUse = -1;
    QList<Use> usePositions = _info->uses(current.temp());
    for (int i = 0, ei = usePositions.size(); i != ei; ++i) {
        const Use &usePosition = usePositions.at(i);
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
    Q_ASSERT(lastUse != -1);
    Q_ASSERT(lastUse < beforePosition);

    LifeTimeInterval newInterval = current.split(lastUse, nextUse);
    if (DebugRegAlloc) {
        QTextStream out(stderr, QIODevice::WriteOnly);
        out << "***** last use = "<<lastUse<<", nextUse = " << nextUse<<endl;
        out << "***** new interval: "; newInterval.dump(out); out << endl;
        out << "***** preceding interval: "; current.dump(out); out << endl;
    }
    if (newInterval.isValid()) {
        if (current.reg() != LifeTimeInterval::InvalidRegister)
            _info->addHint(current.temp(), current.reg());
        newInterval.setReg(LifeTimeInterval::InvalidRegister);
        LifeTimeInterval *newIntervalPtr = new LifeTimeInterval(newInterval);
        _lifeTimeIntervals->add(newIntervalPtr);
        insertReverseSorted(_unhandled, newIntervalPtr);
    }
}

void RegisterAllocator::splitInactiveAtEndOfLifetimeHole(int reg, bool isFPReg, int position)
{
    for (int i = 0, ei = _inactive.size(); i != ei; ++i) {
        LifeTimeInterval &interval = *_inactive[i];
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
    if (_assignedSpillSlots[t.index] != InvalidSpillSlot)
        return;

    for (int i = 0, ei = _activeSpillSlots.size(); i != ei; ++i) {
        if (_activeSpillSlots.at(i) < startPos) {
            _activeSpillSlots[i] = endPos;
            _assignedSpillSlots[t.index] = i;
            return;
        }
    }

    Q_UNREACHABLE();
}

void RegisterAllocator::dump(IR::Function *function) const
{
    QTextStream qout(stdout, QIODevice::WriteOnly);
    IRPrinterWithPositions printer(&qout, _lifeTimeIntervals);

    qout << "Ranges:" << endl;
    QVector<LifeTimeInterval *> handled = _handled;
    std::sort(handled.begin(), handled.end(), LifeTimeInterval::lessThanForTemp);
    foreach (const LifeTimeInterval *r, handled) {
        r->dump(qout);
        qout << endl;
    }

    qout << "Spill slots:" << endl;
    for (unsigned i = 0; i < _assignedSpillSlots.size(); ++i)
        if (_assignedSpillSlots[i] != InvalidSpillSlot)
            qout << "\t%" << i << " -> " << _assignedSpillSlots[i] << endl;

    printer.print(function);
}
