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

#ifndef QT_NO_DEBUG
#  define _LIBCPP_DEBUG2 0
#endif // QT_NO_DEBUG

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
#include <cmath>
#include <iostream>
#include <cassert>
#include <algorithm>

#undef SHOW_SSA
#undef DEBUG_MOVEMAPPING

QT_USE_NAMESPACE

using namespace QV4;
using namespace IR;

namespace {

Q_GLOBAL_STATIC_WITH_ARGS(QTextStream, qout, (stderr, QIODevice::WriteOnly));
#define qout *qout()

void showMeTheCode(IR::Function *function)
{
    static bool showCode = !qgetenv("QV4_SHOW_IR").isNull();
    if (showCode) {
        QVector<Stmt *> code;
        QHash<Stmt *, BasicBlock *> leader;

        foreach (BasicBlock *block, function->basicBlocks()) {
            if (block->isRemoved() || block->isEmpty())
                continue;
            leader.insert(block->statements().first(), block);
            foreach (Stmt *s, block->statements()) {
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
                str.append(QByteArray::number(bb->index()));
                str.append(':');
                if (bb->catchBlock) {
                    str.append(" (exception handler L");
                    str.append(QByteArray::number(bb->catchBlock->index()));
                    str.append(')');
                }
                for (int i = 66 - str.length(); i; --i)
                    str.append(' ');
                qout << str;
                qout << "// predecessor blocks:";
                foreach (BasicBlock *in, bb->in)
                    qout << " L" << in->index();
                if (bb->in.isEmpty())
                    qout << "(none)";
                if (BasicBlock *container = bb->containingGroup())
                    qout << "; container block: L" << container->index();
                if (bb->isGroupStart())
                    qout << "; group start";
                qout << endl;
            }
            Stmt *n = (i + 1) < code.size() ? code.at(i + 1) : 0;

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

            qout << "    " << str;
            qout << endl;

            if (n && s->asCJump()) {
                qout << "    else goto L" << s->asCJump()->iffalse->index() << ";" << endl;
            }
        }

        qout << "}" << endl
             << endl;
    }
}

class ProcessedBlocks
{
    QBitArray processed;

public:
    ProcessedBlocks(IR::Function *function)
    {
        processed = QBitArray(function->basicBlockCount(), false);
    }

    bool alreadyProcessed(BasicBlock *bb) const
    {
        Q_ASSERT(bb);

        return processed.at(bb->index());
    }

    void markAsProcessed(BasicBlock *bb)
    {
        processed.setBit(bb->index());
    }
};

inline bool unescapableTemp(Temp *t, IR::Function *f)
{
    switch (t->kind) {
    case Temp::Formal:
    case Temp::ScopedFormal:
    case Temp::ScopedLocal:
        return false;
    case Temp::Local:
        return !f->variablesCanEscape();
    default:
        return true;
    }
}

inline Temp *unescapableTemp(Expr *e, IR::Function *f)
{
    Temp *t = e->asTemp();
    if (!t)
        return 0;

    return unescapableTemp(t, f) ? t : 0;
}

class BasicBlockSet
{
    typedef std::vector<int> Numbers;
    typedef std::vector<bool> Flags;

    Numbers *blockNumbers;
    Flags *blockFlags;
    IR::Function *function;
    enum { MaxVectorCapacity = 8 };

    // Q_DISABLE_COPY(BasicBlockSet); disabled because MSVC wants assignment operator for std::vector

public:
    class const_iterator
    {
        const BasicBlockSet &set;
        // ### These two members could go into a union, but clang won't compile (https://codereview.qt-project.org/#change,74259)
        Numbers::const_iterator numberIt;
        size_t flagIt;

        friend class BasicBlockSet;
        const_iterator(const BasicBlockSet &set, bool end)
            : set(set)
        {
            if (end) {
                if (set.blockNumbers)
                    numberIt = set.blockNumbers->end();
                else
                    flagIt = set.blockFlags->size();
            } else {
                if (set.blockNumbers)
                    numberIt = set.blockNumbers->begin();
                else
                    flagIt = std::distance(set.blockFlags->begin(),
                                           std::find(set.blockFlags->begin(),
                                                     set.blockFlags->end(),
                                                     true));
            }
        }

    public:
        BasicBlock *operator*() const
        {
            if (set.blockNumbers) {
                return set.function->basicBlock(*numberIt);
            } else {
                Q_ASSERT(flagIt <= INT_MAX);
                return set.function->basicBlock(static_cast<int>(flagIt));
            }
        }

        bool operator==(const const_iterator &other) const
        {
            if (&set != &other.set)
                return false;
            if (set.blockNumbers)
                return numberIt == other.numberIt;
            else
                return flagIt == other.flagIt;
        }

        bool operator!=(const const_iterator &other) const
        { return !(*this == other); }

        const_iterator &operator++()
        {
            if (set.blockNumbers)
                ++numberIt;
            else
                flagIt = std::distance(set.blockFlags->begin(),
                                       std::find(set.blockFlags->begin() + flagIt + 1,
                                                 set.blockFlags->end(),
                                                 true));

            return *this;
        }
    };

    friend class const_iterator;

public:
    BasicBlockSet(): blockNumbers(0), blockFlags(0), function(0) {}
#ifdef Q_COMPILER_RVALUE_REFS
    BasicBlockSet(BasicBlockSet &&other): blockNumbers(0), blockFlags(0)
    {
        std::swap(blockNumbers, other.blockNumbers);
        std::swap(blockFlags, other.blockFlags);
        std::swap(function, other.function);
    }

#endif // Q_COMPILER_RVALUE_REFS
    ~BasicBlockSet() { delete blockNumbers; delete blockFlags; }

    void init(IR::Function *f)
    {
        Q_ASSERT(!function);
        Q_ASSERT(f);
        function = f;
        blockNumbers = new Numbers;
        blockNumbers->reserve(MaxVectorCapacity);
    }

    void insert(BasicBlock *bb)
    {
        if (blockFlags) {
            (*blockFlags)[bb->index()] = true;
            return;
        }

        for (std::vector<int>::const_iterator i = blockNumbers->begin(), ei = blockNumbers->end();
             i != ei; ++i)
            if (*i == bb->index())
                return;

        if (blockNumbers->size() == MaxVectorCapacity) {
            blockFlags = new Flags(function->basicBlockCount(), false);
            for (std::vector<int>::const_iterator i = blockNumbers->begin(), ei = blockNumbers->end();
                 i != ei; ++i)
                blockFlags->operator[](*i) = true;
            delete blockNumbers;
            blockNumbers = 0;
            blockFlags->operator[](bb->index()) = true;
        } else {
            blockNumbers->push_back(bb->index());
        }
    }

    const_iterator begin() const { return const_iterator(*this, false); }
    const_iterator end() const { return const_iterator(*this, true); }

    QList<BasicBlock *> values() const
    {
        QList<BasicBlock *> result;

        for (const_iterator it = begin(), eit = end(); it != eit; ++it)
            result.append(*it);

        return result;
    }
};

class DominatorTree {
    typedef int BasicBlockIndex;
    enum { InvalidBasicBlockIndex = -1 };

    IR::Function *function;
    int N;
    std::vector<int> dfnum; // BasicBlock index -> dfnum
    std::vector<int> vertex;
    std::vector<BasicBlockIndex> parent; // BasicBlock index -> parent BasicBlock index
    std::vector<BasicBlockIndex> ancestor; // BasicBlock index -> ancestor BasicBlock index
    std::vector<BasicBlockIndex> best; // BasicBlock index -> best BasicBlock index
    std::vector<BasicBlockIndex> semi; // BasicBlock index -> semi dominator BasicBlock index
    std::vector<BasicBlockIndex> idom; // BasicBlock index -> immediate dominator BasicBlock index
    std::vector<BasicBlockIndex> samedom; // BasicBlock index -> same dominator BasicBlock index
    std::vector<BasicBlockSet> DF; // BasicBlock index -> dominator frontier

    struct DFSTodo {
        BasicBlockIndex node, parent;

        DFSTodo()
            : node(InvalidBasicBlockIndex)
            , parent(InvalidBasicBlockIndex)
        {}

        DFSTodo(BasicBlockIndex node, BasicBlockIndex parent)
            : node(node)
            , parent(parent)
        {}
    };

    void DFS(BasicBlockIndex node) {
        std::vector<DFSTodo> worklist;
        worklist.reserve(vertex.capacity() / 2);
        DFSTodo todo(node, InvalidBasicBlockIndex);

        while (true) {
            BasicBlockIndex n = todo.node;

            if (dfnum[n] == 0) {
                dfnum[n] = N;
                vertex[N] = n;
                parent[n] = todo.parent;
                ++N;
                const QVector<BasicBlock *> &out = function->basicBlock(n)->out;
                for (int i = out.size() - 1; i > 0; --i)
                    worklist.push_back(DFSTodo(out[i]->index(), n));

                if (out.size() > 0) {
                    todo.node = out.first()->index();
                    todo.parent = n;
                    continue;
                }
            }

            if (worklist.empty())
                break;

            todo = worklist.back();
            worklist.pop_back();
        }

#if defined(SHOW_SSA)
        for (int i = 0; i < nodes.size(); ++i)
            qDebug("\tL%d: dfnum = %d, parent = %d", i, dfnum[i], parent[i]);
#endif // SHOW_SSA
    }

    BasicBlockIndex ancestorWithLowestSemi(BasicBlockIndex v, std::vector<BasicBlockIndex> &worklist) {
        worklist.clear();
        for (BasicBlockIndex it = v; it != InvalidBasicBlockIndex; it = ancestor[it])
            worklist.push_back(it);

        if (worklist.size() < 2)
            return best[v];

        BasicBlockIndex b = InvalidBasicBlockIndex;
        BasicBlockIndex last = worklist.back();
        Q_ASSERT(worklist.size() <= INT_MAX);
        for (int it = static_cast<int>(worklist.size()) - 2; it >= 0; --it) {
            BasicBlockIndex bbIt = worklist[it];
            ancestor[bbIt] = last;
            BasicBlockIndex &best_it = best[bbIt];
            if (b != InvalidBasicBlockIndex && dfnum[semi[b]] < dfnum[semi[best_it]])
                best_it = b;
            else
                b = best_it;
        }
        return b;
    }

    void link(BasicBlockIndex p, BasicBlockIndex n) {
        ancestor[n] = p;
        best[n] = n;
    }

