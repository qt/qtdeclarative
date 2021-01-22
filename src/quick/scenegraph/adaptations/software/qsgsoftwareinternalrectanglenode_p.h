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
    void setAntialiasing(bool antialiasing) override { Q_UNUSED(antialiasing) }
    void setAligned(bool aligned) override;

    void update() override;

    void paint(QPainter *);

    bool isOpaque() const;
    QRectF rect() const;
private:
    void paintRectangle(QPainter *painter, const QRect &rect);
    void generateCornerPixmap();

    QRect m_rect;
    QColor m_color;
    QColor m_penColor;
    double m_penWidth;
    QGradientStops m_stops;
    double m_radius;
    QPen m_pen;
    QBrush m_brush;
    bool m_vertical;

    bool m_cornerPixmapIsDirty;
    QPixmap m_cornerPixmap;

    qreal m_devicePixelRatio;
};

QT_END_NAMESPACE

#endif // QSGSOFTWAREINTERNALRECTANGLENODE_H
