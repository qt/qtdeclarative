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

#include "qsgninepatchnode.h"

QT_BEGIN_NAMESPACE

/*!
  \class QSGNinePatchNode
  \inmodule QtQuick
  \since 5.8
  \internal
 */

/*!
    \fn void QSGNinePatchNode::setTexture(QSGTexture *texture)
    \internal
 */

/*!
    \fn void QSGNinePatchNode::setBounds(const QRectF &bounds)
    \internal
 */

/*!
    \fn void QSGNinePatchNode::setDevicePixelRatio(qreal ratio)
    \internal
 */

/*!
    \fn void QSGNinePatchNode::setPadding(qreal left, qreal top, qreal right, qreal bottom)
    \internal
 */


/*!
    \fn void QSGNinePatchNode::update()
    \internal
 */

void QSGNinePatchNode::rebuildGeometry(QSGTexture *texture, QSGGeometry *geometry, const QVector4D &padding,
                                       const QRectF &bounds, qreal dpr)
{
    if (padding.x() <= 0 && padding.y() <= 0 && padding.z() <= 0 && padding.w() <= 0) {
        geometry->allocate(4, 0);
        QSGGeometry::updateTexturedRectGeometry(geometry, bounds, texture->normalizedTextureSubRect());
        return;
    }

    QRectF tc = texture->normalizedTextureSubRect();
    QSize ts = texture->textureSize();
    ts.setHeight(ts.height() / dpr);
    ts.setWidth(ts.width() / dpr);

    qreal invtw = tc.width() / ts.width();
    qreal invth = tc.height() / ts.height();

    struct Coord { qreal p; qreal t; };
    Coord cx[4] = { { bounds.left(), tc.left() },
                    { bounds.left() + padding.x(), tc.left() + padding.x() * invtw },
                    { bounds.right() - padding.z(), tc.right() - padding.z() * invtw },
                    { bounds.right(), tc.right() }
                  };
    Coord cy[4] = { { bounds.top(), tc.top() },
                    { bounds.top() + padding.y(), tc.top() + padding.y() * invth },
                    { bounds.bottom() - padding.w(), tc.bottom() - padding.w() * invth },
                    { bounds.bottom(), tc.bottom() }
                  };

    geometry->allocate(16, 28);
    QSGGeometry::TexturedPoint2D *v = geometry->vertexDataAsTexturedPoint2D();
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            v->set(cx[x].p, cy[y].p, cx[x].t, cy[y].t);
            ++v;
        }
    }

    quint16 *i = geometry->indexDataAsUShort();
    for (int r = 0; r < 3; ++r) {
        if (r > 0)
            *i++ = 4 * r;
        for (int c = 0; c < 4; ++c) {
            i[0] = 4 * r + c;
            i[1] = 4 * r + c + 4;
            i += 2;
        }
        if (r < 2)
            *i++ = 4 * r + 3 + 4;
    }
}

QT_END_NAMESPACE
