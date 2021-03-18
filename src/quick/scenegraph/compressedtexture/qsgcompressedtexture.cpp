/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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


#include "qsgcompressedtexture_p.h"
#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QDebug>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtGui/private/qrhi_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QSG_LOG_TEXTUREIO, "qt.scenegraph.textureio");

QSGCompressedTexture::QSGCompressedTexture(const QTextureFileData &texData)
    : QSGTexture(*(new QSGCompressedTexturePrivate)),
      m_textureData(texData)
{
    m_size = m_textureData.size();
    m_hasAlpha = !formatIsOpaque(m_textureData.glInternalFormat());
}

QSGCompressedTexture::~QSGCompressedTexture()
{
#if QT_CONFIG(opengl)
    if (m_textureId) {
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        QOpenGLFunctions *funcs = ctx ? ctx->functions() : nullptr;
        if (!funcs)
            return;

        funcs->glDeleteTextures(1, &m_textureId);
    }
#endif

    delete m_texture;
}

int QSGCompressedTexture::textureId() const
{
#if QT_CONFIG(opengl)
    if (!m_textureId) {
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        QOpenGLFunctions *funcs = ctx ? ctx->functions() : nullptr;
        if (!funcs)
            return 0;

        funcs->glGenTextures(1, &m_textureId);
    }
#endif
    return m_textureId;
}

int QSGCompressedTexturePrivate::comparisonKey() const
{
    Q_Q(const QSGCompressedTexture);
    // not textureId() as that would create an id when not yet done - that's not wanted here
    if (q->m_textureId)
        return q->m_textureId;

    if (q->m_texture)
        return int(qintptr(q->m_texture));

    // two textures (and so materials) with not-yet-created texture underneath are never equal
    return int(qintptr(q));
}

QSize QSGCompressedTexture::textureSize() const
{
    return m_size;
}

bool QSGCompressedTexture::hasAlphaChannel() const
{
    return m_hasAlpha;
}

bool QSGCompressedTexture::hasMipmaps() const
{
    return false;
}

