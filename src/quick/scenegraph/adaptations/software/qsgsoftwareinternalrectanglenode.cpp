// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgsoftwareinternalrectanglenode_p.h"
#include <qmath.h>

#include <QtGui/QPainter>

QT_BEGIN_NAMESPACE

QSGSoftwareInternalRectangleNode::QSGSoftwareInternalRectangleNode()
    : m_penWidth(0)
    , m_radius(0)
    , m_vertical(true)
    , m_cornerPixmapIsDirty(true)
    , m_devicePixelRatio(1)
{
    m_pen.setJoinStyle(Qt::MiterJoin);
    m_pen.setMiterLimit(0);
    setMaterial((QSGMaterial*)1);
    setGeometry((QSGGeometry*)1);
}

void QSGSoftwareInternalRectangleNode::setRect(const QRectF &rect)
{
    QRect alignedRect = rect.toAlignedRect();
    if (m_rect != alignedRect) {
        m_rect = alignedRect;
        markDirty(DirtyMaterial);
    }
}

void QSGSoftwareInternalRectangleNode::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        m_cornerPixmapIsDirty = true;
        markDirty(DirtyMaterial);
    }
}

void QSGSoftwareInternalRectangleNode::setPenColor(const QColor &color)
{
    if (m_penColor != color) {
        m_penColor = color;
        m_cornerPixmapIsDirty = true;
        markDirty(DirtyMaterial);
    }
}

void QSGSoftwareInternalRectangleNode::setPenWidth(qreal width)
{
    if (m_penWidth != width) {
        m_penWidth = width;
        m_cornerPixmapIsDirty = true;
        markDirty(DirtyMaterial);
    }
}

//Move first stop by pos relative to seconds
static QGradientStop interpolateStop(const QGradientStop &firstStop, const QGradientStop &secondStop, double newPos)
{
    double distance = secondStop.first - firstStop.first;
    double distanceDelta = newPos - firstStop.first;
    double modifierValue = distanceDelta / distance;
    const auto firstStopRgbColor = firstStop.second.toRgb();
    const auto secondStopRgbColor = secondStop.second.toRgb();
    int redDelta = (secondStopRgbColor.red() - firstStopRgbColor.red()) * modifierValue;
    int greenDelta = (secondStopRgbColor.green() - firstStopRgbColor.green()) * modifierValue;
    int blueDelta = (secondStopRgbColor.blue() - firstStopRgbColor.blue()) * modifierValue;
    int alphaDelta = (secondStopRgbColor.alpha() - firstStopRgbColor.alpha()) * modifierValue;

    QGradientStop newStop;
    newStop.first = newPos;
    newStop.second = QColor(firstStopRgbColor.red() + redDelta,
                            firstStopRgbColor.green() + greenDelta,
                            firstStopRgbColor.blue() + blueDelta,
                            firstStopRgbColor.alpha() + alphaDelta);

    return newStop;
}

