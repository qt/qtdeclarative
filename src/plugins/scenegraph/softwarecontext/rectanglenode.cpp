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
#include "rectanglenode.h"
#include <qmath.h>

#include <QtGui/QPainter>


RectangleNode::RectangleNode()
    : m_penWidth(0)
    , m_radius(0)
    , m_cornerPixmapIsDirty(true)
{
    setMaterial((QSGMaterial*)1);
    setGeometry((QSGGeometry*)1);
}

void RectangleNode::setRect(const QRectF &rect)
{
    if (m_rect != rect) {
        m_rect = rect;
    }
}

void RectangleNode::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        m_cornerPixmapIsDirty = true;
    }
}

void RectangleNode::setPenColor(const QColor &color)
{
    if (m_penColor != color) {
        m_penColor = color;
        m_cornerPixmapIsDirty = true;
    }
}

void RectangleNode::setPenWidth(qreal width)
{
    if (m_penWidth != width) {
        m_penWidth = width;
        m_cornerPixmapIsDirty = true;
    }
}

//Move first stop by pos relative to seconds
static QGradientStop interpolateStop(const QGradientStop &firstStop, const QGradientStop &secondStop, double newPos)
{
    double distance = secondStop.first - firstStop.first;
    double distanceDelta = newPos - firstStop.first;
    double modifierValue = distanceDelta / distance;
    int redDelta = (secondStop.second.red() - firstStop.second.red()) * modifierValue;
    int greenDelta = (secondStop.second.green() - firstStop.second.green()) * modifierValue;
    int blueDelta = (secondStop.second.blue() - firstStop.second.blue()) * modifierValue;
    int alphaDelta = (secondStop.second.alpha() - firstStop.second.alpha()) * modifierValue;

    QGradientStop newStop;
    newStop.first = newPos;
    newStop.second = QColor(firstStop.second.red() + redDelta,
                            firstStop.second.green() + greenDelta,
                            firstStop.second.blue() + blueDelta,
                            firstStop.second.alpha() + alphaDelta);

    return newStop;
}

void RectangleNode::setGradientStops(const QGradientStops &stops)
{
    //normalize stops
    bool needsNormalization = false;
    foreach (const QGradientStop &stop, stops) {
        if (stop.first < 0.0 || stop.first > 1.0) {
            needsNormalization = true;
            continue;
        }
    }

    if (needsNormalization) {
        QGradientStops normalizedStops;
        if (stops.count() == 1) {
            //If there is only one stop, then the position does not matter
            //It is just treated as a color
            QGradientStop stop = stops.at(0);
            stop.first = 0.0;
            normalizedStops.append(stop);
        } else {
            //Clip stops to only the first below 0.0 and above 1.0
            int below = -1;
            int above = -1;
            QVector<int> between;
            for (int i = 0; i < stops.count(); ++i) {
                if (stops.at(i).first < 0.0) {
                    below = i;
                } else if (stops.at(i).first > 1.0) {
                    above = i;
                    break;
                } else {
                    between.append(i);
                }
            }

            //Interpoloate new color values for above and below
            if (below != -1 ) {
                //If there are more than one stops left, interpolate
                if (below + 1 < stops.count()) {
                    normalizedStops.append(interpolateStop(stops.at(below), stops.at(below + 1), 0.0));
                } else {
                    QGradientStop singleStop;
                    singleStop.first = 0.0;
                    singleStop.second = stops.at(below).second;
                    normalizedStops.append(singleStop);
                }
            }

            for (int i = 0; i < between.count(); ++i)
                normalizedStops.append(stops.at(between.at(i)));

            if (above != -1) {
                //If there stops before above, interpolate
                if (above >= 1) {
                    normalizedStops.append(interpolateStop(stops.at(above), stops.at(above - 1), 1.0));
                } else {
                    QGradientStop singleStop;
                    singleStop.first = 1.0;
                    singleStop.second = stops.at(above).second;
                    normalizedStops.append(singleStop);
                }
            }
        }

        m_stops = normalizedStops;

    } else {
        m_stops = stops;
    }
    m_cornerPixmapIsDirty = true;
}

void RectangleNode::setRadius(qreal radius)
{
    if (m_radius != radius) {
        m_radius = radius;
        m_cornerPixmapIsDirty = true;
    }
}

void RectangleNode::setAligned(bool /*aligned*/)
{
}

void RectangleNode::update()
{
    if (!m_penWidth || m_penColor == Qt::transparent) {
        m_pen = Qt::NoPen;
    } else {
        m_pen = QPen(m_penColor);
        m_pen.setWidthF(m_penWidth);
    }

    if (!m_stops.isEmpty()) {
        QLinearGradient gradient(QPoint(0,0), QPoint(0,1));
        gradient.setStops(m_stops);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        m_brush = QBrush(gradient);
    } else {
        m_brush = QBrush(m_color);
    }

    if (m_cornerPixmapIsDirty) {
        //Generate new corner Pixmap
        int radius = qRound(qMin(qMin(m_rect.width(), m_rect.height()) * 0.5f, m_radius));

        m_cornerPixmap = QPixmap(radius * 2, radius * 2);
        m_cornerPixmap.fill(Qt::transparent);

        if (radius > 0) {
            QPainter cornerPainter(&m_cornerPixmap);
            cornerPainter.setRenderHint(QPainter::Antialiasing);
            cornerPainter.setCompositionMode(QPainter::CompositionMode_Source);

            //Paint outer cicle
            if (m_penWidth > 0) {
                cornerPainter.setPen(Qt::NoPen);
                cornerPainter.setBrush(m_penColor);
                cornerPainter.drawRoundedRect(QRectF(0, 0, radius * 2, radius *2), radius, radius);
            }

            //Paint inner circle
            if (radius > m_penWidth) {
                cornerPainter.setPen(Qt::NoPen);
                if (m_stops.isEmpty())
                    cornerPainter.setBrush(m_brush);
                else
                    cornerPainter.setBrush(Qt::transparent);

                QMarginsF adjustmentMargins(m_penWidth, m_penWidth, m_penWidth, m_penWidth);
                QRectF cornerCircleRect = QRectF(0, 0, radius * 2, radius * 2).marginsRemoved(adjustmentMargins);
                cornerPainter.drawRoundedRect(cornerCircleRect, radius, radius);
            }
            cornerPainter.end();
        }
        m_cornerPixmapIsDirty = false;
    }
}

