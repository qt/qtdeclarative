/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef DEFAULT_RECTANGLENODE_H
#define DEFAULT_RECTANGLENODE_H

#include <private/qsgadaptationlayer_p.h>

#include <QtQuick/qsgflatcolormaterial.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QSGMaterial;
class QSGContext;

class QSGDefaultRectangleNode : public QSGRectangleNode
{
public:
    QSGDefaultRectangleNode(QSGContext *context);
    ~QSGDefaultRectangleNode();

    virtual void setRect(const QRectF &rect);
    virtual void setColor(const QColor &color);
    virtual void setPenColor(const QColor &color);
    virtual void setPenWidth(qreal width);
    virtual void setGradientStops(const QGradientStops &stops);
    virtual void setRadius(qreal radius);
    virtual void setAligned(bool aligned);
    virtual void update();

private:
    enum {
        TypeFlat,
        TypeVertexGradient
    };
    QSGGeometryNode *border();

    void updateGeometry();
    void updateGradientTexture();

    QSGGeometryNode *m_border;
    QSGFlatColorMaterial m_border_material;
    QSGFlatColorMaterial m_fill_material;

    QRectF m_rect;
    QGradientStops m_gradient_stops;
    qreal m_radius;
    qreal m_pen_width;

    uint m_aligned : 1;
    uint m_gradient_is_opaque : 1;
    uint m_dirty_geometry : 1;

    uint m_material_type : 2; // Only goes up to 3

    QSGGeometry m_default_geometry;

    QSGContext *m_context;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
