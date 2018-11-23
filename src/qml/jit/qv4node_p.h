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

#ifndef QV4NODE_P_H
#define QV4NODE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmljsmemorypool_p.h>
#include <private/qv4global_p.h>
#include <private/qv4operation_p.h>
#include "qv4util_p.h"

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {

class Use
{
    Q_DISABLE_COPY_MOVE(Use)

public:
    Use(Node *user)
        : m_user(user)
    {}

    ~Use()
    {
        if (m_input)
            removeFromList();
    }

    operator Node *() const { return m_input; }
    Node *input() const { return m_input; }
    Node *user() const { return m_user; }

    inline void set(Node *newInput);

    void validate() const
    {
        Q_ASSERT(m_user);
        if (m_input) {
            if (m_prev != nullptr)
                Q_ASSERT(*m_prev == this);
            if (m_next) {
                Q_ASSERT(m_next->m_input == m_input);
                Q_ASSERT(m_next->m_prev == &m_next);
                m_next->validate();
            }
        }
    }

    inline int inputIndex() const;

protected:
    friend class Node;

    void addToList(Use **list) {
        validate();
        m_next = *list;
        if (m_next)
            m_next->m_prev = &m_next;
        m_prev = list;
        *list = this;
        validate();
    }

    void removeFromList() {
        validate();
        Use **newPrev = m_prev;
        *newPrev = m_next;
        m_prev = nullptr;
        if (m_next)
            m_next->m_prev = newPrev;
        m_next = nullptr;
        m_input = nullptr;
        validate();
    }

private:
    Node *m_input = nullptr;
    Node *m_user = nullptr;
    Use *m_next = nullptr;
    Use **m_prev = nullptr;
};

// A node represents an calculation, action, or marker in the graph. Each node has an operation,
// input dependencies and uses. The operation indicates what kind of node it is, e.g.: JSAdd,
// Constant, Region, and so on. Two nodes can have the same operation, but different inputs.
// For example, the expressions 1 + 2 and 3 + 4 will each have a node with an JSAdd operation
// (which is exactly the same operation for both nodes), but the nodes have different inputs (1, and
// 2 in the first expression, while the second operation has 3 and 4 as inputs).
class Node final
{
    Q_DISABLE_COPY_MOVE(Node)

public:
    using Id = uint32_t;
    using MemoryPool = QQmlJS::MemoryPool;
    class Inputs;

public:
    static Node *create(MemoryPool *pool, Id id, const Operation *op, size_t nInputs,
                        Node * const *inputs, bool inputsAreExtensible = false);
    ~Node() = delete;

    inline bool isDead() const;
    inline void kill();

    Id id() const { return m_id; }

    const Operation *operation() const
    { return m_op; }

    void setOperation(const Operation *op)
    { m_op = op; }

    Operation::Kind opcode() const
    { return operation()->kind(); }

    inline Inputs inputs() const;
    void addInput(MemoryPool *pool, Node *in);
    void removeInput(unsigned index);
    void removeInputs(unsigned start, unsigned count);
    void removeAllInputs();
    uint32_t inputCount() const
    { return m_nInputs; }
    void trimInputCount(unsigned newCount);

    void removeExceptionHandlerUse();

    Node *input(unsigned idx) const
    {
        Q_ASSERT(idx < inputCount());
        return m_inputs.at(idx);
    }

    Node *effectInput(unsigned effectIndex = 0) const
    {
        if (operation()->effectInputCount() == 0)
            return nullptr;
        Q_ASSERT(effectIndex < operation()->effectInputCount());
        return input(operation()->indexOfFirstEffect() + effectIndex);
    }

    Node *controlInput(unsigned controlIndex = 0) const
    {
        if (operation()->controlInputCount() == 0)
            return nullptr;
        Q_ASSERT(controlIndex < operation()->controlInputCount());
        return input(operation()->indexOfFirstControl() + controlIndex);
    }

    Node *frameStateInput() const
    {
        if (operation()->hasFrameStateInput())
            return input(operation()->indexOfFrameStateInput());
        return nullptr;
    }

    void setFrameStateInput(Node *newFramestate)
    {
        if (operation()->hasFrameStateInput())
            replaceInput(operation()->indexOfFrameStateInput(), newFramestate);
    }

    void insertInput(MemoryPool *pool, unsigned index, Node *newInput);

    void replaceInput(Node *oldIn, Node *newIn)
    {
        for (unsigned i = 0, ei = inputCount(); i != ei; ++i) {
            if (input(i) == oldIn)
                replaceInput(i, newIn);
        }
    }

    void replaceInput(unsigned idx, Node *newIn)
    {
        m_inputs[idx].set(newIn);
    }

    class Uses
    {
    public:
        explicit Uses(Node *node)
            : m_node(node)
        {}

        class const_iterator;
        inline const_iterator begin() const;
        inline const_iterator end() const;

        bool isEmpty() const;

    private:
        Node *m_node;
    };

    Uses uses() { return Uses(this); }
    bool hasUses() const { return m_firstUse != nullptr; }
    unsigned useCount() const
    {
        unsigned cnt = 0;
        for (Use *it = m_firstUse; it; it = it->m_next)
            ++cnt;
        return cnt;
    }
    void replaceAllUsesWith(Node *replacement);
    void replaceUses(Node *newValueInput, Node *newEffectInput, Node *newControlInput);

    Node *firstValueUse();

private: // types and utility methods
    friend class Use;
    Node(MemoryPool *pool, Id id, const Operation *op, unsigned nInputs, int capacity);

private: // fields
    Use *m_firstUse = nullptr;
    const Operation *m_op = nullptr;
    QQmlJS::FixedPoolArray<Use> m_inputs;
    unsigned m_nInputs = 0;
    Id m_id = 0;
};

void Use::set(Node *newInput)
{
    if (m_input)
        removeFromList();
    m_input = newInput;
    if (newInput)
        addToList(&newInput->m_firstUse);
}

class Node::Inputs final
{
public:
    using value_type = Node *;

    class const_iterator;
    inline const_iterator begin() const;
    inline const_iterator end() const;

    bool empty() const
    { return m_nInputs == 0; }

    unsigned count() const
    { return m_nInputs; }

    explicit Inputs(const Use *inputs, unsigned nInputs)
        : m_inputs(inputs), m_nInputs(nInputs)
    {}

private:
    const Use *m_inputs = nullptr;
    unsigned m_nInputs = 0;
};

class Node::Inputs::const_iterator final
{
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Node *;
    using pointer = const value_type *;
    using reference = value_type &;

    Node *operator*() const
    { return m_inputs->m_input; }

    bool operator==(const const_iterator &other) const
    { return m_inputs == other.m_inputs; }

    bool operator!=(const const_iterator &other) const
    { return !(*this == other); }

    const_iterator &operator++()
    { ++m_inputs; return *this; }

    const_iterator& operator+=(difference_type offset)
    { m_inputs += offset; return *this; }

    const_iterator operator+(difference_type offset) const
    { return const_iterator(m_inputs + offset); }

    difference_type operator-(const const_iterator &other) const
    { return m_inputs - other.m_inputs; }

private:
    friend class Node::Inputs;

    explicit const_iterator(const Use *inputs)
        : m_inputs(inputs)
    {}

    const Use *m_inputs;
};

Node::Inputs::const_iterator Node::Inputs::begin() const
{ return const_iterator(m_inputs); }

Node::Inputs::const_iterator Node::Inputs::end() const
{ return const_iterator(m_inputs + m_nInputs); }

Node::Inputs Node::inputs() const
{
    return Inputs(m_inputs.begin(), m_nInputs);
}

class Node::Uses::const_iterator final
{
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = int;
    using value_type = Node *;
    using pointer = Node **;
    using reference = Node *&;

    Node *operator*() const
    { return m_current->user(); }

    bool operator==(const const_iterator &other) const
    { return other.m_current == m_current; }

    bool operator!=(const const_iterator &other) const
    { return other.m_current != m_current; }

    const_iterator &operator++()
    { m_current = m_next; setNext(); return *this; }

    unsigned inputIndex() const
    { return m_current->inputIndex(); }

    bool isUsedAsValue() const
    { return inputIndex() < operator*()->operation()->valueInputCount(); }

    bool isUsedAsControl() const
    { return operator*()->operation()->indexOfFirstControl() <= inputIndex(); }

private:
    friend class Node::Uses;

    const_iterator() = default;

    explicit const_iterator(Node* node)
        : m_current(node->m_firstUse)
    { setNext(); }

    void setNext()
    {
        if (m_current)
            m_next = m_current->m_next;
        else
            m_next = nullptr;
    }

private:
    Use *m_current = nullptr;
    Use *m_next = nullptr;
};

Node::Uses::const_iterator Node::Uses::begin() const
{ return const_iterator(this->m_node); }

Node::Uses::const_iterator Node::Uses::end() const
{ return const_iterator(); }

int Use::inputIndex() const
{
    if (!m_user)
        return -1;
    return int(this - m_user->m_inputs.begin());
}

bool Node::isDead() const
{
    Inputs in = inputs();
    return !in.empty() && *in.begin() == nullptr;
}

void Node::kill()
{
    removeAllInputs();
}

class NodeWorkList final
{
    enum State: uint8_t {
        Unvisited = 0,
        Queued,
        Visited,
    };

public:
    NodeWorkList(const Graph *g);

    void reset();

    bool enqueue(Node *n)
    {
        State &s = nodeState(n);
        if (s == Queued || s == Visited)
            return false;

        m_worklist.push_back(n);
        s = Queued;
        return true;
    }

    void enqueue(const std::vector<Node *> &nodes)
    {
        m_worklist.insert(m_worklist.end(), nodes.begin(), nodes.end());
        for (Node *n : nodes)
            nodeState(n) = Queued;
    }

    void reEnqueue(Node *n)
    {
        if (!n)
            return;
        State &s = nodeState(n);
        if (s == Queued)
            return;
        s = Queued;
        m_worklist.push_back(n);
    }

    void enqueueAllInputs(Node *n)
    {
        for (Node *input : n->inputs())
            enqueue(input);
    }

    void reEnqueueAllInputs(Node *n)
    {
        for (Node *input : n->inputs())
            reEnqueue(input);
    }

    void enqueueValueInputs(Node *n)
    {
        for (unsigned i = 0, ei = n->operation()->valueInputCount(); i != ei; ++i)
            enqueue(n->input(i));
    }

    void enqueueEffectInputs(Node *n)
    {
        for (unsigned i = n->operation()->indexOfFirstEffect(), ei = n->operation()->effectInputCount(); i != ei; ++i)
            enqueue(n->input(i));
    }

    void enqueueAllUses(Node *n)
    {
        for (Node *use : n->uses())
            enqueue(use);
    }

    Node *dequeueNextNodeForVisiting()
    {
        while (!m_worklist.empty()) {
            Node *n = m_worklist.back();
            m_worklist.pop_back();
            State &s = nodeState(n);
            Q_ASSERT(s == Queued);
            s = Visited;
            return n;
        }

        return nullptr;
    }

    bool isVisited(Node *n) const
    { return nodeState(n) == Visited; }

    bool isEmpty() const
    { return m_worklist.empty(); }

    QString status(Node *n) const
    {
        QString s = QStringLiteral("status for node %1: ").arg(n->id());
        switch (nodeState(n)) {
        case Queued: s += QLatin1String("queued"); break;
        case Visited: s += QLatin1String("visited"); break;
        case Unvisited: s += QLatin1String("unvisited"); break;
        }
        return s;
    }

private:
    State &nodeState(Node *n)
    {
        const unsigned position(n->id());
        if (position >= m_nodeState.size())
            m_nodeState.resize(position + 1, Unvisited);

        return m_nodeState[position];
    }

    State nodeState(Node *n) const
    { return m_nodeState[unsigned(n->id())]; }

private:
    std::vector<Node *> m_worklist;
    std::vector<State> m_nodeState;
};

class NodeInfo
{
public:
    enum { NoInstructionOffset = -1 };

public:
    NodeInfo() = default;

    Type type() const { return m_type; }
    void setType(Type t) { m_type = t; }

    int currentInstructionOffset() const
    { return m_currentInstructionOffset; }

    int nextInstructionOffset() const
    { return m_nextInstructionOffset; }

    void setBytecodeOffsets(int current, int next)
    {
        Q_ASSERT(current != NoInstructionOffset);
        Q_ASSERT(next != NoInstructionOffset);
        m_currentInstructionOffset = current;
        m_nextInstructionOffset = next;
    }

private:
    Type m_type;
    int m_currentInstructionOffset = NoInstructionOffset;
    int m_nextInstructionOffset = NoInstructionOffset;
};

class NodeCollector
{
public:
    NodeCollector(const Graph *g, bool collectUses = false, bool skipFramestate = false);

    const std::vector<Node *> &reachable() const
    { return m_reachable; }

    void sortById()
    {
        std::sort(m_reachable.begin(), m_reachable.end(), [](Node *n1, Node *n2) {
            return n1->id() < n2->id();
        });
    }

    bool isReachable(Node::Id nodeId) const
    {
        if (nodeId >= Node::Id(m_isReachable.size()))
            return false;
        return m_isReachable.at(int(nodeId));
    }

    void markReachable(Node *node)
    {
        auto nodeId = node->id();
        m_reachable.push_back(node);
        if (nodeId >= Node::Id(m_isReachable.size()))
            m_isReachable.resize(int(nodeId + 1), false);
        m_isReachable.setBit(int(nodeId));
    }

private:
    std::vector<Node *> m_reachable;
    BitVector m_isReachable;
};

} // namespace IR
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4NODE_P_H
