/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSGD3D12PUBLICNODES_P_H
#define QSGD3D12PUBLICNODES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick/qsgrectanglenode.h>
#include <QtQuick/qsgimagenode.h>
#include <QtQuick/qsgninepatchnode.h>
#include "qsgd3d12builtinmaterials_p.h"

QT_BEGIN_NAMESPACE

class QSGD3D12RectangleNode : public QSGRectangleNode
{
public:
    QSGD3D12RectangleNode();

    void setRect(const QRectF &rect) override;
    QRectF rect() const override;

    void setColor(const QColor &color) override;
    QColor color() const override;

private:
    QSGGeometry m_geometry;
    QSGD3D12FlatColorMaterial m_material;
};

class QSGD3D12ImageNode : public QSGImageNode
{
public:
    QSGD3D12ImageNode();
    ~QSGD3D12ImageNode();

    void setRect(const QRectF &rect) override;
    QRectF rect() const override;

    void setSourceRect(const QRectF &r) override;
    QRectF sourceRect() const override;

    void setTexture(QSGTexture *texture) override;
    QSGTexture *texture() const override;

    void setFiltering(QSGTexture::Filtering filtering) override;
    QSGTexture::Filtering filtering() const override;

    void setMipmapFiltering(QSGTexture::Filtering filtering) override;
    QSGTexture::Filtering mipmapFiltering() const override;

    void setTextureCoordinatesTransform(TextureCoordinatesTransformMode mode) override;
    TextureCoordinatesTransformMode textureCoordinatesTransform() const override;

    void setOwnsTexture(bool owns) override;
    bool ownsTexture() const override;

private:
    QSGGeometry m_geometry;
    QSGD3D12TextureMaterial m_material;
    QRectF m_rect;
    QRectF m_sourceRect;
    TextureCoordinatesTransformMode m_texCoordMode;
    uint m_isAtlasTexture : 1;
    uint m_ownsTexture : 1;
};

class QSGD3D12NinePatchNode : public QSGNinePatchNode
{
public:
    QSGD3D12NinePatchNode();
    ~QSGD3D12NinePatchNode();

    void setTexture(QSGTexture *texture) override;
    void setBounds(const QRectF &bounds) override;
    void setDevicePixelRatio(qreal ratio) override;
    void setPadding(qreal left, qreal top, qreal right, qreal bottom) override;
    void update() override;

private:
    QSGGeometry m_geometry;
    QSGD3D12TextureMaterial m_material;
    QRectF m_bounds;
    qreal m_devicePixelRatio;
    QVector4D m_padding;
};

QT_END_NAMESPACE

#endif
