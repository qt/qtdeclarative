// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQuick/qsgrectanglenode.h>
#include <QtQuick/qsgimagenode.h>
#include <QtQuick/qsgninepatchnode.h>

#include "qsgopenvgrenderable.h"

QT_BEGIN_NAMESPACE

class QSGOpenVGRectangleNode : public QSGRectangleNode, public QSGOpenVGRenderable
{
public:
    QSGOpenVGRectangleNode();
    ~QSGOpenVGRectangleNode();

    void setRect(const QRectF &rect) override;
    QRectF rect() const override;

    void setColor(const QColor &color) override;
    QColor color() const override;

    void setTransform(const QOpenVGMatrix &transform) override;

    void render() override;

private:
    QRectF m_rect;
    QColor m_color;


    bool m_pathDirty = true;
    bool m_paintDirty = true;

    VGPath m_rectPath;
    VGPaint m_rectPaint;
};

class QSGOpenVGImageNode : public QSGImageNode, public QSGOpenVGRenderable
{
public:
    QSGOpenVGImageNode();
    ~QSGOpenVGImageNode();

    void setRect(const QRectF &rect) override;
    QRectF rect() const override;

    void setSourceRect(const QRectF &r) override;
    QRectF sourceRect() const override;

    void setTexture(QSGTexture *texture) override;
    QSGTexture *texture() const override;

    void setFiltering(QSGTexture::Filtering filtering) override;
    QSGTexture::Filtering filtering() const override;

    void setMipmapFiltering(QSGTexture::Filtering) override;
    QSGTexture::Filtering mipmapFiltering() const override;

    void setTextureCoordinatesTransform(TextureCoordinatesTransformMode transformNode) override;
    TextureCoordinatesTransformMode textureCoordinatesTransform() const override;

    void setAnisotropyLevel(QSGTexture::AnisotropyLevel level) override;
    QSGTexture::AnisotropyLevel anisotropyLevel() const override;

    void setOwnsTexture(bool owns) override;
    bool ownsTexture() const override;

    void render() override;

private:
    QSGTexture *m_texture;
    QRectF m_rect;
    QRectF m_sourceRect;
    bool m_owns;
    QSGTexture::Filtering m_filtering;
    TextureCoordinatesTransformMode m_transformMode;
};

class QSGOpenVGNinePatchNode : public QSGNinePatchNode, public QSGOpenVGRenderable
{
public:
    QSGOpenVGNinePatchNode();
    ~QSGOpenVGNinePatchNode();

    void setTexture(QSGTexture *texture) override;
    void setBounds(const QRectF &bounds) override;
    void setDevicePixelRatio(qreal ratio) override;
    void setPadding(qreal left, qreal top, qreal right, qreal bottom) override;
    void update() override;

    void render() override;

    QRectF bounds() const;

private:
    QSGTexture *m_texture;
    QRectF m_bounds;
    qreal m_pixelRatio;
    QMarginsF m_margins;
};

QT_END_NAMESPACE

#endif // QSGOPENVGPUBLICNODES_H