void RectangleNode::paint(QPainter *painter)
{
    //Radius should never exceeds half of the width or half of the height
    int radius = qRound(qMin(qMin(m_rect.width(), m_rect.height()) * 0.5, m_radius));

    QPainter::RenderHints previousRenderHints = painter->renderHints();
    painter->setRenderHint(QPainter::Antialiasing, false);

    if (m_penWidth > 0) {
        //Borders can not be more than half the height/width of a rect
        double borderWidth = qMin(m_penWidth, m_rect.width() * 0.5);
        double borderHeight = qMin(m_penWidth, m_rect.height() * 0.5);

        //Fill 4 border Rects
        QRectF borderTop(QPointF(m_rect.x() + radius, m_rect.y()),
                         QPointF(m_rect.x() + m_rect.width() - radius, m_rect.y() + borderHeight));
        painter->fillRect(borderTop, m_penColor);
        QRectF borderBottom(QPointF(m_rect.x() + radius, m_rect.y() + m_rect.height() - borderHeight),
                            QPointF(m_rect.x() + m_rect.width() - radius, m_rect.y() + m_rect.height()));
        painter->fillRect(borderBottom, m_penColor);
        QRectF borderLeft(QPointF(m_rect.x(), m_rect.y() + radius),
                          QPointF(m_rect.x() + borderWidth, m_rect.y() + m_rect.height() - radius));
        painter->fillRect(borderLeft, m_penColor);
        QRectF borderRight(QPointF(m_rect.x() + m_rect.width() - borderWidth, m_rect.y() + radius),
                           QPointF(m_rect.x() + m_rect.width(), m_rect.y() + m_rect.height() - radius));
        painter->fillRect(borderRight, m_penColor);
    }

    if (radius > 0) {
        //blit 4 corners to border
        QRectF topLeftCorner(QPointF(m_rect.x(), m_rect.y()),
                             QPointF(m_rect.x() + radius, m_rect.y() + radius));
        painter->drawPixmap(topLeftCorner, m_cornerPixmap, QRectF(0, 0, radius, radius));
        QRectF topRightCorner(QPointF(m_rect.x() + m_rect.width() - radius, m_rect.y()),
                              QPointF(m_rect.x() + m_rect.width(), m_rect.y() + radius));
        painter->drawPixmap(topRightCorner, m_cornerPixmap, QRectF(radius, 0, radius, radius));
        QRectF bottomLeftCorner(QPointF(m_rect.x(), m_rect.y() + m_rect.height() - radius),
                                QPointF(m_rect.x() + radius, m_rect.y() + m_rect.height()));
        painter->drawPixmap(bottomLeftCorner, m_cornerPixmap, QRectF(0, radius, radius, radius));
        QRectF bottomRightCorner(QPointF(m_rect.x() + m_rect.width() - radius, m_rect.y() + m_rect.height() - radius),
                                 QPointF(m_rect.x() + m_rect.width(), m_rect.y() + m_rect.height()));
        painter->drawPixmap(bottomRightCorner, m_cornerPixmap, QRectF(radius, radius, radius, radius));

    }

    QRectF brushRect = m_rect.marginsRemoved(QMarginsF(m_penWidth, m_penWidth, m_penWidth, m_penWidth));
    if (brushRect.width() < 0)
        brushRect.setWidth(0);
    if (brushRect.height() < 0)
        brushRect.setHeight(0);
    double innerRectRadius = qMax(0.0, radius - m_penWidth);

    //If not completely transparent or has a gradient
    if (m_color.alpha() > 0 || !m_stops.empty()) {
        if (innerRectRadius > 0) {
            //Rounded Rect
            if (m_stops.empty()) {
                //Rounded Rects without gradient need 3 blits
                QRectF centerRect(QPointF(brushRect.x() + innerRectRadius, brushRect.y()),
                                  QPointF(brushRect.x() + brushRect.width() - innerRectRadius, brushRect.y() + brushRect.height()));
                painter->fillRect(centerRect, m_color);
                QRectF leftRect(QPointF(brushRect.x(), brushRect.y() + innerRectRadius),
                                QPointF(brushRect.x() + innerRectRadius, brushRect.y() + brushRect.height() - innerRectRadius));
                painter->fillRect(leftRect, m_color);
                QRectF rightRect(QPointF(brushRect.x() + brushRect.width() - innerRectRadius, brushRect.y() + innerRectRadius),
                                 QPointF(brushRect.x() + brushRect.width(), brushRect.y() + brushRect.height() - innerRectRadius));
                painter->fillRect(rightRect, m_color);
            } else {
                //Rounded Rect with gradient (slow)
                painter->setPen(Qt::NoPen);
                painter->setBrush(m_brush);
                painter->drawRoundedRect(brushRect, innerRectRadius, innerRectRadius);
            }
        } else {
            //non-rounded rects only need 1 blit
            painter->fillRect(brushRect, m_brush);
        }
    }

    painter->setRenderHints(previousRenderHints);
}
