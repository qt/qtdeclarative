/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgcompressedatlastexture_p.h"

#include <QtCore/QVarLengthArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QtMath>

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/QSurface>
#include <QtGui/QWindow>
#include <QDebug>

#include <private/qqmlglobal_p.h>
#include <private/qquickprofiler_p.h>
#include <private/qsgtexture_p.h>
#include <private/qsgcompressedtexture_p.h>

QT_BEGIN_NAMESPACE

namespace QSGCompressedAtlasTexture
{

Atlas::Atlas(QSGDefaultRenderContext *rc, const QSize &size, uint format)
    : QSGRhiAtlasTexture::AtlasBase(rc, size), m_format(format)
{
}

Atlas::~Atlas()
{
}

Texture *Atlas::create(QByteArrayView data, const QSize &size)
{
    // Align reservation to 16x16, >= any compressed block size
    QSize paddedSize(((size.width() + 15) / 16) * 16, ((size.height() + 15) / 16) * 16);
    // No need to lock, as manager already locked it.
    QRect rect = m_allocator.allocate(paddedSize);
    if (rect.width() > 0 && rect.height() > 0) {
        Texture *t = new Texture(this, rect, data, size);
        m_pending_uploads << t;
        return t;
    }
    return nullptr;
}

bool Atlas::generateTexture()
{
    QSGCompressedTexture::FormatInfo fmt = QSGCompressedTexture::formatInfo(m_format);
    QRhiTexture::Flags flags(QRhiTexture::UsedAsTransferSource | QRhiTexture::UsedAsCompressedAtlas);
    flags.setFlag(QRhiTexture::sRGB, fmt.isSRGB);
    m_texture = m_rhi->newTexture(fmt.rhiFormat, m_size, 1, flags);
    if (!m_texture)
        return false;

    if (!m_texture->create()) {
        delete m_texture;
        m_texture = nullptr;
        return false;
    }

    qCDebug(QSG_LOG_TEXTUREIO, "Created compressed atlas of size %dx%d for format 0x%x (rhi: %d)",
            m_size.width(), m_size.height(), m_format, fmt.rhiFormat);

    return true;
}

void Atlas::enqueueTextureUpload(QSGRhiAtlasTexture::TextureBase *t, QRhiResourceUpdateBatch *rcub)
{
    Texture *texture = static_cast<Texture *>(t);

    const QRect &r = texture->atlasSubRect();

    QRhiTextureSubresourceUploadDescription subresDesc(texture->data().constData(),
                                                       texture->sizeInBytes());
    subresDesc.setSourceSize(texture->textureSize());
    subresDesc.setDestinationTopLeft(r.topLeft());

    QRhiTextureUploadDescription desc(QRhiTextureUploadEntry(0, 0, subresDesc));
    rcub->uploadTexture(m_texture, desc);

    qCDebug(QSG_LOG_TEXTUREIO, "compressed atlastexture upload, size %dx%d format 0x%x",
            t->textureSize().width(), t->textureSize().height(), m_format);
}

Texture::Texture(Atlas *atlas, const QRect &textureRect, QByteArrayView data, const QSize &size)
    : QSGRhiAtlasTexture::TextureBase(atlas, textureRect),
      m_nonatlas_texture(nullptr),
      m_data(data.toByteArray()),
      m_size(size)
{
    float w = atlas->size().width();
    float h = atlas->size().height();
    const QRect &r = atlasSubRect();
    // offset by half-pixel to prevent bleeding when scaling
    m_texture_coords_rect = QRectF((r.x() + .5) / w,
                                   (r.y() + .5) / h,
                                   (size.width() - 1.) / w,
                                   (size.height() - 1.) / h);
}

Texture::~Texture()
{
    delete m_nonatlas_texture;
}

bool Texture::hasAlphaChannel() const
{
    return !QSGCompressedTexture::formatIsOpaque(static_cast<Atlas*>(m_atlas)->format());
}

QSGTexture *Texture::removedFromAtlas(QRhiResourceUpdateBatch *) const
{
    if (m_nonatlas_texture) {
        m_nonatlas_texture->setMipmapFiltering(mipmapFiltering());
        m_nonatlas_texture->setFiltering(filtering());
        return m_nonatlas_texture;
    }

    if (!m_data.isEmpty()) {
        QTextureFileData texData;
        texData.setData(m_data);
        texData.setSize(m_size);
        texData.setGLInternalFormat(static_cast<Atlas*>(m_atlas)->format());
        texData.setDataLength(m_data.size());
        texData.setDataOffset(0);
        m_nonatlas_texture = new QSGCompressedTexture(texData);
        m_nonatlas_texture->setMipmapFiltering(mipmapFiltering());
        m_nonatlas_texture->setFiltering(filtering());
    }

    return m_nonatlas_texture;
}

}

QT_END_NAMESPACE
