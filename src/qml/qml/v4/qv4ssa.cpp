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

#include "qv4ssa_p.h"
#include "qv4util_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QSet>
#include <QtCore/QBuffer>
#include <QtCore/QBitArray>
#include <QtCore/QLinkedList>
#include <QtCore/QStack>
#include <qv4runtime_p.h>
#include <qv4context_p.h>
#include <cmath>
#include <iostream>
#include <cassert>

#ifdef CONST
#undef CONST
#endif

#define QV4_NO_LIVENESS
#undef SHOW_SSA

using namespace QQmlJS;

namespace {
QTextStream qout(stdout, QIODevice::WriteOnly);

void showMeTheCode(V4IR::Function *function)
{
    // TODO: maybe we should move this somewhere else?
    static bool showCode = !qgetenv("SHOW_CODE").isNull();
    if (showCode) {
        QVector<V4IR::Stmt *> code;
        QHash<V4IR::Stmt *, V4IR::BasicBlock *> leader;

        foreach (V4IR::BasicBlock *block, function->basicBlocks) {
            if (block->statements.isEmpty())
                continue;
            leader.insert(block->statements.first(), block);
            foreach (V4IR::Stmt *s, block->statements) {
                code.append(s);
            }
        }

        QString name;
        if (function->name && !function->name->isEmpty())
            name = *function->name;
        else
            name.sprintf("%p", function);

        qout << "function " << name << "(";
        for (int i = 0; i < function->formals.size(); ++i) {
            if (i != 0)
                qout << ", ";
            qout << *function->formals.at(i);
        }
        qout << ")" << endl
             << "{" << endl;

        foreach (const QString *local, function->locals) {
            qout << "    var " << *local << ';' << endl;
        }

        for (int i = 0; i < code.size(); ++i) {
            V4IR::Stmt *s = code.at(i);

            if (V4IR::BasicBlock *bb = leader.value(s)) {
                qout << endl;
                QByteArray str;
                str.append('L');
                str.append(QByteArray::number(bb->index));
                str.append(':');
                for (int i = 66 - str.length(); i; --i)
                    str.append(' ');
                qout << str;
                qout << "// predecessor blocks:";
                foreach (V4IR::BasicBlock *in, bb->in)
                    qout << " L" << in->index;
                qout << endl;
            }
            V4IR::Stmt *n = (i + 1) < code.size() ? code.at(i + 1) : 0;
//            if (n && s->asJump() && s->asJump()->target == leader.value(n)) {
//                continue;
//            }

            QByteArray str;
            QBuffer buf(&str);
            buf.open(QIODevice::WriteOnly);
            QTextStream out(&buf);
            s->dump(out, V4IR::Stmt::MIR);
            out.flush();

            if (s->location.isValid())
                qout << "    // line: " << s->location.startLine << " column: " << s->location.startColumn << endl;

#ifndef QV4_NO_LIVENESS
            for (int i = 60 - str.size(); i >= 0; --i)
                str.append(' ');

            qout << "    " << str;

            //        if (! s->uses.isEmpty()) {
            //            qout << " // uses:";
            //            foreach (unsigned use, s->uses) {
            //                qout << " %" << use;
            //            }
            //        }

            //        if (! s->defs.isEmpty()) {
            //            qout << " // defs:";
            //            foreach (unsigned def, s->defs) {
            //                qout << " %" << def;
            //            }
            //        }

#  if 0
            if (! s->d->liveIn.isEmpty()) {
                qout << " // lives in:";
                for (int i = 0; i < s->d->liveIn.size(); ++i) {
                    if (s->d->liveIn.testBit(i))
                        qout << " %" << i;
                }
            }
#  else
            if (! s->d->liveOut.isEmpty()) {
                qout << " // lives out:";
                for (int i = 0; i < s->d->liveOut.size(); ++i) {
                    if (s->d->liveOut.testBit(i))
                        qout << " %" << i;
                }
            }
#  endif
#else
            qout << "    " << str;
#endif

            qout << endl;

            if (n && s->asCJump() /*&& s->asCJump()->iffalse != leader.value(n)*/) {
                qout << "    else goto L" << s->asCJump()->iffalse->index << ";" << endl;
            }
        }

        qout << "}" << endl
             << endl;
    }
}

using namespace V4IR;

class DominatorTree {
    int N = 0;
    QHash<BasicBlock *, int> dfnum;
    QVector<BasicBlock *> vertex;
    QHash<BasicBlock *, BasicBlock *> parent;
    QHash<BasicBlock *, BasicBlock *> ancestor;
    QHash<BasicBlock *, BasicBlock *> best;
    QHash<BasicBlock *, BasicBlock *> semi;
    QHash<BasicBlock *, BasicBlock *> idom;
    QHash<BasicBlock *, BasicBlock *> samedom;
    QHash<BasicBlock *, QSet<BasicBlock *> > bucket;

    void DFS(BasicBlock *p, BasicBlock *n) {
        if (dfnum[n] == 0) {
            dfnum[n] = N;
            vertex[N] = n;
            parent[n] = p;
            ++N;
            foreach (BasicBlock *w, n->out)
                DFS(n, w);
        }
    }

    BasicBlock *ancestorWithLowestSemi(BasicBlock *v) {
        BasicBlock *a = ancestor[v];
        if (ancestor[a]) {
            BasicBlock *b = ancestorWithLowestSemi(a);
            ancestor[v] = ancestor[a];
            if (dfnum[semi[b]] < dfnum[semi[best[v]]])
                best[v] = b;
        }
        return best[v];
    }

    void link(BasicBlock *p, BasicBlock *n) {
        ancestor[n] = p;
        best[n] = n;
    }