void QSGCompressedTexture::bind()
{
#if QT_CONFIG(opengl)
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    QOpenGLFunctions *funcs = ctx ? ctx->functions() : nullptr;
    if (!funcs)
        return;

    if (!textureId())
        return;

    funcs->glBindTexture(GL_TEXTURE_2D, m_textureId);

    if (m_uploaded)
        return;

    if (!m_textureData.isValid()) {
        qCDebug(QSG_LOG_TEXTUREIO, "Invalid texture data for %s", m_textureData.logName().constData());
        funcs->glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    if (Q_UNLIKELY(QSG_LOG_TEXTUREIO().isDebugEnabled())) {
        qCDebug(QSG_LOG_TEXTUREIO) << "Uploading texture" << m_textureData;
        while (funcs->glGetError() != GL_NO_ERROR);
    }

    funcs->glCompressedTexImage2D(GL_TEXTURE_2D, 0, m_textureData.glInternalFormat(),
                                  m_size.width(), m_size.height(), 0, m_textureData.dataLength(),
                                  m_textureData.data().constData() + m_textureData.dataOffset());

    if (Q_UNLIKELY(QSG_LOG_TEXTUREIO().isDebugEnabled())) {
        GLuint error = funcs->glGetError();
        if (error != GL_NO_ERROR) {
            qCDebug(QSG_LOG_TEXTUREIO, "glCompressedTexImage2D failed for %s, error 0x%x", m_textureData.logName().constData(), error);
        }
    }

    m_textureData = QTextureFileData();  // Release this memory, not needed anymore

    updateBindOptions(true);
    m_uploaded = true;
#endif // QT_CONFIG(opengl)
}

static QPair<QRhiTexture::Format, bool> toRhiCompressedFormat(uint glinternalformat)
{
    switch (glinternalformat) {
    case QOpenGLTexture::RGB_DXT1:
        return { QRhiTexture::BC1, false };
    case QOpenGLTexture::SRGB_DXT1:
        return { QRhiTexture::BC1, true };

    case QOpenGLTexture::RGBA_DXT3:
        return { QRhiTexture::BC3, false };
    case QOpenGLTexture::SRGB_Alpha_DXT3:
        return { QRhiTexture::BC3, true };

    case QOpenGLTexture::RGBA_DXT5:
        return { QRhiTexture::BC5, false };
    case QOpenGLTexture::SRGB_Alpha_DXT5:
        return { QRhiTexture::BC5, true };

    case QOpenGLTexture::RGB8_ETC2:
        return { QRhiTexture::ETC2_RGB8, false };
    case QOpenGLTexture::SRGB8_ETC2:
        return { QRhiTexture::ETC2_RGB8, true };

    case QOpenGLTexture::RGB8_PunchThrough_Alpha1_ETC2:
        return { QRhiTexture::ETC2_RGB8A1, false };
    case QOpenGLTexture::SRGB8_PunchThrough_Alpha1_ETC2:
        return { QRhiTexture::ETC2_RGB8A1, true };

    case QOpenGLTexture::RGBA8_ETC2_EAC:
        return { QRhiTexture::ETC2_RGBA8, false };
    case QOpenGLTexture::SRGB8_Alpha8_ETC2_EAC:
        return { QRhiTexture::ETC2_RGBA8, true };

    case QOpenGLTexture::RGBA_ASTC_4x4:
        return { QRhiTexture::ASTC_4x4, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_4x4:
        return { QRhiTexture::ASTC_4x4, true };

    case QOpenGLTexture::RGBA_ASTC_5x4:
        return { QRhiTexture::ASTC_5x4, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_5x4:
        return { QRhiTexture::ASTC_5x4, true };

    case QOpenGLTexture::RGBA_ASTC_5x5:
        return { QRhiTexture::ASTC_5x5, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_5x5:
        return { QRhiTexture::ASTC_5x5, true };

    case QOpenGLTexture::RGBA_ASTC_6x5:
        return { QRhiTexture::ASTC_6x5, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_6x5:
        return { QRhiTexture::ASTC_6x5, true };

    case QOpenGLTexture::RGBA_ASTC_6x6:
        return { QRhiTexture::ASTC_6x6, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_6x6:
        return { QRhiTexture::ASTC_6x6, true };

    case QOpenGLTexture::RGBA_ASTC_8x5:
        return { QRhiTexture::ASTC_8x5, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_8x5:
        return { QRhiTexture::ASTC_8x5, true };

    case QOpenGLTexture::RGBA_ASTC_8x6:
        return { QRhiTexture::ASTC_8x6, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_8x6:
        return { QRhiTexture::ASTC_8x6, true };

    case QOpenGLTexture::RGBA_ASTC_8x8:
        return { QRhiTexture::ASTC_8x8, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_8x8:
        return { QRhiTexture::ASTC_8x8, true };

    case QOpenGLTexture::RGBA_ASTC_10x5:
        return { QRhiTexture::ASTC_10x5, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_10x5:
        return { QRhiTexture::ASTC_10x5, true };

    case QOpenGLTexture::RGBA_ASTC_10x6:
        return { QRhiTexture::ASTC_10x6, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_10x6:
        return { QRhiTexture::ASTC_10x6, true };

    case QOpenGLTexture::RGBA_ASTC_10x8:
        return { QRhiTexture::ASTC_10x8, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_10x8:
        return { QRhiTexture::ASTC_10x8, true };

    case QOpenGLTexture::RGBA_ASTC_10x10:
        return { QRhiTexture::ASTC_10x10, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_10x10:
        return { QRhiTexture::ASTC_10x10, true };

    case QOpenGLTexture::RGBA_ASTC_12x10:
        return { QRhiTexture::ASTC_12x10, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_12x10:
        return { QRhiTexture::ASTC_12x10, true };

    case QOpenGLTexture::RGBA_ASTC_12x12:
        return { QRhiTexture::ASTC_12x12, false };
    case QOpenGLTexture::SRGB8_Alpha8_ASTC_12x12:
        return { QRhiTexture::ASTC_12x12, true };

    default:
        return { QRhiTexture::UnknownFormat, false };
    }
}

QRhiTexture *QSGCompressedTexturePrivate::rhiTexture() const
{
    Q_Q(const QSGCompressedTexture);
    return q->m_texture;
}

void QSGCompressedTexturePrivate::updateRhiTexture(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_Q(QSGCompressedTexture);
    if (q->m_uploaded)
        return;

    q->m_uploaded = true; // even if fails, no point in trying again

    if (!q->m_textureData.isValid()) {
        qCDebug(QSG_LOG_TEXTUREIO, "Invalid texture data for %s", q->m_textureData.logName().constData());
        return;
    }

    const QPair<QRhiTexture::Format, bool> fmt = toRhiCompressedFormat(q->m_textureData.glInternalFormat());
    if (fmt.first == QRhiTexture::UnknownFormat) {
        qWarning("Unknown compressed format 0x%x", q->m_textureData.glInternalFormat());
        return;
    }

    QRhiTexture::Flags texFlags;
    if (fmt.second)
        texFlags |= QRhiTexture::sRGB;

    if (!rhi->isTextureFormatSupported(fmt.first, texFlags)) {
        qWarning("Unsupported compressed format 0x%x", q->m_textureData.glInternalFormat());
        return;
    }

    if (!q->m_texture) {
        q->m_texture = rhi->newTexture(fmt.first, q->m_size, 1, texFlags);
        if (!q->m_texture->build()) {
            qWarning("Failed to create QRhiTexture for compressed data");
            delete q->m_texture;
            q->m_texture = nullptr;
            return;
        }
    }

    // only upload mip level 0 since we never do mipmapping for compressed textures (for now?)
    resourceUpdates->uploadTexture(q->m_texture, QRhiTextureUploadEntry(0, 0,
        { q->m_textureData.data().constData() + q->m_textureData.dataOffset(), q->m_textureData.dataLength() }));

    q->m_textureData = QTextureFileData(); // Release this memory, not needed anymore
}

QTextureFileData QSGCompressedTexture::textureData() const
{
    return m_textureData;
}

bool QSGCompressedTexture::formatIsOpaque(quint32 glTextureFormat)
{
    switch (glTextureFormat) {
    case QOpenGLTexture::RGB_DXT1:
    case QOpenGLTexture::R_ATI1N_UNorm:
    case QOpenGLTexture::R_ATI1N_SNorm:
    case QOpenGLTexture::RG_ATI2N_UNorm:
    case QOpenGLTexture::RG_ATI2N_SNorm:
    case QOpenGLTexture::RGB_BP_UNSIGNED_FLOAT:
    case QOpenGLTexture::RGB_BP_SIGNED_FLOAT:
    case QOpenGLTexture::R11_EAC_UNorm:
    case QOpenGLTexture::R11_EAC_SNorm:
    case QOpenGLTexture::RG11_EAC_UNorm:
    case QOpenGLTexture::RG11_EAC_SNorm:
    case QOpenGLTexture::RGB8_ETC2:
    case QOpenGLTexture::SRGB8_ETC2:
    case QOpenGLTexture::RGB8_ETC1:
    case QOpenGLTexture::SRGB_DXT1:
        return true;
        break;
    default:
        return false;
    }
}

QSGCompressedTextureFactory::QSGCompressedTextureFactory(const QTextureFileData &texData)
    : m_textureData(texData)
{
}

QSGTexture *QSGCompressedTextureFactory::createTexture(QQuickWindow *window) const
{
    if (!m_textureData.isValid())
        return nullptr;

    // attempt to atlas the texture
    QSGRenderContext *context = QQuickWindowPrivate::get(window)->context;
    QSGTexture *t = context->compressedTextureForFactory(this);
    if (t)
        return t;

    return new QSGCompressedTexture(m_textureData);
}

int QSGCompressedTextureFactory::textureByteCount() const
{
    return qMax(0, m_textureData.data().size() - m_textureData.dataOffset());
}

QSize QSGCompressedTextureFactory::textureSize() const
{
    return m_textureData.size();
}

QT_END_NAMESPACE
