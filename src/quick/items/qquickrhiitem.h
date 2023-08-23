// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKRHIITEM_H
#define QQUICKRHIITEM_H

#include <QtQuick/QQuickItem>

QT_BEGIN_NAMESPACE

class QQuickRhiItem;
class QQuickRhiItemPrivate;
class QRhi;
class QRhiCommandBuffer;
class QRhiTexture;
class QRhiRenderBuffer;
class QRhiRenderTarget;

class Q_QUICK_EXPORT QQuickRhiItemRenderer : public QObject
{
public:
    QQuickRhiItemRenderer();
    virtual ~QQuickRhiItemRenderer();

    virtual void initialize(QRhiCommandBuffer *cb) = 0;
    virtual void synchronize(QQuickRhiItem *item) = 0;
    virtual void render(QRhiCommandBuffer *cb) = 0;

    void update();

    QRhi *rhi() const;
    QRhiTexture *colorTexture() const;
    QRhiRenderBuffer *msaaColorBuffer() const;
    QRhiTexture *resolveTexture() const;
    QRhiRenderBuffer *depthStencilBuffer() const;
    QRhiRenderTarget *renderTarget() const;

private:
    void *data;
    friend class QQuickRhiItem;
};

class Q_QUICK_EXPORT QQuickRhiItem : public QQuickItem
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickRhiItem)

    Q_PROPERTY(int sampleCount READ sampleCount WRITE setSampleCount NOTIFY sampleCountChanged FINAL)
    Q_PROPERTY(TextureFormat textureFormat READ textureFormat WRITE setTextureFormat NOTIFY textureFormatChanged FINAL)
    Q_PROPERTY(bool autoRenderTarget READ isAutoRenderTargetEnabled WRITE setAutoRenderTarget NOTIFY autoRenderTargetChanged FINAL)
    Q_PROPERTY(bool mirrorVertically READ isMirrorVerticallyEnabled WRITE setMirrorVertically NOTIFY mirrorVerticallyChanged FINAL)
    Q_PROPERTY(bool alphaBlending READ alphaBlending WRITE setAlphaBlending NOTIFY alphaBlendingChanged)
    Q_PROPERTY(int explicitTextureWidth READ explicitTextureWidth WRITE setExplicitTextureWidth NOTIFY explicitTextureWidthChanged)
    Q_PROPERTY(int explicitTextureHeight READ explicitTextureHeight WRITE setExplicitTextureHeight NOTIFY explicitTextureHeightChanged)
    Q_PROPERTY(QSize effectiveTextureSize READ effectiveTextureSize NOTIFY effectiveTextureSizeChanged)

public:
    enum class TextureFormat {
        RGBA8,
        RGBA16F,
        RGBA32F,
        RGB10A2
    };
    Q_ENUM(TextureFormat)

    QQuickRhiItem(QQuickItem *parent = nullptr);

    int sampleCount() const;
    void setSampleCount(int samples);

    TextureFormat textureFormat() const;
    void setTextureFormat(TextureFormat format);

    bool isAutoRenderTargetEnabled() const;
    void setAutoRenderTarget(bool enabled);

    bool isMirrorVerticallyEnabled() const;
    void setMirrorVertically(bool enable);

    bool alphaBlending() const;
    void setAlphaBlending(bool enable);

    int explicitTextureWidth() const;
    void setExplicitTextureWidth(int width);
    int explicitTextureHeight() const;
    void setExplicitTextureHeight(int height);

    QSize effectiveTextureSize() const;

    virtual QQuickRhiItemRenderer *createRenderer() = 0;

Q_SIGNALS:
    void sampleCountChanged();
    void textureFormatChanged();
    void autoRenderTargetChanged();
    void mirrorVerticallyChanged();
    void alphaBlendingChanged();
    void explicitTextureWidthChanged();
    void explicitTextureHeightChanged();
    void effectiveTextureSizeChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void releaseResources() override;
    bool isTextureProvider() const override;
    QSGTextureProvider *textureProvider() const override;

private Q_SLOTS:
    void invalidateSceneGraph();

    friend class QQuickRhiItemNode;
};

QT_END_NAMESPACE

#endif // QQUICKRHIITEM_H
