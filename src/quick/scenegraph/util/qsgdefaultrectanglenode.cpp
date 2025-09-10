// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgdefaultrectanglenode_p.h"
#include "qsgflatcolormaterial.h"

QT_BEGIN_NAMESPACE

// Unlike our predecessor, QSGSimpleRectNode, use QSGVertexColorMaterial
// instead of Flat in order to allow better batching in the renderer.

QSGDefaultRectangleNode::QSGDefaultRectangleNode()
    : m_geometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 4)
{
    QSGGeometry::updateColoredRectGeometry(&m_geometry, QRectF());
    setMaterial(&m_material);
    setGeometry(&m_geometry);
    setColor(QColor(255, 255, 255));
#ifdef QSG_RUNTIME_DESCRIPTION
    qsgnode_set_description(this, QLatin1String("rectangle"));
#endif
}

void QSGDefaultRectangleNode::setRect(const QRectF &rect)
{
    QSGGeometry::updateColoredRectGeometry(&m_geometry, rect);
    markDirty(QSGNode::DirtyGeometry);
}

QRectF QSGDefaultRectangleNode::rect() const
{
    const QSGGeometry::ColoredPoint2D *pts = m_geometry.vertexDataAsColoredPoint2D();
    return QRectF(pts[0].x,
                  pts[0].y,
                  pts[3].x - pts[0].x,
                  pts[3].y - pts[0].y);
}

void QSGDefaultRectangleNode::setColor(const QColor &color)
{
    if (color != m_color) {
        float r, g, b, a;
        color.getRgbF(&r, &g, &b, &a);
        QSGGeometry::ColoredPoint2D *pts = m_geometry.vertexDataAsColoredPoint2D();
        for (int i = 0; i < 4; ++i) {
            pts[i].r = uchar(qRound(r * a * 255));
            pts[i].g = uchar(qRound(g * a * 255));
            pts[i].b = uchar(qRound(b * a * 255));
            pts[i].a = uchar(qRound(a * 255));
        }
        markDirty(QSGNode::DirtyGeometry);
    }
}

QColor QSGDefaultRectangleNode::color() const
{
    return m_color;
}

QT_END_NAMESPACE