void QSGSoftwareInternalRectangleNode::setGradientStops(const QGradientStops &stops)
{
    //normalize stops
    bool needsNormalization = false;
    for (const QGradientStop &stop : std::as_const(stops)) {
        if (stop.first < 0.0 || stop.first > 1.0) {
            needsNormalization = true;
            break;
        }
    }

    if (needsNormalization) {
        QGradientStops normalizedStops;
        if (stops.size() == 1) {
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
            for (int i = 0; i < stops.size(); ++i) {
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
                if (below + 1 < stops.size()) {
                    normalizedStops.append(interpolateStop(stops.at(below), stops.at(below + 1), 0.0));
                } else {
                    QGradientStop singleStop;
                    singleStop.first = 0.0;
                    singleStop.second = stops.at(below).second;
                    normalizedStops.append(singleStop);
                }
            }

            for (int i = 0; i < between.size(); ++i)
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
    markDirty(DirtyMaterial);
}

void QSGSoftwareInternalRectangleNode::setGradientVertical(bool vertical)
{
    if (m_vertical != vertical) {
        m_vertical = vertical;
        m_cornerPixmapIsDirty = true;
        markDirty(DirtyMaterial);
    }
}

void QSGSoftwareInternalRectangleNode::setRadius(qreal radius)
{
    if (m_radius != radius) {
        m_radius = radius;
        m_cornerPixmapIsDirty = true;
        markDirty(DirtyMaterial);
    }
}

void QSGSoftwareInternalRectangleNode::setAligned(bool /*aligned*/)
{
}

void QSGSoftwareInternalRectangleNode::update()
{
    if (!m_penWidth || m_penColor == Qt::transparent) {
        m_pen = Qt::NoPen;
    } else {
        m_pen = QPen(m_penColor);
        m_pen.setWidthF(m_penWidth);
    }

    if (!m_stops.isEmpty()) {
        QLinearGradient gradient(QPoint(0,0), QPoint(m_vertical ? 0 : m_rect.width(), m_vertical ? m_rect.height() : 0));
        gradient.setStops(m_stops);
        m_brush = QBrush(gradient);
    } else {
        m_brush = QBrush(m_color);
    }

    if (m_cornerPixmapIsDirty) {
        generateCornerPixmap();
        m_cornerPixmapIsDirty = false;
    }
}

void QSGSoftwareInternalRectangleNode::paint(QPainter *painter)
{
    //We can only check for a device pixel ratio change when we know what
    //paint device is being used.
    if (!qFuzzyCompare(painter->device()->devicePixelRatio(), m_devicePixelRatio)) {
        m_devicePixelRatio = painter->device()->devicePixelRatio();
        generateCornerPixmap();
    }

    if (painter->transform().isRotating()) {
        //Rotated rectangles lose the benefits of direct rendering, and have poor rendering
        //quality when using only blits and fills.

        if (m_radius == 0 && m_penWidth == 0) {
            //Non-Rounded Rects without borders (fall back to drawRect)
            //Most common case
            painter->setPen(Qt::NoPen);
            painter->setBrush(m_brush);
            painter->drawRect(m_rect);
        } else {
            //Rounded Rects and Rects with Borders
            //Avoids broken behaviors of QPainter::drawRect/roundedRect
            QPixmap pixmap = QPixmap(qRound(m_rect.width() * m_devicePixelRatio), qRound(m_rect.height() * m_devicePixelRatio));
            pixmap.fill(Qt::transparent);
            pixmap.setDevicePixelRatio(m_devicePixelRatio);
            QPainter pixmapPainter(&pixmap);
            paintRectangle(&pixmapPainter, QRect(0, 0, m_rect.width(), m_rect.height()));

            QPainter::RenderHints previousRenderHints = painter->renderHints();
            painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter->drawPixmap(m_rect, pixmap);
            painter->setRenderHints(previousRenderHints);
        }


    } else {
        //Paint directly
        paintRectangle(painter, m_rect);
    }

}

bool QSGSoftwareInternalRectangleNode::isOpaque() const
{
    if (m_radius > 0.0f)
        return false;
    if (m_color.alpha() < 255)
        return false;
    if (m_penWidth > 0.0f && m_penColor.alpha() < 255)
        return false;
    if (m_stops.size() > 0) {
        for (const QGradientStop &stop : std::as_const(m_stops)) {
            if (stop.second.alpha() < 255)
                return false;
        }
    }

    return true;
}

QRectF QSGSoftwareInternalRectangleNode::rect() const
{
    //TODO: double check that this is correct.
    return m_rect;
}

void QSGSoftwareInternalRectangleNode::paintRectangle(QPainter *painter, const QRect &rect)
{
    //Radius should never exceeds half of the width or half of the height
    int radius = qFloor(qMin(qMin(rect.width(), rect.height()) * 0.5, m_radius));

    QPainter::RenderHints previousRenderHints = painter->renderHints();
    painter->setRenderHint(QPainter::Antialiasing, false);

    if (m_penWidth > 0) {
        //Fill border Rects

        //Borders can not be more than half the height/width of a rect
        double borderWidth = qMin(m_penWidth, rect.width() * 0.5);
        double borderHeight = qMin(m_penWidth, rect.height() * 0.5);



        if (borderWidth > radius) {
            //4 Rects
            QRectF borderTopOutside(QPointF(rect.x() + radius, rect.y()),
                                    QPointF(rect.x() + rect.width() - radius, rect.y() + radius));
            QRectF borderTopInside(QPointF(rect.x() + borderWidth, rect.y() + radius),
                                   QPointF(rect.x() + rect.width() - borderWidth, rect.y() + borderHeight));
            QRectF borderBottomOutside(QPointF(rect.x() + radius, rect.y() + rect.height() - radius),
                                       QPointF(rect.x() + rect.width() - radius, rect.y() + rect.height()));
            QRectF borderBottomInside(QPointF(rect.x() + borderWidth, rect.y() + rect.height() - borderHeight),
                                      QPointF(rect.x() + rect.width() - borderWidth, rect.y() + rect.height() - radius));

            if (borderTopOutside.isValid())
                painter->fillRect(borderTopOutside, m_penColor);
            if (borderTopInside.isValid())
                painter->fillRect(borderTopInside, m_penColor);
            if (borderBottomOutside.isValid())
                painter->fillRect(borderBottomOutside, m_penColor);
            if (borderBottomInside.isValid())
                painter->fillRect(borderBottomInside, m_penColor);

        } else {
            //2 Rects
            QRectF borderTop(QPointF(rect.x() + radius, rect.y()),
                             QPointF(rect.x() + rect.width() - radius, rect.y() + borderHeight));
            QRectF borderBottom(QPointF(rect.x() + radius, rect.y() + rect.height() - borderHeight),
                                QPointF(rect.x() + rect.width() - radius, rect.y() + rect.height()));
            if (borderTop.isValid())
                painter->fillRect(borderTop, m_penColor);
            if (borderBottom.isValid())
                painter->fillRect(borderBottom, m_penColor);
        }
        QRectF borderLeft(QPointF(rect.x(), rect.y() + radius),
                          QPointF(rect.x() + borderWidth, rect.y() + rect.height() - radius));
        QRectF borderRight(QPointF(rect.x() + rect.width() - borderWidth, rect.y() + radius),
                           QPointF(rect.x() + rect.width(), rect.y() + rect.height() - radius));
        if (borderLeft.isValid())
            painter->fillRect(borderLeft, m_penColor);
        if (borderRight.isValid())
            painter->fillRect(borderRight, m_penColor);
    }


    if (radius > 0) {

        if (radius * 2 >= rect.width() && radius * 2 >= rect.height()) {
            //Blit whole pixmap for circles
            painter->drawPixmap(rect, m_cornerPixmap, m_cornerPixmap.rect());
        } else {

            //blit 4 corners to border
            int scaledRadius = qRound(radius * m_devicePixelRatio);
            QRectF topLeftCorner(QPointF(rect.x(), rect.y()),
                                 QPointF(rect.x() + radius, rect.y() + radius));
            painter->drawPixmap(topLeftCorner, m_cornerPixmap, QRectF(0, 0, scaledRadius, scaledRadius));
            QRectF topRightCorner(QPointF(rect.x() + rect.width() - radius, rect.y()),
                                  QPointF(rect.x() + rect.width(), rect.y() + radius));
            painter->drawPixmap(topRightCorner, m_cornerPixmap, QRectF(scaledRadius, 0, scaledRadius, scaledRadius));
            QRectF bottomLeftCorner(QPointF(rect.x(), rect.y() + rect.height() - radius),
                                    QPointF(rect.x() + radius, rect.y() + rect.height()));
            painter->drawPixmap(bottomLeftCorner, m_cornerPixmap, QRectF(0, scaledRadius, scaledRadius, scaledRadius));
            QRectF bottomRightCorner(QPointF(rect.x() + rect.width() - radius, rect.y() + rect.height() - radius),
                                     QPointF(rect.x() + rect.width(), rect.y() + rect.height()));
            painter->drawPixmap(bottomRightCorner, m_cornerPixmap, QRectF(scaledRadius, scaledRadius, scaledRadius, scaledRadius));

        }

    }

    QRectF brushRect = QRectF(rect).marginsRemoved(QMarginsF(m_penWidth, m_penWidth, m_penWidth, m_penWidth));
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

void QSGSoftwareInternalRectangleNode::generateCornerPixmap()
{
    //Generate new corner Pixmap
    int radius = qFloor(qMin(qMin(m_rect.width(), m_rect.height()) * 0.5, m_radius));
    const auto width = qRound(radius * 2 * m_devicePixelRatio);

    if (m_cornerPixmap.width() != width)
        m_cornerPixmap = QPixmap(width, width);

    m_cornerPixmap.setDevicePixelRatio(m_devicePixelRatio);
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
}

QT_END_NAMESPACE
