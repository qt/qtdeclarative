/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qloggingcategory.h>
#include <QtCore/qbuffer.h>

#include "qv4domtree_p.h"

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace IR {

Q_LOGGING_CATEGORY(lcDomTree, "qt.v4.ir.domTree")
Q_LOGGING_CATEGORY(lcDomFrontier, "qt.v4.ir.domFrontier")

DominatorTree::DominatorTree(MIFunction *f)
    : m_function(f)
    , m_data(new Data)
{
    calculateIDoms();
    m_data.reset();
}

void DominatorTree::dumpImmediateDominators() const
{
    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    QTextStream qout(&buf);
    qout << "Immediate dominators for " << m_function->irFunction()->name() << ":" << endl;
    for (MIBlock *to : m_function->blocks()) {
        MIBlock::Index from = m_idom.at(to->index());
        if (from != MIBlock::InvalidIndex)
            qout << "        " << from;
        else
            qout << "   (none)";
        qout << " dominates " << to->index() << endl;
    }
    qCDebug(lcDomTree, "%s", buf.data().constData());
}

void DominatorTree::setImmediateDominator(MIBlock::Index dominated, MIBlock::Index dominator)
{
    if (m_idom.size() <= size_t(dominated))
        m_idom.resize(dominated + 1);
    m_idom[dominated] = dominator;
}

bool DominatorTree::dominates(MIBlock::Index dominator, MIBlock::Index dominated) const
{
    // dominator can be Invalid when the dominated block has no dominator (i.e. the start node)
    Q_ASSERT(dominated != MIBlock::InvalidIndex);

    if (dominator == dominated)
        return false;

    for (MIBlock::Index it = m_idom[dominated]; it != MIBlock::InvalidIndex; it = m_idom[it]) {
        if (it == dominator)
            return true;
    }

    return false;
}

// Calculate a depth-first iteration order on the nodes of the dominator tree.
//
// The order of the nodes in the vector is not the same as one where a recursive depth-first
// iteration is done on a tree. Rather, the nodes are (reverse) sorted on tree depth.
// So for the:
//    1 dominates 2
//    2 dominates 3
//    3 dominates 4
//    2 dominates 5
// the order will be:
//    4, 3, 5, 2, 1
// or:
//    4, 5, 3, 2, 1
// So the order of nodes on the same depth is undefined, but it will be after the nodes
// they dominate, and before the nodes that dominate them.
//
// The reason for this order is that a proper DFS pre-/post-order would require inverting
// the idom vector by either building a real tree datastructure or by searching the idoms
// for siblings and children. Both have a higher time complexity than sorting by depth.
std::vector<MIBlock *> DominatorTree::calculateDFNodeIterOrder() const
{
    std::vector<int> depths = calculateNodeDepths();
    std::vector<MIBlock *> order = m_function->blocks();
    std::sort(order.begin(), order.end(), [&depths](MIBlock *one, MIBlock *two) -> bool {
        return depths.at(one->index()) > depths.at(two->index());
    });
    return order;
}

// Algorithm:
//  - for each node:
//    - get the depth of a node. If it's unknown (-1):
//      - get the depth of the immediate dominator.
//      - if that's unknown too, calculate it by calling calculateNodeDepth
//      - set the current node's depth to that of immediate dominator + 1
std::vector<int> DominatorTree::calculateNodeDepths() const
{
    std::vector<int> nodeDepths(size_t(m_function->blockCount()), -1);
    for (MIBlock *bb : m_function->blocks()) {
        int &bbDepth = nodeDepths[bb->index()];
        if (bbDepth == -1) {
            const int immDom = m_idom[bb->index()];
            if (immDom == -1) {
                // no immediate dominator, so it's either the start block, or an unreachable block
                bbDepth = 0;
            } else {
                int immDomDepth = nodeDepths[immDom];
                if (immDomDepth == -1)
                    immDomDepth = calculateNodeDepth(immDom, nodeDepths);
                bbDepth = immDomDepth + 1;
            }
        }
    }
    return nodeDepths;
}

// Algorithm:
//   - search for the first dominator of a node that has a known depth. As all nodes are
//     reachable from the start node, and that node's depth is 0, this is finite.
//   - while doing that search, put all unknown nodes in the worklist
//   - pop all nodes from the worklist, and set their depth to the previous' (== dominating)
//     node's depth + 1
// This way every node's depth is calculated once, and the complexity is O(n).
int DominatorTree::calculateNodeDepth(MIBlock::Index nodeIdx, std::vector<int> &nodeDepths) const
{
    std::vector<int> worklist;
    worklist.reserve(8);
    int depth = -1;

    do {
        worklist.push_back(nodeIdx);
        nodeIdx = m_idom[nodeIdx];
        depth = nodeDepths[nodeIdx];
    } while (depth == -1);

    for (auto it = worklist.rbegin(), eit = worklist.rend(); it != eit; ++it)
        nodeDepths[*it] = ++depth;

    return depth;
}

namespace {
struct DFSTodo {
    MIBlock::Index node = MIBlock::InvalidIndex;
    MIBlock::Index parent = MIBlock::InvalidIndex;

    DFSTodo() = default;
    DFSTodo(MIBlock::Index node, MIBlock::Index parent)
        : node(node)
        , parent(parent)
    {}
};
} // anonymous namespace

void DominatorTree::dfs(MIBlock::Index node)
{
    std::vector<DFSTodo> worklist;
    worklist.reserve(m_data->vertex.capacity() / 2);
    DFSTodo todo(node, MIBlock::InvalidIndex);

    while (true) {
        MIBlock::Index n = todo.node;

        if (m_data->dfnum[n] == 0) {
            m_data->dfnum[n] = m_data->size;
            m_data->vertex[m_data->size] = n;
            m_data->parent[n] = todo.parent;
            ++m_data->size;

            MIBlock::OutEdges out = m_function->block(n)->outEdges();
            for (int i = out.size() - 1; i > 0; --i)
                worklist.emplace_back(out[i]->index(), n);

            if (!out.isEmpty()) {
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
}

void DominatorTree::link(MIBlock::Index p, MIBlock::Index n)
{
    m_data->ancestor[n] = p;
    m_data->best[n] = n;
}

void DominatorTree::calculateIDoms()
{
    Q_ASSERT(m_function->block(0)->inEdges().count() == 0);

    const size_t bbCount = m_function->blockCount();
    m_data->vertex = std::vector<MIBlock::Index>(bbCount, MIBlock::InvalidIndex);
    m_data->parent = std::vector<MIBlock::Index>(bbCount, MIBlock::InvalidIndex);
    m_data->dfnum = std::vector<unsigned>(bbCount, 0);
    m_data->semi = std::vector<MIBlock::Index>(bbCount, MIBlock::InvalidIndex);
    m_data->ancestor = std::vector<MIBlock::Index>(bbCount, MIBlock::InvalidIndex);
    m_idom = std::vector<MIBlock::Index>(bbCount, MIBlock::InvalidIndex);
    m_data->samedom = std::vector<MIBlock::Index>(bbCount, MIBlock::InvalidIndex);
    m_data->best = std::vector<MIBlock::Index>(bbCount, MIBlock::InvalidIndex);

    QHash<MIBlock::Index, std::vector<MIBlock::Index>> bucket;
    bucket.reserve(int(bbCount));

    dfs(m_function->block(0)->index());

    std::vector<MIBlock::Index> worklist;
    worklist.reserve(m_data->vertex.capacity() / 2);

    for (int i = m_data->size - 1; i > 0; --i) {
        MIBlock::Index n = m_data->vertex[i];
        MIBlock::Index p = m_data->parent[n];
        MIBlock::Index s = p;

        for (auto inEdge : m_function->block(n)->inEdges()) {
            if (inEdge->isDeoptBlock())
                continue;
            MIBlock::Index v = inEdge->index();
            MIBlock::Index ss = MIBlock::InvalidIndex;
            if (m_data->dfnum[v] <= m_data->dfnum[n])
                ss = v;
            else
                ss = m_data->semi[ancestorWithLowestSemi(v, worklist)];
            if (m_data->dfnum[ss] < m_data->dfnum[s])
                s = ss;
        }
        m_data->semi[n] = s;
        bucket[s].push_back(n);
        link(p, n);
        if (bucket.contains(p)) {
            for (MIBlock::Index v : bucket[p]) {
                MIBlock::Index y = ancestorWithLowestSemi(v, worklist);
                MIBlock::Index semi_v = m_data->semi[v];
                if (m_data->semi[y] == semi_v)
                    m_idom[v] = semi_v;
                else
                    m_data->samedom[v] = y;
            }
            bucket.remove(p);
        }
    }

    for (unsigned i = 1; i < m_data->size; ++i) {
        MIBlock::Index n = m_data->vertex[i];
        Q_ASSERT(n != MIBlock::InvalidIndex);
        Q_ASSERT(!bucket.contains(n));
        Q_ASSERT(m_data->ancestor[n] != MIBlock::InvalidIndex);
        Q_ASSERT((m_data->semi[n] != MIBlock::InvalidIndex
                    && m_data->dfnum[m_data->ancestor[n]] <= m_data->dfnum[m_data->semi[n]])
                 || m_data->semi[n] == n);
        MIBlock::Index sdn = m_data->samedom[n];
        if (sdn != MIBlock::InvalidIndex)
            m_idom[n] = m_idom[sdn];
    }

    if (lcDomTree().isDebugEnabled())
        dumpImmediateDominators();

    m_data.reset(nullptr);
}

MIBlock::Index DominatorTree::ancestorWithLowestSemi(MIBlock::Index v,
                                                     std::vector<MIBlock::Index> &worklist)
{
    worklist.clear();
    for (MIBlock::Index it = v; it != MIBlock::InvalidIndex; it = m_data->ancestor[it])
        worklist.push_back(it);

    if (worklist.size() < 2)
        return m_data->best[v];

    MIBlock::Index b = MIBlock::InvalidIndex;
    MIBlock::Index last = worklist.back();
    Q_ASSERT(worklist.size() <= INT_MAX);
    for (int it = static_cast<int>(worklist.size()) - 2; it >= 0; --it) {
        MIBlock::Index bbIt = worklist[it];
        m_data->ancestor[bbIt] = last;
        MIBlock::Index &best_it = m_data->best[bbIt];
        if (b != MIBlock::InvalidIndex
                && m_data->dfnum[m_data->semi[b]] < m_data->dfnum[m_data->semi[best_it]]) {
            best_it = b;
        } else {
            b = best_it;
        }
    }
    return b;
}

void DominatorFrontier::compute(const DominatorTree &domTree)
{
    struct NodeProgress {
        std::vector<MIBlock::Index> children;
        std::vector<MIBlock::Index> todo;
    };

    MIFunction *function = domTree.function();
    m_df.resize(function->blockCount());

    // compute children of each node in the dominator tree
    std::vector<std::vector<MIBlock::Index> > children; // BasicBlock index -> children
    children.resize(function->blockCount());
    for (MIBlock *n : function->blocks()) {
        const MIBlock::Index nodeIndex = n->index();
        Q_ASSERT(function->block(nodeIndex) == n);
        const MIBlock::Index nodeDominator = domTree.immediateDominator(nodeIndex);
        if (nodeDominator == MIBlock::InvalidIndex)
            continue; // there is no dominator to add this node to as a child (e.g. the start node)
        children[nodeDominator].push_back(nodeIndex);
    }

    // Fill the worklist and initialize the node status for each basic-block
    std::vector<NodeProgress> nodeStatus;
    nodeStatus.resize(function->blockCount());
    std::vector<MIBlock::Index> worklist;
    worklist.reserve(function->blockCount());
    for (MIBlock *bb : function->blocks()) {
        MIBlock::Index nodeIndex = bb->index();
        worklist.push_back(nodeIndex);
        NodeProgress &np = nodeStatus[nodeIndex];
        np.children = children[nodeIndex];
        np.todo = children[nodeIndex];
    }

    BitVector DF_done(int(function->blockCount()), false);

    while (!worklist.empty()) {
        MIBlock::Index node = worklist.back();

        if (DF_done.at(node)) {
            worklist.pop_back();
            continue;
        }

        NodeProgress &np = nodeStatus[node];
        auto it = np.todo.begin();
        while (it != np.todo.end()) {
            if (DF_done.at(*it)) {
                it = np.todo.erase(it);
            } else {
                worklist.push_back(*it);
                break;
            }
        }

        if (np.todo.empty()) {
            MIBlockSet &miBlockSet = m_df[node];
            miBlockSet.init(function);
            for (MIBlock *y : function->block(node)->outEdges()) {
                if (domTree.immediateDominator(y->index()) != node)
                    miBlockSet.insert(y);
            }
            for (MIBlock::Index child : np.children) {
                const MIBlockSet &ws = m_df[child];
                for (auto w : ws) {
                    const MIBlock::Index wIndex = w->index();
                    if (node == wIndex || !domTree.dominates(node, w->index()))
                        miBlockSet.insert(w);
                }
            }
            DF_done.setBit(node);
            worklist.pop_back();
        }
    }

    if (lcDomFrontier().isDebugEnabled())
        dump(domTree.function());
}

void DominatorFrontier::dump(MIFunction *function)
{
    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    QTextStream qout(&buf);
    qout << "Dominator Frontiers:" << endl;
    for (MIBlock *n : function->blocks()) {
        qout << "\tDF[" << n->index() << "]: {";
        const MIBlockSet &SList = m_df[n->index()];
        for (MIBlockSet::const_iterator i = SList.begin(), ei = SList.end(); i != ei; ++i) {
            if (i != SList.begin())
                qout << ", ";
            qout << (*i)->index();
        }
        qout << "}" << endl;
    }
    qCDebug(lcDomFrontier, "%s", buf.data().constData());
}

} // IR namespace
} // QV4 namespace
QT_END_NAMESPACE
