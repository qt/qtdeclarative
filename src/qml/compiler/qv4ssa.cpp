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

#include "qv4ssa_p.h"
#include "qv4isel_util_p.h"
#include "qv4util_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QSet>
#include <QtCore/QBuffer>
#include <QtCore/QLinkedList>
#include <QtCore/QStack>
#include <qv4runtime_p.h>
#include <qv4context_p.h>
#include <private/qqmlpropertycache_p.h>
#include <cmath>
#include <iostream>
#include <cassert>
#include <algorithm>

#ifdef CONST
#undef CONST
#endif

#define QV4_NO_LIVENESS
#undef SHOW_SSA
#undef DEBUG_MOVEMAPPING

QT_USE_NAMESPACE

using namespace QQmlJS;
using namespace V4IR;

namespace {

QTextStream qout(stdout, QIODevice::WriteOnly);

void showMeTheCode(Function *function)
{
    static bool showCode = !qgetenv("QV4_SHOW_IR").isNull();
    if (showCode) {
        QVector<Stmt *> code;
        QHash<Stmt *, BasicBlock *> leader;

        foreach (BasicBlock *block, function->basicBlocks) {
            if (block->statements.isEmpty())
                continue;
            leader.insert(block->statements.first(), block);
            foreach (Stmt *s, block->statements) {
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
            Stmt *s = code.at(i);
            Q_ASSERT(s);

            if (BasicBlock *bb = leader.value(s)) {
                qout << endl;
                QByteArray str;
                str.append('L');
                str.append(QByteArray::number(bb->index));
                str.append(':');
                if (bb->catchBlock) {
                    str.append(" (exception handler L");
                    str.append(QByteArray::number(bb->catchBlock->index));
                    str.append(')');
                }
                for (int i = 66 - str.length(); i; --i)
                    str.append(' ');
                qout << str;
                qout << "// predecessor blocks:";
                foreach (BasicBlock *in, bb->in)
                    qout << " L" << in->index;
                if (bb->in.isEmpty())
                    qout << "(none)";
                if (BasicBlock *container = bb->containingGroup())
                    qout << "; container block: L" << container->index;
                if (bb->isGroupStart())
                    qout << "; group start";
                qout << endl;
            }
            Stmt *n = (i + 1) < code.size() ? code.at(i + 1) : 0;
//            if (n && s->asJump() && s->asJump()->target == leader.value(n)) {
//                continue;
//            }

            QByteArray str;
            QBuffer buf(&str);
            buf.open(QIODevice::WriteOnly);
            QTextStream out(&buf);
            if (s->id > 0)
                out << s->id << ": ";
            s->dump(out, Stmt::MIR);
            if (s->location.isValid()) {
                out.flush();
                for (int i = 58 - str.length(); i > 0; --i)
                    out << ' ';
                out << "    // line: " << s->location.startLine << " column: " << s->location.startColumn;
            }

            out.flush();

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

class DominatorTree {
    int N;
    QHash<BasicBlock *, int> dfnum;
    QVector<BasicBlock *> vertex;
    QHash<BasicBlock *, BasicBlock *> parent;
    QHash<BasicBlock *, BasicBlock *> ancestor;
    QHash<BasicBlock *, BasicBlock *> best;
    QHash<BasicBlock *, BasicBlock *> semi;
    QHash<BasicBlock *, BasicBlock *> idom;
    QHash<BasicBlock *, BasicBlock *> samedom;
    QHash<BasicBlock *, QSet<BasicBlock *> > bucket;

    struct DFSTodo {
        BasicBlock *node, *parent;

        DFSTodo():node(0),parent(0){}
        DFSTodo(BasicBlock *node, BasicBlock *parent):node(node),parent(parent){}
    };

    void DFS(BasicBlock *node) {
        QVector<DFSTodo> worklist;
        worklist.reserve(vertex.capacity());
        DFSTodo todo(node, 0);

        while (true) {
            BasicBlock *n = todo.node;

            if (dfnum[n] == 0) {
                dfnum[n] = N;
                vertex[N] = n;
                parent[n] = todo.parent;
                ++N;
                for (int i = n->out.size() - 1; i > 0; --i)
                    worklist.append(DFSTodo(n->out[i], n));

                if (n->out.size() > 0) {
                    todo.node = n->out.first();
                    todo.parent = n;
                    continue;
                }
            }

            if (worklist.isEmpty())
                break;

            todo = worklist.last();
            worklist.removeLast();
        }
    }

    BasicBlock *ancestorWithLowestSemi(BasicBlock *v) {
        QVector<BasicBlock *> worklist;
        worklist.reserve(vertex.capacity());
        for (BasicBlock *it = v; it; it = ancestor[it])
            worklist.append(it);

        if (worklist.size() < 2)
            return best[v];

        BasicBlock *b = 0;
        BasicBlock *last = worklist.last();
        for (int it = worklist.size() - 2; it >= 0; --it) {
            BasicBlock *bbIt = worklist[it];
            ancestor[bbIt] = last;
            BasicBlock *&best_it = best[bbIt];
            if (b && dfnum[semi[b]] < dfnum[semi[best_it]])
                best_it = b;
            else
                b = best_it;
        }
        return b;
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

        DFS(nodes.first());
        Q_ASSERT(N == nodes.size()); // fails with unreachable nodes, but those should have been removed before.

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
                BasicBlock *semi_v = semi[v];
                if (semi[y] == semi_v)
                    idom[v] = semi_v;
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

    struct NodeProgress {
        QSet<BasicBlock *> children;
        QSet<BasicBlock *> todo;
    };

    void computeDF(const QVector<BasicBlock *> &nodes) {
        QHash<BasicBlock *, NodeProgress> nodeStatus;
        nodeStatus.reserve(nodes.size());
        QVector<BasicBlock *> worklist;
        worklist.reserve(nodes.size() * 2);
        for (int i = 0, ei = nodes.size(); i != ei; ++i) {
            BasicBlock *node = nodes[i];
            worklist.append(node);
            NodeProgress &np = nodeStatus[node];
            np.children = children[node];
            np.todo = children[node];
        }

        while (!worklist.isEmpty()) {
            BasicBlock *node = worklist.last();

            if (DF.contains(node)) {
                worklist.removeLast();
                continue;
            }

            NodeProgress &np = nodeStatus[node];
            QSet<BasicBlock *>::iterator it = np.todo.begin();
            while (it != np.todo.end()) {
                if (DF.contains(*it)) {
                    it = np.todo.erase(it);
                } else {
                    worklist.append(*it);
                    break;
                }
            }

            if (np.todo.isEmpty()) {
                QSet<BasicBlock *> S;
                foreach (BasicBlock *y, node->out)
                    if (idom[y] != node)
                        if (!S.contains(y))
                            S.insert(y);
                foreach (BasicBlock *child, np.children)
                    foreach (BasicBlock *w, DF[child])
                        if (!dominates(node, w) || node == w)
                            if (!S.contains(w))
                                S.insert(w);
                DF.insert(node, S);
                worklist.removeLast();
            }
        }

#if defined(SHOW_SSA)
        qout << "Dominator Frontiers:" << endl;
        foreach (BasicBlock *n, nodes) {
            qout << "\tDF[" << n->index << "]: {";
            QList<BasicBlock *> SList = DF[n].values();
            for (int i = 0; i < SList.size(); ++i) {
                if (i > 0)
                    qout << ", ";
                qout << SList[i]->index;
            }
            qout << "}" << endl;
        }
#endif // SHOW_SSA
#if !defined(QT_NO_DEBUG) && defined(CAN_TAKE_LOSTS_OF_TIME)
        foreach (BasicBlock *n, nodes) {
            foreach (BasicBlock *fBlock, DF[n]) {
                Q_ASSERT(!dominates(n, fBlock) || fBlock == n);
                bool hasDominatedSucc = false;
                foreach (BasicBlock *succ, fBlock->in) {
                    if (dominates(n, succ)) {
                        hasDominatedSucc = true;
                        break;
                    }
                }
                if (!hasDominatedSucc) {
                    qout << fBlock->index << " in DF[" << n->index << "] has no dominated predecessors" << endl;
                }
                Q_ASSERT(hasDominatedSucc);
            }
        }
#endif // !QT_NO_DEBUG
    }

    QHash<BasicBlock *, QSet<BasicBlock *> > children;
    QHash<BasicBlock *, QSet<BasicBlock *> > DF;

public:
    DominatorTree(const QVector<BasicBlock *> &nodes)
        : N(0)
    {
        calculateIDoms(nodes);

        // compute children of n
        foreach (BasicBlock *n, nodes) {
            QSet<BasicBlock *> &c = children[idom[n]];
            if (!c.contains(n))
                c.insert(n);
        }

        computeDF(nodes);
    }

    QSet<BasicBlock *> operator[](BasicBlock *n) const {
        return DF[n];
    }

    BasicBlock *immediateDominator(BasicBlock *bb) const {
        return idom[bb];
    }
};

class VariableCollector: public StmtVisitor, ExprVisitor {
    QHash<Temp, QSet<BasicBlock *> > _defsites;
    QHash<BasicBlock *, QSet<Temp> > A_orig;
    QSet<Temp> nonLocals;
    QSet<Temp> killed;

    BasicBlock *currentBB;
    const bool variablesCanEscape;
    bool isCollectable(Temp *t) const
    {
        switch (t->kind) {
        case Temp::Formal:
        case Temp::ScopedFormal:
        case Temp::ScopedLocal:
            return false;
        case Temp::Local:
            return !variablesCanEscape;
        case Temp::VirtualRegister:
            return true;
        default:
            // PhysicalRegister and StackSlot can only get inserted later.
            Q_ASSERT(!"Invalid temp kind!");
            return false;
        }
    }

public:
    VariableCollector(Function *function)
        : variablesCanEscape(function->variablesCanEscape())
    {
#if defined(SHOW_SSA)
        qout << "Variables collected:" << endl;
#endif // SHOW_SSA

        foreach (BasicBlock *bb, function->basicBlocks) {
            currentBB = bb;
            killed.clear();
            killed.reserve(bb->statements.size() / 2);
            foreach (Stmt *s, bb->statements) {
                s->accept(this);
            }
        }

#if defined(SHOW_SSA)
        qout << "Non-locals:" << endl;
        foreach (const Temp &nonLocal, nonLocals) {
            qout << "\t";
            nonLocal.dump(qout);
            qout << endl;
        }

        qout << "end collected variables." << endl;
#endif // SHOW_SSA
    }

    QList<Temp> vars() const {
        return _defsites.keys();
    }

    QSet<BasicBlock *> defsite(const Temp &n) const {
        return _defsites[n];
    }

    QSet<Temp> inBlock(BasicBlock *n) const {
        return A_orig[n];
    }

    bool isNonLocal(const Temp &var) const { return nonLocals.contains(var); }

protected:
    virtual void visitPhi(Phi *) {};
    virtual void visitConvert(Convert *e) { e->expr->accept(this); };

    virtual void visitConst(Const *) {}
    virtual void visitString(String *) {}
    virtual void visitRegExp(RegExp *) {}
    virtual void visitName(Name *) {}
    virtual void visitClosure(Closure *) {}
    virtual void visitUnop(Unop *e) { e->expr->accept(this); }
    virtual void visitBinop(Binop *e) { e->left->accept(this); e->right->accept(this); }
    virtual void visitSubscript(Subscript *e) { e->base->accept(this); e->index->accept(this); }
    virtual void visitMember(Member *e) { e->base->accept(this); }
    virtual void visitExp(Exp *s) { s->expr->accept(this); }
    virtual void visitJump(Jump *) {}
    virtual void visitCJump(CJump *s) { s->cond->accept(this); }
    virtual void visitRet(Ret *s) { s->expr->accept(this); }

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

    virtual void visitMove(Move *s) {
        s->source->accept(this);

        if (Temp *t = s->target->asTemp()) {
            if (isCollectable(t)) {
#if defined(SHOW_SSA)
                qout << '\t';
                t->dump(qout);
                qout << " -> L" << currentBB->index << endl;
#endif // SHOW_SSA

                _defsites[*t].insert(currentBB);
                A_orig[currentBB].insert(*t);

                // For semi-pruned SSA:
                killed.insert(*t);
            }
        } else {
            s->target->accept(this);
        }
    }

    virtual void visitTemp(Temp *t)
    {
        if (isCollectable(t))
            if (!killed.contains(*t))
                nonLocals.insert(*t);
    }
};

void insertPhiNode(const Temp &a, BasicBlock *y, Function *f) {
#if defined(SHOW_SSA)
    qout << "-> inserted phi node for variable ";
    a.dump(qout);
    qout << " in block " << y->index << endl;
#endif

    Phi *phiNode = f->New<Phi>();
    phiNode->d = new Stmt::Data;
    phiNode->targetTemp = f->New<Temp>();
    phiNode->targetTemp->init(a.kind, a.index, 0);
    y->statements.prepend(phiNode);

    phiNode->d->incoming.resize(y->in.size());
    for (int i = 0, ei = y->in.size(); i < ei; ++i) {
        Temp *t = f->New<Temp>();
        t->init(a.kind, a.index, 0);
        phiNode->d->incoming[i] = t;
    }
}

class VariableRenamer: public StmtVisitor, public ExprVisitor
{
    Function *function;
    QHash<Temp, QStack<unsigned> > stack;
    QSet<BasicBlock *> seen;

    QHash<Temp, unsigned> defCounts;

    const bool variablesCanEscape;
    bool isRenamable(Temp *t) const
    {
        switch (t->kind) {
        case Temp::Formal:
        case Temp::ScopedFormal:
        case Temp::ScopedLocal:
            return false;
        case Temp::Local:
            return !variablesCanEscape;
        case Temp::VirtualRegister:
            return true;
        default:
            Q_ASSERT(!"Invalid temp kind!");
            return false;
        }
    }
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
    VariableRenamer(Function *f)
        : function(f)
        , variablesCanEscape(f->variablesCanEscape())
    {
        if (!variablesCanEscape) {
            Temp t;
            t.init(Temp::Local, 0, 0);
            for (int i = 0, ei = f->locals.size(); i != ei; ++i) {
                t.index = i;
                stack[t].push(nextFreeTemp());
            }
        }

        Temp t;
        t.init(Temp::VirtualRegister, 0, 0);
        for (int i = 0, ei = f->tempCount; i != ei; ++i) {
            t.index = i;
            stack[t].push(i);
        }
    }

    void run() {
        foreach (BasicBlock *n, function->basicBlocks)
            rename(n);

#ifdef SHOW_SSA
//        qout << "Temp to local mapping:" << endl;
//        foreach (int key, tempMapping.keys())
//            qout << '\t' << key << " -> " << tempMapping[key] << endl;
#endif
    }

    void rename(BasicBlock *n) {
        if (seen.contains(n))
            return;
        seen.insert(n);
//        qDebug() << "I: L"<<n->index;

        // [1]:
        foreach (Stmt *s, n->statements)
            s->accept(this);

        QHash<Temp, unsigned> dc = defCounts;
        defCounts.clear();

        // [2]:
        foreach (BasicBlock *Y, n->out) {
            const int j = Y->in.indexOf(n);
            Q_ASSERT(j >= 0 && j < Y->in.size());
            foreach (Stmt *s, Y->statements) {
                if (Phi *phi = s->asPhi()) {
                    Temp *t = phi->d->incoming[j]->asTemp();
                    unsigned newTmp = stack[*t].top();
//                    qDebug()<<"I: replacing phi use"<<a<<"with"<<newTmp<<"in L"<<Y->index;
                    t->index = newTmp;
                    t->kind = Temp::VirtualRegister;
                } else {
                    break;
                }
            }
        }

        // [3]:
        foreach (BasicBlock *X, n->out)
            rename(X);

        // [4]:
        for (QHash<Temp, unsigned>::const_iterator i = dc.begin(), ei = dc.end(); i != ei; ++i) {
//            qDebug()<<i.key() <<" -> " << i.value();
            for (unsigned j = 0, ej = i.value(); j < ej; ++j)
                stack[i.key()].pop();
        }
    }

protected:
    virtual void visitTemp(Temp *e) { // only called for uses, not defs
        if (isRenamable(e)) {
//            qDebug()<<"I: replacing use of"<<e->index<<"with"<<stack[e->index].top();
            e->index = stack[*e].top();
            e->kind = Temp::VirtualRegister;
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
        if (isRenamable(t)) {
            defCounts[*t] = defCounts.value(*t, 0) + 1;
            const int newIdx = nextFreeTemp();
            stack[*t].push(newIdx);
//            qDebug()<<"I: replacing def of"<<a<<"with"<<newIdx;
            t->kind = Temp::VirtualRegister;
            t->index = newIdx;
        }
    }

    virtual void visitConvert(Convert *e) { e->expr->accept(this); }
    virtual void visitPhi(Phi *s) { renameTemp(s->targetTemp); }

    virtual void visitExp(Exp *s) { s->expr->accept(this); }

    virtual void visitJump(Jump *) {}
    virtual void visitCJump(CJump *s) { s->cond->accept(this); }
    virtual void visitRet(Ret *s) { s->expr->accept(this); }

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

void convertToSSA(Function *function, const DominatorTree &df)
{
#ifdef SHOW_SSA
    qout << "Converting function ";
    if (function->name)
        qout << *function->name;
    else
        qout << "<no name>";
    qout << " to SSA..." << endl;
#endif // SHOW_SSA

    // Collect all applicable variables:
    VariableCollector variables(function);

    // Place phi functions:
    QHash<BasicBlock *, QSet<Temp> > A_phi;
    foreach (Temp a, variables.vars()) {
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
    VariableRenamer(function).run();
}

struct UntypedTemp {
    Temp temp;
    UntypedTemp(const Temp &t): temp(t) {}
};
inline uint qHash(const UntypedTemp &t, uint seed = 0) Q_DECL_NOTHROW
{ return t.temp.index ^ (t.temp.kind | (t.temp.scope << 3)) ^ seed; }
inline bool operator==(const UntypedTemp &t1, const UntypedTemp &t2) Q_DECL_NOTHROW
{ return t1.temp.index == t2.temp.index && t1.temp.scope == t2.temp.scope && t1.temp.kind == t2.temp.kind; }

class DefUsesCalculator: public StmtVisitor, public ExprVisitor {
public:
    struct DefUse {
        DefUse()
            : defStmt(0)
            , blockOfStatement(0)
        {}
        Stmt *defStmt;
        BasicBlock *blockOfStatement;
        QList<Stmt *> uses;
    };

private:
    const bool _variablesCanEscape;
    QHash<UntypedTemp, DefUse> _defUses;
    QHash<Stmt *, QList<Temp> > _usesPerStatement;

    BasicBlock *_block;
    Stmt *_stmt;

    bool isCollectible(Temp *t) const {
        switch (t->kind) {
        case Temp::Formal:
        case Temp::ScopedFormal:
        case Temp::ScopedLocal:
            return false;
        case Temp::Local:
            return !_variablesCanEscape;
        case Temp::VirtualRegister:
            return true;
        default:
            Q_UNREACHABLE();
            return false;
        }
    }

    void addUse(Temp *t) {
        Q_ASSERT(t);
        if (!isCollectible(t))
            return;

        _defUses[*t].uses.append(_stmt);
        _usesPerStatement[_stmt].append(*t);
    }

    void addDef(Temp *t) {
        if (!isCollectible(t))
            return;

        Q_ASSERT(!_defUses.contains(*t) || _defUses.value(*t).defStmt == 0 || _defUses.value(*t).defStmt == _stmt);

        DefUse &defUse = _defUses[*t];
        defUse.defStmt = _stmt;
        defUse.blockOfStatement = _block;
    }

public:
    DefUsesCalculator(Function *function)
        : _variablesCanEscape(function->variablesCanEscape())
    {
        foreach (BasicBlock *bb, function->basicBlocks) {
            _block = bb;
            foreach (Stmt *stmt, bb->statements) {
                _stmt = stmt;
                stmt->accept(this);
            }
        }

        QMutableHashIterator<UntypedTemp, DefUse> it(_defUses);
        while (it.hasNext()) {
            it.next();
            if (!it.value().defStmt)
                it.remove();
        }
    }

    void addTemp(Temp *newTemp, Stmt *defStmt, BasicBlock *defBlock)
    {
        DefUse &defUse = _defUses[*newTemp];
        defUse.defStmt = defStmt;
        defUse.blockOfStatement = defBlock;
    }

    QList<Temp> defs() const {
        QList<Temp> res;
        res.reserve(_defUses.size());
        foreach (const UntypedTemp &t, _defUses.keys())
            res.append(t.temp);
        return res;
    }

    void removeDef(const Temp &var) {
        _defUses.remove(var);
    }

    void addUses(const Temp &variable, const QList<Stmt *> &newUses)
    { _defUses[variable].uses.append(newUses); }

    void addUse(const Temp &variable, Stmt * newUse)
    { _defUses[variable].uses.append(newUse); }

    int useCount(const Temp &variable) const
    { return _defUses[variable].uses.size(); }

    Stmt *defStmt(const Temp &variable) const
    { return _defUses[variable].defStmt; }

    BasicBlock *defStmtBlock(const Temp &variable) const
    { return _defUses[variable].blockOfStatement; }

    void removeUse(Stmt *usingStmt, const Temp &var)
    { _defUses[var].uses.removeAll(usingStmt); }

    QList<Temp> usedVars(Stmt *s) const
    { return _usesPerStatement[s]; }

    QList<Stmt *> uses(const Temp &var) const
    { return _defUses[var].uses; }

    QVector<Stmt*> removeDefUses(Stmt *s)
    {
        QVector<Stmt*> defStmts;
        foreach (const Temp &usedVar, usedVars(s)) {
            defStmts += defStmt(usedVar);
            removeUse(s, usedVar);
        }
        if (Move *m = s->asMove()) {
            if (Temp *t = m->target->asTemp())
                removeDef(*t);
        } else if (Phi *p = s->asPhi()) {
            removeDef(*p->targetTemp);
        }

        return defStmts;
    }

    void dump() const
    {
        foreach (const UntypedTemp &var, _defUses.keys()) {
            const DefUse &du = _defUses[var];
            var.temp.dump(qout);
            qout<<" -> defined in block "<<du.blockOfStatement->index<<", statement: ";
            du.defStmt->dump(qout);
            qout<<endl<<"     uses:"<<endl;
            foreach (Stmt *s, du.uses) {
                qout<<"       ";s->dump(qout);qout<<endl;
            }
        }
    }

protected:
    virtual void visitExp(Exp *s) { s->expr->accept(this); }
    virtual void visitJump(Jump *) {}
    virtual void visitCJump(CJump *s) { s->cond->accept(this); }
    virtual void visitRet(Ret *s) { s->expr->accept(this); }

    virtual void visitPhi(Phi *s) {
        addDef(s->targetTemp);
        foreach (Expr *e, s->d->incoming)
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
};

bool hasPhiOnlyUses(Phi *phi, const DefUsesCalculator &defUses, QSet<Phi *> &collectedPhis)
{
    collectedPhis.insert(phi);
    foreach (Stmt *use, defUses.uses(*phi->targetTemp)) {
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
    foreach (const Temp &def, defUses.defs())
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
        Temp targetVar = *phi->targetTemp;

        BasicBlock *bb = defUses.defStmtBlock(targetVar);
        int idx = bb->statements.indexOf(phi);
        bb->statements[idx]->destroyData();
        bb->statements.remove(idx);

        foreach (const Temp &usedVar, defUses.usedVars(phi))
            defUses.removeUse(phi, usedVar);
        defUses.removeDef(targetVar);
    }
}

class EliminateDeadCode: public ExprVisitor {
    DefUsesCalculator &_defUses;
    QVector<Stmt *> _worklist;
    const bool _variablesCanEscape;
    bool _sideEffect;
    QVector<Temp *> _collectedTemps;

public:
    EliminateDeadCode(DefUsesCalculator &defUses, QVector<Stmt *> worklist, bool variablesCanEscape)
        : _defUses(defUses)
        , _worklist(worklist)
        , _variablesCanEscape(variablesCanEscape)
    {
        _collectedTemps.reserve(8);
    }

    void run(Expr *&expr, Stmt *stmt) {
        if (!checkForSideEffects(expr)) {
            expr = 0;
            foreach (Temp *t, _collectedTemps) {
                _defUses.removeUse(stmt, *t);
                _worklist += _defUses.defStmt(*t);
            }
        }
    }

private:
    bool checkForSideEffects(Expr *expr)
    {
        bool sideEffect = false;
        qSwap(_sideEffect, sideEffect);
        expr->accept(this);
        qSwap(_sideEffect, sideEffect);
        return sideEffect;
    }

    void markAsSideEffect()
    {
        _sideEffect = true;
        _collectedTemps.clear();
    }

    bool isCollectable(Temp *t) const
    {
        switch (t->kind) {
        case Temp::Formal:
        case Temp::ScopedFormal:
        case Temp::ScopedLocal:
            return false;
        case Temp::Local:
            return !_variablesCanEscape;
        default:
            return true;
        }
    }

protected:
    virtual void visitConst(Const *) {}
    virtual void visitString(String *) {}
    virtual void visitRegExp(RegExp *) {}

    virtual void visitName(Name *e)
    {
        // TODO: maybe we can distinguish between built-ins of which we know that they do not have
        // a side-effect.
        if (e->builtin == Name::builtin_invalid || (e->id && *e->id != QStringLiteral("this")))
            markAsSideEffect();
    }

    virtual void visitTemp(Temp *e)
    {
        if (isCollectable(e))
            _collectedTemps.append(e);
    }

    virtual void visitClosure(Closure *)
    {
        markAsSideEffect();
    }

    virtual void visitConvert(Convert *e) {
        e->expr->accept(this);

        switch (e->expr->type) {
        case StringType:
        case VarType:
            markAsSideEffect();
            break;
        default:
            break;
        }
    }

    virtual void visitUnop(Unop *e) {
        e->expr->accept(this);

        switch (e->op) {
        case OpUPlus:
        case OpUMinus:
        case OpNot:
        case OpIncrement:
        case OpDecrement:
            if (e->expr->type == VarType || e->expr->type == StringType)
                markAsSideEffect();
            break;

        default:
            break;
        }
    }

    virtual void visitBinop(Binop *e) {
        // TODO: prune parts that don't have a side-effect. For example, in:
        //   function f(x) { +x+1; return 0; }
        // we can prune the binop and leave the unop/conversion.
        _sideEffect = checkForSideEffects(e->left);
        _sideEffect |= checkForSideEffects(e->right);

        if (e->left->type == VarType || e->left->type == StringType
                || e->right->type == VarType || e->right->type == StringType)
            markAsSideEffect();
    }

    virtual void visitSubscript(Subscript *e) {
        e->base->accept(this);
        e->index->accept(this);
        markAsSideEffect();
    }

    virtual void visitMember(Member *e) {
        e->base->accept(this);
        markAsSideEffect();
    }

    virtual void visitCall(Call *e) {
        e->base->accept(this);
        for (ExprList *args = e->args; args; args = args->next)
            args->expr->accept(this);
        markAsSideEffect(); // TODO: there are built-in functions that have no side effect.
    }

    virtual void visitNew(New *e) {
        e->base->accept(this);
        for (ExprList *args = e->args; args; args = args->next)
            args->expr->accept(this);
        markAsSideEffect(); // TODO: there are built-in types that have no side effect.
    }
};

class TypeInference: public StmtVisitor, public ExprVisitor {
    bool _variablesCanEscape;
    const DefUsesCalculator &_defUses;
    QHash<Temp, int> _tempTypes;
    QSet<Stmt *> _worklist;
    struct TypingResult {
        int type;
        bool fullyTyped;

        TypingResult(int type, bool fullyTyped): type(type), fullyTyped(fullyTyped) {}
        explicit TypingResult(int type = UnknownType): type(type), fullyTyped(type != UnknownType) {}
    };
    TypingResult _ty;

public:
    TypeInference(const DefUsesCalculator &defUses)
        : _defUses(defUses)
        , _ty(UnknownType)
    {}

    void run(Function *function) {
        _variablesCanEscape = function->variablesCanEscape();

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

        PropagateTempTypes(_tempTypes).run(function);
    }

private:
    class PropagateTempTypes: public StmtVisitor, ExprVisitor
    {
    public:
        PropagateTempTypes(const QHash<Temp, int> &tempTypes)
            : _tempTypes(tempTypes)
        {}

        void run(Function *function)
        {
            foreach (BasicBlock *bb, function->basicBlocks)
                foreach (Stmt *s, bb->statements)
                    s->accept(this);
        }

    protected:
        virtual void visitConst(Const *) {}
        virtual void visitString(String *) {}
        virtual void visitRegExp(RegExp *) {}
        virtual void visitName(Name *) {}
        virtual void visitTemp(Temp *e) { e->type = (Type) _tempTypes[*e]; }
        virtual void visitClosure(Closure *) {}
        virtual void visitConvert(Convert *e) { e->expr->accept(this); }
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

        virtual void visitExp(Exp *s) {s->expr->accept(this);}
        virtual void visitMove(Move *s) {
            s->source->accept(this);
            s->target->accept(this);
        }

        virtual void visitJump(Jump *) {}
        virtual void visitCJump(CJump *s) { s->cond->accept(this); }
        virtual void visitRet(Ret *s) { s->expr->accept(this); }
        virtual void visitPhi(Phi *s) {
            s->targetTemp->accept(this);
            foreach (Expr *e, s->d->incoming)
                e->accept(this);
        }

    private:
        QHash<Temp, int> _tempTypes;
    };

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

    bool isAlwaysAnObject(Temp *t) {
        switch (t->kind) {
        case Temp::Formal:
        case Temp::ScopedFormal:
        case Temp::ScopedLocal:
            return true;
        case Temp::Local:
            return _variablesCanEscape;
        default:
            return false;
        }
    }

    void setType(Expr *e, int ty) {
        if (Temp *t = e->asTemp()) {
#if defined(SHOW_SSA)
            qout<<"Setting type for "<< (t->scope?"scoped temp ":"temp ") <<t->index<< " to "<<typeName(Type(ty)) << " (" << ty << ")" << endl;
#endif
            if (isAlwaysAnObject(t))
                ty = VarType;
            if (_tempTypes[*t] != ty) {
                _tempTypes[*t] = ty;

#if defined(SHOW_SSA)
                foreach (Stmt *s, _defUses.uses(*t)) {
                    qout << "Pushing back dependent stmt: ";
                    s->dump(qout);
                    qout << endl;
                }
#endif

                _worklist += QSet<Stmt *>::fromList(_defUses.uses(*t));
            }
        } else {
            e->type = (Type) ty;
        }
    }

protected:
    virtual void visitConst(Const *e) {
        if (e->type & NumberType) {
            if (canConvertToSignedInteger(e->value))
                _ty = TypingResult(SInt32Type);
            else if (canConvertToUnsignedInteger(e->value))
                _ty = TypingResult(UInt32Type);
            else
                _ty = TypingResult(e->type);
        } else
            _ty = TypingResult(e->type);
    }
    virtual void visitString(String *) { _ty = TypingResult(StringType); }
    virtual void visitRegExp(RegExp *) { _ty = TypingResult(VarType); }
    virtual void visitName(Name *) { _ty = TypingResult(VarType); }
    virtual void visitTemp(Temp *e) {
        if (isAlwaysAnObject(e))
            _ty = TypingResult(VarType);
        else
            _ty = TypingResult(_tempTypes.value(*e, UnknownType));
        setType(e, _ty.type);
    }
    virtual void visitClosure(Closure *) { _ty = TypingResult(VarType); }
    virtual void visitConvert(Convert *e) {
        _ty = TypingResult(e->type);
    }

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
            if (leftTy.type & VarType || rightTy.type & VarType)
                _ty.type = VarType;
            else if (leftTy.type & StringType || rightTy.type & StringType)
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
        _ty.type = VarType;
    }
    virtual void visitNew(New *e) {
        _ty = run(e->base);
        for (ExprList *it = e->args; it; it = it->next)
            _ty.fullyTyped &= run(it->expr).fullyTyped;
        _ty.type = VarType;
    }
    virtual void visitSubscript(Subscript *e) {
        _ty.fullyTyped = run(e->base).fullyTyped && run(e->index).fullyTyped;
        _ty.type = VarType;
    }

    virtual void visitMember(Member *e) {
        if (e->type == Member::MemberOfQObject
            && !e->property->isEnum() // Enums need to go through run-time getters/setters to ensure correct string handling.
           ) {
            _ty = TypingResult(irTypeFromPropertyType(e->property->propType));
            return;
        }
        _ty = run(e->base);
        _ty.type = VarType;
    }

    virtual void visitExp(Exp *s) { _ty = run(s->expr); }
    virtual void visitMove(Move *s) {
        TypingResult sourceTy = run(s->source);
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
    virtual void visitPhi(Phi *s) {
        _ty = run(s->d->incoming[0]);
        for (int i = 1, ei = s->d->incoming.size(); i != ei; ++i) {
            TypingResult ty = run(s->d->incoming[i]);
            if (!ty.fullyTyped && _ty.fullyTyped) {
                // When one of the temps not fully typed, we already know that we cannot completely type this node.
                // So, pick the type we calculated upto this point, and wait until the unknown one will be typed.
                // At that point, this statement will be re-scheduled, and then we can fully type this node.
                _ty.fullyTyped = false;
                break;
            }
            _ty.type |= ty.type;
            _ty.fullyTyped &= ty.fullyTyped;
        }

        switch (_ty.type) {
        case UnknownType:
        case UndefinedType:
        case NullType:
        case BoolType:
        case SInt32Type:
        case UInt32Type:
        case DoubleType:
        case StringType:
        case VarType:
            // The type is not a combination of two or more types, so we're done.
            break;

        default:
            // There are multiple types involved, so:
            if ((_ty.type & NumberType) && !(_ty.type & ~NumberType))
                // The type is any combination of double/int32/uint32, but nothing else. So we can
                // type it as double.
                _ty.type = DoubleType;
            else
                // There just is no single type that can hold this combination, so:
                _ty.type = VarType;
        }

        setType(s->targetTemp, _ty.type);
    }

    static int irTypeFromPropertyType(int propType)
    {
        switch (propType) {
        case QMetaType::Bool: return BoolType;
        case QMetaType::Int: return SInt32Type;
        case QMetaType::Double: return DoubleType;
        case QMetaType::QString: return StringType;
        default: break;
        }
        return VarType;
    }
};

void convertConst(Const *c, Type targetType)
{
    switch (targetType) {
    case DoubleType:
        break;
    case SInt32Type:
        c->value = QV4::Primitive::toInt32(c->value);
        break;
    case UInt32Type:
        c->value = QV4::Primitive::toUInt32(c->value);
        break;
    case BoolType:
        c->value = !(c->value == 0 || std::isnan(c->value));
        break;
    default:
        Q_UNIMPLEMENTED();
        Q_ASSERT(!"Unimplemented!");
        break;
    }
    c->type = targetType;
}

class TypePropagation: public StmtVisitor, public ExprVisitor {
    DefUsesCalculator &_defUses;
    Type _ty;
    Function *_f;

    bool run(Expr *&e, Type requestedType = UnknownType, bool insertConversion = true) {
        qSwap(_ty, requestedType);
        e->accept(this);
        qSwap(_ty, requestedType);

        if (requestedType != UnknownType) {
            if (e->type != requestedType) {
                if (requestedType & NumberType || requestedType == BoolType) {
#ifdef SHOW_SSA
                    QTextStream os(stdout, QIODevice::WriteOnly);
                    os << "adding conversion from " << typeName(e->type)
                       << " to " << typeName(requestedType) << " for expression ";
                    e->dump(os);
                    os << " in statement ";
                    _currStmt->dump(os);
                    os << endl;
#endif
                    if (insertConversion)
                        addConversion(e, requestedType);
                    return true;
                }
            }
        }

        return false;
    }

    struct Conversion {
        Expr **expr;
        Type targetType;
        Stmt *stmt;

        Conversion(Expr **expr = 0, Type targetType = UnknownType, Stmt *stmt = 0)
            : expr(expr)
            , targetType(targetType)
            , stmt(stmt)
        {}
    };

    Stmt *_currStmt;
    QVector<Conversion> _conversions;

    void addConversion(Expr *&expr, Type targetType) {
        _conversions.append(Conversion(&expr, targetType, _currStmt));
    }

public:
    TypePropagation(DefUsesCalculator &defUses) : _defUses(defUses), _ty(UnknownType) {}

    void run(Function *f) {
        _f = f;
        foreach (BasicBlock *bb, f->basicBlocks) {
            _conversions.clear();

            foreach (Stmt *s, bb->statements) {
                _currStmt = s;
                s->accept(this);
            }

            foreach (const Conversion &conversion, _conversions) {
                V4IR::Move *move = conversion.stmt->asMove();

                // Note: isel only supports move into member when source is a temp, so convert
                // is not a supported source.
                if (move && move->source->asTemp() && !move->target->asMember()) {
                    *conversion.expr = bb->CONVERT(*conversion.expr, conversion.targetType);
                } else if (Const *c = (*conversion.expr)->asConst()) {
                    convertConst(c, conversion.targetType);
                } else if (Temp *t = (*conversion.expr)->asTemp()) {
                    Temp *target = bb->TEMP(bb->newTemp());
                    target->type = conversion.targetType;
                    Expr *convert = bb->CONVERT(t, conversion.targetType);
                    Move *convCall = f->New<Move>();
                    convCall->init(target, convert);
                    _defUses.addTemp(target, convCall, bb);
                    _defUses.addUse(*t, convCall);

                    Temp *source = bb->TEMP(target->index);
                    source->type = conversion.targetType;
                    _defUses.removeUse(conversion.stmt, *t);
                    _defUses.addUse(*source, conversion.stmt);

                    if (Phi *phi = conversion.stmt->asPhi()) {
                        int idx = phi->d->incoming.indexOf(t);
                        Q_ASSERT(idx != -1);
                        QVector<Stmt *> &stmts = bb->in[idx]->statements;
                        stmts.insert(stmts.size() - 1, convCall);
                    } else {
                        int idx = bb->statements.indexOf(conversion.stmt);
                        bb->statements.insert(idx, convCall);
                    }

                    *conversion.expr = source;
                } else {
                    Q_UNREACHABLE();
                }
            }
        }
    }

protected:
    virtual void visitConst(Const *c) {
        if (_ty & NumberType && c->type & NumberType) {
            if (_ty == SInt32Type)
                c->value = QV4::Primitive::toInt32(c->value);
            else if (_ty == UInt32Type)
                c->value = QV4::Primitive::toUInt32(c->value);
            c->type = _ty;
        }
    }

    virtual void visitString(String *) {}
    virtual void visitRegExp(RegExp *) {}
    virtual void visitName(Name *) {}
    virtual void visitTemp(Temp *) {}
    virtual void visitClosure(Closure *) {}
    virtual void visitConvert(Convert *e) { run(e->expr, e->type); }
    virtual void visitUnop(Unop *e) { run(e->expr, e->type); }
    virtual void visitBinop(Binop *e) {
        // FIXME: This routine needs more tuning!
        switch (e->op) {
        case OpAdd:
        case OpSub:
        case OpMul:
        case OpDiv:
        case OpMod:
        case OpBitAnd:
        case OpBitOr:
        case OpBitXor:
            run(e->left, e->type);
            run(e->right, e->type);
            break;

        case OpLShift:
        case OpRShift:
        case OpURShift:
            run(e->left, SInt32Type);
            run(e->right, SInt32Type);
            break;

        case OpGt:
        case OpLt:
        case OpGe:
        case OpLe:
        case OpEqual:
        case OpNotEqual:
            if (e->left->type == DoubleType) {
                run(e->right, DoubleType);
            } else if (e->right->type == DoubleType) {
                run(e->left, DoubleType);
            } else {
                run(e->left, e->left->type);
                run(e->right, e->right->type);
            }
            break;

        case OpStrictEqual:
        case OpStrictNotEqual:
        case OpInstanceof:
        case OpIn:
            run(e->left, e->left->type);
            run(e->right, e->right->type);
            break;

        default:
            Q_UNIMPLEMENTED();
            Q_UNREACHABLE();
        }
    }
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
    virtual void visitMove(Move *s) {
        if (s->source->asConvert())
            return; // this statement got inserted for a phi-node type conversion

        run(s->target);

        if (Unop *u = s->source->asUnop()) {
            if (u->op == OpUPlus) {
                if (run(u->expr, s->target->type, false)) {
                    Convert *convert = _f->New<Convert>();
                    convert->init(u->expr, s->target->type);
                    s->source = convert;
                } else {
                    s->source = u->expr;
                }

                return;
            }
        }

        // Don't convert when writing to QObject properties. All sorts of extra behavior
        // is defined when writing to them, for example resettable properties are reset
        // when writing undefined to them, and an exception is thrown when they're missing
        // a reset function.
        const Member *targetMember = s->target->asMember();
        const bool inhibitConversion = targetMember && targetMember->type == Member::MemberOfQObject && targetMember->property;

        run(s->source, s->target->type, !inhibitConversion);
    }
    virtual void visitJump(Jump *) {}
    virtual void visitCJump(CJump *s) {
        run(s->cond, BoolType);
    }
    virtual void visitRet(Ret *s) { run(s->expr); }
    virtual void visitPhi(Phi *s) {
        Type ty = s->targetTemp->type;
        for (int i = 0, ei = s->d->incoming.size(); i != ei; ++i)
            run(s->d->incoming[i], ty);
    }
};

void splitCriticalEdges(Function *f)
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
                    BasicBlock *newBB = new BasicBlock(f, bb->containingGroup(), bb->catchBlock);
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

QHash<BasicBlock *, BasicBlock *> scheduleBlocks(Function *function, const DominatorTree &df)
{
    struct I {
        const DominatorTree &df;
        QHash<BasicBlock *, BasicBlock *> &startEndLoops;
        QSet<BasicBlock *> visited;
        QVector<BasicBlock *> &sequence;
        BasicBlock *currentGroup;
        QList<BasicBlock *> postponed;

        I(const DominatorTree &df, QVector<BasicBlock *> &sequence,
          QHash<BasicBlock *, BasicBlock *> &startEndLoops)
            : df(df)
            , startEndLoops(startEndLoops)
            , sequence(sequence)
            , currentGroup(0)
        {}

        void DFS(BasicBlock *bb) {
            Q_ASSERT(bb);
            if (visited.contains(bb))
                return;

            if (bb->containingGroup() != currentGroup) {
                postponed.append(bb);
                return;
            }
            if (bb->isGroupStart())
                currentGroup = bb;
            else if (bb->in.size() > 1)
                foreach (BasicBlock *inBB, bb->in)
                    if (!visited.contains(inBB))
                        return;

            Q_ASSERT(df.immediateDominator(bb) == 0 || sequence.contains(df.immediateDominator(bb)));
            layout(bb);
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

            if (bb->isGroupStart()) {
                currentGroup = bb->containingGroup();
                startEndLoops.insert(bb, sequence.last());
                QList<BasicBlock *> p = postponed;
                foreach (BasicBlock *pBB, p)
                    DFS(pBB);
            }
        }

        void layout(BasicBlock *bb) {
            sequence.append(bb);
            visited.insert(bb);
            postponed.removeAll(bb);
        }
    };

    QVector<BasicBlock *> sequence;
    sequence.reserve(function->basicBlocks.size());
    QHash<BasicBlock *, BasicBlock *> startEndLoops;
    I(df, sequence, startEndLoops).DFS(function->basicBlocks.first());
    qSwap(function->basicBlocks, sequence);

    return startEndLoops;
}

#ifndef QT_NO_DEBUG
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
#endif

void cleanupBasicBlocks(Function *function, bool renumber)
{
    showMeTheCode(function);

    // Algorithm: this is the iterative version of a depth-first search for all blocks that are
    // reachable through outgoing edges, starting with the start block and all exception handler
    // blocks.
    QSet<BasicBlock *> postponed, done;
    QSet<BasicBlock *> toRemove;
    toRemove.reserve(function->basicBlocks.size());
    done.reserve(function->basicBlocks.size());
    postponed.reserve(8);
    for (int i = 0, ei = function->basicBlocks.size(); i != ei; ++i) {
        BasicBlock *bb = function->basicBlocks[i];
        if (i == 0 || bb->isExceptionHandler)
            postponed.insert(bb);
        else
            toRemove.insert(bb);
    }

    while (!postponed.isEmpty()) {
        QSet<BasicBlock *>::iterator it = postponed.begin();
        BasicBlock *bb = *it;
        postponed.erase(it);
        done.insert(bb);

        foreach (BasicBlock *outBB, bb->out) {
            if (!done.contains(outBB)) {
                postponed.insert(outBB);
                toRemove.remove(outBB);
            }
        }
    }

    foreach (BasicBlock *bb, toRemove) {
        foreach (BasicBlock *outBB, bb->out) {
            if (toRemove.contains(outBB))
                continue; // We do not need to unlink from blocks that are scheduled to be removed.
                          // Actually, it is potentially dangerous: if that block was already
                          // destroyed, this could result in a use-after-free.

            int idx = outBB->in.indexOf(bb);
            if (idx != -1) {
                outBB->in.remove(idx);
                foreach (Stmt *s, outBB->statements) {
                    if (Phi *phi = s->asPhi())
                        phi->d->incoming.remove(idx);
                    else
                        break;
                }
            }
        }

        foreach (Stmt *s, bb->statements)
            s->destroyData();
        int idx = function->basicBlocks.indexOf(bb);
        if (idx != -1)
            function->basicBlocks.remove(idx);
        delete bb;
    }

    if (renumber)
        for (int i = 0; i < function->basicBlocks.size(); ++i)
            function->basicBlocks[i]->index = i;

    showMeTheCode(function);
}

inline Const *isConstPhi(Phi *phi)
{
    if (Const *c = phi->d->incoming[0]->asConst()) {
        for (int i = 1, ei = phi->d->incoming.size(); i != ei; ++i) {
            if (Const *cc = phi->d->incoming[i]->asConst()) {
                if (c->value != cc->value)
                    return 0;
                if (!(c->type == cc->type || (c->type & NumberType && cc->type & NumberType)))
                    return 0;
                if (int(c->value) == 0 && int(cc->value) == 0)
                    if (isNegative(c->value) != isNegative(cc->value))
                        return 0;
            } else {
                return 0;
            }
        }
        return c;
    }
    return 0;
}

static Expr *clone(Expr *e, Function *function) {
    if (Temp *t = e->asTemp()) {
        return CloneExpr::cloneTemp(t, function);
    } else if (Const *c = e->asConst()) {
        return CloneExpr::cloneConst(c, function);
    } else if (Name *n = e->asName()) {
        return CloneExpr::cloneName(n, function);
    } else {
        Q_UNREACHABLE();
        return e;
    }
}

class ExprReplacer: public StmtVisitor, public ExprVisitor
{
    DefUsesCalculator &_defUses;
    Function* _function;
    Temp *_toReplace;
    Expr *_replacement;

public:
    ExprReplacer(DefUsesCalculator &defUses, Function *function)
        : _defUses(defUses)
        , _function(function)
        , _toReplace(0)
        , _replacement(0)
    {}

    QVector<Stmt *> operator()(Temp *toReplace, Expr *replacement)
    {
        Q_ASSERT(replacement->asTemp() || replacement->asConst() || replacement->asName());

//        qout << "Replacing ";toReplace->dump(qout);qout<<" by ";replacement->dump(qout);qout<<endl;

        qSwap(_toReplace, toReplace);
        qSwap(_replacement, replacement);

        QList<Stmt *> uses = _defUses.uses(*_toReplace);
//        qout << "        " << uses.size() << " uses:"<<endl;
        QVector<Stmt *> result;
        result.reserve(uses.size());
        foreach (Stmt *use, uses) {
//            qout<<"        ";use->dump(qout);qout<<"\n";
            use->accept(this);
//            qout<<"     -> ";use->dump(qout);qout<<"\n";
            result.append(use);
        }

        qSwap(_replacement, replacement);
        qSwap(_toReplace, toReplace);
        return result;
    }

protected:
    virtual void visitConst(Const *) {}
    virtual void visitString(String *) {}
    virtual void visitRegExp(RegExp *) {}
    virtual void visitName(Name *) {}
    virtual void visitTemp(Temp *) {}
    virtual void visitClosure(Closure *) {}
    virtual void visitConvert(Convert *e) { check(e->expr); }
    virtual void visitUnop(Unop *e) { check(e->expr); }
    virtual void visitBinop(Binop *e) { check(e->left); check(e->right); }
    virtual void visitCall(Call *e) {
        check(e->base);
        for (ExprList *it = e->args; it; it = it->next)
            check(it->expr);
    }
    virtual void visitNew(New *e) {
        check(e->base);
        for (ExprList *it = e->args; it; it = it->next)
            check(it->expr);
    }
    virtual void visitSubscript(Subscript *e) { check(e->base); check(e->index); }
    virtual void visitMember(Member *e) { check(e->base); }
    virtual void visitExp(Exp *s) { check(s->expr); }
    virtual void visitMove(Move *s) { check(s->target); check(s->source); }
    virtual void visitJump(Jump *) {}
    virtual void visitCJump(CJump *s) { check(s->cond); }
    virtual void visitRet(Ret *s) { check(s->expr); }
    virtual void visitPhi(Phi *s) {
        for (int i = 0, ei = s->d->incoming.size(); i != ei; ++i)
            check(s->d->incoming[i]);
    }

private:
    void check(Expr *&e) {
        if (equals(e, _toReplace))
            e = clone(_replacement, _function);
        else
            e->accept(this);
    }

    // This only calculates equality for everything needed by constant propagation
    bool equals(Expr *e1, Expr *e2) const {
        if (e1 == e2)
            return true;

        if (Const *c1 = e1->asConst()) {
            if (Const *c2 = e2->asConst())
                return c1->value == c2->value && (c1->type == c2->type || (c1->type & NumberType && c2->type & NumberType));
        } else if (Temp *t1 = e1->asTemp()) {
            if (Temp *t2 = e2->asTemp())
                return *t1 == *t2;
        } else if (Name *n1 = e1->asName()) {
            if (Name *n2 = e2->asName()) {
                if (n1->id) {
                    if (n2->id)
                        return *n1->id == *n2->id;
                } else {
                    return n1->builtin == n2->builtin;
                }
            }
        }

        return false;
    }
};

inline Temp *unescapableTemp(Expr *e, bool variablesCanEscape)
{
    Temp *t = e->asTemp();
    if (!t)
        return 0;

    switch (t->kind) {
    case Temp::VirtualRegister:
        return t;
    case Temp::Local:
        return variablesCanEscape ? 0 : t;
    default:
        return 0;
    }
}

namespace {
/// This function removes the basic-block from the function's list, unlinks any uses and/or defs,
/// and removes unreachable staements from the worklist, so that optimiseSSA won't consider them
/// anymore.
/// Important: this assumes that there are no critical edges in the control-flow graph!
void purgeBB(BasicBlock *bb, Function *func, DefUsesCalculator &defUses, QVector<Stmt *> &W)
{
    // don't purge blocks that are entry points for catch statements. They might not be directly
    // connected, but are required anyway
    if (bb->isExceptionHandler)
        return;

    QVector<BasicBlock *> toPurge;
    toPurge.append(bb);

    while (!toPurge.isEmpty()) {
        bb = toPurge.first();
        toPurge.removeFirst();

        int bbIdx = func->basicBlocks.indexOf(bb);
        if (bbIdx == -1)
            continue;
        else
            func->basicBlocks.remove(bbIdx);

        // unlink all incoming edges
        foreach (BasicBlock *in, bb->in) {
            int idx = in->out.indexOf(bb);
            if (idx != -1)
                in->out.remove(idx);
        }

        // unlink all outgoing edges, including "arguments" to phi statements
        foreach (BasicBlock *out, bb->out) {
            int idx = out->in.indexOf(bb);
            if (idx != -1) {
                out->in.remove(idx);
                foreach (Stmt *outStmt, out->statements) {
                    if (!outStmt)
                        continue;
                    if (Phi *phi = outStmt->asPhi()) {
                        if (Temp *t = phi->d->incoming[idx]->asTemp())
                            defUses.removeUse(phi, *t);
                        phi->d->incoming.remove(idx);
                        W += phi;
                    } else
                        break;
                }
            }

            // if a successor has no incoming edges after unlinking the current basic block, then
            // it is unreachable, and can be purged too
            if (out->in.isEmpty())
                toPurge.append(out);

        }

        // unlink all defs/uses from the statements in the basic block
        foreach (Stmt *s, bb->statements) {
            if (!s)
                continue;

            W << defUses.removeDefUses(s);
            for (int idx = W.indexOf(s); idx != -1; idx = W.indexOf(s))
                W.remove(idx);

            // clean-up the statement's data
            s->destroyData();
        }
        bb->statements.clear();

        delete bb;
    }
}

bool tryOptimizingComparison(Expr *&expr)
{
    Binop *b = expr->asBinop();
    if (!b)
        return false;
    Const *leftConst = b->left->asConst();
    if (!leftConst || leftConst->type == StringType || leftConst->type == VarType)
        return false;
    Const *rightConst = b->right->asConst();
    if (!rightConst || rightConst->type == StringType || rightConst->type == VarType)
        return false;

    QV4::Primitive l = convertToValue(leftConst);
    QV4::Primitive r = convertToValue(rightConst);

    switch (b->op) {
    case OpGt:
        leftConst->value = __qmljs_cmp_gt(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    case OpLt:
        leftConst->value = __qmljs_cmp_lt(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    case OpGe:
        leftConst->value = __qmljs_cmp_ge(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    case OpLe:
        leftConst->value = __qmljs_cmp_le(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    case OpStrictEqual:
        if (!strictlyEqualTypes(leftConst->type, rightConst->type))
            return false;
        // intentional fall-through
    case OpEqual:
        leftConst->value = __qmljs_cmp_eq(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    case OpStrictNotEqual:
        if (!strictlyEqualTypes(leftConst->type, rightConst->type))
            return false;
        // intentional fall-through
    case OpNotEqual:
        leftConst->value = __qmljs_cmp_ne(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    default:
        break;
    }

    return false;
}
} // anonymous namespace

void optimizeSSA(Function *function, DefUsesCalculator &defUses)
{
    const bool variablesCanEscape = function->variablesCanEscape();

    QHash<Stmt*,Stmt**> ref;
    QVector<Stmt *> W;
    foreach (BasicBlock *bb, function->basicBlocks) {
        for (int i = 0, ei = bb->statements.size(); i != ei; ++i) {
            Stmt **s = &bb->statements[i];
            W.append(*s);
            ref.insert(*s, s);
        }
    }

    ExprReplacer replaceUses(defUses, function);

    while (!W.isEmpty()) {
        Stmt *s = W.last();
        W.removeLast();
        if (!s)
            continue;

        if (Phi *phi = s->asPhi()) {
            // constant propagation:
            if (Const *c = isConstPhi(phi)) {
                W += replaceUses(phi->targetTemp, c);
                defUses.removeDef(*phi->targetTemp);
                *ref[s] = 0;
                continue;
            }

            // copy propagation:
            if (phi->d->incoming.size() == 1) {
                Temp *t = phi->targetTemp;
                Expr *e = phi->d->incoming.first();

                QVector<Stmt *> newT2Uses = replaceUses(t, e);
                W += newT2Uses;
                if (Temp *t2 = e->asTemp()) {
                    defUses.removeUse(s, *t2);
                    defUses.addUses(*t2, QList<Stmt*>::fromVector(newT2Uses));
                }
                defUses.removeDef(*t);
                *ref[s] = 0;
                continue;
            }
        } else  if (Move *m = s->asMove()) {
            if (Convert *convert = m->source->asConvert()) {
                if (Const *sourceConst = convert->expr->asConst()) {
                    convertConst(sourceConst, convert->type);
                    m->source = sourceConst;
                    W += m;
                    continue;
                } else if (Temp *sourceTemp = convert->expr->asTemp()) {
                    if (sourceTemp->type == convert->type) {
                        m->source = sourceTemp;
                        W += m;
                        continue;
                    }
                }
            }

            if (Temp *targetTemp = unescapableTemp(m->target, variablesCanEscape)) {
                // dead code elimination:
                if (defUses.useCount(*targetTemp) == 0) {
                    EliminateDeadCode(defUses, W, variablesCanEscape).run(m->source, s);
                    if (!m->source)
                        *ref[s] = 0;
                    continue;
                }

                // constant propagation:
                if (Const *sourceConst = m->source->asConst()) {
                    if (sourceConst->type & NumberType || sourceConst->type == BoolType) {
                        // TODO: when propagating other constants, e.g. undefined, the other
                        // optimization passes have to be changed to cope with them.
                        W += replaceUses(targetTemp, sourceConst);
                        defUses.removeDef(*targetTemp);
                        *ref[s] = 0;
                    }
                    continue;
                }

                // copy propagation:
                if (Temp *sourceTemp = unescapableTemp(m->source, variablesCanEscape)) {
                    QVector<Stmt *> newT2Uses = replaceUses(targetTemp, sourceTemp);
                    W += newT2Uses;
                    defUses.removeUse(s, *sourceTemp);
                    defUses.addUses(*sourceTemp, QList<Stmt*>::fromVector(newT2Uses));
                    defUses.removeDef(*targetTemp);
                    *ref[s] = 0;
                    continue;
                }

                if (Unop *unop = m->source->asUnop()) {
                    // Constant unary expression evaluation:
                    if (Const *constOperand = unop->expr->asConst()) {
                        if (constOperand->type & NumberType || constOperand->type == BoolType) {
                            // TODO: implement unop propagation for other constant types
                            bool doneSomething = false;
                            switch (unop->op) {
                            case OpNot:
                                constOperand->value = !constOperand->value;
                                constOperand->type = BoolType;
                                doneSomething = true;
                                break;
                            case OpUMinus:
                                if (int(constOperand->value) == 0 && int(constOperand->value) == constOperand->value) {
                                    if (isNegative(constOperand->value))
                                        constOperand->value = 0;
                                    else
                                        constOperand->value = -1 / Q_INFINITY;
                                    constOperand->type = DoubleType;
                                    doneSomething = true;
                                    break;
                                }

                                constOperand->value = -constOperand->value;
                                doneSomething = true;
                                break;
                            case OpUPlus:
                                constOperand->type = DoubleType;
                                doneSomething = true;
                                break;
                            case OpCompl:
                                constOperand->value = ~QV4::Primitive::toInt32(constOperand->value);
                                constOperand->type = SInt32Type;
                                doneSomething = true;
                                break;
                            case OpIncrement:
                                constOperand->value = constOperand->value + 1;
                                doneSomething = true;
                                break;
                            case OpDecrement:
                                constOperand->value = constOperand->value - 1;
                                doneSomething = true;
                                break;
                            default:
                                break;
                            };

                            if (doneSomething) {
                                m->source = constOperand;
                                W += m;
                            }
                        }
                    }
                    // TODO: if the result of a unary not operation is only used in a cjump,
                    //       then inline it.

                    continue;
                }

                if (Binop *binop = m->source->asBinop()) {
                    // TODO: More constant binary expression evaluation
                    // TODO: If the result of the move is only used in one single cjump, then
                    //       inline the binop into the cjump.
                    Const *leftConst = binop->left->asConst();
                    if (!leftConst || leftConst->type == StringType || leftConst->type == VarType)
                        continue;
                    Const *rightConst = binop->right->asConst();
                    if (!rightConst || rightConst->type == StringType || rightConst->type == VarType)
                        continue;

                    QV4::Primitive lc = convertToValue(leftConst);
                    QV4::Primitive rc = convertToValue(rightConst);
                    double l = __qmljs_to_number(&lc);
                    double r = __qmljs_to_number(&rc);

                    switch (binop->op) {
                    case OpMul:
                        leftConst->value = l * r;
                        leftConst->type = DoubleType;
                        m->source = leftConst;
                        W += m;
                        break;
                    case OpAdd:
                        leftConst->value = l + r;
                        leftConst->type = DoubleType;
                        m->source = leftConst;
                        W += m;
                        break;
                    case OpSub:
                        leftConst->value = l - r;
                        leftConst->type = DoubleType;
                        m->source = leftConst;
                        W += m;
                        break;
                    case OpDiv:
                        leftConst->value = l / r;
                        leftConst->type = DoubleType;
                        m->source = leftConst;
                        W += m;
                        break;
                    case OpMod:
                        leftConst->value = std::fmod(l, r);
                        leftConst->type = DoubleType;
                        m->source = leftConst;
                        W += m;
                        break;
                    default:
                        if (tryOptimizingComparison(m->source))
                            W += m;
                        break;
                    }

                    continue;
                }
            } // TODO: var{#0} = double{%10} where %10 is defined once and used once. E.g.: function(t){t = t % 2; return t; }

        } else if (CJump *cjump = s->asCJump()) {
            if (Const *constantCondition = cjump->cond->asConst()) {
                // Note: this assumes that there are no critical edges! Meaning, we can safely purge
                //       any basic blocks that are found to be unreachable.
                Jump *jump = function->New<Jump>();
                if (convertToValue(constantCondition).toBoolean()) {
                    jump->target = cjump->iftrue;
                    purgeBB(cjump->iffalse, function, defUses, W);
                } else {
                    jump->target = cjump->iffalse;
                    purgeBB(cjump->iftrue, function, defUses, W);
                }
                *ref[s] = jump;

                continue;
            } else if (cjump->cond->asBinop()) {
                if (tryOptimizingComparison(cjump->cond))
                    W += cjump;
                continue;
            }
            // TODO: Constant unary expression evaluation
            // TODO: if the expression is an unary not operation, lift the expression, and switch
            //       the then/else blocks.
        }
    }

    foreach (BasicBlock *bb, function->basicBlocks) {
        for (int i = 0; i < bb->statements.size();) {
            if (bb->statements[i])
                ++i;
            else
                bb->statements.remove(i);
        }
    }
}

class InputOutputCollector: protected StmtVisitor, protected ExprVisitor {
    const bool variablesCanEscape;

public:
    QList<Temp> inputs;
    QList<Temp> outputs;

    InputOutputCollector(bool variablesCanEscape): variablesCanEscape(variablesCanEscape) {}

    void collect(Stmt *s) {
        inputs.clear();
        outputs.clear();
        s->accept(this);
    }

protected:
    virtual void visitConst(Const *) {}
    virtual void visitString(String *) {}
    virtual void visitRegExp(RegExp *) {}
    virtual void visitName(Name *) {}
    virtual void visitTemp(Temp *e) {
        switch (e->kind) {
        case Temp::Local:
            if (!variablesCanEscape)
                inputs.append(*e);
            break;

        case Temp::VirtualRegister:
            inputs.append(*e);
            break;

        default:
            break;
        }
    }
    virtual void visitClosure(Closure *) {}
    virtual void visitConvert(Convert *e) { e->expr->accept(this); }
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
    virtual void visitSubscript(Subscript *e) { e->base->accept(this); e->index->accept(this); }
    virtual void visitMember(Member *e) { e->base->accept(this); }
    virtual void visitExp(Exp *s) { s->expr->accept(this); }
    virtual void visitMove(Move *s) {
        s->source->accept(this);
        if (Temp *t = s->target->asTemp()) {
            if ((t->kind == Temp::Local && !variablesCanEscape) || t->kind == Temp::VirtualRegister)
                outputs.append(*t);
            else
                s->target->accept(this);
        } else {
            s->target->accept(this);
        }
    }
    virtual void visitJump(Jump *) {}
    virtual void visitCJump(CJump *s) { s->cond->accept(this); }
    virtual void visitRet(Ret *s) { s->expr->accept(this); }
    virtual void visitPhi(Phi *) {
        // Handled separately
    }
};

/*
 * The algorithm is described in:
 *
 *   Linear Scan Register Allocation on SSA Form
 *   Christian Wimmer & Michael Franz, CGO'10, April 24-28, 2010
 *
 * There is one slight difference w.r.t. the phi-nodes: in the artice, the phi nodes are attached
 * to the basic-blocks. Therefore, in the algorithm in the article, the ranges for input parameters
 * for phi nodes run from their definition upto the branch instruction into the block with the phi
 * node. In our representation, phi nodes are mostly treaded as normal instructions, so we have to
 * enlarge the range to cover the phi node itself.
 */
class LifeRanges {
    typedef QSet<Temp> LiveRegs;

    QHash<BasicBlock *, LiveRegs> _liveIn;
    QHash<Temp, LifeTimeInterval> _intervals;
    QVector<LifeTimeInterval> _sortedRanges;

public:
    LifeRanges(Function *function, const QHash<BasicBlock *, BasicBlock *> &startEndLoops)
    {
        int id = 0;
        foreach (BasicBlock *bb, function->basicBlocks) {
            foreach (Stmt *s, bb->statements) {
                if (s->asPhi())
                    s->id = id + 1;
                else
                    s->id = ++id;
            }
        }

        for (int i = function->basicBlocks.size() - 1; i >= 0; --i) {
            BasicBlock *bb = function->basicBlocks[i];
            buildIntervals(bb, startEndLoops.value(bb, 0), function->variablesCanEscape());
        }

        _sortedRanges.reserve(_intervals.size());
        for (QHash<Temp, LifeTimeInterval>::const_iterator i = _intervals.begin(), ei = _intervals.end(); i != ei; ++i) {
            LifeTimeInterval range = i.value();
            range.setTemp(i.key());
            _sortedRanges.append(range);
        }
        std::sort(_sortedRanges.begin(), _sortedRanges.end(), LifeTimeInterval::lessThan);
    }

    QVector<LifeTimeInterval> ranges() const { return _sortedRanges; }

    void dump() const
    {
        qout << "Life ranges:" << endl;
        qout << "Intervals:" << endl;
        foreach (const LifeTimeInterval &range, _sortedRanges) {
            range.dump(qout);
            qout << endl;
        }

        foreach (BasicBlock *bb, _liveIn.keys()) {
            qout << "L" << bb->index <<" live-in: ";
            QList<Temp> live = QList<Temp>::fromSet(_liveIn.value(bb));
            std::sort(live.begin(), live.end());
            for (int i = 0; i < live.size(); ++i) {
                if (i > 0) qout << ", ";
                live[i].dump(qout);
            }
            qout << endl;
        }
    }

private:
    void buildIntervals(BasicBlock *bb, BasicBlock *loopEnd, bool variablesCanEscape)
    {
        LiveRegs live;
        foreach (BasicBlock *successor, bb->out) {
            live.unite(_liveIn[successor]);
            const int bbIndex = successor->in.indexOf(bb);
            Q_ASSERT(bbIndex >= 0);

            foreach (Stmt *s, successor->statements) {
                if (Phi *phi = s->asPhi()) {
                    if (Temp *t = phi->d->incoming[bbIndex]->asTemp())
                        live.insert(*t);
                } else {
                    break;
                }
            }
        }

        foreach (const Temp &opd, live)
            _intervals[opd].addRange(bb->statements.first()->id, bb->statements.last()->id);

        InputOutputCollector collector(variablesCanEscape);
        for (int i = bb->statements.size() - 1; i >= 0; --i) {
            Stmt *s = bb->statements[i];
            if (Phi *phi = s->asPhi()) {
                live.remove(*phi->targetTemp);
                continue;
            }
            collector.collect(s);
            foreach (const Temp &opd, collector.outputs) {
                _intervals[opd].setFrom(s);
                live.remove(opd);
            }
            foreach (const Temp &opd, collector.inputs) {
                _intervals[opd].addRange(bb->statements.first()->id, s->id);
                live.insert(opd);
            }
        }

        if (loopEnd) { // Meaning: bb is a loop header, because loopEnd is set to non-null.
            foreach (const Temp &opd, live)
                _intervals[opd].addRange(bb->statements.first()->id, loopEnd->statements.last()->id);
        }

        _liveIn[bb] = live;
    }
};
} // anonymous namespace

void LifeTimeInterval::setFrom(Stmt *from) {
    Q_ASSERT(from && from->id > 0);

    if (_ranges.isEmpty()) { // this is the case where there is no use, only a define
        _ranges.push_front(Range(from->id, from->id));
        if (_end == Invalid)
            _end = from->id;
    } else {
        _ranges.first().start = from->id;
    }
}

void LifeTimeInterval::addRange(int from, int to) {
    Q_ASSERT(from > 0);
    Q_ASSERT(to > 0);
    Q_ASSERT(to >= from);

    if (_ranges.isEmpty()) {
        _ranges.push_front(Range(from, to));
        _end = to;
        return;
    }

    Range *p = &_ranges.first();
    if (to + 1 >= p->start && p->end + 1 >= from) {
        p->start = qMin(p->start, from);
        p->end = qMax(p->end, to);
        while (_ranges.count() > 1) {
            Range *p1 = &_ranges[1];
            if (p->end + 1 < p1->start || p1->end + 1 < p->start)
                break;
            p1->start = qMin(p->start, p1->start);
            p1->end = qMax(p->end, p1->end);
            _ranges.pop_front();
            p = &_ranges.first();
        }
    } else {
        if (to < p->start) {
            _ranges.push_front(Range(from, to));
        } else {
            Q_ASSERT(from > _ranges.last().end);
            _ranges.push_back(Range(from, to));
        }
    }

    _end = _ranges.last().end;
}

LifeTimeInterval LifeTimeInterval::split(int atPosition, int newStart)
{
    Q_ASSERT(atPosition < newStart || newStart == Invalid);

    if (_ranges.isEmpty() || atPosition < _ranges.first().start)
        return LifeTimeInterval();

    LifeTimeInterval newInterval = *this;
    newInterval.setSplitFromInterval(true);

    for (int i = 0, ei = _ranges.size(); i < ei; ++i) {
        if (_ranges[i].covers(atPosition)) {
            while (_ranges.size() > i + 1)
                _ranges.removeLast();
            while (newInterval._ranges.size() > ei - i)
                newInterval._ranges.removeFirst();
            break;
        }
    }

    if (newInterval._ranges.first().end == atPosition)
        newInterval._ranges.removeFirst();

    if (newStart == Invalid) {
        newInterval._ranges.clear();
        newInterval._end = Invalid;
    } else {
        newInterval._ranges.first().start = newStart;
        _end = newStart;
    }

    _ranges.last().end = atPosition;

    return newInterval;
}

void LifeTimeInterval::dump(QTextStream &out) const {
    _temp.dump(out);
    out << ": ends at " << _end << " with ranges ";
    if (_ranges.isEmpty())
        out << "(none)";
    for (int i = 0; i < _ranges.size(); ++i) {
        if (i > 0) out << ", ";
        out << _ranges[i].start << " - " << _ranges[i].end;
    }
    if (_reg != Invalid)
        out << " (register " << _reg << ")";
}

bool LifeTimeInterval::lessThan(const LifeTimeInterval &r1, const LifeTimeInterval &r2) {
    if (r1._ranges.first().start == r2._ranges.first().start)
        return r1._ranges.last().end < r2._ranges.last().end;
    else
        return r1._ranges.first().start < r2._ranges.first().start;
}

bool LifeTimeInterval::lessThanForTemp(const LifeTimeInterval &r1, const LifeTimeInterval &r2)
{
    return r1.temp() < r2.temp();
}

void Optimizer::run()
{
#if defined(SHOW_SSA)
    qout << "##### NOW IN FUNCTION " << (function->name ? qPrintable(*function->name) : "anonymous!")
         << " with " << function->basicBlocks.size() << " basic blocks." << endl << flush;
#endif

    // Number all basic blocks, so we have nice numbers in the dumps:
    for (int i = 0; i < function->basicBlocks.size(); ++i)
        function->basicBlocks[i]->index = i;
//    showMeTheCode(function);

    cleanupBasicBlocks(function, true);

    function->removeSharedExpressions();

//    showMeTheCode(function);

    static bool doSSA = qgetenv("QV4_NO_SSA").isEmpty();
    static bool doOpt = qgetenv("QV4_NO_OPT").isEmpty();

    if (!function->hasTry && !function->hasWith && !function->module->debugMode && doSSA) {
//        qout << "SSA for " << *function->name << endl;
//        qout << "Starting edge splitting..." << endl;
        splitCriticalEdges(function);
//        showMeTheCode(function);

        // Calculate the dominator tree:
        DominatorTree df(function->basicBlocks);

//        qout << "Converting to SSA..." << endl;
        convertToSSA(function, df);
//        showMeTheCode(function);

//        qout << "Starting def/uses calculation..." << endl;
        DefUsesCalculator defUses(function);

//        qout << "Cleaning up phi nodes..." << endl;
        cleanupPhis(defUses);
//        showMeTheCode(function);

//        qout << "Running type inference..." << endl;
        TypeInference(defUses).run(function);
//        showMeTheCode(function);

//        qout << "Doing type propagation..." << endl;
        TypePropagation(defUses).run(function);
//        showMeTheCode(function);

        if (doOpt) {
//            qout << "Running SSA optimization..." << endl;
            optimizeSSA(function, defUses);
//            showMeTheCode(function);
        }

//        qout << "Doing block merging..." << endl;
//        mergeBasicBlocks(function);
//        showMeTheCode(function);

        // Basic-block cycles that are unreachable (i.e. for loops in a then-part where the
        // condition is calculated to be always false) are not yet removed. This will choke the
        // block scheduling, so remove those now.
//        qout << "Cleaning up unreachable basic blocks..." << endl;
        cleanupBasicBlocks(function, false);
//        showMeTheCode(function);

//        qout << "Doing block scheduling..." << endl;
        startEndLoops = scheduleBlocks(function, df);
//        showMeTheCode(function);

#ifndef QT_NO_DEBUG
        checkCriticalEdges(function->basicBlocks);
#endif

//        qout << "Finished SSA." << endl;
        inSSA = true;
    } else {
        inSSA = false;
    }
}

void Optimizer::convertOutOfSSA() {
    if (!inSSA)
        return;

    // There should be no critical edges at this point.

    foreach (BasicBlock *bb, function->basicBlocks) {
        const int id = bb->statements.last()->id;
        MoveMapping moves;

        foreach (BasicBlock *successor, bb->out) {
            const int inIdx = successor->in.indexOf(bb);
            Q_ASSERT(inIdx >= 0);
            foreach (Stmt *s, successor->statements) {
                if (Phi *phi = s->asPhi()) {
                    moves.add(clone(phi->d->incoming[inIdx], function),
                              clone(phi->targetTemp, function)->asTemp(), id);
                } else {
                    break;
                }
            }
        }

    #if defined(DEBUG_MOVEMAPPING)
        QTextStream os(stdout, QIODevice::WriteOnly);
        os << "Move mapping for function ";
        if (function->name)
            os << *function->name;
        else
            os << (void *) function;
        os << " on basic-block L" << bb->index << ":" << endl;
        moves.dump();
    #endif // DEBUG_MOVEMAPPING

        moves.order();

        moves.insertMoves(bb, function, true);
    }

    foreach (BasicBlock *bb, function->basicBlocks) {
        while (!bb->statements.isEmpty()) {
            if (Phi *phi = bb->statements.first()->asPhi()) {
                phi->destroyData();
                bb->statements.removeFirst();
            } else {
                break;
            }
        }
    }
}

QVector<LifeTimeInterval> Optimizer::lifeRanges() const
{
    Q_ASSERT(isInSSA());

    LifeRanges lifeRanges(function, startEndLoops);
//    lifeRanges.dump();
//    showMeTheCode(function);
    return lifeRanges.ranges();
}

QSet<Jump *> Optimizer::calculateOptionalJumps()
{
    QSet<Jump *> optional;
    QSet<BasicBlock *> reachableWithoutJump;

    const int maxSize = function->basicBlocks.size();
    optional.reserve(maxSize);
    reachableWithoutJump.reserve(maxSize);

    for (int i = function->basicBlocks.size() - 1; i >= 0; --i) {
        BasicBlock *bb = function->basicBlocks[i];

        if (Jump *jump = bb->statements.last()->asJump()) {
            if (reachableWithoutJump.contains(jump->target)) {
                if (bb->statements.size() > 1)
                    reachableWithoutJump.clear();
                optional.insert(jump);
                reachableWithoutJump.insert(bb);
                continue;
            }
        }

        reachableWithoutJump.clear();
        reachableWithoutJump.insert(bb);
    }

#if 0
    QTextStream out(stdout, QIODevice::WriteOnly);
    out << "Jumps to ignore:" << endl;
    foreach (Jump *j, removed) {
        out << "\t" << j->id << ": ";
        j->dump(out, Stmt::MIR);
        out << endl;
    }
#endif

    return optional;
}

void Optimizer::showMeTheCode(Function *function)
{
    ::showMeTheCode(function);
}

static inline bool overlappingStorage(const Temp &t1, const Temp &t2)
{
    // This is the same as the operator==, but for one detail: memory locations are not sensitive
    // to types, and neither are general-purpose registers.

    if (t1.scope != t2.scope)
        return false;
    if (t1.index != t2.index)
        return false; // different position, where-ever that may (physically) be.
    if (t1.kind != t2.kind)
        return false; // formal/local/(physical-)register/stack do never overlap
    if (t1.kind != Temp::PhysicalRegister) // Other than registers, ...
        return t1.kind == t2.kind; // ... everything else overlaps: any memory location can hold everything.

    // So now the index is the same, and we know that both stored in a register. If both are
    // floating-point registers, they are the same. Or, if both are non-floating-point registers,
    // generally called general-purpose registers, they are also the same.
    return (t1.type == DoubleType && t2.type == DoubleType)
            || (t1.type != DoubleType && t2.type != DoubleType);
}

int MoveMapping::isUsedAsSource(Expr *e) const
{
    if (Temp *t = e->asTemp())
        for (int i = 0, ei = _moves.size(); i != ei; ++i)
            if (Temp *from = _moves[i].from->asTemp())
                if (overlappingStorage(*from, *t))
                    return i;

    return -1;
}

void MoveMapping::add(Expr *from, Temp *to, int id) {
    if (Temp *t = from->asTemp()) {
        if (overlappingStorage(*t, *to)) {
            // assignments like fp1 = fp1 or var{&1} = double{&1} can safely be skipped.
#if defined(DEBUG_MOVEMAPPING)
            QTextStream os(stderr, QIODevice::WriteOnly);
            os << "Skipping ";
            to->dump(os);
            os << " <- ";
            from->dump(os);
            os << endl;
#endif // DEBUG_MOVEMAPPING
            return;
        }
    }

    Move m(from, to, id);
    if (_moves.contains(m))
        return;
    _moves.append(m);
}

void MoveMapping::order()
{
    QList<Move> todo = _moves;
    QList<Move> output, swaps;
    output.reserve(_moves.size());
    QList<Move> delayed;
    delayed.reserve(_moves.size());

    while (!todo.isEmpty()) {
        const Move m = todo.first();
        todo.removeFirst();
        schedule(m, todo, delayed, output, swaps);
    }

    output += swaps;

    Q_ASSERT(todo.isEmpty());
    Q_ASSERT(delayed.isEmpty());
    qSwap(_moves, output);
}

void MoveMapping::insertMoves(BasicBlock *bb, Function *function, bool atEnd) const
{
    int insertionPoint = atEnd ? bb->statements.size() - 1 : 0;
    foreach (const Move &m, _moves) {
        V4IR::Move *move = function->New<V4IR::Move>();
        move->init(m.to, m.from);
        move->id = m.id;
        move->swap = m.needsSwap;
        bb->statements.insert(insertionPoint++, move);
    }
}

void MoveMapping::dump() const
{
#if defined(DEBUG_MOVEMAPPING)
    QTextStream os(stdout, QIODevice::WriteOnly);
    os << "Move mapping has " << _moves.size() << " moves..." << endl;
    foreach (const Move &m, _moves) {
        os << "\t";
        m.to->dump(os);
        if (m.needsSwap)
            os << " <-> ";
        else
            os << " <-- ";
        m.from->dump(os);
        os << endl;
    }
#endif // DEBUG_MOVEMAPPING
}

MoveMapping::Action MoveMapping::schedule(const Move &m, QList<Move> &todo, QList<Move> &delayed,
                                          QList<Move> &output, QList<Move> &swaps) const
{
    int useIdx = isUsedAsSource(m.to);
    if (useIdx != -1) {
        const Move &dependency = _moves[useIdx];
        if (!output.contains(dependency)) {
            if (delayed.contains(dependency)) {
                // We have a cycle! Break it by swapping instead of assigning.
#if defined(DEBUG_MOVEMAPPING)
                delayed+=m;
                QTextStream out(stderr, QIODevice::WriteOnly);
                out<<"we have a cycle! temps:" << endl;
                foreach (const Move &m, delayed) {
                    out<<"\t";
                    m.to->dump(out);
                    out<<" <- ";
                    m.from->dump(out);
                    out<<endl;
                }
                delayed.removeOne(m);
#endif // DEBUG_MOVEMAPPING
                return NeedsSwap;
            } else {
                delayed.append(m);
                todo.removeOne(dependency);
                Action action = schedule(dependency, todo, delayed, output, swaps);
                delayed.removeOne(m);
                Move mm(m);
                if (action == NeedsSwap) {
                    mm.needsSwap = true;
                    swaps.append(mm);
                } else {
                    output.append(mm);
                }
                return action;
            }
        }
    }

    output.append(m);
    return NormalMove;
}
