/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qsgd3d12texture_p.h"
#include "qsgd3d12engine_p.h"
#include <private/qsgcontext_p.h>

QT_BEGIN_NAMESPACE

void QSGD3D12Texture::setImage(const QImage &image, uint flags)
{
    // ### mipmap, atlas

    const bool alphaRequest = flags & QSGRenderContext::CreateTexture_Alpha;
    m_alphaWanted = alphaRequest && image.hasAlphaChannel();

    m_size = image.size();

    QSGD3D12Engine::TextureCreateFlags createFlags = 0;
    if (m_alphaWanted)
        createFlags |= QSGD3D12Engine::CreateWithAlpha;

    m_id = m_engine->createTexture(image.format(), image.size(), createFlags);
    if (!m_id) {
        qWarning("Failed to allocate texture of size %dx%d", image.width(), image.height());
        return;
    }

    QSGD3D12Engine::TextureUploadFlags uploadFlags = 0;
    m_engine->queueTextureUpload(m_id, image, uploadFlags);
}

QSGD3D12Texture::~QSGD3D12Texture()
{
    if (m_id)
        m_engine->releaseTexture(m_id);
}

int QSGD3D12Texture::textureId() const
{
    return m_id;
}

QSize QSGD3D12Texture::textureSize() const
{
    return m_size;
}

bool QSGD3D12Texture::hasAlphaChannel() const
{
    return m_alphaWanted;
}

bool QSGD3D12Texture::hasMipmaps() const
{
    return false; // ###
}

QRectF QSGD3D12Texture::normalizedTextureSubRect() const
{
    return QRectF(0, 0, 1, 1);
}

bool QSGD3D12Texture::isAtlasTexture() const
{
    return false; // ###
}

QSGTexture *QSGD3D12Texture::removedFromAtlas() const
{
    return nullptr; // ###
}

void QSGD3D12Texture::bind()
{
    // Called when the texture material updates the pipeline state. Here we
    // know that the texture is going to be used in the current frame. Notify
    // the engine so that it can wait for possible pending uploads in endFrame().
    m_engine->addFrameTextureDep(m_id);
}

SIZE_T QSGD3D12Texture::srv() const
{
    Q_ASSERT(m_id);
    return m_engine->textureSRV(m_id);
}

QT_END_NAMESPACE