    void calculateIDoms(const QVector<BasicBlock *> &nodes) {
        Q_ASSERT(nodes.first()->in.isEmpty());
        vertex.resize(nodes.size());
        foreach (BasicBlock *n, nodes) {
            dfnum[n] = 0;
            semi[n] = 0;
            ancestor[n] = 0;
            idom[n] = 0;
            samedom[n] = 0;
        }

        DFS(0, nodes.first());
        Q_ASSERT(N == nodes.size()); // fails with unreachable nodes...

        for (int i = N - 1; i > 0; --i) {
            BasicBlock *n = vertex[i];
            BasicBlock *p = parent[n];
            BasicBlock *s = p;

            foreach (BasicBlock *v, n->in) {
                BasicBlock *ss;
                if (dfnum[v] <= dfnum[n])
                    ss = v;
                else
                    ss = semi[ancestorWithLowestSemi(v)];
                if (dfnum[ss] < dfnum[s])
                    s = ss;
            }
            semi[n] = s;
            bucket[s].insert(n);
            link(p, n);
            foreach (BasicBlock *v, bucket[p]) {
                BasicBlock *y = ancestorWithLowestSemi(v);
                Q_ASSERT(semi[y] == p);
                if (semi[y] == semi[v])
                    idom[v] = p;
                else
                    samedom[v] = y;
            }
            bucket[p].clear();
        }
        for (int i = 1; i < N; ++i) {
            BasicBlock *n = vertex[i];
            Q_ASSERT(ancestor[n] && ((semi[n] && dfnum[ancestor[n]] <= dfnum[semi[n]]) || semi[n] == n));
            Q_ASSERT(bucket[n].isEmpty());
            if (BasicBlock *sdn = samedom[n])
                idom[n] = idom[sdn];
        }

#ifdef SHOW_SSA
        qout << "Immediate dominators:" << endl;
        foreach (BasicBlock *to, nodes) {
            qout << '\t';
            if (BasicBlock *from = idom.value(to))
                qout << from->index;
            else
                qout << "(none)";
            qout << " -> " << to->index << endl;
        }
#endif // SHOW_SSA
    }

    bool dominates(BasicBlock *dominator, BasicBlock *dominated) const {
        for (BasicBlock *it = dominated; it; it = idom[it]) {
            if (it == dominator)
                return true;
        }

        return false;
    }

    void computeDF(BasicBlock *n) {
        if (DF.contains(n))
            return; // TODO: verify this!

        QSet<BasicBlock *> S;
        foreach (BasicBlock *y, n->out)
            if (idom[y] != n)
                S.insert(y);

        /*
         * foreach child c of n in the dominator tree
         *   computeDF[c]
         *   foreach element w of DF[c]
         *     if n does not dominate w or if n = w
         *       S.insert(w)
         * DF[n] = S;
         */
        foreach (BasicBlock *c, children[n]) {
            computeDF(c);
            foreach (BasicBlock *w, DF[c])
                if (!dominates(n, w) || n == w)
                    S.insert(w);
        }
        DF[n] = S;

#ifdef SHOW_SSA
        qout << "\tDF[" << n->index << "]: {";
        QList<BasicBlock *> SList = S.values();
        for (int i = 0; i < SList.size(); ++i) {
            if (i > 0)
                qout << ", ";
            qout << SList[i]->index;
        }
        qout << "}" << endl;
#endif // SHOW_SSA
#ifndef QT_NO_DEBUG
        foreach (BasicBlock *fBlock, S) {
            Q_ASSERT(!dominates(n, fBlock) || fBlock == n);
            bool hasDominatedSucc = false;
            foreach (BasicBlock *succ, fBlock->in)
                if (dominates(n, succ))
                    hasDominatedSucc = true;
            if (!hasDominatedSucc) {
                qout << fBlock->index << " in DF[" << n->index << "] has no dominated predecessors" << endl;
            }
            Q_ASSERT(hasDominatedSucc);
        }
#endif // !QT_NO_DEBUG
    }

    QHash<BasicBlock *, QSet<BasicBlock *> > children;
    QHash<BasicBlock *, QSet<BasicBlock *> > DF;

public:
    DominatorTree(const QVector<BasicBlock *> &nodes) {
        calculateIDoms(nodes);

        // compute children of n
        foreach (BasicBlock *n, nodes)
            children[idom[n]].insert(n);

#ifdef SHOW_SSA
        qout << "Dominator Frontiers:" << endl;
#endif // SHOW_SSA
        foreach (BasicBlock *n, nodes)
            computeDF(n);
    }

    QSet<BasicBlock *> operator[](BasicBlock *n) const {
        return DF[n];
    }
};

class VariableCollector: public StmtVisitor, ExprVisitor {
    QHash<int, QSet<BasicBlock *> > _defsites;
    QHash<BasicBlock *, QSet<int> > A_orig;
    QSet<int> nonLocals;
    QSet<int> killed;

    BasicBlock *currentBB;
    int lastUncollectible;

public:
    VariableCollector(Function *function)
        : lastUncollectible(function->variablesCanEscape() ? function->locals.size() - 1 : -1)
    {
#ifdef SHOW_SSA
        qout << "Variables collected:" << endl;
#endif // SHOW_SSA

        if (!function->variablesCanEscape() && false) {
            BasicBlock *entryBlock = function->basicBlocks.at(0);
            for (int i = -function->formals.size(); i < 0; ++i) {
                _defsites[i].insert(entryBlock);
                A_orig[entryBlock].insert(i);
#ifdef SHOW_SSA
                qout << "\t#" << -i << " -> L0" << endl;
#endif // SHOW_SSA
            }
        }

        foreach (BasicBlock *bb, function->basicBlocks) {
            currentBB = bb;
            killed.clear();
            killed.reserve(bb->statements.size() / 2);
            foreach (Stmt *s, bb->statements) {
                s->accept(this);
            }
        }

#ifdef SHOW_SSA
        qout << "Non-locals:" << endl;
        foreach (int nonLocal, nonLocals)
            qout << "\t" << nonLocal << endl;

        qout << "end collected variables." << endl;
#endif // SHOW_SSA
    }

    QList<int> vars() const {
        return _defsites.keys();
    }

    QSet<BasicBlock *> defsite(int n) const {
        return _defsites[n];
    }

    QSet<int> inBlock(BasicBlock *n) const {
        return A_orig[n];
    }

    bool isNonLocal(int var) const { return nonLocals.contains(var); }

protected:
    virtual void visitPhi(Phi *) {};

    virtual void visitConst(Const *) {}
    virtual void visitString(String *) {}
    virtual void visitRegExp(RegExp *) {}
    virtual void visitName(Name *) {}
    virtual void visitClosure(Closure *) {}
    virtual void visitUnop(V4IR::Unop *e) { e->expr->accept(this); }
    virtual void visitBinop(V4IR::Binop *e) { e->left->accept(this); e->right->accept(this); }
    virtual void visitSubscript(V4IR::Subscript *e) { e->base->accept(this); e->index->accept(this); }
    virtual void visitMember(V4IR::Member *e) { e->base->accept(this); }
    virtual void visitExp(V4IR::Exp *s) { s->expr->accept(this); }
    virtual void visitEnter(V4IR::Enter *s) { s->expr->accept(this); }
    virtual void visitLeave(V4IR::Leave *) {}
    virtual void visitJump(V4IR::Jump *) {}
    virtual void visitCJump(V4IR::CJump *s) { s->cond->accept(this); }
    virtual void visitRet(V4IR::Ret *s) { s->expr->accept(this); }
    virtual void visitTry(V4IR::Try *) { // ### TODO
    }