    void calculateIDoms() {
        Q_ASSERT(function->basicBlock(0)->in.isEmpty());

        const int bbCount = function->basicBlockCount();
        vertex = std::vector<int>(bbCount, InvalidBasicBlockIndex);
        parent = std::vector<int>(bbCount, InvalidBasicBlockIndex);
        dfnum = std::vector<int>(bbCount, 0);
        semi = std::vector<BasicBlockIndex>(bbCount, InvalidBasicBlockIndex);
        ancestor = std::vector<BasicBlockIndex>(bbCount, InvalidBasicBlockIndex);
        idom = std::vector<BasicBlockIndex>(bbCount, InvalidBasicBlockIndex);
        samedom = std::vector<BasicBlockIndex>(bbCount, InvalidBasicBlockIndex);
        best = std::vector<BasicBlockIndex>(bbCount, InvalidBasicBlockIndex);

        QHash<BasicBlockIndex, std::vector<BasicBlockIndex> > bucket;
        bucket.reserve(bbCount);

        DFS(function->basicBlock(0)->index());
        Q_ASSERT(N == function->liveBasicBlocksCount());

        std::vector<BasicBlockIndex> worklist;
        worklist.reserve(vertex.capacity() / 2);

        for (int i = N - 1; i > 0; --i) {
            BasicBlockIndex n = vertex[i];
            BasicBlockIndex p = parent[n];
            BasicBlockIndex s = p;

            foreach (BasicBlock *v, function->basicBlock(n)->in) {
                BasicBlockIndex ss = InvalidBasicBlockIndex;
                if (dfnum[v->index()] <= dfnum[n])
                    ss = v->index();
                else
                    ss = semi[ancestorWithLowestSemi(v->index(), worklist)];
                if (dfnum[ss] < dfnum[s])
                    s = ss;
            }
            semi[n] = s;
            bucket[s].push_back(n);
            link(p, n);
            if (bucket.contains(p)) {
                foreach (BasicBlockIndex v, bucket[p]) {
                    BasicBlockIndex y = ancestorWithLowestSemi(v, worklist);
                    BasicBlockIndex semi_v = semi[v];
                    if (semi[y] == semi_v)
                        idom[v] = semi_v;
                    else
                        samedom[v] = y;
                }
                bucket.remove(p);
            }
        }

#if defined(SHOW_SSA)
        for (int i = 0; i < nodes.size(); ++i)
            qDebug("\tL%d: ancestor = %d, semi = %d, samedom = %d", i, ancestor[i], semi[i], samedom[i]);
#endif // SHOW_SSA

        for (int i = 1; i < N; ++i) {
            BasicBlockIndex n = vertex[i];
            Q_ASSERT(n != InvalidBasicBlockIndex);
            Q_ASSERT(!bucket.contains(n));
            Q_ASSERT(ancestor[n] != InvalidBasicBlockIndex
                        && ((semi[n] != InvalidBasicBlockIndex
                                && dfnum[ancestor[n]] <= dfnum[semi[n]]) || semi[n] == n));
            BasicBlockIndex sdn = samedom[n];
            if (sdn != InvalidBasicBlockIndex)
                idom[n] = idom[sdn];
        }

#if defined(SHOW_SSA)
        qout << "Immediate dominators:" << endl;
        foreach (BasicBlock *to, nodes) {
            qout << '\t';
            BasicBlockIndex from = idom.at(to->index);
            if (from != InvalidBasicBlockIndex)
                qout << from;
            else
                qout << "(none)";
            qout << " -> " << to->index << endl;
        }
        qout << "N = " << N << endl;
#endif // SHOW_SSA
    }

    struct NodeProgress {
        std::vector<BasicBlockIndex> children;
        std::vector<BasicBlockIndex> todo;
    };

    void computeDF() {
        // compute children of each node in the dominator tree
        std::vector<std::vector<BasicBlockIndex> > children; // BasicBlock index -> children
        children.resize(function->basicBlockCount());
        foreach (BasicBlock *n, function->basicBlocks()) {
            if (n->isRemoved())
                continue;
            const BasicBlockIndex nodeIndex = n->index();
            Q_ASSERT(function->basicBlock(nodeIndex) == n);
            const BasicBlockIndex nodeDominator = idom[nodeIndex];
            if (nodeDominator == InvalidBasicBlockIndex)
                continue; // there is no dominator to add this node to as a child (e.g. the start node)
            children[nodeDominator].push_back(nodeIndex);
        }

        // Fill the worklist and initialize the node status for each basic-block
        QHash<BasicBlockIndex, NodeProgress> nodeStatus;
        nodeStatus.reserve(function->basicBlockCount());
        std::vector<BasicBlockIndex> worklist;
        worklist.reserve(function->basicBlockCount() * 2);
        foreach (BasicBlock *bb, function->basicBlocks()) {
            if (bb->isRemoved())
                continue;
            BasicBlockIndex nodeIndex = bb->index();
            worklist.push_back(nodeIndex);
            NodeProgress &np = nodeStatus[nodeIndex];
            np.children = children[nodeIndex];
            np.todo = children[nodeIndex];
        }

        std::vector<bool> DF_done(function->basicBlockCount(), false);

        while (!worklist.empty()) {
            BasicBlockIndex node = worklist.back();

            if (DF_done[node]) {
                worklist.pop_back();
                continue;
            }

            NodeProgress &np = nodeStatus[node];
            std::vector<BasicBlockIndex>::iterator it = np.todo.begin();
            while (it != np.todo.end()) {
                if (DF_done[*it]) {
                    it = np.todo.erase(it);
                } else {
                    worklist.push_back(*it);
                    break;
                }
            }

            if (np.todo.empty()) {
                BasicBlockSet &S = DF[node];
                S.init(function);
                foreach (BasicBlock *y, function->basicBlock(node)->out)
                    if (idom[y->index()] != node)
                        S.insert(y);
                foreach (BasicBlockIndex child, np.children) {
                    const BasicBlockSet &ws = DF[child];
                    for (BasicBlockSet::const_iterator it = ws.begin(), eit = ws.end(); it != eit; ++it) {
                        BasicBlock *w = *it;
                        const BasicBlockIndex wIndex = w->index();
                        if (node == wIndex || !dominates(node, w->index()))
                            S.insert(w);
                    }
                }
                DF_done[node] = true;
                worklist.pop_back();
            }
        }

#if defined(SHOW_SSA)
        qout << "Dominator Frontiers:" << endl;
        foreach (BasicBlock *n, nodes) {
            qout << "\tDF[" << n->index << "]: {";
            QList<BasicBlock *> SList = DF[n->index].values();
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
            const BasicBlockSet &fBlocks = DF[n->index];
            for (BasicBlockSet::const_iterator it = fBlocks.begin(), eit = fBlocks.end(); it != eit; ++it) {
                BasicBlock *fBlock = *it;
                Q_ASSERT(!dominates(n, fBlock) || fBlock == n);
                bool hasDominatedSucc = false;
                foreach (BasicBlock *succ, fBlock->in) {
                    if (dominates(n, succ)) {
                        hasDominatedSucc = true;
                        break;
                    }
                }
                if (!hasDominatedSucc) {
                    qout << fBlock << " in DF[" << n->index << "] has no dominated predecessors" << endl;
                }
                Q_ASSERT(hasDominatedSucc);
            }
        }
#endif // !QT_NO_DEBUG
    }

public:
    DominatorTree(IR::Function *function)
        : function(function)
        , N(0)
    {
        DF.resize(function->basicBlockCount());
        calculateIDoms();
        computeDF();
    }

    const BasicBlockSet &dominatorFrontier(BasicBlock *n) const {
        return DF[n->index()];
    }

    BasicBlock *immediateDominator(BasicBlock *bb) const {
        return function->basicBlock(idom[bb->index()]);
    }

    void dumpImmediateDominators() const
    {
        qDebug() << "Immediate dominators for" << idom.size() << "nodes:";
        for (size_t i = 0, ei = idom.size(); i != ei; ++i)
            if (idom[i] == InvalidBasicBlockIndex)
                qDebug("\tnone -> L%d", int(i));
            else
                qDebug("\tL%d -> L%d", idom[i], int(i));
    }

    void updateImmediateDominator(BasicBlock *bb, BasicBlock *newDominator)
    {
        Q_ASSERT(bb->index() >= 0);

        if (static_cast<std::vector<BasicBlockIndex>::size_type>(bb->index()) >= idom.size()) {
            // This is a new block, probably introduced by edge splitting. So, we'll have to grow
            // the array before inserting the immediate dominator.
            idom.resize(function->basicBlockCount(), InvalidBasicBlockIndex);
        }

        idom[bb->index()] = newDominator->index();
    }

    bool dominates(BasicBlock *dominator, BasicBlock *dominated) const {
        return dominates(dominator->index(), dominated->index());
    }

private:
    bool dominates(BasicBlockIndex dominator, BasicBlockIndex dominated) const {
        // dominator can be Invalid when the dominated block has no dominator (i.e. the start node)
        Q_ASSERT(dominated != InvalidBasicBlockIndex);

        if (dominator == dominated)
            return false;

        for (BasicBlockIndex it = idom[dominated]; it != InvalidBasicBlockIndex; it = idom[it]) {
            if (it == dominator)
                return true;
        }

        return false;
    }
};

class VariableCollector: public StmtVisitor, ExprVisitor {
    typedef QHash<Temp, QSet<BasicBlock *> > DefSites;
    DefSites _defsites;
    QVector<QSet<Temp> > A_orig;
    QSet<Temp> nonLocals;
    QSet<Temp> killed;

