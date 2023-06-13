// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgplaintexture_p.h"
#include <QtQuick/private/qsgcontext_p.h>
#include <qmath.h>
#include <private/qquickprofiler_p.h>
#include <private/qqmlglobal_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpa/qplatformnativeinterface.h>
#include <rhi/qrhi.h>
#include <QtQuick/private/qsgrhisupport_p.h>

#include <qtquick_tracepoints_p.h>

QT_BEGIN_NAMESPACE

QSGPlainTexture::QSGPlainTexture()
    : QSGTexture(*(new QSGPlainTexturePrivate(this)))
    , m_texture(nullptr)
    , m_has_alpha(false)
    , m_dirty_texture(false)
    , m_owns_texture(true)
    , m_mipmaps_generated(false)
    , m_retain_image(false)
    , m_mipmap_warned(false)
{
}

QSGPlainTexture::QSGPlainTexture(QSGPlainTexturePrivate &dd)
    : QSGTexture(dd)
    , m_texture(nullptr)
    , m_has_alpha(false)
    , m_dirty_texture(false)
    , m_owns_texture(true)
    , m_mipmaps_generated(false)
    , m_retain_image(false)
    , m_mipmap_warned(false)
{
}

QSGPlainTexture::~QSGPlainTexture()
{
    if (m_texture && m_owns_texture)
        delete m_texture;
}

void QSGPlainTexture::setImage(const QImage &image)
{
    m_image = image;
    m_texture_size = image.size();
    m_has_alpha = image.hasAlphaChannel();
    m_dirty_texture = true;
    m_mipmaps_generated = false;
}

void QSGPlainTexture::setTexture(QRhiTexture *texture) // RHI only
{
    if (m_texture && m_owns_texture && m_texture != texture)
        delete m_texture;

    m_texture = texture;
    m_dirty_texture = false;
    m_image = QImage();
    m_mipmaps_generated = false;
}

void QSGPlainTexture::setTextureFromNativeTexture(QRhi *rhi,
                                                  quint64 nativeObjectHandle,
                                                  int nativeLayoutOrState,
                                                  uint nativeFormat,
                                                  const QSize &size,
                                                  QQuickWindow::CreateTextureOptions options,
                                                  QQuickWindowPrivate::TextureFromNativeTextureFlags flags)
{
    QRhiTexture::Flags texFlags;
    if (options.testFlag(QQuickWindow::TextureHasMipmaps))
        texFlags |= QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips;
    if (flags.testFlag(QQuickWindowPrivate::NativeTextureIsExternalOES))
        texFlags |= QRhiTexture::ExternalOES;

    QRhiTexture::Format format = QRhiTexture::RGBA8;

    QRhiTexture::Flags formatFlags;
    auto rhiFormat = QSGRhiSupport::instance()->toRhiTextureFormat(nativeFormat, &formatFlags);
    if (rhiFormat != QRhiTexture::UnknownFormat) {
        format = rhiFormat;
        texFlags |= formatFlags;
    }

    QRhiTexture *t = rhi->newTexture(format, size, 1, texFlags);

    // ownership of the native object is never taken
    t->createFrom({nativeObjectHandle, nativeLayoutOrState});

    setTexture(t);
}

qint64 QSGPlainTexture::comparisonKey() const
{
    if (m_texture)
        return qint64(m_texture);

    // two textures (and so materials) with not-yet-created texture underneath are never equal
    return qint64(this);
}

QRhiTexture *QSGPlainTexture::rhiTexture() const
{
    return m_texture;
}

