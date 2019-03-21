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

#include "qv4graph_p.h"
#include "qv4operation_p.h"

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace IR {

Graph *Graph::create(Function *function)
{
    auto storage = function->pool()->allocate(sizeof(Graph));
    auto g = new (storage) Graph(function);
    g->m_undefinedNode = g->createNode(g->opBuilder()->get<Meta::Undefined>());
    g->m_emptyNode = g->createNode(g->opBuilder()->get<Meta::Empty>());
    g->m_nullNode = g->createNode(g->opBuilder()->getConstant(QV4::Value::nullValue()));
    g->m_trueNode = g->createNode(g->opBuilder()->getConstant(QV4::Value::fromBoolean(true)));
    g->m_falseNode = g->createNode(g->opBuilder()->getConstant(QV4::Value::fromBoolean(false)));
    return g;
}

Graph::MemoryPool *Graph::pool() const
{
    return m_function->pool();
}

Node *Graph::createNode(const Operation *op, Node *const operands[], size_t opCount,
                        bool incomplete)
{
    return Node::create(pool(), m_nextNodeId++, op, opCount, operands, incomplete);
}

Node *Graph::createConstantBoolNode(bool value)
{
    return createNode(opBuilder()->getConstant(Primitive::fromBoolean(value)));
}

Node *Graph::createConstantIntNode(int value)
{
    return createNode(opBuilder()->getConstant(Primitive::fromInt32(value)));
}

Graph::Graph(Function *function)
    : m_function(function)
    , m_opBuilder(OperationBuilder::create(pool()))
{}

Node *Graph::createConstantHeapNode(Heap::Base *heap)
{
    return createNode(opBuilder()->getConstant(Primitive::fromHeapObject(heap)));
}

void Graph::addEndInput(Node *n)
{
    if (m_endNode) {
        auto newEnd = m_opBuilder->getEnd(m_endNode->operation()->controlInputCount() + 1);
        m_endNode->setOperation(newEnd);
        m_endNode->addInput(m_function->pool(), n);
    }
}

} // IR namespace
} // QV4 namespace
QT_END_NAMESPACE
