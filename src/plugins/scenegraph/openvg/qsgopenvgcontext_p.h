// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGOPENVGCONTEXT_H
#define QSGOPENVGCONTEXT_H

#include <private/qsgcontext_p.h>
#include <qsgrendererinterface.h>

Q_DECLARE_LOGGING_CATEGORY(QSG_OPENVG_LOG_TIME_RENDERLOOP)

QT_BEGIN_NAMESPACE

class QOpenVGContext;
class QSGOpenVGFontGlyphCache;
class QSGOpenVGFontGlyphCacheManager;

class QSGOpenVGRenderContext : public QSGRenderContext, public QSGRendererInterface
{
    Q_OBJECT
public:
    QSGOpenVGRenderContext(QSGContext *context);

    static const int INIT_PARAMS_MAGIC = 0x51E;
    struct InitParams : public QSGRenderContext::InitParams {
        int sType = INIT_PARAMS_MAGIC;
        QOpenVGContext *context = nullptr;
    };

    void initialize(const QSGRenderContext::InitParams *params) override;
    void invalidate() override;
    void renderNextFrame(QSGRenderer *renderer) override;
    QSGTexture *createTexture(const QImage &image, uint flags) const override;
    QSGRenderer *createRenderer(QSGRendererInterface::RenderMode renderMode = QSGRendererInterface::RenderMode2D) override;
    int maxTextureSize() const override;

    // QSGRendererInterface interface
    GraphicsApi graphicsApi() const override;
    ShaderType shaderType() const override;
    ShaderCompilationTypes shaderCompilationType() const override;
    ShaderSourceTypes shaderSourceType() const override;

    QOpenVGContext* vgContext() { return m_vgContext; }
    QSGOpenVGFontGlyphCache* glyphCache(const QRawFont &rawFont);

private:
    QOpenVGContext *m_vgContext;
    QSGOpenVGFontGlyphCacheManager *m_glyphCacheManager;

};

class QSGOpenVGContext : public QSGContext
{
    Q_OBJECT
public:
    QSGOpenVGContext(QObject *parent = nullptr);

    QSGRenderContext *createRenderContext() override;
    QSGRectangleNode *createRectangleNode() override;
    QSGImageNode *createImageNode() override;
    QSGPainterNode *createPainterNode(QQuickPaintedItem *item) override;
    QSGGlyphNode *createGlyphNode(QSGRenderContext *rc, bool preferNativeGlyphNode, int renderTypeQuality) override;
    QSGNinePatchNode *createNinePatchNode() override;
    QSGLayer *createLayer(QSGRenderContext *renderContext) override;
    QSurfaceFormat defaultSurfaceFormat() const override;
    QSGInternalRectangleNode *createInternalRectangleNode() override;
    QSGInternalImageNode *createInternalImageNode(QSGRenderContext *renderContext) override;
#if QT_CONFIG(quick_sprite)
    QSGSpriteNode *createSpriteNode() override;
#endif
    QSGRendererInterface *rendererInterface(QSGRenderContext *renderContext) override;
};

QT_END_NAMESPACE

#endif // QSGOPENVGCONTEXT_H
