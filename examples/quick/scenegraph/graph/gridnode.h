// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GRIDNODE_H
#define GRIDNODE_H

#include <QtQuick/QSGGeometryNode>
#include <QtQuick/QSGFlatColorMaterial>

class GridNode : public QSGGeometryNode
{
public:
    GridNode();

    void setRect(const QRectF &rect);

private:
    QSGFlatColorMaterial m_material;
    QSGGeometry m_geometry;
};

#endif // GRIDNODE_H
