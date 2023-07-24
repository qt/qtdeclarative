// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RHITEXTUREITEM_H
#define RHITEXTUREITEM_H

#include <QQuickItem>
#include <QSGSimpleTextureNode>
#include <QSGTextureProvider>
#include <rhi/qrhi.h>

class LogoRenderer;
class RhiItem;
class RhiItemNode;

//! [rendererbase]
class RhiItemRenderer
{
public:
    virtual ~RhiItemRenderer() { }
    virtual void initialize(QRhi *rhi, QRhiTexture *outputTexture) = 0;
    virtual void synchronize(RhiItem *item) = 0;
    virtual void render(QRhiCommandBuffer *cb) = 0;
//! [rendererbase]
    void update();

private:
    void *data;
    friend class RhiItem;
};

//! [itembase]
class RhiItem : public QQuickItem
{
    Q_OBJECT
public:
    RhiItem(QQuickItem *parent = nullptr);

    virtual RhiItemRenderer *createRenderer() = 0;

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void releaseResources() override;
    bool isTextureProvider() const override;
    QSGTextureProvider *textureProvider() const override;

private Q_SLOTS:
    void invalidateSceneGraph();

private:
    mutable RhiItemNode *node = nullptr;
};
//! [itembase]

//! [itemnode]
class RhiItemNode : public QSGTextureProvider, public QSGSimpleTextureNode
//! [itemnode]
{
    Q_OBJECT

public:
    RhiItemNode(RhiItem *item);

    QSGTexture *texture() const override;

    void sync();
    bool isValid() const { return m_rhi && m_sgTexture; }
    void scheduleUpdate();
    bool hasRenderer() const { return m_renderer != nullptr; }
    void setRenderer(RhiItemRenderer *r) { m_renderer.reset(r); }

private slots:
    void render();

private:
    RhiItem *m_item;
    QQuickWindow *m_window;
    QSize m_pixelSize;
    qreal m_dpr = 0.0f;
    QRhi *m_rhi = nullptr;
    bool m_renderPending = true;
    std::unique_ptr<QSGTexture> m_sgTexture;
    std::unique_ptr<RhiItemRenderer> m_renderer;
};

//! [item]
class ExampleRhiItem : public RhiItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ExampleRhiItem)
    Q_PROPERTY(float angle READ angle WRITE setAngle NOTIFY angleChanged)

public:
    float angle() const { return m_angle; }
    void setAngle(float a);

    RhiItemRenderer *createRenderer() override;

signals:
    void angleChanged();

private:
    float m_angle = 0.0f;
};
//! [item]

class ExampleRhiItemRenderer : public RhiItemRenderer
{
public:
    void initialize(QRhi *rhi, QRhiTexture *outputTexture) override;
    void synchronize(RhiItem *item) override;
    void render(QRhiCommandBuffer *cb) override;

private:
    QRhi *m_rhi = nullptr;
    QRhiTexture *m_output = nullptr;
    std::unique_ptr<QRhiRenderBuffer> m_ds;
    std::unique_ptr<QRhiTextureRenderTarget> m_rt;
    std::unique_ptr<QRhiRenderPassDescriptor> m_rp;

    struct {
        QRhiResourceUpdateBatch *resourceUpdates = nullptr;
        std::unique_ptr<QRhiBuffer> vbuf;
        std::unique_ptr<QRhiBuffer> ubuf;
        std::unique_ptr<QRhiShaderResourceBindings> srb;
        std::unique_ptr<QRhiGraphicsPipeline> ps;
        float logoAngle = 0.0f;
    } scene;

    void createGeometry();
    void quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4);
    void extrude(qreal x1, qreal y1, qreal x2, qreal y2);
    QMatrix4x4 calculateModelViewMatrix() const;

    QList<QVector3D> m_vertices;
    QList<QVector3D> m_normals;
};

#endif
