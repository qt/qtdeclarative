/******************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick 2d Renderer module.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
******************************************************************************/

#include "ninepatchnode.h"
#include "pixmaptexture.h"
#include "imagenode.h"

NinePatchNode::NinePatchNode()
{
    setMaterial((QSGMaterial*)1);
    setGeometry((QSGGeometry*)1);
}

void NinePatchNode::setTexture(QSGTexture *texture)
{
    PixmapTexture *pt = qobject_cast<PixmapTexture*>(texture);
    if (!pt) {
        qWarning() << "Image used with invalid texture format.";
        return;
    }
    m_pixmap = pt->pixmap();
    markDirty(DirtyMaterial);
}

void NinePatchNode::setBounds(const QRectF &bounds)
{
    if (m_bounds == bounds)
        return;

    m_bounds = bounds;
    markDirty(DirtyGeometry);
}

void NinePatchNode::setDevicePixelRatio(qreal ratio)
{
    if (m_pixelRatio == ratio)
        return;

    m_pixelRatio = ratio;
    markDirty(DirtyGeometry);
}

void NinePatchNode::setPadding(qreal left, qreal top, qreal right, qreal bottom)
{
    QMargins margins(qRound(left), qRound(top), qRound(right), qRound(bottom));
    if (m_margins == margins)
        return;

    m_margins = QMargins(qRound(left), qRound(top), qRound(right), qRound(bottom));
    markDirty(DirtyGeometry);
}

void NinePatchNode::update()
{
}

void NinePatchNode::paint(QPainter *painter)
{
    if (m_margins.isNull())
        painter->drawPixmap(m_bounds, m_pixmap, QRectF(0, 0, m_pixmap.width(), m_pixmap.height()));
    else
        SoftwareContext::qDrawBorderPixmap(painter, m_bounds.toRect(), m_margins, m_pixmap, QRect(0, 0, m_pixmap.width(), m_pixmap.height()),
                                           m_margins, Qt::StretchTile, QDrawBorderPixmap::DrawingHints(0));
}
