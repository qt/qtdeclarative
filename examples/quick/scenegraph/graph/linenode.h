// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef LINENODE_H
#define LINENODE_H

#include <QSGGeometryNode>

class LineNode : public QSGGeometryNode
{
public:
    LineNode(float size, float spread, const QColor &color);

    void updateGeometry(const QRectF &bounds, const QList<qreal> &samples);

private:
    QSGGeometry m_geometry;

};

#endif // LINENODE_H
