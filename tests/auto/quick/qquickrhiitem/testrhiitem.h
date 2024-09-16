// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TESTRHIITEM_H
#define TESTRHIITEM_H

#include <QQuickRhiItem>
#include <rhi/qrhi.h>

class TestRenderer : public QQuickRhiItemRenderer
{
protected:
    void initialize(QRhiCommandBuffer *cb) override;
    void synchronize(QQuickRhiItem *item) override;
    void render(QRhiCommandBuffer *cb) override;

private:
    QRhi *m_rhi = nullptr;
    int m_sampleCount = 1;

    struct {
        QRhiResourceUpdateBatch *resourceUpdates = nullptr;
        std::unique_ptr<QRhiBuffer> vbuf;
        std::unique_ptr<QRhiBuffer> ubuf;
        std::unique_ptr<QRhiShaderResourceBindings> srb;
        std::unique_ptr<QRhiGraphicsPipeline> ps;
        std::unique_ptr<QRhiSampler> sampler;
        std::unique_ptr<QRhiTexture> tex;
        QMatrix4x4 mvp;
    } scene;

    struct {
        QColor color;
    } itemData;

    void initScene();
    void updateTexture();

    friend class tst_QQuickRhiItem;
};

class TestRhiItem : public QQuickRhiItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TestRhiItem)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    QQuickRhiItemRenderer *createRenderer() override { return new TestRenderer; }

    QColor color() const { return m_color; }
    void setColor(QColor c);

signals:
    void colorChanged();

private:
    QColor m_color;
};

#endif
