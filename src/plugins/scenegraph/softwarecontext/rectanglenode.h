/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt SceneGraph Raster Add-on.
**
** $QT_BEGIN_LICENSE$
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef RECTANGLENODE_H
#define RECTANGLENODE_H

#include <private/qsgadaptationlayer_p.h>

#include <QPen>
#include <QBrush>
#include <QPixmap>

class RectangleNode : public QSGRectangleNode
{
public:
    RectangleNode();

    virtual void setRect(const QRectF &rect);
    virtual void setColor(const QColor &color);
    virtual void setPenColor(const QColor &color);
    virtual void setPenWidth(qreal width);
    virtual void setGradientStops(const QGradientStops &stops);
    virtual void setRadius(qreal radius);
    virtual void setAntialiasing(bool antialiasing) { Q_UNUSED(antialiasing) }
    virtual void setAligned(bool aligned);

    virtual void update();

    void paint(QPainter *);

private:
    void paintRectangle(QPainter *painter, const QRect &rect);

    QRect m_rect;
    QColor m_color;
    QColor m_penColor;
    double m_penWidth;
    QGradientStops m_stops;
    double m_radius;
    QPen m_pen;
    QBrush m_brush;

    bool m_cornerPixmapIsDirty;
    QPixmap m_cornerPixmap;
};

#endif // RECTANGLENODE_H
