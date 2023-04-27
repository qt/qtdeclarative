// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef RHIITEM_H
#define RHIITEM_H

#include <QQuickItem>
#include <rhi/qrhi.h>

class RhiItem;
class RhiItemPrivate;

class RhiItemRenderer
{
public:
    virtual ~RhiItemRenderer();
    virtual void initialize(QRhi *rhi, QRhiTexture *outputTexture);
    virtual void synchronize(RhiItem *item);
    virtual void render(QRhiCommandBuffer *cb);

    void update();

private:
    void *data;
    friend class RhiItem;
};

class RhiItem : public QQuickItem
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(RhiItem)

    Q_PROPERTY(int explicitTextureWidth READ explicitTextureWidth WRITE setExplicitTextureWidth NOTIFY explicitTextureWidthChanged)
    Q_PROPERTY(int explicitTextureHeight READ explicitTextureHeight WRITE setExplicitTextureHeight NOTIFY explicitTextureHeightChanged)
    Q_PROPERTY(QSize effectiveTextureSize READ effectiveTextureSize NOTIFY effectiveTextureSizeChanged)
    Q_PROPERTY(bool alphaBlending READ alphaBlending WRITE setAlphaBlending NOTIFY alphaBlendingChanged)
    Q_PROPERTY(bool mirrorVertically READ mirrorVertically WRITE setMirrorVertically NOTIFY mirrorVerticallyChanged)

public:
    RhiItem(QQuickItem *parent = nullptr);

    virtual RhiItemRenderer *createRenderer() = 0;

    int explicitTextureWidth() const;
    void setExplicitTextureWidth(int w);
    int explicitTextureHeight() const;
    void setExplicitTextureHeight(int h);

    QSize effectiveTextureSize() const;

    bool alphaBlending() const;
    void setAlphaBlending(bool enable);

    bool mirrorVertically() const;
    void setMirrorVertically(bool enable);

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void releaseResources() override;
    bool isTextureProvider() const override;
    QSGTextureProvider *textureProvider() const override;

Q_SIGNALS:
    void explicitTextureWidthChanged();
    void explicitTextureHeightChanged();
    void effectiveTextureSizeChanged();
    void alphaBlendingChanged();
    void mirrorVerticallyChanged();

private Q_SLOTS:
    void invalidateSceneGraph();
};

#endif
