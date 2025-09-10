// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qquickclipnode_p.h"

#include <QtGui/qvector2d.h>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

QQuickDefaultClipNode::QQuickDefaultClipNode(const QRectF &rect)
    : m_rect(rect)
    , m_radius(0)
    , m_dirty_geometry(true)
    , m_geometry(QSGGeometry::defaultAttributes_Point2D(), 0)
{
    Q_UNUSED(m_reserved);
    setGeometry(&m_geometry);
    setIsRectangular(true);
}

void QQuickDefaultClipNode::setRect(const QRectF &rect)
{
    m_rect = rect;
    m_dirty_geometry = true;
}

void QQuickDefaultClipNode::setRadius(qreal radius)
{
    m_radius = radius;
    m_dirty_geometry = true;
    setIsRectangular(radius == 0);
}

void QQuickDefaultClipNode::update()
{
    if (m_dirty_geometry) {
        updateGeometry();
        m_dirty_geometry = false;
    }
}

void QQuickDefaultClipNode::updateGeometry()
{
    QSGGeometry *g = geometry();

    if (qFuzzyIsNull(m_radius)) {
        g->allocate(4);
        QSGGeometry::updateRectGeometry(g, m_rect);

    } else {
        int vertexCount = 0;

        // Radius should never exceeds half of the width or half of the height
        qreal radius = qMin(qMin(m_rect.width() / 2, m_rect.height() / 2), m_radius);
        QRectF rect = m_rect;
        rect.adjust(radius, radius, -radius, -radius);

        int segments = qMin(30, qCeil(radius)); // Number of segments per corner.

        g->allocate((segments + 1) * 4);

        QVector2D *vertices = (QVector2D *)g->vertexData();

        for (int part = 0; part < 2; ++part) {
            for (int i = 0; i <= segments; ++i) {
                //### Should change to calculate sin/cos only once.
                qreal angle = qreal(0.5 * M_PI) * (part + i / qreal(segments));
                qreal s = qFastSin(angle);
                qreal c = qFastCos(angle);
                qreal y = (part ? rect.bottom() : rect.top()) - radius * c; // current inner y-coordinate.
                qreal lx = rect.left() - radius * s; // current inner left x-coordinate.
                qreal rx = rect.right() + radius * s; // current inner right x-coordinate.

                vertices[vertexCount++] = QVector2D(rx, y);
                vertices[vertexCount++] = QVector2D(lx, y);
            }
        }

    }
#ifdef QSG_RUNTIME_DESCRIPTION
#ifndef QT_NO_DEBUG_STREAM
    QString desc;
    {
        QDebug dbg(&desc);
        dbg << m_rect;
        if (!qFuzzyIsNull(m_radius))
            dbg << "radius" << m_radius;
    }
    qsgnode_set_description(this, desc);
#endif
#endif
    setClipRect(m_rect);
    markDirty(DirtyGeometry);
}

QT_END_NAMESPACE
