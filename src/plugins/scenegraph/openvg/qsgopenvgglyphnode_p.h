// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGOPENVGGLYPHNODE_H
#define QSGOPENVGGLYPHNODE_H

#include <private/qsgadaptationlayer_p.h>
#include <QtCore/QVector>

#include <VG/openvg.h>

#include "qsgopenvgrenderable.h"
#include "qsgopenvgfontglyphcache.h"

QT_BEGIN_NAMESPACE

class QSGOpenVGFontGlyphCache;
class QSGOpenVGRenderContext;
class QSGRenderContext;

class QSGOpenVGGlyphNode : public QSGGlyphNode, public QSGOpenVGRenderable
{
public:
    QSGOpenVGGlyphNode(QSGRenderContext *rc);
    ~QSGOpenVGGlyphNode();

    void setGlyphs(const QPointF &position, const QGlyphRun &glyphs) override;
    void setColor(const QColor &color) override;
    void setStyle(QQuickText::TextStyle style) override;
    void setStyleColor(const QColor &color) override;
    QPointF baseLine() const override;
    void setPreferredAntialiasingMode(AntialiasingMode) override;
    void update() override;

    void render() override;
    void setOpacity(float opacity) override;

private:
    void drawGlyphsAtOffset(const QPointF &offset);

    QPointF m_position;
    QGlyphRun m_glyphRun;
    QColor m_color;
    QSGGeometry m_geometry;
    QQuickText::TextStyle m_style;
    QColor m_styleColor;

    QSGOpenVGFontGlyphCache *m_glyphCache;
    QVector<VGfloat> m_xAdjustments;
    QVector<VGfloat> m_yAdjustments;
    VGPaint m_fontColorPaint;
    VGPaint m_styleColorPaint;

    QSGOpenVGRenderContext *m_renderContext;
};

QT_END_NAMESPACE

#endif // QSGOPENVGGLYPHNODE_H
