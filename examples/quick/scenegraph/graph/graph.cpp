// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "graph.h"

#include "noisynode.h"
#include "gridnode.h"
#include "linenode.h"

Graph::Graph()
    : m_samplesChanged(false)
    , m_geometryChanged(false)
{
    setFlag(ItemHasContents, true);
}


void Graph::appendSample(qreal value)
{
    m_samples << value;
    m_samplesChanged = true;
    update();
}


void Graph::removeFirstSample()
{
    m_samples.removeFirst();
    m_samplesChanged = true;
    update();
}

void Graph::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_geometryChanged = true;
    update();
    QQuickItem::geometryChange(newGeometry, oldGeometry);
}


class GraphNode : public QSGNode
{
public:
    NoisyNode *background;
    GridNode *grid;
    LineNode *line;
    LineNode *shadow;
};


QSGNode *Graph::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    GraphNode *n= static_cast<GraphNode *>(oldNode);

    QRectF rect = boundingRect();

    if (rect.isEmpty()) {
        delete n;
        return nullptr;
    }

    if (!n) {
        n = new GraphNode();

        n->background = new NoisyNode(window());
        n->grid = new GridNode();
        n->line = new LineNode(10, 0.5, QColor("steelblue"));
        n->shadow = new LineNode(20, 0.2f, QColor::fromRgbF(0.2, 0.2, 0.2, 0.4));

        n->appendChildNode(n->background);
        n->appendChildNode(n->grid);
        n->appendChildNode(n->shadow);
        n->appendChildNode(n->line);
    }

    if (m_geometryChanged) {
        n->background->setRect(rect);
        n->grid->setRect(rect);
    }

    if (m_geometryChanged || m_samplesChanged) {
        n->line->updateGeometry(rect, m_samples);
        // We don't need to calculate the geometry twice, so just steal it from the other one...
        n->shadow->setGeometry(n->line->geometry());
    }

    m_geometryChanged = false;
    m_samplesChanged = false;

    return n;
}
