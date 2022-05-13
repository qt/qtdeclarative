// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GRAPH_H
#define GRAPH_H

#include <QQuickItem>

class Graph : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
public:
    Graph();

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry);

public slots:
    void appendSample(qreal value);
    void removeFirstSample();

private:
    QList<qreal> m_samples;

    bool m_samplesChanged;
    bool m_geometryChanged;
};

#endif // GRAPH_H
