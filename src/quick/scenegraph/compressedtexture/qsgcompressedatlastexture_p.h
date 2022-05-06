/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QSGCOMPRESSEDATLASTEXTURE_P_H
#define QSGCOMPRESSEDATLASTEXTURE_P_H

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

#include <QtCore/QSize>

#include <QtQuick/QSGTexture>
#include <QtQuick/private/qsgareaallocator_p.h>
#include <QtQuick/private/qsgrhiatlastexture_p.h>

QT_BEGIN_NAMESPACE

class QSGCompressedTextureFactory;

namespace QSGCompressedAtlasTexture {

class Texture;

class Atlas : public QSGRhiAtlasTexture::AtlasBase
{
public:
    Atlas(QSGDefaultRenderContext *rc, const QSize &size, uint format);
    ~Atlas();

    bool generateTexture() override;
    void enqueueTextureUpload(QSGRhiAtlasTexture::TextureBase *t,
                              QRhiResourceUpdateBatch *rcub) override;

    Texture *create(QByteArrayView data, const QSize &size);

    uint format() const { return m_format; }

private:
    uint m_format;
};

class Texture : public QSGRhiAtlasTexture::TextureBase
{
    Q_OBJECT
public:
    Texture(Atlas *atlas, const QRect &textureRect, QByteArrayView data, const QSize &size);
    ~Texture();

    QSize textureSize() const override { return m_size; }
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override { return false; }

    QRectF normalizedTextureSubRect() const override { return m_texture_coords_rect; }

    QSGTexture *removedFromAtlas(QRhiResourceUpdateBatch *) const override;

    const QByteArray &data() const { return m_data; }
    int sizeInBytes() const { return m_data.length(); }

private:
    QRectF m_texture_coords_rect;
    mutable QSGTexture *m_nonatlas_texture;
    QByteArray m_data;
    QSize m_size;
};

}

QT_END_NAMESPACE

#endif
