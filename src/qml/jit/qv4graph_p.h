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

#ifndef QV4GRAPH_P_H
#define QV4GRAPH_P_H

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
#include <private/qv4node_p.h>

#include <array>

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {

class Function;
class Operation;
class OperationBuilder;

class Graph final
{
    Q_DISABLE_COPY_MOVE(Graph)

public:
    using MemoryPool = QQmlJS::MemoryPool;

public:
    static Graph *create(Function *function);
    ~Graph() = delete;

    MemoryPool *pool() const;
    OperationBuilder *opBuilder() const
    { return m_opBuilder; }

    Node *createNode(const Operation *op, Node * const operands[] = nullptr, size_t opCount = 0,
                     bool incomplete = false);
    template <typename... Nodes>
    Node *createNode(Operation *op, Nodes*... nodes) {
        std::array<Node *, sizeof...(nodes)> nodesArray {{ nodes... }};
        return createNode(op, nodesArray.data(), nodesArray.size());
    }
    Node *createConstantBoolNode(bool value);
    Node *createConstantIntNode(int value);
    Node *createConstantHeapNode(Heap::Base *heap);

    Node *undefinedNode() const { return m_undefinedNode; }
    Node *emptyNode() const { return m_emptyNode; }
    Node *nullNode() const { return m_nullNode; }
    Node *trueConstant() const { return m_trueNode; }
    Node *falseConstant() const { return m_falseNode; }

    Node *startNode() const { return m_startNode; }
    Node *engineNode() const { return m_engineNode; }
    Node *functionNode() const { return m_functionNode; }
    Node *cppFrameNode() const { return m_cppFrameNode; }
    Node *endNode() const { return m_endNode; }
    Node *initialFrameState() const { return m_initialFrameState; }
    void setStartNode(Node *n) { m_startNode = n; }
    void setEngineNode(Node *n) { m_engineNode = n; }
    void setFunctionNode(Node *n) { m_functionNode = n; }
    void setCppFrameNode(Node *n) { m_cppFrameNode = n; }
    void setEndNode(Node *n) { m_endNode = n; }
    void setInitialFrameState(Node *n) { m_initialFrameState = n; }

    unsigned nodeCount() const
    { return unsigned(m_nextNodeId); }

    void addEndInput(Node *n);

private: // types and methods
    Graph(Function *function);

private: // fields
    Function *m_function;
    OperationBuilder *m_opBuilder;
    Node::Id m_nextNodeId = 0;
    Node *m_undefinedNode = nullptr;
    Node *m_emptyNode = nullptr;
    Node *m_nullNode = nullptr;
    Node *m_trueNode = nullptr;
    Node *m_falseNode = nullptr;
    Node *m_startNode = nullptr;
    Node *m_engineNode = nullptr;
    Node *m_functionNode = nullptr;
    Node *m_cppFrameNode = nullptr;
    Node *m_endNode = nullptr;
    Node *m_initialFrameState = nullptr;
};

} // namespace IR
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4GRAPH_P_H
