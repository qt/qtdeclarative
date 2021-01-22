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

#ifndef QSGTEXTUREMATERIAL_H
#define QSGTEXTUREMATERIAL_H

#include <QtQuick/qsgmaterial.h>
#include <QtQuick/qsgtexture.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGOpaqueTextureMaterial : public QSGMaterial
{
public:
    QSGOpaqueTextureMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader() const override;
    int compare(const QSGMaterial *other) const override;

    void setTexture(QSGTexture *texture);
    QSGTexture *texture() const { return m_texture; }

    void setMipmapFiltering(QSGTexture::Filtering filteringType) { m_mipmap_filtering = filteringType; }
    QSGTexture::Filtering mipmapFiltering() const { return QSGTexture::Filtering(m_mipmap_filtering); }

    void setFiltering(QSGTexture::Filtering filteringType) { m_filtering = filteringType; }
    QSGTexture::Filtering filtering() const { return QSGTexture::Filtering(m_filtering); }

    void setHorizontalWrapMode(QSGTexture::WrapMode mode) { m_horizontal_wrap = mode; }
    QSGTexture::WrapMode horizontalWrapMode() const { return QSGTexture::WrapMode(m_horizontal_wrap); }

    void setVerticalWrapMode(QSGTexture::WrapMode mode) { m_vertical_wrap = mode; }
    QSGTexture::WrapMode verticalWrapMode() const { return QSGTexture::WrapMode(m_vertical_wrap); }

    void setAnisotropyLevel(QSGTexture::AnisotropyLevel level) { m_anisotropy_level = level; }
    QSGTexture::AnisotropyLevel anisotropyLevel() const { return QSGTexture::AnisotropyLevel(m_anisotropy_level); }

protected:
    QSGTexture *m_texture;

    uint m_filtering: 2;
    uint m_mipmap_filtering: 2;
    uint m_horizontal_wrap : 1;
    uint m_vertical_wrap: 1;
    uint m_anisotropy_level : 3;
    uint m_reserved : 23;
};


class Q_QUICK_EXPORT QSGTextureMaterial : public QSGOpaqueTextureMaterial
{
public:
    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader() const override;
};

QT_END_NAMESPACE

#endif