    BasicBlock *currentBB;
    IR::Function *function;
    bool isCollectable(Temp *t) const
    {
        Q_ASSERT(t->kind != Temp::PhysicalRegister && t->kind != Temp::StackSlot);
        return unescapableTemp(t, function);
    }

public:
    VariableCollector(IR::Function *function)
        : function(function)
    {
        _defsites.reserve(function->tempCount);
        A_orig.resize(function->basicBlockCount());
        for (int i = 0, ei = A_orig.size(); i != ei; ++i)
            A_orig[i].reserve(8);

#if defined(SHOW_SSA)
        qout << "Variables collected:" << endl;
#endif // SHOW_SSA

        foreach (BasicBlock *bb, function->basicBlocks()) {
            if (bb->isRemoved())
                continue;

            currentBB = bb;
            killed.clear();
            killed.reserve(bb->statements().size() / 2);
            foreach (Stmt *s, bb->statements()) {
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
        return A_orig.at(n->index());
    }

    bool isNonLocal(const Temp &var) const { return nonLocals.contains(var); }

protected:
    virtual void visitPhi(Phi *) {};
    virtual void visitConvert(Convert *e) { e->expr->accept(this); };

    virtual void visitConst(Const *) {}
    virtual void visitString(IR::String *) {}
    virtual void visitRegExp(IR::RegExp *) {}
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

                DefSites::iterator defsitesIt = _defsites.find(*t);
                if (defsitesIt == _defsites.end()) {
                    QSet<BasicBlock *> bbs;
                    bbs.reserve(4);
                    defsitesIt = _defsites.insert(*t, bbs);
                }
                defsitesIt->insert(currentBB);

                A_orig[currentBB->index()].insert(*t);

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

void insertPhiNode(const Temp &a, BasicBlock *y, IR::Function *f) {
#if defined(SHOW_SSA)
    qout << "-> inserted phi node for variable ";
    a.dump(qout);
    qout << " in block " << y->index << endl;
#endif

    Phi *phiNode = f->New<Phi>();
    phiNode->d = new Stmt::Data;
    phiNode->targetTemp = f->New<Temp>();
    phiNode->targetTemp->init(a.kind, a.index, 0);
    y->prependStatement(phiNode);

    phiNode->d->incoming.resize(y->in.size());
    for (int i = 0, ei = y->in.size(); i < ei; ++i) {
        Temp *t = f->New<Temp>();
        t->init(a.kind, a.index, 0);
        phiNode->d->incoming[i] = t;
    }
}

// High-level (recursive) algorithm:
//   Mapping: old temp number -> new temp number
//
//   Start:
//     Rename(start-node)
//
//   Rename(node, mapping):
//     for each statement S in block n
//       if S not in a phi-function
//         for each use of some variable x in S
//           y = mapping[x]
//           replace the use of x with y in S
//       for each definition of some variable a in S                        [1]
//         a_new = generate new/unique temp
//         mapping[a] = a_new
//         replace definition of a with definition of a_new in S
//     for each successor Y of block n
//       Suppose n is the j-th predecessor of Y
//       for each phi function in Y
//         suppose the j-th operand of the phi-function is a
//         i = mapping[a]
//         replace the j-th operand with a_i
//     for each child X of n                                                [2]
//       Rename(X)
//     for each newly generated temp from step [1] restore the old value    [3]
//
// This algorithm can run out of CPU stack space when there are lots of basic-blocks, like in a
// switch statement with 8000 cases that all fall-through. The iterativer version below uses a
// work-item stack, where step [1] from the algorithm above also pushes an "undo mapping change",
// and step [2] pushes a "rename(X)" action. This eliminates step [3].
//
// Iterative version:
//   Mapping: old temp number -> new temp number
//
//   The stack can hold two kinds of actions:
//     "Rename basic block n"
//     "Restore count for temp"
//
//   Start:
//     counter = 0
//     push "Rename start node" onto the stack
//     while the stack is not empty:
//       take the last item, and process it
//
//   Rename(n) =
//     for each statement S in block n
//       if S not in a phi-function
//         for each use of some variable x in S
//           y = mapping[x]
//           replace the use of x with y in S
//       for each definition of some variable a in S
//         old = mapping[a]
//         push Undo(a, old)
//         counter = counter + 1
//         new = counter;
//         mapping[a] = new
//         replace definition of a with definition of a_new in S
//     for each successor Y of block n
//       Suppose n is the j-th predecessor of Y
//       for each phi function in Y
//         suppose the j-th operand of the phi-function is a
//         i = mapping[a]
//         replace the j-th operand with a_i
//     for each child X of n
//       push Rename(X)
//
//   Undo(t, c) =
//     mapping[t] = c
class VariableRenamer: public StmtVisitor, public ExprVisitor
{
    IR::Function *function;
    unsigned tempCount;

    typedef QHash<unsigned, int> Mapping; // maps from existing/old temp number to the new and unique temp number.
    enum { Absent = -1 };
    Mapping localMapping;
    Mapping vregMapping;
    ProcessedBlocks processed;

    bool isRenamable(Temp *t) const
    {
        Q_ASSERT(t->kind != Temp::PhysicalRegister && t->kind != Temp::StackSlot);
        return unescapableTemp(t, function);
    }

    struct TodoAction {
        enum { RestoreLocal, RestoreVReg, Rename } action;
        union {
            struct {
                unsigned temp;
                int previous;
            } restoreData;
            struct {
                BasicBlock *basicBlock;
            } renameData;
        };

        bool isValid() const { return action != Rename || renameData.basicBlock != 0; }

        TodoAction()
        {
            action = Rename;
            renameData.basicBlock = 0;
        }

        TodoAction(const Temp &t, int prev)
        {
            Q_ASSERT(t.kind == Temp::Local || t.kind == Temp::VirtualRegister);

            action = t.kind == Temp::Local ? RestoreLocal : RestoreVReg;
            restoreData.temp = t.index;
            restoreData.previous = prev;
        }

        TodoAction(BasicBlock *bb)
        {
            Q_ASSERT(bb);

            action = Rename;
            renameData.basicBlock = bb;
        }
    };

    QVector<TodoAction> todo;

public:
    VariableRenamer(IR::Function *f)
        : function(f)
        , tempCount(0)
        , processed(f)
    {
        localMapping.reserve(f->tempCount);
        vregMapping.reserve(f->tempCount);
        todo.reserve(f->basicBlockCount());
    }

    void run() {
        todo.append(TodoAction(function->basicBlock(0)));

        while (!todo.isEmpty()) {
            TodoAction todoAction = todo.back();
            Q_ASSERT(todoAction.isValid());
            todo.pop_back();

            switch (todoAction.action) {
            case TodoAction::Rename:
                rename(todoAction.renameData.basicBlock);
                break;
            case TodoAction::RestoreLocal:
                restore(localMapping, todoAction.restoreData.temp, todoAction.restoreData.previous);
                break;
            case TodoAction::RestoreVReg:
                restore(vregMapping, todoAction.restoreData.temp, todoAction.restoreData.previous);
                break;
            default:
                Q_UNREACHABLE();
            }
        }

        function->tempCount = tempCount;
    }

private:
    static inline void restore(Mapping &mapping, unsigned temp, int previous)
    {
        if (previous == Absent)
            mapping.remove(temp);
        else
            mapping[temp] = previous;
    }

    void rename(BasicBlock *bb)
    {
        while (bb && !processed.alreadyProcessed(bb)) {
            renameStatementsAndPhis(bb);
            processed.markAsProcessed(bb);

            BasicBlock *next = 0;
            foreach (BasicBlock *out, bb->out) {
                if (processed.alreadyProcessed(out))
                    continue;
                if (!next)
                    next = out;
                else
                    todo.append(TodoAction(out));
            }
            bb = next;
        }
    }

    void renameStatementsAndPhis(BasicBlock *bb)
    {
        foreach (Stmt *s, bb->statements())
            s->accept(this);

        foreach (BasicBlock *Y, bb->out) {
            const int j = Y->in.indexOf(bb);
            Q_ASSERT(j >= 0 && j < Y->in.size());
            foreach (Stmt *s, Y->statements()) {
                if (Phi *phi = s->asPhi()) {
                    Temp *t = phi->d->incoming[j]->asTemp();
                    unsigned newTmp = currentNumber(*t);
//                    qDebug()<<"I: replacing phi use"<<a<<"with"<<newTmp<<"in L"<<Y->index;
                    t->index = newTmp;
                    t->kind = Temp::VirtualRegister;
                } else {
                    break;
                }
            }
        }
    }

    unsigned currentNumber(const Temp &t)
    {
        int nr = Absent;
        switch (t.kind) {
        case Temp::Local:
            nr = localMapping.value(t.index, Absent);
            break;
        case Temp::VirtualRegister:
            nr = vregMapping.value(t.index, Absent);
            break;
        default:
            Q_UNREACHABLE();
            nr = Absent;
            break;
        }
        if (nr == Absent) {
            // Special case: we didn't prune the Phi nodes yet, so for proper temps (virtual
            // registers) the SSA algorithm might insert superfluous Phis that have uses without
            // definition. E.g.: if a temporary got introduced in the "then" clause, it "could"
            // reach the "end-if" block, so there will be a phi node for that temp. A later pass
            // will clean this up by looking for uses-without-defines in phi nodes. So, what we do
            // is to generate a new unique number, and leave it dangling.
            nr = nextFreeTemp(t);
        }

        return nr;
    }

    unsigned nextFreeTemp(const Temp &t)
    {
        unsigned newIndex = tempCount++;
        Q_ASSERT(newIndex <= INT_MAX);
        int oldIndex = Absent;

        switch (t.kind) {
        case Temp::Local:
            oldIndex = localMapping.value(t.index, Absent);
            localMapping.insert(t.index, newIndex);
            break;
        case Temp::VirtualRegister:
            oldIndex = vregMapping.value(t.index, Absent);
            vregMapping.insert(t.index, newIndex);
            break;
        default:
            Q_UNREACHABLE();
        }

        todo.append(TodoAction(t, oldIndex));

        return newIndex;
    }

protected:
    virtual void visitTemp(Temp *e) { // only called for uses, not defs
        if (isRenamable(e)) {
//            qDebug()<<"I: replacing use of"<<e->index<<"with"<<stack[e->index].top();
            e->index = currentNumber(*e);
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
            const int newIdx = nextFreeTemp(*t);
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
    virtual void visitString(IR::String *) {}
    virtual void visitRegExp(IR::RegExp *) {}
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

void convertToSSA(IR::Function *function, const DominatorTree &df)
{
#if defined(SHOW_SSA)
    qout << "Converting function ";
    if (function->name)
        qout << *function->name;
    else
        qout << "<no name>";
    qout << " to SSA..." << endl;
#endif // SHOW_SSA

    // Collect all applicable variables:
    VariableCollector variables(function);

    // Prepare for phi node insertion:
    QVector<QSet<Temp> > A_phi;
    A_phi.resize(function->basicBlockCount());
    for (int i = 0, ei = A_phi.size(); i != ei; ++i) {
        QSet<Temp> temps;
        temps.reserve(4);
        A_phi[i] = temps;
    }

    // Place phi functions:
    foreach (Temp a, variables.vars()) {
        if (!variables.isNonLocal(a))
            continue; // for semi-pruned SSA

        QList<BasicBlock *> W = QList<BasicBlock *>::fromSet(variables.defsite(a));
        while (!W.isEmpty()) {
            BasicBlock *n = W.first();
            W.removeFirst();
            const BasicBlockSet &dominatorFrontierForN = df.dominatorFrontier(n);
            for (BasicBlockSet::const_iterator it = dominatorFrontierForN.begin(), eit = dominatorFrontierForN.end();
                 it != eit; ++it) {
                BasicBlock *y = *it;
                if (!A_phi.at(y->index()).contains(a)) {
                    insertPhiNode(a, y, function);
                    A_phi[y->index()].insert(a);
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
    UntypedTemp() {}
    UntypedTemp(const Temp &t): temp(t) {}
};
inline uint qHash(const UntypedTemp &t, uint seed = 0) Q_DECL_NOTHROW
{ return t.temp.index ^ (t.temp.kind | (t.temp.scope << 3)) ^ seed; }
inline bool operator==(const UntypedTemp &t1, const UntypedTemp &t2) Q_DECL_NOTHROW
{ return t1.temp.index == t2.temp.index && t1.temp.scope == t2.temp.scope && t1.temp.kind == t2.temp.kind; }
inline bool operator!=(const UntypedTemp &t1, const UntypedTemp &t2) Q_DECL_NOTHROW
{ return !(t1 == t2); }

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
    IR::Function *function;
    typedef QHash<UntypedTemp, DefUse> DefUses;
    DefUses _defUses;
    QHash<Stmt *, QList<Temp> > _usesPerStatement;

    BasicBlock *_block;
    Stmt *_stmt;

    bool isCollectible(Temp *t) const {
        Q_ASSERT(t->kind != Temp::PhysicalRegister && t->kind != Temp::StackSlot);
        return unescapableTemp(t, function);
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
    DefUsesCalculator(IR::Function *function)
        : function(function)
    {
        foreach (BasicBlock *bb, function->basicBlocks()) {
            if (bb->isRemoved())
                continue;

            _block = bb;
            foreach (Stmt *stmt, bb->statements()) {
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

    QList<UntypedTemp> defsUntyped() const { return _defUses.keys(); }

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

    int useCount(const UntypedTemp &variable) const
    { return _defUses[variable].uses.size(); }

    Stmt *defStmt(const UntypedTemp &variable) const
    { return _defUses[variable].defStmt; }

    BasicBlock *defStmtBlock(const Temp &variable) const
    { return _defUses[variable].blockOfStatement; }

    void removeUse(Stmt *usingStmt, const Temp &var)
    { _defUses[var].uses.removeAll(usingStmt); }

    QList<Temp> usedVars(Stmt *s) const
    { return _usesPerStatement[s]; }

    const QList<Stmt *> &uses(const UntypedTemp &var) const
    {
        static const QList<Stmt *> noUses;

        DefUses::const_iterator it = _defUses.find(var);
        if (it == _defUses.end())
            return noUses;
        else
            return it->uses;
    }

    QVector<Stmt*> removeDefUses(Stmt *s)
    {
        QVector<Stmt*> defStmts;
        foreach (const Temp &usedVar, usedVars(s)) {
            if (Stmt *ds = defStmt(usedVar))
                defStmts += ds;
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
            qout<<" -> defined in block "<<du.blockOfStatement->index()<<", statement: ";
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

        defUses.defStmtBlock(targetVar)->removeStatement(phi);

        foreach (const Temp &usedVar, defUses.usedVars(phi))
            defUses.removeUse(phi, usedVar);
        defUses.removeDef(targetVar);
    }
}

class StatementWorklist
{
    QVector<Stmt *> worklist;
    QBitArray inWorklist;
    QSet<Stmt *> removed;
    QHash<Stmt*,Stmt*> replaced;

    Q_DISABLE_COPY(StatementWorklist)

public:
    StatementWorklist(IR::Function *function)
    {
        QVector<Stmt *> w;
        int stmtCount = 0;

        // Put in all statements, and number them on the fly. The numbering is used to index the
        // bit array.
        foreach (BasicBlock *bb, function->basicBlocks()) {
            if (bb->isRemoved())
                continue;

            foreach (Stmt *s, bb->statements()) {
                s->id = stmtCount++;
                w.append(s);
            }
        }

        // For QVector efficiency reasons, we process statements from the back. However, it is more
        // effective to process the statements in ascending order. So we need to invert the
        // order.
        worklist.reserve(w.size());
        for (int i = w.size() - 1; i >= 0; --i)
            worklist.append(w.at(i));

        inWorklist = QBitArray(stmtCount, true);
    }

    // This will clear the entry for the statement in the basic block. After processing all
    // statements, the cleanup method needs to be run to remove all null-pointers.
    void clear(Stmt *stmt)
    {
        Q_ASSERT(!inWorklist.at(stmt->id));
        removed.insert(stmt);
    }

    void replace(Stmt *oldStmt, Stmt *newStmt)
    {
        Q_ASSERT(oldStmt);
        Q_ASSERT(newStmt);
        Q_ASSERT(!removed.contains(oldStmt));

        if (newStmt->id == -1)
            newStmt->id = oldStmt->id;
        QHash<Stmt *, Stmt *>::const_iterator it = replaced.find(oldStmt);
        if (it != replaced.end())
            oldStmt = it.key();
        replaced[oldStmt] = newStmt;
    }

    void cleanup(IR::Function *function)
    {
        foreach (BasicBlock *bb, function->basicBlocks()) {
            if (bb->isRemoved())
                continue;

            for (int i = 0; i < bb->statementCount();) {
                Stmt *stmt = bb->statements()[i];
                QHash<Stmt *, Stmt *>::const_iterator it = replaced.find(stmt);
                if (it != replaced.end() && !removed.contains(it.value())) {
                    bb->replaceStatement(i, it.value());
                } else if (removed.contains(stmt)) {
                    bb->removeStatement(i);
                    continue;
                }

                ++i;
            }
        }
    }

    StatementWorklist &operator+=(const QVector<Stmt *> &stmts)
    {
        foreach (Stmt *s, stmts)
            this->operator+=(s);

        return *this;
    }


    StatementWorklist &operator+=(Stmt *s)
    {
        if (!s)
            return *this;

        Q_ASSERT(s->id >= 0);
        Q_ASSERT(s->id < inWorklist.size());

        if (!inWorklist.at(s->id)) {
            worklist.append(s);
            inWorklist.setBit(s->id);
        }

        return *this;
    }

    StatementWorklist &operator-=(Stmt *s)
    {
        Q_ASSERT(s->id >= 0);
        Q_ASSERT(s->id < inWorklist.size());

        if (inWorklist.at(s->id)) {
            worklist.remove(worklist.indexOf(s));
            inWorklist.clearBit(s->id);
        }

        return *this;
    }

    bool isEmpty() const
    {
        return worklist.isEmpty();
    }

    Stmt *takeOne()
    {
        if (isEmpty())
            return 0;

        Stmt *s = worklist.last();
        Q_ASSERT(s->id < inWorklist.size());
        worklist.removeLast();
        inWorklist.clearBit(s->id);
        return s;
    }
};

class EliminateDeadCode: public ExprVisitor {
    DefUsesCalculator &_defUses;
    StatementWorklist &_worklist;
    IR::Function *function;
    bool _sideEffect;
    QVector<Temp *> _collectedTemps;

public:
    EliminateDeadCode(DefUsesCalculator &defUses, StatementWorklist &worklist, IR::Function *function)
        : _defUses(defUses)
        , _worklist(worklist)
        , function(function)
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
        return unescapableTemp(t, function);
    }

protected:
    virtual void visitConst(Const *) {}
    virtual void visitString(IR::String *) {}
    virtual void visitRegExp(IR::RegExp *) {}

    virtual void visitName(Name *e)
    {
        if (e->freeOfSideEffects)
            return;
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
        case QObjectType:
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
            if (e->expr->type == VarType || e->expr->type == StringType || e->expr->type == QObjectType)
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

        if (e->left->type == VarType || e->left->type == StringType || e->left->type == QObjectType
                || e->right->type == VarType || e->right->type == StringType || e->right->type == QObjectType)
            markAsSideEffect();
    }

    virtual void visitSubscript(Subscript *e) {
        e->base->accept(this);
        e->index->accept(this);
        markAsSideEffect();
    }

    virtual void visitMember(Member *e) {
        e->base->accept(this);
        if (e->freeOfSideEffects)
            return;
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

struct DiscoveredType {
    int type;
    MemberExpressionResolver memberResolver;

    DiscoveredType() : type(UnknownType) {}
    DiscoveredType(Type t) : type(t) { Q_ASSERT(type != QObjectType); }
    explicit DiscoveredType(int t) : type(t) { Q_ASSERT(type != QObjectType); }
    explicit DiscoveredType(MemberExpressionResolver memberResolver) : type(QObjectType), memberResolver(memberResolver) {}

    bool test(Type t) const { return type & t; }
    bool isNumber() const { return (type & NumberType) && !(type & ~NumberType); }

    bool operator!=(Type other) const { return type != other; }
    bool operator==(Type other) const { return type == other; }
    bool operator==(const DiscoveredType &other) const { return type == other.type; }
    bool operator!=(const DiscoveredType &other) const { return type != other.type; }
};

class PropagateTempTypes: public StmtVisitor, ExprVisitor
{
    const DefUsesCalculator &defUses;
    UntypedTemp theTemp;
    DiscoveredType newType;

public:
    PropagateTempTypes(const DefUsesCalculator &defUses)
        : defUses(defUses)
    {}

    void run(const UntypedTemp &temp, const DiscoveredType &type)
    {
        newType = type;
        theTemp = temp;
        if (Stmt *defStmt = defUses.defStmt(temp))
            defStmt->accept(this);
        foreach (Stmt *use, defUses.uses(temp))
            use->accept(this);
    }

protected:
    virtual void visitConst(Const *) {}
    virtual void visitString(IR::String *) {}
    virtual void visitRegExp(IR::RegExp *) {}
    virtual void visitName(Name *) {}
    virtual void visitTemp(Temp *e) {
        if (theTemp == UntypedTemp(*e)) {
            e->type = static_cast<Type>(newType.type);
            e->memberResolver = newType.memberResolver;
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
};

class TypeInference: public StmtVisitor, public ExprVisitor {
    QQmlEnginePrivate *qmlEngine;
    IR::Function *function;
    const DefUsesCalculator &_defUses;
    typedef QHash<Temp, DiscoveredType> TempTypes;
    TempTypes _tempTypes;
    QList<Stmt *> _worklist;
    struct TypingResult {
        DiscoveredType type;
        bool fullyTyped;

        TypingResult(const DiscoveredType &type = DiscoveredType()) : type(type), fullyTyped(type.type != UnknownType) {}
        explicit TypingResult(MemberExpressionResolver memberResolver): type(memberResolver), fullyTyped(true) {}
    };
    TypingResult _ty;

public:
    TypeInference(QQmlEnginePrivate *qmlEngine, const DefUsesCalculator &defUses)
        : qmlEngine(qmlEngine)
        , _defUses(defUses)
        , _ty(UnknownType)
    {}

    void run(IR::Function *f) {
        function = f;

        // TODO: the worklist handling looks a bit inefficient... check if there is something better
        _worklist.clear();
        for (int i = 0, ei = function->basicBlockCount(); i != ei; ++i) {
            BasicBlock *bb = function->basicBlock(i);
            if (bb->isRemoved())
                continue;
            if (i == 0 || !bb->in.isEmpty())
                _worklist += bb->statements().toList();
        }

        while (!_worklist.isEmpty()) {
            QList<Stmt *> worklist = QSet<Stmt *>::fromList(_worklist).toList();
            _worklist.clear();
            while (!worklist.isEmpty()) {
                Stmt *s = worklist.first();
                worklist.removeFirst();
                if (s->asJump())
                    continue;

#if defined(SHOW_SSA)
                qout<<"Typing stmt ";s->dump(qout);qout<<endl;
#endif

                if (!run(s)) {
                    _worklist += s;
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

        PropagateTempTypes propagator(_defUses);
        for (QHash<Temp, DiscoveredType>::const_iterator i = _tempTypes.begin(), ei = _tempTypes.end(); i != ei; ++i)
            propagator.run(i.key(), i.value());
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

    bool isAlwaysVar(Temp *t) {
        if (unescapableTemp(t, function))
            return false;
        t->type = VarType;
        return true;
    }

    void setType(Expr *e, DiscoveredType ty) {
        if (Temp *t = e->asTemp()) {
#if defined(SHOW_SSA)
            qout<<"Setting type for "<< (t->scope?"scoped temp ":"temp ") <<t->index<< " to "<<typeName(Type(ty)) << " (" << ty << ")" << endl;
#endif
            if (isAlwaysVar(t))
                ty = DiscoveredType(VarType);
            TempTypes::iterator it = _tempTypes.find(*t);
            if (it == _tempTypes.end())
                it = _tempTypes.insert(*t, DiscoveredType());
            if (it.value() != ty) {
                it.value() = ty;

#if defined(SHOW_SSA)
                foreach (Stmt *s, _defUses.uses(*t)) {
                    qout << "Pushing back dependent stmt: ";
                    s->dump(qout);
                    qout << endl;
                }
#endif

                _worklist += _defUses.uses(*t);
            }
        } else {
            e->type = (Type) ty.type;
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
    virtual void visitString(IR::String *) { _ty = TypingResult(StringType); }
    virtual void visitRegExp(IR::RegExp *) { _ty = TypingResult(VarType); }
    virtual void visitName(Name *) { _ty = TypingResult(VarType); }
    virtual void visitTemp(Temp *e) {
        if (isAlwaysVar(e))
            _ty = TypingResult(VarType);
        else if (e->memberResolver.isValid())
            _ty = TypingResult(e->memberResolver);
        else
            _ty = TypingResult(_tempTypes.value(*e));
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
            if (leftTy.type.test(VarType) || leftTy.type.test(QObjectType) || rightTy.type.test(VarType) || rightTy.type.test(QObjectType))
                _ty.type = VarType;
            else if (leftTy.type.test(StringType) || rightTy.type.test(StringType))
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
        _ty = run(e->base);

        if (_ty.fullyTyped && _ty.type.memberResolver.isValid()) {
            MemberExpressionResolver &resolver = _ty.type.memberResolver;
            _ty.type.type = resolver.resolveMember(qmlEngine, &resolver, e);
        } else
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
            _ty.type.type |= ty.type.type;
            _ty.fullyTyped &= ty.fullyTyped;
            if (_ty.type.test(QObjectType))
                _ty.type.memberResolver.clear(); // ### TODO: find common ancestor meta-object
        }

        switch (_ty.type.type) {
        case UnknownType:
        case UndefinedType:
        case NullType:
        case BoolType:
        case SInt32Type:
        case UInt32Type:
        case DoubleType:
        case StringType:
        case QObjectType:
        case VarType:
            // The type is not a combination of two or more types, so we're done.
            break;

        default:
            // There are multiple types involved, so:
            if (_ty.type.isNumber())
                // The type is any combination of double/int32/uint32, but nothing else. So we can
                // type it as double.
                _ty.type = DoubleType;
            else
                // There just is no single type that can hold this combination, so:
                _ty.type = VarType;
        }

        setType(s->targetTemp, _ty.type);
    }
};

class ReverseInference
{
    const DefUsesCalculator &_defUses;

public:
    ReverseInference(const DefUsesCalculator &defUses)
        : _defUses(defUses)
    {}

    void run(IR::Function *f)
    {
        QVector<UntypedTemp> knownOk;
        QList<UntypedTemp> candidates = _defUses.defsUntyped();
        while (!candidates.isEmpty()) {
            UntypedTemp temp = candidates.last();
            candidates.removeLast();

            if (knownOk.contains(temp))
                continue;

            if (!isUsedAsInt32(temp, knownOk))
                continue;

            Stmt *s = _defUses.defStmt(temp);
            Move *m = s->asMove();
            if (!m)
                continue;
            Temp *target = m->target->asTemp();
            if (!target || temp != UntypedTemp(*target) || target->type == SInt32Type)
                continue;
            if (Temp *t = m->source->asTemp()) {
                candidates.append(*t);
            } else if (m->source->asConvert()) {
                break;
            } else if (Binop *b = m->source->asBinop()) {
                switch (b->op) {
                case OpAdd:
                    if (b->left->type & NumberType || b->right->type & NumberType)
                        break;
                    else
                        continue;
                case OpBitAnd:
                case OpBitOr:
                case OpBitXor:
                case OpSub:
                case OpMul:
                case OpLShift:
                case OpRShift:
                case OpURShift:
                    break;
                default:
                    continue;
                }
                if (Temp *lt = unescapableTemp(b->left, f))
                    candidates.append(*lt);
                if (Temp *rt = unescapableTemp(b->right, f))
                    candidates.append(*rt);
            } else if (Unop *u = m->source->asUnop()) {
                if (u->op == OpCompl || u->op == OpUPlus) {
                    if (Temp *t = unescapableTemp(u->expr, f))
                        candidates.append(*t);
                }
            } else {
                continue;
            }

            knownOk.append(temp);
        }

        PropagateTempTypes propagator(_defUses);
        foreach (const UntypedTemp &t, knownOk) {
            propagator.run(t, SInt32Type);
            if (Stmt *defStmt = _defUses.defStmt(t)) {
                if (Move *m = defStmt->asMove()) {
                    if (Convert *c = m->source->asConvert()) {
                        c->type = SInt32Type;
                    } else if (Unop *u = m->source->asUnop()) {
                        if (u->op != OpUMinus)
                            u->type = SInt32Type;
                    } else if (Binop *b = m->source->asBinop()) {
                        b->type = SInt32Type;
                    }
                }
            }
        }
    }

private:
    bool isUsedAsInt32(const UntypedTemp &t, const QVector<UntypedTemp> &knownOk) const
    {
        const QList<Stmt *> &uses = _defUses.uses(t);
        if (uses.isEmpty())
            return false;

        foreach (Stmt *use, uses) {
            if (Move *m = use->asMove()) {
                Temp *targetTemp = m->target->asTemp();

                if (m->source->asTemp()) {
                    if (!targetTemp || !knownOk.contains(*targetTemp))
                        return false;
                } else if (m->source->asConvert()) {
                    continue;
                } else if (Binop *b = m->source->asBinop()) {
                    switch (b->op) {
                    case OpAdd:
                    case OpSub:
                    case OpMul:
                        if (!targetTemp || !knownOk.contains(*targetTemp))
                            return false;
                    case OpBitAnd:
                    case OpBitOr:
                    case OpBitXor:
                    case OpRShift:
                    case OpLShift:
                    case OpURShift:
                        continue;
                    default:
                        return false;
                    }
                } else if (Unop *u = m->source->asUnop()) {
                    if (u->op == OpUPlus) {
                        if (!targetTemp || !knownOk.contains(*targetTemp))
                            return false;
                    } else if (u->op != OpCompl) {
                        return false;
                    }
                } else {
                    return false;
                }
            } else
                return false;
        }

        return true;
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
    case NullType:
    case UndefinedType:
        c->value = qSNaN();
        c->type = targetType;
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
    IR::Function *_f;

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

    void run(IR::Function *f) {
        _f = f;
        foreach (BasicBlock *bb, f->basicBlocks()) {
            if (bb->isRemoved())
                continue;
            _conversions.clear();

            foreach (Stmt *s, bb->statements()) {
                _currStmt = s;
                s->accept(this);
            }

            foreach (const Conversion &conversion, _conversions) {
                IR::Move *move = conversion.stmt->asMove();

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
                        bb->in[idx]->insertStatementBeforeTerminator(convCall);
                    } else {
                        bb->insertStatementBefore(conversion.stmt, convCall);
                    }

                    *conversion.expr = source;
                } else if (Unop *u = (*conversion.expr)->asUnop()) {
                    // convert:
                    //   int32{%2} = double{-double{%1}};
                    // to:
                    //   double{%3} = double{-double{%1}};
                    //   int32{%2} = int32{convert(double{%3})};
                    Temp *tmp = bb->TEMP(bb->newTemp());
                    tmp->type = u->type;
                    Move *extraMove = f->New<Move>();
                    extraMove->init(tmp, u);
                    _defUses.addTemp(tmp, extraMove, bb);

                    if (Temp *unopOperand = u->expr->asTemp()) {
                        _defUses.addUse(*unopOperand, extraMove);
                        _defUses.removeUse(move, *unopOperand);
                    }

                    bb->insertStatementBefore(conversion.stmt, extraMove);

                    *conversion.expr = bb->CONVERT(tmp, conversion.targetType);
                    _defUses.addUse(*tmp, move);
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

    virtual void visitString(IR::String *) {}
    virtual void visitRegExp(IR::RegExp *) {}
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

        const Member *targetMember = s->target->asMember();
        const bool inhibitConversion = targetMember && targetMember->inhibitTypeConversionOnWrite;

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

void splitCriticalEdges(IR::Function *f, DominatorTree &df)
{
    foreach (BasicBlock *bb, f->basicBlocks()) {
        if (bb->isRemoved())
            continue;
        if (bb->in.size() < 2)
            continue;

        for (int inIdx = 0, eInIdx = bb->in.size(); inIdx != eInIdx; ++inIdx) {
            BasicBlock *inBB = bb->in[inIdx];
            if (inBB->out.size() < 2)
                continue;

            // We found a critical edge.
            BasicBlock *containingGroup = inBB->isGroupStart() ? inBB : inBB->containingGroup();

            // create the basic block:
            BasicBlock *newBB = f->newBasicBlock(containingGroup, bb->catchBlock);
            Jump *s = f->New<Jump>();
            s->init(bb);
            newBB->appendStatement(s);

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
            } else if (terminator->asRet()) {
                Q_ASSERT(!"A block with a RET at the end cannot have outgoing edges.");
            } else {
                Q_ASSERT(!"Unknown terminator!");
            }

            // Set the immediate dominator of the new block to inBB
            df.updateImmediateDominator(newBB, inBB);
        }
    }
}

// High-level algorithm:
//  0. start with the first node (the start node) of a function
//  1. emit the node
//  2. add all outgoing edges that are not yet emitted to the postponed stack
//  3. When the postponed stack is empty, pop a stack from the loop stack. If that is empty too,
//     we're done.
//  4. pop a node from the postponed stack, and check if it can be scheduled:
//     a. if all incoming edges are scheduled, go to 4.
//     b. if an incoming edge is unscheduled, but it's a back-edge (an edge in a loop that jumps
//        back to the start of the loop), ignore it
//     c. if there is any unscheduled edge that is not a back-edge, ignore this node, and go to 4.
//  5. if this node is the start of a loop, push the postponed stack on the loop stack.
//  6. go back to 1.
//
// The postponing action in step 2 will put the node into its containing group. The case where this
// is important is when a (labeled) continue or a (labeled) break statement occur in a loop: the
// outgoing edge points to a node that is not part of the current loop (and possibly not of the
// parent loop).
//
// Linear scan register allocation benefits greatly from short life-time intervals with few holes
// (see for example section 4 (Lifetime Analysis) of [Wimmer1]). This algorithm makes sure that the
// blocks of a group are scheduled together, with no non-loop blocks in between. This applies
// recursively for nested loops. It also schedules groups of if-then-else-endif blocks together for
// the same reason.
class BlockScheduler
{
    IR::Function *function;
    const DominatorTree &dominatorTree;

    struct WorkForGroup
    {
        BasicBlock *group;
        QStack<BasicBlock *> postponed;

        WorkForGroup(BasicBlock *group = 0): group(group) {}
    };
    WorkForGroup currentGroup;
    QStack<WorkForGroup> postponedGroups;
    QVector<BasicBlock *> sequence;
    ProcessedBlocks emitted;
    QHash<BasicBlock *, BasicBlock *> loopsStartEnd;

    bool checkCandidate(BasicBlock *candidate)
    {
        Q_ASSERT(candidate->containingGroup() == currentGroup.group);

        foreach (BasicBlock *in, candidate->in) {
            if (emitted.alreadyProcessed(in))
                continue;

            if (dominatorTree.dominates(candidate, in))
                // this is a loop, where there in -> candidate edge is the jump back to the top of the loop.
                continue;

            return false; // an incoming edge that is not yet emitted, and is not a back-edge
        }

        if (candidate->isGroupStart()) {
            // postpone everything, and schedule the loop first.
            postponedGroups.push(currentGroup);
            currentGroup = WorkForGroup(candidate);
        }

        return true;
    }

    BasicBlock *pickNext()
    {
        while (true) {
            while (currentGroup.postponed.isEmpty()) {
                if (postponedGroups.isEmpty())
                    return 0;
                if (currentGroup.group) // record the first and the last node of a group
                    loopsStartEnd.insert(currentGroup.group, sequence.last());
                currentGroup = postponedGroups.pop();
            }

            BasicBlock *next = currentGroup.postponed.pop();
            if (checkCandidate(next))
                return next;
        }

        Q_UNREACHABLE();
        return 0;
    }

    void emitBlock(BasicBlock *bb)
    {
        Q_ASSERT(!bb->isRemoved());
        if (emitted.alreadyProcessed(bb))
            return;

        sequence.append(bb);
        emitted.markAsProcessed(bb);
    }

    void schedule(BasicBlock *functionEntryPoint)
    {
        BasicBlock *next = functionEntryPoint;

        while (next) {
            emitBlock(next);
            for (int i = next->out.size(); i != 0; ) {
                // postpone all outgoing edges, if they were not already processed
                --i;
                BasicBlock *out = next->out[i];
                if (!emitted.alreadyProcessed(out))
                    postpone(out);
            }
            next = pickNext();
        }
    }

    void postpone(BasicBlock *bb)
    {
        if (currentGroup.group == bb->containingGroup()) {
            currentGroup.postponed.append(bb);
            return;
        }

        for (int i = postponedGroups.size(); i != 0; ) {
            --i;
            WorkForGroup &g = postponedGroups[i];
            if (g.group == bb->containingGroup()) {
                g.postponed.append(bb);
                return;
            }
        }

        Q_UNREACHABLE();
    }

public:
    BlockScheduler(IR::Function *function, const DominatorTree &dominatorTree)
        : function(function)
        , dominatorTree(dominatorTree)
        , sequence(0)
        , emitted(function)
    {}

    QHash<BasicBlock *, BasicBlock *> go()
    {
        showMeTheCode(function);
        schedule(function->basicBlock(0));

#if defined(SHOW_SSA)
        qDebug() << "Block sequence:";
        foreach (BasicBlock *bb, sequence)
            qDebug("\tL%d", bb->index());
#endif // SHOW_SSA

        Q_ASSERT(function->liveBasicBlocksCount() == sequence.size());
        function->setScheduledBlocks(sequence);
        function->renumberBasicBlocks();
        return loopsStartEnd;
    }
};

#ifndef QT_NO_DEBUG
void checkCriticalEdges(QVector<BasicBlock *> basicBlocks) {
    foreach (BasicBlock *bb, basicBlocks) {
        if (bb && bb->out.size() > 1) {
            foreach (BasicBlock *bb2, bb->out) {
                if (bb2 && bb2->in.size() > 1) {
                    qout << "found critical edge between block "
                         << bb->index() << " and block " << bb2->index();
                    Q_ASSERT(false);
                }
            }
        }
    }
}
#endif

void cleanupBasicBlocks(IR::Function *function)
{
    showMeTheCode(function);

    // Algorithm: this is the iterative version of a depth-first search for all blocks that are
    // reachable through outgoing edges, starting with the start block and all exception handler
    // blocks.
    QBitArray reachableBlocks(function->basicBlockCount());
    QVector<BasicBlock *> postponed;
    postponed.reserve(16);
    for (int i = 0, ei = function->basicBlockCount(); i != ei; ++i) {
        BasicBlock *bb = function->basicBlock(i);
        if (i == 0 || bb->isExceptionHandler())
            postponed.append(bb);
    }

    while (!postponed.isEmpty()) {
        BasicBlock *bb = postponed.back();
        postponed.pop_back();
        if (bb->isRemoved()) // this block was removed before, we don't need to clean it up.
            continue;

        reachableBlocks.setBit(bb->index());

        foreach (BasicBlock *outBB, bb->out) {
            if (!reachableBlocks.at(outBB->index()))
                postponed.append(outBB);
        }
    }

    foreach (BasicBlock *bb, function->basicBlocks()) {
        if (bb->isRemoved()) // the block has already been removed, so ignore it
            continue;
        if (reachableBlocks.at(bb->index())) // the block is reachable, so ignore it
            continue;

        foreach (BasicBlock *outBB, bb->out) {
            if (outBB->isRemoved() || !reachableBlocks.at(outBB->index()))
                continue; // We do not need to unlink from blocks that are scheduled to be removed.

            int idx = outBB->in.indexOf(bb);
            if (idx != -1) {
                outBB->in.remove(idx);
                foreach (Stmt *s, outBB->statements()) {
                    if (Phi *phi = s->asPhi())
                        phi->d->incoming.remove(idx);
                    else
                        break;
                }
            }
        }

        function->removeBasicBlock(bb);
    }

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

static Expr *clone(Expr *e, IR::Function *function) {
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
    IR::Function* _function;
    Temp *_toReplace;
    Expr *_replacement;

public:
    ExprReplacer(DefUsesCalculator &defUses, IR::Function *function)
        : _defUses(defUses)
        , _function(function)
        , _toReplace(0)
        , _replacement(0)
    {}

    void operator()(Temp *toReplace, Expr *replacement, StatementWorklist &W, QList<Stmt *> *newUses = 0)
    {
        Q_ASSERT(replacement->asTemp() || replacement->asConst() || replacement->asName());

//        qout << "Replacing ";toReplace->dump(qout);qout<<" by ";replacement->dump(qout);qout<<endl;

        qSwap(_toReplace, toReplace);
        qSwap(_replacement, replacement);

        const QList<Stmt *> &uses = _defUses.uses(*_toReplace);
        if (newUses)
            newUses->reserve(uses.size());

//        qout << "        " << uses.size() << " uses:"<<endl;
        foreach (Stmt *use, uses) {
//            qout<<"        ";use->dump(qout);qout<<"\n";
            use->accept(this);
//            qout<<"     -> ";use->dump(qout);qout<<"\n";
            W += use;
            if (newUses)
                newUses->append(use);
        }

        qSwap(_replacement, replacement);
        qSwap(_toReplace, toReplace);
    }

protected:
    virtual void visitConst(Const *) {}
    virtual void visitString(IR::String *) {}
    virtual void visitRegExp(IR::RegExp *) {}
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

        if (e1->type == IR::NullType && e2->type == IR::NullType)
            return true;
        if (e1->type == IR::UndefinedType && e2->type == IR::UndefinedType)
            return true;

        return false;
    }
};

namespace {
/// This function removes the basic-block from the function's list, unlinks any uses and/or defs,
/// and removes unreachable staements from the worklist, so that optimiseSSA won't consider them
/// anymore.
/// Important: this assumes that there are no critical edges in the control-flow graph!
void purgeBB(BasicBlock *bb, IR::Function *func, DefUsesCalculator &defUses, StatementWorklist &W,
             DominatorTree &df)
{
    // TODO: after the change above: if we keep on detaching the block from predecessors or
    //       successors, update the DominatorTree too.

    // don't purge blocks that are entry points for catch statements. They might not be directly
    // connected, but are required anyway
    if (bb->isExceptionHandler())
        return;

    QVector<BasicBlock *> toPurge;
    toPurge.reserve(8);
    toPurge.append(bb);

    while (!toPurge.isEmpty()) {
        bb = toPurge.first();
        toPurge.removeFirst();

        if (bb->isRemoved())
            continue;

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
                foreach (Stmt *outStmt, out->statements()) {
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

            if (out->in.isEmpty()) {
                // if a successor has no incoming edges after unlinking the current basic block, then
                // it is unreachable, and can be purged too
                toPurge.append(out);
            } else if (out->in.size() == 1) {
                // if the successor now has only one incoming edge, we that edge is the new
                // immediate dominator
                df.updateImmediateDominator(out, out->in.first());
            }
        }

        // unlink all defs/uses from the statements in the basic block
        foreach (Stmt *s, bb->statements()) {
            if (!s)
                continue;

            W += defUses.removeDefUses(s);
            W -= s;
        }

        func->removeBasicBlock(bb);
    }
}

bool tryOptimizingComparison(Expr *&expr)
{
    Binop *b = expr->asBinop();
    if (!b)
        return false;
    Const *leftConst = b->left->asConst();
    if (!leftConst || leftConst->type == StringType || leftConst->type == VarType || leftConst->type == QObjectType)
        return false;
    Const *rightConst = b->right->asConst();
    if (!rightConst || rightConst->type == StringType || rightConst->type == VarType || rightConst->type == QObjectType)
        return false;

    QV4::Primitive l = convertToValue(leftConst);
    QV4::Primitive r = convertToValue(rightConst);

    switch (b->op) {
    case OpGt:
        leftConst->value = Runtime::compareGreaterThan(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    case OpLt:
        leftConst->value = Runtime::compareLessThan(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    case OpGe:
        leftConst->value = Runtime::compareGreaterEqual(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    case OpLe:
        leftConst->value = Runtime::compareLessEqual(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    case OpStrictEqual:
        leftConst->value = Runtime::compareStrictEqual(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    case OpEqual:
        leftConst->value = Runtime::compareEqual(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    case OpStrictNotEqual:
        leftConst->value = Runtime::compareStrictNotEqual(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    case OpNotEqual:
        leftConst->value = Runtime::compareNotEqual(&l, &r);
        leftConst->type = BoolType;
        expr = leftConst;
        return true;
    default:
        break;
    }

    return false;
}
} // anonymous namespace

void optimizeSSA(IR::Function *function, DefUsesCalculator &defUses, DominatorTree &df)
{
    StatementWorklist W(function);
    ExprReplacer replaceUses(defUses, function);

    while (!W.isEmpty()) {
        Stmt *s = W.takeOne();
        if (!s)
            continue;

        if (Phi *phi = s->asPhi()) {
            // constant propagation:
            if (Const *c = isConstPhi(phi)) {
                replaceUses(phi->targetTemp, c, W);
                defUses.removeDef(*phi->targetTemp);
                W.clear(s);
                continue;
            }

            // copy propagation:
            if (phi->d->incoming.size() == 1) {
                Temp *t = phi->targetTemp;
                Expr *e = phi->d->incoming.first();

                QList<Stmt *> newT2Uses;
                replaceUses(t, e, W, &newT2Uses);
                if (Temp *t2 = e->asTemp()) {
                    defUses.removeUse(s, *t2);
                    defUses.addUses(*t2, newT2Uses);
                }
                defUses.removeDef(*t);
                W.clear(s);
                continue;
            }

            // dead code elimination:
            if (defUses.useCount(*phi->targetTemp) == 0) {
                foreach (Expr *in, phi->d->incoming) {
                    if (Temp *t = in->asTemp())
                        W += defUses.defStmt(*t);
                }

                defUses.removeDef(*phi->targetTemp);
                W.clear(s);
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

            if (Temp *targetTemp = unescapableTemp(m->target, function)) {
                // dead code elimination:
                if (defUses.useCount(*targetTemp) == 0) {
                    EliminateDeadCode(defUses, W, function).run(m->source, s);
                    if (!m->source)
                        W.clear(s);
                    continue;
                }

                // constant propagation:
                if (Const *sourceConst = m->source->asConst()) {
                    replaceUses(targetTemp, sourceConst, W);
                    defUses.removeDef(*targetTemp);
                    W.clear(s);
                    continue;
                }
                if (Member *member = m->source->asMember()) {
                    if (member->kind == Member::MemberOfEnum) {
                        Const *c = function->New<Const>();
                        const int enumValue = member->attachedPropertiesIdOrEnumValue;
                        c->init(SInt32Type, enumValue);
                        replaceUses(targetTemp, c, W);
                        defUses.removeDef(*targetTemp);
                        W.clear(s);
                        defUses.removeUse(s, *member->base->asTemp());
                        continue;
                    } else if (member->attachedPropertiesIdOrEnumValue != 0 && member->property && member->base->asTemp()) {
                        // Attached properties have no dependency on their base. Isel doesn't
                        // need it and we can eliminate the temp used to initialize it.
                        defUses.removeUse(s, *member->base->asTemp());
                        Const *c = function->New<Const>();
                        c->init(SInt32Type, 0);
                        member->base = c;
                        continue;
                    }
                }

                // copy propagation:
                if (Temp *sourceTemp = unescapableTemp(m->source, function)) {
                    QList<Stmt *> newT2Uses;
                    replaceUses(targetTemp, sourceTemp, W, &newT2Uses);
                    defUses.removeUse(s, *sourceTemp);
                    defUses.addUses(*sourceTemp, newT2Uses);
                    defUses.removeDef(*targetTemp);
                    W.clear(s);
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
                                constOperand->type = unop->type;
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
                    Const *leftConst = binop->left->asConst();
                    Const *rightConst = binop->right->asConst();

                    { // Typical casts to int32:
                        Expr *casted = 0;
                        switch (binop->op) {
                        case OpBitAnd:
                            if (leftConst && !rightConst && QV4::Primitive::toUInt32(leftConst->value) == 0xffffffff)
                                casted = binop->right;
                            else if (!leftConst && rightConst && QV4::Primitive::toUInt32(rightConst->value) == 0xffffffff)
                                casted = binop->left;
                            break;
                        case OpBitOr:
                            if (leftConst && !rightConst && QV4::Primitive::toInt32(leftConst->value) == 0)
                                casted = binop->right;
                            else if (!leftConst && rightConst && QV4::Primitive::toUInt32(rightConst->value) == 0)
                                casted = binop->left;
                            break;
                        default:
                            break;
                        }
                        if (casted) {
                            Q_ASSERT(casted->type == SInt32Type);
                            m->source = casted;
                            W += m;
                            continue;
                        }
                    }
                    if (rightConst) { // mask right hand side of shift operations
                        switch (binop->op) {
                        case OpLShift:
                        case OpRShift:
                        case OpURShift:
                            rightConst->value = QV4::Primitive::toInt32(rightConst->value) & 0x1f;
                            rightConst->type = SInt32Type;
                            break;
                        default:
                            break;
                        }
                    }

                    // TODO: More constant binary expression evaluation
                    // TODO: If the result of the move is only used in one single cjump, then
                    //       inline the binop into the cjump.
                    if (!leftConst || leftConst->type == StringType || leftConst->type == VarType || leftConst->type == QObjectType)
                        continue;
                    if (!rightConst || rightConst->type == StringType || rightConst->type == VarType || rightConst->type == QObjectType)
                        continue;

                    QV4::Primitive lc = convertToValue(leftConst);
                    QV4::Primitive rc = convertToValue(rightConst);
                    double l = lc.toNumber();
                    double r = rc.toNumber();

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
                    purgeBB(cjump->iffalse, function, defUses, W, df);
                } else {
                    jump->target = cjump->iffalse;
                    purgeBB(cjump->iftrue, function, defUses, W, df);
                }
                W.replace(s, jump);

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

    W.cleanup(function);
}

class InputOutputCollector: protected StmtVisitor, protected ExprVisitor {
    IR::Function *function;

public:
    QList<Temp> inputs;
    QList<Temp> outputs;

    InputOutputCollector(IR::Function *f): function(f) {}

    void collect(Stmt *s) {
        inputs.clear();
        outputs.clear();
        s->accept(this);
    }

protected:
    virtual void visitConst(Const *) {}
    virtual void visitString(IR::String *) {}
    virtual void visitRegExp(IR::RegExp *) {}
    virtual void visitName(Name *) {}
    virtual void visitTemp(Temp *e) {
        if (unescapableTemp(e, function))
            inputs.append(*e);
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
            if (unescapableTemp(t, function))
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

    QVector<LiveRegs> _liveIn;
    QHash<Temp, LifeTimeInterval> _intervals;
    typedef QVector<LifeTimeInterval> LifeTimeIntervals;
    LifeTimeIntervals _sortedIntervals;

public:
    LifeRanges(IR::Function *function, const QHash<BasicBlock *, BasicBlock *> &startEndLoops)
    {
        _liveIn.resize(function->basicBlockCount());

        int id = 0;
        foreach (BasicBlock *bb, function->basicBlocks()) {
            foreach (Stmt *s, bb->statements()) {
                if (s->asPhi())
                    s->id = id + 1;
                else
                    s->id = ++id;
            }
        }

        for (int i = function->basicBlockCount() - 1; i >= 0; --i) {
            BasicBlock *bb = function->basicBlock(i);
            buildIntervals(bb, startEndLoops.value(bb, 0), function);
        }

        _sortedIntervals.reserve(_intervals.size());
        for (QHash<Temp, LifeTimeInterval>::const_iterator i = _intervals.begin(), ei = _intervals.end(); i != ei; ++i) {
            LifeTimeIntervals::iterator lti = _sortedIntervals.insert(_sortedIntervals.end(), i.value());
            lti->setTemp(i.key());
        }
        std::sort(_sortedIntervals.begin(), _sortedIntervals.end(), LifeTimeInterval::lessThan);
        _intervals.clear();
    }

    QVector<LifeTimeInterval> intervals() const { return _sortedIntervals; }

    void dump() const
    {
        qout << "Life ranges:" << endl;
        qout << "Intervals:" << endl;
        foreach (const LifeTimeInterval &range, _sortedIntervals) {
            range.dump(qout);
            qout << endl;
        }

        for (int i = 0, ei = _liveIn.size(); i != ei; ++i) {
            qout << "L" << i <<" live-in: ";
            QList<Temp> live = QList<Temp>::fromSet(_liveIn.at(i));
            std::sort(live.begin(), live.end());
            for (int i = 0; i < live.size(); ++i) {
                if (i > 0) qout << ", ";
                live[i].dump(qout);
            }
            qout << endl;
        }
    }

private:
    void buildIntervals(BasicBlock *bb, BasicBlock *loopEnd, IR::Function *function)
    {
        LiveRegs live;
        foreach (BasicBlock *successor, bb->out) {
            live.unite(_liveIn[successor->index()]);
            const int bbIndex = successor->in.indexOf(bb);
            Q_ASSERT(bbIndex >= 0);

            foreach (Stmt *s, successor->statements()) {
                if (Phi *phi = s->asPhi()) {
                    if (Temp *t = phi->d->incoming[bbIndex]->asTemp())
                        live.insert(*t);
                } else {
                    break;
                }
            }
        }

        QVector<Stmt *> statements = bb->statements();

        foreach (const Temp &opd, live)
            _intervals[opd].addRange(statements.first()->id, statements.last()->id);

        InputOutputCollector collector(function);
        for (int i = statements.size() - 1; i >= 0; --i) {
            Stmt *s = statements[i];
            if (Phi *phi = s->asPhi()) {
                LiveRegs::iterator it = live.find(*phi->targetTemp);
                if (it == live.end()) {
                    // a phi node target that is only defined, but never used
                    _intervals[*phi->targetTemp].setFrom(s);
                } else {
                    live.erase(it);
                }
                continue;
            }
            collector.collect(s);
            foreach (const Temp &opd, collector.outputs) {
                _intervals[opd].setFrom(s);
                live.remove(opd);
            }
            foreach (const Temp &opd, collector.inputs) {
                _intervals[opd].addRange(statements.first()->id, s->id);
                live.insert(opd);
            }
        }

        if (loopEnd) { // Meaning: bb is a loop header, because loopEnd is set to non-null.
            foreach (const Temp &opd, live)
                _intervals[opd].addRange(statements.first()->id, loopEnd->terminator()->id);
        }

        _liveIn[bb->index()] = live;
    }
};

void removeUnreachleBlocks(IR::Function *function)
{
    QVector<BasicBlock *> newSchedule;
    newSchedule.reserve(function->basicBlockCount());
    foreach (BasicBlock *bb, function->basicBlocks())
        if (!bb->isRemoved())
            newSchedule.append(bb);
    function->setScheduledBlocks(newSchedule);
    function->renumberBasicBlocks();
}
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

    // search where to split the interval
    for (int i = 0, ei = _ranges.size(); i < ei; ++i) {
        if (_ranges.at(i).start <= atPosition) {
            if (_ranges.at(i).end >= atPosition) {
                // split happens in the middle of a range. Keep this range in both the old and the
                // new interval, and correct the end/start later
                _ranges.resize(i + 1);
                newInterval._ranges.remove(0, i);
                break;
            }
        } else {
            // split happens between two ranges.
            _ranges.resize(i);
            newInterval._ranges.remove(0, i);
            break;
        }
    }

    if (newInterval._ranges.first().end == atPosition)
        newInterval._ranges.removeFirst();

    if (newStart == Invalid) {
        // the temp stays inactive for the rest of its lifetime
        newInterval = LifeTimeInterval();
    } else {
        // find the first range where the temp will get active again:
        while (!newInterval._ranges.isEmpty()) {
            const Range &range = newInterval._ranges.first();
            if (range.start > newStart) {
                // The split position is before the start of the range. Either we managed to skip
                // over the correct range, or we got an invalid split request. Either way, this
                // Should Never Happen <TM>.
                Q_ASSERT(range.start > newStart);
                return LifeTimeInterval();
            } else if (range.start <= newStart && range.end >= newStart) {
                // yay, we found the range that should be the new first range in the new interval!
                break;
            } else {
                // the temp stays inactive for this interval, so remove it.
                newInterval._ranges.removeFirst();
            }
        }
        Q_ASSERT(!newInterval._ranges.isEmpty());
        newInterval._ranges.first().start = newStart;
        _end = newStart;
    }

    // if we're in the middle of a range, set the end to the split position
    if (_ranges.last().end > atPosition)
        _ranges.last().end = atPosition;

    validate();
    newInterval.validate();

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
    if (r1._ranges.first().start == r2._ranges.first().start) {
        if (r1.isSplitFromInterval() == r2.isSplitFromInterval())
            return r1._ranges.last().end < r2._ranges.last().end;
        else
            return r1.isSplitFromInterval();
    } else
        return r1._ranges.first().start < r2._ranges.first().start;
}

bool LifeTimeInterval::lessThanForTemp(const LifeTimeInterval &r1, const LifeTimeInterval &r2)
{
    return r1.temp() < r2.temp();
}

Optimizer::Optimizer(IR::Function *function)
    : function(function)
    , inSSA(false)
{}

void Optimizer::run(QQmlEnginePrivate *qmlEngine)
{
#if defined(SHOW_SSA)
    qout << "##### NOW IN FUNCTION " << (function->name ? qPrintable(*function->name) : "anonymous!")
         << " with " << function->basicBlocks.size() << " basic blocks." << endl << flush;
#endif

//    showMeTheCode(function);

    cleanupBasicBlocks(function);

    function->removeSharedExpressions();

//    showMeTheCode(function);

    static bool doSSA = qgetenv("QV4_NO_SSA").isEmpty();

    if (!function->hasTry && !function->hasWith && !function->module->debugMode && doSSA) {
//        qout << "SSA for " << (function->name ? qPrintable(*function->name) : "<anonymous>") << endl;

        // Calculate the dominator tree:
        DominatorTree df(function);

//        qout << "Converting to SSA..." << endl;
        convertToSSA(function, df);
//        showMeTheCode(function);

//        qout << "Starting def/uses calculation..." << endl;
        DefUsesCalculator defUses(function);

//        qout << "Cleaning up phi nodes..." << endl;
        cleanupPhis(defUses);
//        showMeTheCode(function);

//        qout << "Running type inference..." << endl;
        TypeInference(qmlEngine, defUses).run(function);
//        showMeTheCode(function);

//        qout << "Doing reverse inference..." << endl;
        ReverseInference(defUses).run(function);
//        showMeTheCode(function);

//        qout << "Doing type propagation..." << endl;
        TypePropagation(defUses).run(function);
//        showMeTheCode(function);

        // Transform the CFG into edge-split SSA.
//        qout << "Starting edge splitting..." << endl;
        splitCriticalEdges(function, df);
//        showMeTheCode(function);

        static bool doOpt = qgetenv("QV4_NO_OPT").isEmpty();
        if (doOpt) {
//            qout << "Running SSA optimization..." << endl;
            optimizeSSA(function, defUses, df);
//            showMeTheCode(function);
        }

//        qout << "Doing block merging..." << endl;
//        mergeBasicBlocks(function);
//        showMeTheCode(function);

        // Basic-block cycles that are unreachable (i.e. for loops in a then-part where the
        // condition is calculated to be always false) are not yet removed. This will choke the
        // block scheduling, so remove those now.
//        qout << "Cleaning up unreachable basic blocks..." << endl;
        cleanupBasicBlocks(function);
//        showMeTheCode(function);

//        qout << "Doing block scheduling..." << endl;
//        df.dumpImmediateDominators();
        startEndLoops = BlockScheduler(function, df).go();
        showMeTheCode(function);

#ifndef QT_NO_DEBUG
        checkCriticalEdges(function->basicBlocks());
#endif

//        qout << "Finished SSA." << endl;
        inSSA = true;
    } else {
        removeUnreachleBlocks(function);
        inSSA = false;
    }
}

void Optimizer::convertOutOfSSA() {
    if (!inSSA)
        return;

    // There should be no critical edges at this point.

    foreach (BasicBlock *bb, function->basicBlocks()) {
        MoveMapping moves;

        foreach (BasicBlock *successor, bb->out) {
            const int inIdx = successor->in.indexOf(bb);
            Q_ASSERT(inIdx >= 0);
            foreach (Stmt *s, successor->statements()) {
                if (Phi *phi = s->asPhi()) {
                    moves.add(clone(phi->d->incoming[inIdx], function),
                              clone(phi->targetTemp, function)->asTemp());
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

    foreach (BasicBlock *bb, function->basicBlocks()) {
        while (!bb->isEmpty()) {
            if (bb->statements().first()->asPhi()) {
                bb->removeStatement(0);
            } else {
                break;
            }
        }
    }
}

QVector<LifeTimeInterval> Optimizer::lifeTimeIntervals() const
{
    Q_ASSERT(isInSSA());

    LifeRanges lifeRanges(function, startEndLoops);
//    lifeRanges.dump();
//    showMeTheCode(function);
    return lifeRanges.intervals();
}

QSet<Jump *> Optimizer::calculateOptionalJumps()
{
    QSet<Jump *> optional;
    QSet<BasicBlock *> reachableWithoutJump;

    const int maxSize = function->basicBlockCount();
    optional.reserve(maxSize);
    reachableWithoutJump.reserve(maxSize);

    for (int i = maxSize - 1; i >= 0; --i) {
        BasicBlock *bb = function->basicBlock(i);
        if (bb->isRemoved())
            continue;

        if (Jump *jump = bb->statements().last()->asJump()) {
            if (reachableWithoutJump.contains(jump->target)) {
                if (bb->statements().size() > 1)
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

void Optimizer::showMeTheCode(IR::Function *function)
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

MoveMapping::Moves MoveMapping::sourceUsages(Expr *e, const Moves &moves)
{
    Moves usages;

    if (Temp *t = e->asTemp()) {
        for (int i = 0, ei = moves.size(); i != ei; ++i) {
            const Move &move = moves[i];
            if (Temp *from = move.from->asTemp())
                if (overlappingStorage(*from, *t))
                    usages.append(move);
        }
    }

    return usages;
}

void MoveMapping::add(Expr *from, Temp *to) {
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

    Move m(from, to);
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

QList<IR::Move *> MoveMapping::insertMoves(BasicBlock *bb, IR::Function *function, bool atEnd) const
{
    QList<IR::Move *> newMoves;
    newMoves.reserve(_moves.size());

    int insertionPoint = atEnd ? bb->statements().size() - 1 : 0;
    foreach (const Move &m, _moves) {
        IR::Move *move = function->New<IR::Move>();
        move->init(clone(m.to, function), clone(m.from, function));
        move->swap = m.needsSwap;
        bb->insertStatementBefore(insertionPoint++, move);
        newMoves.append(move);
    }

    return newMoves;
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
    Moves usages = sourceUsages(m.to, todo) + sourceUsages(m.to, delayed);
    foreach (const Move &dependency, usages) {
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

// References:
//  [Wimmer1] C. Wimmer and M. Franz. Linear Scan Register Allocation on SSA Form. In Proceedings of
//            CGO’10, ACM Press, 2010
//  [Wimmer2] C. Wimmer and H. Mossenbock. Optimized Interval Splitting in a Linear Scan Register
//            Allocator. In Proceedings of the ACM/USENIX International Conference on Virtual
//            Execution Environments, pages 132–141. ACM Press, 2005.
//  [Briggs]  P. Briggs, K.D. Cooper, T.J. Harvey, and L.T. Simpson. Practical Improvements to the
//            Construction and Destruction of Static Single Assignment Form.
//  [Appel]   A.W. Appel. Modern Compiler Implementation in Java. Second edition, Cambridge
//            University Press.
