/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick 2D Renderer module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CONTEXT_H
#define CONTEXT_H

#include <private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>

Q_DECLARE_LOGGING_CATEGORY(QSG_RASTER_LOG_TIME_RENDERLOOP)
Q_DECLARE_LOGGING_CATEGORY(QSG_RASTER_LOG_TIME_COMPILATION)
Q_DECLARE_LOGGING_CATEGORY(QSG_RASTER_LOG_TIME_TEXTURE)
Q_DECLARE_LOGGING_CATEGORY(QSG_RASTER_LOG_TIME_GLYPH)
Q_DECLARE_LOGGING_CATEGORY(QSG_RASTER_LOG_TIME_RENDERER)
Q_DECLARE_LOGGING_CATEGORY(QSG_RASTER_LOG_INFO)
Q_DECLARE_LOGGING_CATEGORY(QSG_RASTER_LOG_RENDERLOOP)

namespace SoftwareContext
{

class RenderContext : public QSGRenderContext
{
public:
    RenderContext(QSGContext *ctx);
    void initialize(QOpenGLContext *context) override;
    void initializeIfNeeded();
    void invalidate() override;
    void renderNextFrame(QSGRenderer *renderer, GLuint fbo) override;
    QSGTexture *createTexture(const QImage &image, uint flags = CreateTexture_Alpha) const override;
    QSGRenderer *createRenderer() override;

    QWindow *currentWindow;
    bool m_initialized;
};

class Context : public QSGContext
{
    Q_OBJECT
public:
    explicit Context(QObject *parent = nullptr);

    QSGRenderContext *createRenderContext() override { return new RenderContext(this); }
    QSGRectangleNode *createRectangleNode() override;
    QSGImageNode *createImageNode() override;
    QSGPainterNode *createPainterNode(QQuickPaintedItem *item) override;
    QSGGlyphNode *createGlyphNode(QSGRenderContext *rc, bool preferNativeGlyphNode) override;
    QSGNinePatchNode *createNinePatchNode() override;
    QSGLayer *createLayer(QSGRenderContext *renderContext) override;
    QSurfaceFormat defaultSurfaceFormat() const override;
};

} // namespace

#endif // CONTEXT_H