void QSGPlainTexture::commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_D(QSGPlainTexture);

    const bool hasMipMaps = mipmapFiltering() != QSGTexture::None;
    const bool mipmappingChanged = m_texture && ((hasMipMaps && !m_texture->flags().testFlag(QRhiTexture::MipMapped)) // did not have it before
            || (!hasMipMaps && m_texture->flags().testFlag(QRhiTexture::MipMapped))); // does not have it anymore

    if (!m_dirty_texture) {
        if (!m_texture)
            return;
        if (m_texture && !mipmappingChanged) {
            if (hasMipMaps && !m_mipmaps_generated) {
                resourceUpdates->generateMips(m_texture);
                m_mipmaps_generated = true;
            }
            return;
        }
    }

    if (m_image.isNull()) {
        if (!m_dirty_texture && mipmappingChanged) {
            // Full Mipmap Panic!
            if (!m_mipmap_warned) {
                qWarning("QSGPlainTexture: Mipmap settings changed without having image data available. "
                         "Call setImage() again or enable m_retain_image. "
                         "Falling back to previous mipmap filtering mode.");
                m_mipmap_warned = true;
            }
            // leave the texture valid and rather ignore the mipmap mode change attempt
            setMipmapFiltering(d->m_last_mipmap_filter);
            return;
        }

        if (m_texture && m_owns_texture)
            delete m_texture;

        m_texture = nullptr;
        m_texture_size = QSize();
        m_has_alpha = false;

        m_dirty_texture = false;
        return;
    }

    m_dirty_texture = false;

    QImage tmp;
    bool bgra = false;
    bool needsConvert = false;
    if (m_image.format() == QImage::Format_RGB32 || m_image.format() == QImage::Format_ARGB32_Premultiplied) {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        if (rhi->isTextureFormatSupported(QRhiTexture::BGRA8)) {
            tmp = m_image;
            bgra = true;
        } else {
            needsConvert = true;
        }
#else
        needsConvert = true;
#endif
    } else if (m_image.format() == QImage::Format_RGBX8888 || m_image.format() == QImage::Format_RGBA8888_Premultiplied) {
        tmp = m_image;
    } else {
        needsConvert = true;
    }

    if (needsConvert)
        tmp = m_image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);

    // Downscale the texture to fit inside the max texture limit if it is too big.
    // It would be better if the image was already downscaled to the right size,
    // but this information is not always available at that time, so as a last
    // resort we can do it here. Texture coordinates are normalized, so it
    // won't cause any problems and actual texture sizes will be written
    // based on QSGTexture::textureSize which is updated after this, so that
    // should be ok.
    const int max = rhi->resourceLimit(QRhi::TextureSizeMax);
    if (tmp.width() > max || tmp.height() > max) {
        tmp = tmp.scaled(qMin(max, tmp.width()), qMin(max, tmp.height()), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        m_texture_size = tmp.size();
    }

    if ((mipmapFiltering() != QSGTexture::None
            || horizontalWrapMode() != QSGTexture::ClampToEdge
            || verticalWrapMode() != QSGTexture::ClampToEdge)
            && !rhi->isFeatureSupported(QRhi::NPOTTextureRepeat))
    {
        const int w = qNextPowerOfTwo(tmp.width() - 1);
        const int h = qNextPowerOfTwo(tmp.height() - 1);
        if (tmp.width() != w || tmp.height() != h) {
            tmp = tmp.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            m_texture_size = tmp.size();
        }
    }

    bool needsRebuild = false;

    if (m_texture && m_texture->pixelSize() != m_texture_size) {
        m_texture->setPixelSize(m_texture_size);
        needsRebuild = true;
    }

    if (mipmappingChanged) {
        QRhiTexture::Flags f = m_texture->flags();
        f.setFlag(QRhiTexture::MipMapped, hasMipMaps);
        f.setFlag(QRhiTexture::UsedWithGenerateMips, hasMipMaps);
        m_texture->setFlags(f);
        needsRebuild = true;
    }

    if (!m_texture) {
        QRhiTexture::Flags f;
        if (hasMipMaps)
            f |= QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips;

        m_texture = rhi->newTexture(bgra ? QRhiTexture::BGRA8 : QRhiTexture::RGBA8, m_texture_size, 1, f);
        needsRebuild = true;
    }

    if (needsRebuild) {
        if (!m_texture->create()) {
            qWarning("Failed to build texture for QSGPlainTexture (size %dx%d)",
                     m_texture_size.width(), m_texture_size.height());
            return;
        }
    }

    if (tmp.width() * 4 != tmp.bytesPerLine())
        tmp = tmp.copy();

    resourceUpdates->uploadTexture(m_texture, tmp);

    if (hasMipMaps) {
        resourceUpdates->generateMips(m_texture);
        m_mipmaps_generated = true;
    }

    d->m_last_mipmap_filter = mipmapFiltering();
    m_texture_rect = QRectF(0, 0, 1, 1);

    if (!m_retain_image)
        m_image = QImage();
}

QT_END_NAMESPACE

#include "moc_qsgplaintexture_p.cpp"
