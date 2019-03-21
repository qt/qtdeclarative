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

#include "qv4node_p.h"
#include "qv4graph_p.h"

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace IR {

Node *Node::create(Node::MemoryPool *pool, Node::Id id, const Operation *op, size_t nInputs,
                   Node *const *inputs, bool inputsAreExtensible)
{
    size_t capacity = nInputs;
    if (inputsAreExtensible)
        capacity += 3;

    Node *node = new (pool->allocate(sizeof(Node))) Node(pool, id, op, unsigned(nInputs),
                                                         int(capacity));
    for (uint i = 0; i < capacity; ++i)
        new (&node->m_inputs[int(i)]) Use(node);
    for (size_t i = 0; i < nInputs; ++i) {
        Q_ASSERT(inputs[i] != nullptr);
        node->replaceInput(unsigned(i), inputs[i]);
    }

    return node;
}

void Node::addInput(MemoryPool *pool, Node *in)
{
    Q_ASSERT(in);
    ++m_nInputs;
    if (m_nInputs >= unsigned(m_inputs.size())) {
        QQmlJS::FixedPoolArray<Use> oldInputs = m_inputs;
        m_inputs = QQmlJS::FixedPoolArray<Use>(pool, int(m_nInputs + 3));
        for (Use &input : m_inputs)
            new (&input) Use(this);
        for (int i = 0, ei = oldInputs.size(); i != ei; ++i) {
            Node *in = oldInputs[i].m_input;
            oldInputs[i].set(nullptr);
            m_inputs[i].set(in);
        }
    }
    m_inputs.at(int(m_nInputs - 1)).set(in);
}

void Node::removeInput(unsigned index)
{
    Q_ASSERT(index < inputCount());
    for (unsigned i = index, ei = inputCount(); i < ei - 1; ++i)
        replaceInput(i, input(i + 1));
    trimInputCount(inputCount() - 1);
}

void Node::removeInputs(unsigned start, unsigned count)
{
    for (unsigned idx = start; idx < start + count; ++idx)
        m_inputs.at(int(idx)).set(nullptr);
}

void Node::removeAllInputs()
{
    removeInputs(0, inputCount());
}

void Node::trimInputCount(unsigned newCount)
{
    unsigned currentCount = inputCount();
    if (newCount == currentCount)
        return;
    Q_ASSERT(newCount < currentCount);
    removeInputs(newCount, currentCount - newCount);
    m_nInputs = newCount;
}

void Node::removeExceptionHandlerUse()
{
    for (Use* use = m_firstUse; use; use = use->m_next) {
        if (use->m_input->opcode() == Meta::OnException) {
            use->set(nullptr);
            break;
        }
    }
}

void Node::insertInput(Node::MemoryPool *pool, unsigned index, Node *newInput)
{
    Q_ASSERT(index < inputCount());
    addInput(pool, input(inputCount() - 1));
    for (unsigned i = inputCount() - 1; i > index; --i)
        replaceInput(i, input(i - 1));
    replaceInput(index, newInput);
}

void Node::replaceAllUsesWith(Node *replacement)
{
    for (Use *use = m_firstUse; use; ) {
        Use *next = use->m_next;
        const unsigned inIdx = use->inputIndex();
        use->user()->replaceInput(inIdx, replacement);
        use = next;
    }
}

void Node::replaceUses(Node *newValueInput, Node *newEffectInput, Node *newControlInput)
{
    for (Use *use = m_firstUse; use; ) {
        Use *next = use->m_next;
        const Operation *inOp = use->user()->operation();
        const unsigned inIdx = use->inputIndex();
        if (inIdx < inOp->valueInputCount())
            use->user()->replaceInput(inIdx, newValueInput);
        else if (inIdx < inOp->indexOfFirstControl())
            use->user()->replaceInput(inIdx, newEffectInput);
        else
            use->user()->replaceInput(inIdx, newControlInput);
        use = next;
    }
}

Node *Node::firstValueUse()
{
    for (auto it = uses().begin(), eit = uses().end(); it != eit; ++it) {
        if (it.isUsedAsValue())
            return *it;
    }
    return nullptr;
}

Node::Node(MemoryPool *pool, Node::Id id, const Operation *op, unsigned nInputs, int capacity)
    : m_op(op)
    , m_inputs(pool, capacity)
    , m_nInputs(nInputs)
    , m_id(id)
{
}

NodeWorkList::NodeWorkList(const Graph *g)
    : m_nodeState(g->nodeCount(), Unvisited)
{ m_worklist.reserve(64); }

void NodeWorkList::reset()
{
    std::fill(m_nodeState.begin(), m_nodeState.end(), Unvisited);
    m_worklist.clear();
    if (m_worklist.capacity() < 64)
        m_worklist.reserve(64);
}

NodeCollector::NodeCollector(const Graph *g, bool collectUses, bool skipFramestate)
{
    markReachable(g->endNode());
    for (size_t i = 0; i < m_reachable.size(); ++i) { // _reachable.size() is on purpose!
        Node *n = m_reachable.at(i);
        for (auto input : n->inputs()) {
            if (input == nullptr)
                continue;
            if (isReachable(input->id()))
                continue;
            if (skipFramestate && input->opcode() == Meta::FrameState)
                continue;
            markReachable(input);
        }

        if (collectUses) {
            for (Node *use : n->uses()) {
                if (use && !isReachable(use->id()))
                    markReachable(use);
            }
        }
    }
}

} // IR namespace
} // QV4 namespace
QT_END_NAMESPACE