    virtual void visitCall(V4IR::Call *e) {
        e->base->accept(this);
        for (V4IR::ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }

    virtual void visitNew(V4IR::New *e) {
        e->base->accept(this);
        for (V4IR::ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }

    virtual void visitMove(V4IR::Move *s) {
        s->source->accept(this);

        if (Temp *t = s->target->asTemp()) {
            if (t->scope == 0 && lastUncollectible < t->index) {
#ifdef SHOW_SSA
                qout << '\t';
                t->dump(qout);
                qout << " -> L" << currentBB->index << endl;
#endif // SHOW_SSA

                _defsites[t->index].insert(currentBB);
                A_orig[currentBB].insert(t->index);

                // For semi-pruned SSA:
                killed.insert(t->index);
            }
        }
    }

    virtual void visitTemp(Temp *t)
    {
        if (t->scope == 0 && lastUncollectible < t->index)
            if (!killed.contains(t->index))
                nonLocals.insert(t->index);
    }
};

void insertPhiNode(int a, BasicBlock *y, Function *f) {
#ifdef SHOW_SSA
    qout << "-> inserted phi node for variable " << a << " in block " << y->index << endl;
#endif

    Phi *phiNode = f->New<Phi>();
    phiNode->targetTemp = f->New<Temp>();
    phiNode->targetTemp->init(a);
    y->statements.prepend(phiNode);

    phiNode->incoming.resize(y->in.size());
    for (int i = 0, ei = y->in.size(); i < ei; ++i) {
        Temp *t = f->New<Temp>();
        t->init(a);
        phiNode->incoming[i] = t;
    }
}

class VariableRenamer: public StmtVisitor, public ExprVisitor
{
    Function *function;
    QHash<int, QStack<int> > stack;
    QSet<BasicBlock *> seen;

    QHash<int, unsigned> defCounts;
    QHash<int, int> tempMapping;

    int lastUncollectible;

    int nextFreeTemp() {
        const int next = function->tempCount++;
//        qDebug()<<"Next free temp:"<<next;
        return next;
    }

    /*

    Initialization:
      for each variable a
        count[a] = 0;
        stack[a] = empty;
        push 0 onto stack

    Rename(n) =
      for each statement S in block n [1]
        if S not in a phi-function
          for each use of some variable x in S
            i = top(stack[x])
            replace the use of x with x_i in S
        for each definition of some variable a in S
          count[a] = count[a] + 1
          i = count[a]
          push i onto stack[a]
          replace definition of a with definition of a_i in S
      for each successor Y of block n [2]
        Suppose n is the j-th predecessor of Y
        for each phi function in Y
          suppose the j-th operand of the phi-function is a
          i = top(stack[a])
          replace the j-th operand with a_i
      for each child X of n [3]
        Rename(X)
      for each statement S in block n [4]
        for each definition of some variable a in S
          pop stack[a]

     */

public:
    VariableRenamer(Function *f) {
        function = f;

        int a = f->variablesCanEscape() ? f->locals.size() : a = 0;
        lastUncollectible = a - 1;

        for (; a < f->tempCount; ++a) {
            stack[a].push(a);
        }
    }

    QHash<int, int> run() {
        foreach (BasicBlock *n, function->basicBlocks)
            rename(n);

#ifdef SHOW_SSA
        qout << "Temp to local mapping:" << endl;
        foreach (int key, tempMapping.keys())
            qout << '\t' << key << " -> " << tempMapping[key] << endl;
#endif

        return tempMapping;
    }

    void rename(BasicBlock *n) {
        if (seen.contains(n))
            return;
        seen.insert(n);
//        qDebug() << "I: L"<<n->index;

        // [1]:
        foreach (Stmt *s, n->statements)
            s->accept(this);

        QHash<int, unsigned> dc = defCounts;
        defCounts.clear();

        // [2]:
        foreach (BasicBlock *Y, n->out) {
            const int j = Y->in.indexOf(n);
            Q_ASSERT(j >= 0 && j < Y->in.size());
            foreach (Stmt *s, Y->statements) {
                if (Phi *phi = s->asPhi()) {
                    int &a = phi->incoming[j]->asTemp()->index;
                    int newTmp = stack[a].top();
//                    qDebug()<<"I: replacing phi use"<<a<<"with"<<newTmp<<"in L"<<Y->index;
                    a = newTmp;
                } else {
                    break;
                }
            }
        }

        // [3]:
        foreach (BasicBlock *X, n->out)
            rename(X);

        // [4]:
        for (QHash<int, unsigned>::const_iterator i = dc.begin(), ei = dc.end(); i != ei; ++i) {
//            qDebug()<<i.key() <<" -> " << i.value();
            for (unsigned j = 0, ej = i.value(); j < ej; ++j)
                stack[i.key()].pop();
        }
    }

protected:
    virtual void visitTemp(Temp *e) { // only called for uses, not defs
        if (e->scope == 0 && lastUncollectible < e->index) {
//            qDebug()<<"I: replacing use of"<<e->index<<"with"<<stack[e->index].top();
            e->index = stack[e->index].top();
        }
    }

    virtual void visitMove(Move *s) {
        // uses:
        s->source->accept(this);

        // defs:
        if (Temp *t = s->target->asTemp())
            renameTemp(t);
        else
            s->target->accept(this);
    }

    void renameTemp(Temp *t) {
        if (t->scope == 0 && lastUncollectible < t->index) {
            int &a = t->index;
            defCounts[a] = defCounts.value(a, 0) + 1;
            const int newIdx = nextFreeTemp();
            stack[a].push(newIdx);
            updateLocalMapping(a, newIdx);
//            qDebug()<<"I: replacing def of"<<a<<"with"<<newIdx;
            a = newIdx;
        }
    }

    void updateLocalMapping(int origIdx, int newIdx) {
        if (origIdx < function->locals.size()) {
            tempMapping[newIdx] = origIdx;
            return;
        }

        int next = tempMapping.value(newIdx, INT32_MIN);
        if (next == INT32_MIN)
            return;
        tempMapping[newIdx] = origIdx;
    }

    virtual void visitPhi(Phi *s) { renameTemp(s->targetTemp); }

    virtual void visitExp(Exp *s) { s->expr->accept(this); }
    virtual void visitEnter(Enter *) { Q_UNIMPLEMENTED(); abort(); }
    virtual void visitLeave(Leave *) { Q_UNIMPLEMENTED(); abort(); }

    virtual void visitJump(Jump *) {}
    virtual void visitCJump(CJump *s) { s->cond->accept(this); }
    virtual void visitRet(Ret *s) { s->expr->accept(this); }
    virtual void visitTry(Try *s) { /* this should never happen */ }

    virtual void visitConst(Const *) {}
    virtual void visitString(String *) {}
    virtual void visitRegExp(RegExp *) {}
    virtual void visitName(Name *) {}
    virtual void visitClosure(Closure *) {}
    virtual void visitUnop(Unop *e) { e->expr->accept(this); }
    virtual void visitBinop(Binop *e) { e->left->accept(this); e->right->accept(this); }
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

    virtual void visitSubscript(Subscript *e) {
        e->base->accept(this);
        e->index->accept(this);
    }

    virtual void visitMember(Member *e) {
        e->base->accept(this);
    }
};

QHash<int, int> convertToSSA(Function *function)
{
#ifdef SHOW_SSA
    qout << "Converting function ";
    if (function->name)
        qout << *function->name;
    else
        qout << "<no name>";
    qout << " to SSA..." << endl;
#endif // SHOW_SSA

    QVector<BasicBlock *> &nodes = function->basicBlocks;

    // Calculate the dominator tree:
    DominatorTree df(nodes);

    // Collect all applicable variables:
    VariableCollector variables(function);

    // Place phi functions:
    QHash<BasicBlock *, QSet<int> > A_phi;
    foreach (int a, variables.vars()) {
        if (!variables.isNonLocal(a))
            continue; // for semi-pruned SSA

        QList<BasicBlock *> W = QList<BasicBlock *>::fromSet(variables.defsite(a));
        while (!W.isEmpty()) {
            BasicBlock *n = W.first();
            W.removeFirst();
            foreach (BasicBlock *y, df[n]) {
                if (!A_phi[y].contains(a)) {
                    insertPhiNode(a, y, function);
                    A_phi[y].insert(a);
                    if (!variables.inBlock(y).contains(a))
                        W.append(y);
                }
            }
        }
    }

    // Rename variables:
    return VariableRenamer(function).run();
}

class DefUsesCalculator: public StmtVisitor, public ExprVisitor {
public:
    struct DefUse {
        Stmt *defStmt;
        BasicBlock *blockOfStatement;
        QList<Stmt *> uses;
    };

private:
    int _lastUncollectible;
    QHash<int, DefUse> _defUses;
    QHash<Stmt *, QList<int> > _usesPerStatement;

    BasicBlock *_block;
    Stmt *_stmt;

    void addUse(Temp *t) {
        Q_ASSERT(t);
        if (t->scope || t->index <= _lastUncollectible)
            return;

        _defUses[t->index].uses.append(_stmt);
        _usesPerStatement[_stmt].append(t->index);
    }

    void addDef(Temp *t) {
        if (t->scope || t->index <= _lastUncollectible)
            return;

        Q_ASSERT(!_defUses.contains(t->index) || _defUses.value(t->index).defStmt == 0 || _defUses.value(t->index).defStmt == _stmt);

        DefUse &defUse = _defUses[t->index];
        defUse.defStmt = _stmt;
        defUse.blockOfStatement = _block;
    }

public:
    DefUsesCalculator(Function *function) {
        if (function->variablesCanEscape())
            _lastUncollectible = function->locals.size() - 1;
        else
            _lastUncollectible = -1;

        foreach (BasicBlock *bb, function->basicBlocks) {
            _block = bb;
            foreach (Stmt *stmt, bb->statements) {
                _stmt = stmt;
                stmt->accept(this);
            }
        }

        QMutableHashIterator<int, DefUse> it(_defUses);
        while (it.hasNext()) {
            it.next();
            if (!it.value().defStmt)
                it.remove();
        }
    }

    QList<int> defs() const {
        return _defUses.keys();
    }

    void removeDef(int var) {
        _defUses.remove(var);
    }

    void addUses(int variable, QList<Stmt *>newUses)
    { _defUses[variable].uses.append(newUses); }

    int useCount(int variable) const
    { return _defUses[variable].uses.size(); }

    Stmt *defStmt(int variable) const
    { return _defUses[variable].defStmt; }

    BasicBlock *defStmtBlock(int variable) const
    { return _defUses[variable].blockOfStatement; }

    void removeUse(Stmt *usingStmt, int var)
    { _defUses[var].uses.removeAll(usingStmt); }

    QList<int> usedVars(Stmt *s) const
    { return _usesPerStatement[s]; }

    QList<Stmt *> uses(int var) const
    { return _defUses[var].uses; }

    void dump() const
    {
        foreach (int var, _defUses.keys()) {
            const DefUse &du = _defUses[var];
            qout<<var<<" -> defined in block "<<du.blockOfStatement->index<<", statement: ";
            du.defStmt->dump(qout);
            qout<<endl<<"     uses:"<<endl;
            foreach (Stmt *s, du.uses) {
                qout<<"       ";s->dump(qout);qout<<endl;
            }
        }
    }

protected:
    virtual void visitExp(Exp *s) { s->expr->accept(this); }
    virtual void visitEnter(Enter *) {}
    virtual void visitLeave(Leave *) {}
    virtual void visitJump(Jump *) {}
    virtual void visitCJump(CJump *s) { s->cond->accept(this); }
    virtual void visitRet(Ret *s) { s->expr->accept(this); }
    virtual void visitTry(Try *) {}

    virtual void visitPhi(Phi *s) {
        addDef(s->targetTemp);
        foreach (Expr *e, s->incoming)
            addUse(e->asTemp());
    }

    virtual void visitMove(Move *s) {
        if (Temp *t = s->target->asTemp())
            addDef(t);
        else
            s->target->accept(this);

        s->source->accept(this);
    }

    virtual void visitTemp(Temp *e) { addUse(e); }

    virtual void visitConst(Const *) {}
    virtual void visitString(String *) {}
    virtual void visitRegExp(RegExp *) {}
    virtual void visitName(Name *) {}
    virtual void visitClosure(Closure *) {}
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
};

bool hasPhiOnlyUses(Phi *phi, const DefUsesCalculator &defUses, QSet<Phi *> &collectedPhis)
{
    collectedPhis.insert(phi);
    foreach (Stmt *use, defUses.uses(phi->targetTemp->index)) {
        if (Phi *dependentPhi = use->asPhi()) {
            if (!collectedPhis.contains(dependentPhi)) {
                if (!hasPhiOnlyUses(dependentPhi, defUses, collectedPhis))
                    return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

void cleanupPhis(DefUsesCalculator &defUses)
{
    QLinkedList<Phi *> phis;
    foreach (int def, defUses.defs())
        if (Phi *phi = defUses.defStmt(def)->asPhi())
            phis.append(phi);

    QSet<Phi *> toRemove;
    while (!phis.isEmpty()) {
        Phi *phi = phis.first();
        phis.removeFirst();
        if (toRemove.contains(phi))
            continue;
        QSet<Phi *> collectedPhis;
        if (hasPhiOnlyUses(phi, defUses, collectedPhis))
            toRemove.unite(collectedPhis);
    }

    foreach (Phi *phi, toRemove) {
        int targetVar = phi->targetTemp->index;

        BasicBlock *bb = defUses.defStmtBlock(targetVar);
        int idx = bb->statements.indexOf(phi);
        bb->statements.remove(idx);

        foreach (int usedVar, defUses.usedVars(phi))
            defUses.removeUse(phi, usedVar);
        defUses.removeDef(targetVar);
    }
}

class DeadCodeElimination: public ExprVisitor {
    int _lastUncollectible;
    DefUsesCalculator &_defUses;
    QVector<int> _worklist;

public:
    DeadCodeElimination(DefUsesCalculator &defUses, Function *function)
        : _defUses(defUses)
    {
        _worklist = QVector<int>::fromList(_defUses.defs());

        if (function->variablesCanEscape())
            _lastUncollectible = function->locals.size() - 1;
        else
            _lastUncollectible = -1;
    }

    void run() {
        while (!_worklist.isEmpty()) {
            const int v = _worklist.first();
            _worklist.removeFirst();

            if (_defUses.useCount(v) == 0) {
//                qDebug()<<"-"<<v<<"has no uses...";
                Stmt *s = _defUses.defStmt(v);
                if (!s) {
                    _defUses.removeDef(v);
                } else if (!hasSideEffect(s)) {
//                    qDebug()<<"-- defining stmt for"<<v<<"has no side effect";
                    QVector<Stmt *> &stmts = _defUses.defStmtBlock(v)->statements;
                    int idx = stmts.indexOf(s);
                    if (idx != -1)
                        stmts.remove(idx);
                    foreach (int usedVar, _defUses.usedVars(s)) {
                        _defUses.removeUse(s, usedVar);
                        _worklist.append(usedVar);
                    }
                    _defUses.removeDef(v);
                }
            }
        }

#ifdef SHOW_SSA
        qout<<"******************* After dead-code elimination:";
        _defUses.dump();
#endif
    }

private:
    bool _sideEffect;

    bool hasSideEffect(Stmt *s) {
        // TODO: check if this can be moved to IR building.
        _sideEffect = false;
        if (Move *move = s->asMove()) {
            if (Temp *t = move->target->asTemp())
                if (t->index <= _lastUncollectible || t->scope)
                    return true;
            move->source->accept(this);
        }
        return _sideEffect;
    }

protected:
    virtual void visitConst(Const *) {}
    virtual void visitString(String *) {}
    virtual void visitRegExp(RegExp *) {}
    virtual void visitName(Name *e) {
        // TODO: maybe we can distinguish between built-ins of which we know that they do not have
        // a side-effect.
        if (e->builtin == Name::builtin_invalid || (e->id && *e->id != QStringLiteral("this")))
            _sideEffect = true;
    }
    virtual void visitTemp(Temp *e) {
    }
    virtual void visitClosure(Closure *) {}
    virtual void visitUnop(Unop *e) {
        switch (e->op) {
        case V4IR::OpIncrement:
        case V4IR::OpDecrement:
            _sideEffect = true;
            break;

        default:
            break;
        }

        if (!_sideEffect) e->expr->accept(this);
    }
    virtual void visitBinop(Binop *e) { if (!_sideEffect) e->left->accept(this); if (!_sideEffect) e->right->accept(this); }
    virtual void visitSubscript(Subscript *e) {
        // TODO: see if we can have subscript accesses without side effect
        _sideEffect = true;
    }
    virtual void visitMember(Member *e) {
        // TODO: see if we can have member accesses without side effect
        _sideEffect = true;
    }
    virtual void visitCall(Call *e) {
        _sideEffect = true; // TODO: there are built-in functions that have no side effect.
    }
    virtual void visitNew(New *e) {
        _sideEffect = true; // TODO: there are built-in types that have no side effect.
    }
};

class TypeInference: public StmtVisitor, public ExprVisitor {
    const DefUsesCalculator &_defUses;
    QHash<int, int> _tempTypes;
    QSet<Stmt *> _worklist;
    struct TypingResult {
        int type;
        bool fullyTyped;

        TypingResult(int type, bool fullyTyped): type(type), fullyTyped(fullyTyped) {}
        explicit TypingResult(int type = UnknownType): type(type), fullyTyped(type != UnknownType) {}
    };
    TypingResult _ty;
    int _localCount;

public:
    TypeInference(const DefUsesCalculator &defUses): _defUses(defUses), _ty(UnknownType) {}

    void run(Function *function) {
        _localCount = function->variablesCanEscape() ? function->locals.size() : 0;

        // TODO: the worklist handling looks a bit inefficient... check if there is something better
        _worklist.clear();
        for (int i = 0, ei = function->basicBlocks.size(); i != ei; ++i) {
            BasicBlock *bb = function->basicBlocks[i];
            if (i == 0 || !bb->in.isEmpty())
                foreach (Stmt *s, bb->statements)
                    _worklist.insert(s);
        }

        while (!_worklist.isEmpty()) {
            QList<Stmt *> worklist = _worklist.values();
            _worklist.clear();
            while (!worklist.isEmpty()) {
                Stmt *s = worklist.first();
                worklist.removeFirst();
#if defined(SHOW_SSA)
                qout<<"Typing stmt ";s->dump(qout);qout<<endl;
#endif

                if (!run(s)) {
                    _worklist.insert(s);
#if defined(SHOW_SSA)
                    qout<<"Pushing back stmt: ";
                    s->dump(qout);qout<<endl;
                } else {
                    qout<<"Finished: ";
                    s->dump(qout);qout<<endl;
#endif
                }
            }
        }
    }

private:
    bool run(Stmt *s) {
        TypingResult ty;
        std::swap(_ty, ty);
        s->accept(this);
        std::swap(_ty, ty);
        return ty.fullyTyped;
    }

    TypingResult run(Expr *e) {
        TypingResult ty;
        std::swap(_ty, ty);
        e->accept(this);
        std::swap(_ty, ty);

        if (ty.type != UnknownType)
            setType(e, ty.type);
        return ty;
    }

    void setType(Expr *e, int ty) {
        if (Temp *t = e->asTemp()) {
#if defined(SHOW_SSA)
            qout<<"Setting type for "<< (t->scope?"scoped temp ":"temp ") <<t->index<< " to "<<typeName(Type(ty)) << " (" << ty << ")" << endl;
#endif
            if (t->scope || t->index < _localCount) {
                e->type = ObjectType;
            } else {
                e->type = (Type) ty;

                if (_tempTypes[t->index] != ty) {
                    _tempTypes[t->index] = ty;

#if defined(SHOW_SSA)
                    foreach (Stmt *s, _defUses.uses(t->index)) {
                        qout << "Pushing back dependent stmt: ";
                        s->dump(qout);
                        qout << endl;
                    }
#endif

                    _worklist += QSet<Stmt *>::fromList(_defUses.uses(t->index));
                }
            }
        } else {
            e->type = (Type) ty;
        }
    }

protected:
    virtual void visitConst(Const *e) { _ty = TypingResult(e->type); }
    virtual void visitString(String *) { _ty = TypingResult(StringType); }
    virtual void visitRegExp(RegExp *) { _ty = TypingResult(ObjectType); }
    virtual void visitName(Name *) { _ty = TypingResult(ObjectType); }
    virtual void visitTemp(Temp *e) {
        if (e->scope)
            _ty = TypingResult(ObjectType);
        else if (e->index < _localCount)
            _ty = TypingResult(ObjectType);
        else
            _ty = TypingResult(_tempTypes.value(e->index, UnknownType));
        setType(e, _ty.type);
    }
    virtual void visitClosure(Closure *) { _ty = TypingResult(ObjectType); } // TODO: VERIFY THIS!
    virtual void visitUnop(Unop *e) {
        _ty = run(e->expr);
        switch (e->op) {
        case OpUPlus: _ty.type = DoubleType; return;
        case OpUMinus: _ty.type = DoubleType; return;
        case OpCompl: _ty.type = SInt32Type; return;
        case OpNot: _ty.type = BoolType; return;

        case OpIncrement:
        case OpDecrement:
            Q_ASSERT(!"Inplace operators should have been removed!");
        default:
            Q_UNIMPLEMENTED();
            Q_UNREACHABLE();
        }
    }

    virtual void visitBinop(Binop *e) {
        TypingResult leftTy = run(e->left);
        TypingResult rightTy = run(e->right);
        _ty.fullyTyped = leftTy.fullyTyped && rightTy.fullyTyped;

        switch (e->op) {
        case OpAdd:
            if (leftTy.type & StringType || rightTy.type & StringType)
                _ty.type = StringType;
            else if (leftTy.type != UnknownType && rightTy.type != UnknownType)
                _ty.type = DoubleType;
            else
                _ty.type = UnknownType;
            break;
        case OpSub:
            _ty.type = DoubleType;
            break;

        case OpMul:
        case OpDiv:
        case OpMod:
            _ty.type = DoubleType;
            break;

        case OpBitAnd:
        case OpBitOr:
        case OpBitXor:
        case OpLShift:
        case OpRShift:
            _ty.type = SInt32Type;
            break;
        case OpURShift:
            _ty.type = UInt32Type;
            break;

        case OpGt:
        case OpLt:
        case OpGe:
        case OpLe:
        case OpEqual:
        case OpNotEqual:
        case OpStrictEqual:
        case OpStrictNotEqual:
        case OpAnd:
        case OpOr:
        case OpInstanceof:
        case OpIn:
            _ty.type = BoolType;
            break;

        default:
            Q_UNIMPLEMENTED();
            Q_UNREACHABLE();
        }
    }

    virtual void visitCall(Call *e) {
        _ty = run(e->base);
        for (ExprList *it = e->args; it; it = it->next)
            _ty.fullyTyped &= run(it->expr).fullyTyped;
        _ty.type = ObjectType;
    }
    virtual void visitNew(New *e) {
        _ty = run(e->base);
        for (ExprList *it = e->args; it; it = it->next)
            _ty.fullyTyped &= run(it->expr).fullyTyped;
        _ty.type = ObjectType;
    }
    virtual void visitSubscript(Subscript *e) {
        _ty.fullyTyped = run(e->base).fullyTyped && run(e->index).fullyTyped;
        _ty.type = ObjectType;
    }

    virtual void visitMember(Member *e) {
        // TODO: for QML, try to do a static lookup
        _ty = run(e->base);
        _ty.type = ObjectType;
    }

    virtual void visitExp(Exp *s) { _ty = run(s->expr); }
    virtual void visitEnter(Enter *s) { _ty = run(s->expr); }
    virtual void visitLeave(Leave *) { _ty = TypingResult(MissingType); }
    virtual void visitMove(Move *s) {
        TypingResult sourceTy = run(s->source);
        Q_ASSERT(s->op == OpInvalid);
        if (Temp *t = s->target->asTemp()) {
            setType(t, sourceTy.type);
            _ty = sourceTy;
            return;
        }

        _ty = run(s->target);
        _ty.fullyTyped &= sourceTy.fullyTyped;
    }

    virtual void visitJump(Jump *) { _ty = TypingResult(MissingType); }
    virtual void visitCJump(CJump *s) { _ty = run(s->cond); }
    virtual void visitRet(Ret *s) { _ty = run(s->expr); }
    virtual void visitTry(Try *s) { setType(s->exceptionVar, ObjectType); _ty = TypingResult(MissingType); }
    virtual void visitPhi(Phi *s) {
        _ty = run(s->incoming[0]);
        for (int i = 1, ei = s->incoming.size(); i != ei; ++i) {
            TypingResult ty = run(s->incoming[i]);
            _ty.type |= ty.type;
            _ty.fullyTyped &= ty.fullyTyped;
        }

        // TODO: check & double check the next condition!
        if (_ty.type & ObjectType || _ty.type & UndefinedType || _ty.type & NullType)
            _ty.type = ObjectType;
        else if (_ty.type & NumberType)
            _ty.type = DoubleType;

        setType(s->targetTemp, _ty.type);
    }
};

class TypePropagation: public StmtVisitor, public ExprVisitor {
    Type _ty;

    void run(Expr *e, Type requestedType = UnknownType) {
        qSwap(_ty, requestedType);
        e->accept(this);
        qSwap(_ty, requestedType);
    }

public:
    TypePropagation() : _ty(UnknownType) {}

    void run(Function *f) {
        foreach (BasicBlock *bb, f->basicBlocks)
            foreach (Stmt *s, bb->statements)
                s->accept(this);
    }

protected:
    virtual void visitConst(Const *c) {
        if (_ty & NumberType && c->type & NumberType) {
            c->type = _ty;
        }
    }

    virtual void visitString(String *) {}
    virtual void visitRegExp(RegExp *) {}
    virtual void visitName(Name *) {}
    virtual void visitTemp(Temp *) {}
    virtual void visitClosure(Closure *) {}
    virtual void visitUnop(Unop *e) { run(e->expr, e->type); }
    virtual void visitBinop(Binop *e) { run(e->left, e->type); run(e->right, e->type); }
    virtual void visitCall(Call *e) {
        run(e->base);
        for (ExprList *it = e->args; it; it = it->next)
            run(it->expr);
    }
    virtual void visitNew(New *e) {
        run(e->base);
        for (ExprList *it = e->args; it; it = it->next)
            run(it->expr);
    }
    virtual void visitSubscript(Subscript *e) { run(e->base); run(e->index); }
    virtual void visitMember(Member *e) { run(e->base); }
    virtual void visitExp(Exp *s) { run(s->expr); }
    virtual void visitEnter(Enter *s) { run(s->expr); }
    virtual void visitLeave(Leave *) {}
    virtual void visitMove(Move *s) {
        run(s->target);
        run(s->source, s->target->type);
    }
    virtual void visitJump(Jump *) {}
    virtual void visitCJump(CJump *s) {
        run(s->cond, BoolType);
    }
    virtual void visitRet(Ret *s) { run(s->expr); }
    virtual void visitTry(Try *) {}
    virtual void visitPhi(Phi *s) {
        Type ty = s->targetTemp->type;
        foreach (Expr *e, s->incoming)
            run(e, ty);
    }
};

void insertMove(Function *function, BasicBlock *basicBlock, Temp *target, Expr *source) {
    Move *s = function->New<Move>();
    s->init(target, source, OpInvalid);
    basicBlock->statements.insert(basicBlock->statements.size() - 1, s);
}

bool doEdgeSplitting(Function *f)
{
    const QVector<BasicBlock *> oldBBs = f->basicBlocks;

    foreach (BasicBlock *bb, oldBBs) {
        if (bb->in.size() > 1) {
            for (int inIdx = 0, eInIdx = bb->in.size(); inIdx != eInIdx; ++inIdx) {
                BasicBlock *inBB = bb->in[inIdx];
                if (inBB->out.size() > 1) { // this should have been split!
#if defined(SHOW_SSA)
                    qDebug() << "Splitting edge from block" << inBB->index << "to block" << bb->index;
#endif

                    // create the basic block:
                    BasicBlock *newBB = new BasicBlock(f, bb->containingGroup());
                    newBB->index = f->basicBlocks.last()->index + 1;
                    f->basicBlocks.append(newBB);
                    Jump *s = f->New<Jump>();
                    s->init(bb);
                    newBB->statements.append(s);

                    // rewire the old outgoing edge
                    int outIdx = inBB->out.indexOf(bb);
                    inBB->out[outIdx] = newBB;
                    newBB->in.append(inBB);

                    // rewire the old incoming edge
                    bb->in[inIdx] = newBB;
                    newBB->out.append(bb);

                    // patch the terminator
                    Stmt *terminator = inBB->terminator();
                    if (Jump *j = terminator->asJump()) {
                        Q_ASSERT(outIdx == 0);
                        j->target = newBB;
                    } else if (CJump *j = terminator->asCJump()) {
                        if (outIdx == 0)
                            j->iftrue = newBB;
                        else if (outIdx == 1)
                            j->iffalse = newBB;
                        else
                            Q_ASSERT(!"Invalid out edge index for CJUMP!");
                    } else {
                        Q_ASSERT(!"Unknown terminator!");
                    }
                }
            }
        }
    }
}

void scheduleBlocks(QVector<BasicBlock *> &basicBlocks)
{
    // FIXME: this should not do DFS scheduling.
    struct I {
        QSet<BasicBlock *> visited;
        QVector<BasicBlock *> &sequence;

        I(QVector<BasicBlock *> &sequence): sequence(sequence) {}

        void DFS(BasicBlock *bb) {
            if (visited.contains(bb))
                return;
            visited.insert(bb);
            sequence.append(bb);
            if (Stmt *terminator = bb->terminator()) {
                if (Jump *j = terminator->asJump()) {
                    Q_ASSERT(bb->out.size() == 1);
                    DFS(j->target);
                } else if (CJump *cj = terminator->asCJump()) {
                    Q_ASSERT(bb->out.size() == 2);
                    DFS(cj->iftrue);
                    DFS(cj->iffalse);
                } else if (terminator->asRet()) {
                    Q_ASSERT(bb->out.size() == 0);
                    // nothing to do.
                } else {
                    Q_UNREACHABLE();
                }
            } else {
                Q_UNREACHABLE();
            }
        }
    };

    QVector<BasicBlock *> sequence;
    sequence.reserve(basicBlocks.size());
    I(sequence).DFS(basicBlocks.first());
    qSwap(basicBlocks, sequence);
}

/*
 * Quick function to convert out of SSA, so we can put the stuff through the ISel phases. This
 * has to be replaced by a phase in the specific ISel back-ends and do register allocation at the
 * same time. That way the huge number of redundant moves generated by this function are eliminated.
 */
void convertOutOfSSA(Function *function, const QHash<int, int> &tempMapping) {
    // We assume that edge-splitting is already done.
    foreach (BasicBlock *bb, function->basicBlocks) {
        QVector<Stmt *> &stmts = bb->statements;
        while (!stmts.isEmpty()) {
            Stmt *s = stmts.first();
            if (Phi *phi = s->asPhi()) {
                stmts.removeFirst();
                for (int i = 0, ei = phi->incoming.size(); i != ei; ++i)
                    insertMove(function, bb->in[i], phi->targetTemp, phi->incoming[i]);
            } else {
                break;
            }
        }
    }
}

void checkCriticalEdges(QVector<BasicBlock *> basicBlocks) {
    foreach (BasicBlock *bb, basicBlocks) {
        if (bb && bb->out.size() > 1) {
            foreach (BasicBlock *bb2, bb->out) {
                if (bb2 && bb2->in.size() > 1) {
                    qout << "found critical edge between block "
                         << bb->index << " and block " << bb2->index;
                    Q_ASSERT(false);
                }
            }
        }
    }
}

} // end of anonymous namespace



void QQmlJS::linearize(V4IR::Function *function)
{
#if defined(SHOW_SSA)
    qout << "##### NOW IN FUNCTION " << (function->name ? qPrintable(*function->name) : "anonymous!") << endl << flush;
#endif
#if 0
    V4IR::BasicBlock *exitBlock = function->basicBlocks.last();
    assert(exitBlock->isTerminated());
    assert(exitBlock->terminator()->asRet());

    QSet<V4IR::BasicBlock *> V;
    V.insert(exitBlock);

    QVector<V4IR::BasicBlock *> trace;

    for (int i = 0; i < function->basicBlocks.size(); ++i) {
        V4IR::BasicBlock *block = function->basicBlocks.at(i);
        if (!block->isTerminated() && (i + 1) < function->basicBlocks.size()) {
            V4IR::BasicBlock *next = function->basicBlocks.at(i + 1);
            block->JUMP(next);
        }
    }

    struct I { static void trace(V4IR::BasicBlock *block, QSet<V4IR::BasicBlock *> *V,
                                 QVector<V4IR::BasicBlock *> *output) {
            if (block == 0 || V->contains(block))
                return;

            V->insert(block);
            block->index = output->size();
            output->append(block);

            if (V4IR::Stmt *term = block->terminator()) {
                if (V4IR::Jump *j = term->asJump()) {
                    trace(j->target, V, output);
                } else if (V4IR::CJump *cj = term->asCJump()) {
                    if (! V->contains(cj->iffalse))
                        trace(cj->iffalse, V, output);
                    else
                        trace(cj->iftrue, V, output);
                } else if (V4IR::Try *t = term->asTry()) {
                    trace(t->tryBlock, V, output);
                    trace(t->catchBlock, V, output);
                }
            }

            // We could do this for each type above, but it is safer to have a
            // "catchall" here
            for (int ii = 0; ii < block->out.count(); ++ii)
                trace(block->out.at(ii), V, output);
        }
    };

    I::trace(function->basicBlocks.first(), &V, &trace);

    V.insert(exitBlock);
    exitBlock->index = trace.size();
    trace.append(exitBlock);

    QVarLengthArray<V4IR::BasicBlock*> blocksToDelete;
    foreach (V4IR::BasicBlock *b, function->basicBlocks) {
        if (!V.contains(b)) {
            foreach (V4IR::BasicBlock *out, b->out) {
                int idx = out->in.indexOf(b);
                if (idx >= 0)
                    out->in.remove(idx);
            }
            blocksToDelete.append(b);
        }
    }
    foreach (V4IR::BasicBlock *b, blocksToDelete)
        foreach (V4IR::Stmt *s, b->statements)
            s->destroyData();
    qDeleteAll(blocksToDelete);
    function->basicBlocks = trace;
#else
    {
//        showMeTheCode(function);

        // remove all basic blocks that have no incoming edges, but skip the entry block
        QVector<BasicBlock *> W = function->basicBlocks;
        W.removeFirst();
        QSet<BasicBlock *> toRemove;

        while (!W.isEmpty()) {
            BasicBlock *bb = W.first();
            W.removeFirst();
            if (toRemove.contains(bb))
                continue;
            if (bb->in.isEmpty()) {
                foreach (BasicBlock *outBB, bb->out) {
                    int idx = outBB->in.indexOf(bb);
                    if (idx != -1) {
                        outBB->in.remove(idx);
                        W.append(outBB);
                    }
                }
                toRemove.insert(bb);
            }
        }

        // TODO: merge 2 basic blocks A and B if A has one outgoing edge (to B), B has one incoming
        // edge (from A), but not when A has more than 1 incoming edge and B has more than one
        // outgoing edge.

        foreach (BasicBlock *bb, toRemove) {
            foreach (Stmt *s, bb->statements)
                s->destroyData();
            int idx = function->basicBlocks.indexOf(bb);
            if (idx != -1)
                function->basicBlocks.remove(idx);
            delete bb;
        }

        // number all basic blocks:
        for (int i = 0; i < function->basicBlocks.size(); ++i)
            function->basicBlocks[i]->index = i;
    }
#endif
    function->removeSharedExpressions();
//    if (qgetenv("NO_OPT").isEmpty())
//        ConstantPropagation().run(function);

//#ifndef QV4_NO_LIVENESS
//    liveness(function);
//#endif

//    if (qgetenv("NO_OPT").isEmpty()) {
//        removeDeadAssignments(function);
//        removeUnreachableBlocks(function);
//    }

//    showMeTheCode(function);
//    splitEdges(function);
//    showMeTheCode(function);

    showMeTheCode(function);

    if (!function->hasTry && !function->hasWith) {
//        qout << "Starting edge splitting..." << endl;
        doEdgeSplitting(function);
//        showMeTheCode(function);

//        qout << "Starting def/uses calculation..." << endl;
        QHash<int, int> tempMapping = convertToSSA(function);
//        showMeTheCode(function);

        DefUsesCalculator defUses(function);

//        qout << "Cleaning up phi nodes..." << endl;
        cleanupPhis(defUses);
//        showMeTheCode(function);

//        qout << "Starting dead-code elimination..." << endl;
        DeadCodeElimination(defUses, function).run();
//        showMeTheCode(function);

//        qout << "Running type inference..." << endl;
        TypeInference(defUses).run(function);
//        showMeTheCode(function);

//        qout << "Doing type propagation..." << endl;
        TypePropagation().run(function);
//        showMeTheCode(function);
//        scheduleBlocks(function->basicBlocks);
//        showMeTheCode(function);

//        qout << "Converting out of SSA..." << endl;
        convertOutOfSSA(function, tempMapping);
        showMeTheCode(function);

#ifndef QT_NO_DEBUG
        checkCriticalEdges(function->basicBlocks);
#endif

//        qout << "Finished." << endl;
    }

    // TODO: remove blocks that only have a JUMP statement, and re-wire their predecessor/successor.
}
