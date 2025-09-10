// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGOPENVGINTERNALRECTANGLENODE_H
#define QSGOPENVGINTERNALRECTANGLENODE_H

#include <private/qsgadaptationlayer_p.h>
#include "qsgopenvgrenderable.h"
#include "qopenvgoffscreensurface.h"

#include <VG/openvg.h>

QT_BEGIN_NAMESPACE

class QSGOpenVGInternalRectangleNode : public QSGInternalRectangleNode, public QSGOpenVGRenderable
{
public:
    QSGOpenVGInternalRectangleNode();
    ~QSGOpenVGInternalRectangleNode();

    void setRect(const QRectF &rect) override;
    void setColor(const QColor &color) override;
    void setPenColor(const QColor &color) override;
    void setPenWidth(qreal width) override;
    void setGradientStops(const QGradientStops &stops) override;
    void setGradientVertical(bool vertical) override;
    void setRadius(qreal radius) override;
    void setAligned(bool aligned) override;
    void update() override;

    void render() override;
    void setOpacity(float opacity) override;
    void setTransform(const QOpenVGMatrix &transform) override;

private:
    void createVGResources();
    void destroyVGResources();

    void generateRectanglePath(const QRectF &rect, float radius, VGPath path) const;
    void generateRectangleAndBorderPaths(const QRectF &rect, float penWidth, float radius, VGPath inside, VGPath outside) const;
    void generateBorderPath(const QRectF &rect, float borderWidth, float borderHeight, float radius, VGPath path) const;

    bool m_pathDirty = true;
    bool m_fillDirty = true;
    bool m_strokeDirty = true;

    QRectF m_rect;
    QColor m_fillColor;
    QColor m_strokeColor;
    qreal m_penWidth = 0.0;
    qreal m_radius = 0.0;
    bool m_aligned = false;
    bool m_vertical = true;
    QGradientStops m_gradientStops;

    VGPath m_rectanglePath;
    VGPath m_borderPath;
    VGPaint m_rectanglePaint;
    VGPaint m_borderPaint;

    QOpenVGOffscreenSurface *m_offscreenSurface = nullptr;
};

QT_END_NAMESPACE

#endif // QSGOPENVGINTERNALRECTANGLENODE_H
