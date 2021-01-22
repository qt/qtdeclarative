/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
        m_color = color;
        QSGGeometry::ColoredPoint2D *pts = m_geometry.vertexDataAsColoredPoint2D();
        for (int i = 0; i < 4; ++i) {
            pts[i].r = uchar(qRound(m_color.redF() * m_color.alphaF() * 255));
            pts[i].g = uchar(qRound(m_color.greenF() * m_color.alphaF() * 255));
            pts[i].b = uchar(qRound(m_color.blueF() * m_color.alphaF() * 255));
            pts[i].a = uchar(qRound(m_color.alphaF() * 255));
        }
        markDirty(QSGNode::DirtyGeometry);
    }
}

QColor QSGDefaultRectangleNode::color() const
{
    return m_color;
}

QT_END_NAMESPACE
