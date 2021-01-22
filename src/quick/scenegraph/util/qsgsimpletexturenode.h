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

#ifndef QSGSIMPLETEXTURENODE_H
#define QSGSIMPLETEXTURENODE_H

#include <QtQuick/qsgnode.h>
#include <QtQuick/qsggeometry.h>
#include <QtQuick/qsgtexturematerial.h>

QT_BEGIN_NAMESPACE

class QSGSimpleTextureNodePrivate;

class Q_QUICK_EXPORT QSGSimpleTextureNode : public QSGGeometryNode
{
public:
    QSGSimpleTextureNode();
    ~QSGSimpleTextureNode() override;

    void setRect(const QRectF &rect);
    inline void setRect(qreal x, qreal y, qreal w, qreal h) { setRect(QRectF(x, y, w, h)); }
    QRectF rect() const;

    void setSourceRect(const QRectF &r);
    inline void setSourceRect(qreal x, qreal y, qreal w, qreal h) { setSourceRect(QRectF(x, y, w, h)); }
    QRectF sourceRect() const;

    void setTexture(QSGTexture *texture);
    QSGTexture *texture() const;

    void setFiltering(QSGTexture::Filtering filtering);
    QSGTexture::Filtering filtering() const;

    enum TextureCoordinatesTransformFlag {
        NoTransform        = 0x00,
        MirrorHorizontally = 0x01,
        MirrorVertically   = 0x02
    };
    Q_DECLARE_FLAGS(TextureCoordinatesTransformMode, TextureCoordinatesTransformFlag)

    void setTextureCoordinatesTransform(TextureCoordinatesTransformMode mode);
    TextureCoordinatesTransformMode textureCoordinatesTransform() const;

    void setOwnsTexture(bool owns);
    bool ownsTexture() const;

private:
    QSGGeometry m_geometry;
    QSGOpaqueTextureMaterial m_opaque_material;
    QSGTextureMaterial m_material;

    QRectF m_rect;

    Q_DECLARE_PRIVATE(QSGSimpleTextureNode)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGSimpleTextureNode::TextureCoordinatesTransformMode)

QT_END_NAMESPACE

#endif // QSGSIMPLETEXTURENODE_H
