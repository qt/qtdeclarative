// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGSOFTWAREINTERNALRECTANGLENODE_H
#define QSGSOFTWAREINTERNALRECTANGLENODE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qsgadaptationlayer_p.h>

#include <QPen>
#include <QBrush>
#include <QPixmap>

QT_BEGIN_NAMESPACE

class QSGSoftwareInternalRectangleNode : public QSGInternalRectangleNode
{
public:
    QSGSoftwareInternalRectangleNode();

    void setRect(const QRectF &rect) override;
    void setColor(const QColor &color) override;
    void setPenColor(const QColor &color) override;
    void setPenWidth(qreal width) override;
    void setGradientStops(const QGradientStops &stops) override;
    void setGradientVertical(bool vertical) override;
    void setRadius(qreal radius) override;
    void setTopLeftRadius(qreal radius) override;
    void setTopRightRadius(qreal radius) override;
    void setBottomLeftRadius(qreal radius) override;
    void setBottomRightRadius(qreal radius) override;
    void setAntialiasing(bool antialiasing) override { Q_UNUSED(antialiasing); }
    void setAligned(bool aligned) override;

    void update() override;

    void paint(QPainter *);

    bool isOpaque() const;
    QRectF rect() const;
private:
    void paintRectangle(QPainter *painter, const QRect &rect);
    void paintRectangleIndividualCorners(QPainter *painter, const QRect &rect);
    void generateCornerPixmap();

    QRect m_rect;
    QColor m_color;
    QColor m_penColor;
    qreal m_penWidth;
    QGradientStops m_stops;
    qreal m_radius;
    qreal m_topLeftRadius;
    qreal m_topRightRadius;
    qreal m_bottomLeftRadius;
    qreal m_bottomRightRadius;
    QPen m_pen;
    QBrush m_brush;
    bool m_vertical;

    bool m_cornerPixmapIsDirty;
    QPixmap m_cornerPixmap;

    qreal m_devicePixelRatio;
};

QT_END_NAMESPACE

#endif // QSGSOFTWAREINTERNALRECTANGLENODE_H
