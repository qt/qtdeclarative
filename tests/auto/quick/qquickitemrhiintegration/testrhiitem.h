// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TESTRHIITEM_H
#define TESTRHIITEM_H

#include "rhiitem.h"

class TestRenderer : public RhiItemRenderer
{
public:
    void initialize(QRhi *rhi, QRhiTexture *outputTexture) override;
    void synchronize(RhiItem *item) override;
    void render(QRhiCommandBuffer *cb) override;

private:
    QRhi *m_rhi = nullptr;
    QRhiTexture *m_output = nullptr;
    QScopedPointer<QRhiRenderBuffer> m_ds;
    QScopedPointer<QRhiTextureRenderTarget> m_rt;
    QScopedPointer<QRhiRenderPassDescriptor> m_rp;

    struct {
        QRhiResourceUpdateBatch *resourceUpdates = nullptr;
        QScopedPointer<QRhiBuffer> vbuf;
        QScopedPointer<QRhiBuffer> ubuf;
        QScopedPointer<QRhiShaderResourceBindings> srb;
        QScopedPointer<QRhiGraphicsPipeline> ps;
        QScopedPointer<QRhiSampler> sampler;
        QScopedPointer<QRhiTexture> tex;
        QMatrix4x4 mvp;
    } scene;

    struct {
        QColor color;
    } itemData;

    void initScene();
    void updateTexture();
};

class TestRhiItem : public RhiItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TestRhiItem)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    RhiItemRenderer *createRenderer() override { return new TestRenderer; }

    QColor color() const { return m_color; }
    void setColor(QColor c);

signals:
    void colorChanged();

private:
    QColor m_color;
};

#endif
