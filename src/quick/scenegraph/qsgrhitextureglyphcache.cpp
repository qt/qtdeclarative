// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgrhitextureglyphcache_p.h"
#include "qsgdefaultrendercontext_p.h"
#include <qrgb.h>
#include <private/qdrawhelper_p.h>

QT_BEGIN_NAMESPACE

QSGRhiTextureGlyphCache::QSGRhiTextureGlyphCache(QSGDefaultRenderContext *rc,
                                                 QFontEngine::GlyphFormat format, const QTransform &matrix,
                                                 const QColor &color)
    : QImageTextureGlyphCache(format, matrix, color),
      m_rc(rc),
      m_rhi(rc->rhi())
{
    // Some OpenGL implementations, for instance macOS, have issues with
    // GL_ALPHA render targets. Similarly, BGRA may be problematic on GLES 2.0.
    // So stick with plain image uploads on GL.
    m_resizeWithTextureCopy = m_rhi->backend() != QRhi::OpenGLES2;
}

QSGRhiTextureGlyphCache::~QSGRhiTextureGlyphCache()
{
    m_rc->deferredReleaseGlyphCacheTexture(m_texture);
}

QRhiTexture *QSGRhiTextureGlyphCache::createEmptyTexture(QRhiTexture::Format format)
{
    QRhiTexture *t = m_rhi->newTexture(format, m_size, 1, QRhiTexture::UsedAsTransferSource);
    if (!t->create()) {
        qWarning("Failed to build new glyph cache texture of size %dx%d", m_size.width(), m_size.height());
        return nullptr;
    }

    QRhiResourceUpdateBatch *resourceUpdates = m_rc->glyphCacheResourceUpdates();

    // The new texture must be cleared to 0 always, this cannot be avoided
    // otherwise artifacts will occur around the glyphs.
    QByteArray data;
    if (format == QRhiTexture::RED_OR_ALPHA8)
        data.fill(0, m_size.width() * m_size.height());
    else
        data.fill(0, m_size.width() * m_size.height() * 4);
    QRhiTextureSubresourceUploadDescription subresDesc(data.constData(), data.size());
    subresDesc.setSourceSize(m_size);
    resourceUpdates->uploadTexture(t, QRhiTextureUploadEntry(0, 0, subresDesc));

    return t;
}

void QSGRhiTextureGlyphCache::createTextureData(int width, int height)
{
    width = qMax(128, width);
    height = qMax(32, height);

    if (!m_resizeWithTextureCopy)
        QImageTextureGlyphCache::createTextureData(width, height);

    m_size = QSize(width, height);
}

void QSGRhiTextureGlyphCache::resizeTextureData(int width, int height)
{
    width = qMax(128, width);
    height = qMax(32, height);

    if (m_size.width() >= width && m_size.height() >= height)
        return;

    m_size = QSize(width, height);

    if (m_texture) {
        QRhiTexture *t = createEmptyTexture(m_texture->format());
        if (!t)
            return;

        QRhiResourceUpdateBatch *resourceUpdates = m_rc->glyphCacheResourceUpdates();
        if (m_resizeWithTextureCopy) {
            resourceUpdates->copyTexture(t, m_texture);
        } else {
            QImageTextureGlyphCache::resizeTextureData(width, height);
            QImage img = image();
            prepareGlyphImage(&img);
            QRhiTextureSubresourceUploadDescription subresDesc(img);
            const QSize oldSize = m_texture->pixelSize();
            subresDesc.setSourceSize(QSize(qMin(oldSize.width(), width), qMin(oldSize.height(), height)));
            resourceUpdates->uploadTexture(t, QRhiTextureUploadEntry(0, 0, subresDesc));
        }

        m_rc->deferredReleaseGlyphCacheTexture(m_texture);
        m_texture = t;
    }
}

void QSGRhiTextureGlyphCache::beginFillTexture()
{
    Q_ASSERT(m_uploads.isEmpty());
}

void QSGRhiTextureGlyphCache::prepareGlyphImage(QImage *img)
{
    const int maskWidth = img->width();
    const int maskHeight = img->height();
#if Q_BYTE_ORDER != Q_BIG_ENDIAN
    const bool supportsBgra = m_rhi->isTextureFormatSupported(QRhiTexture::BGRA8);
#endif
    m_bgra = false;

    if (img->format() == QImage::Format_Mono) {
        *img = std::move(*img).convertToFormat(QImage::Format_Grayscale8);
    } else if (img->format() == QImage::Format_RGB32 || img->format() == QImage::Format_ARGB32_Premultiplied) {
        // We need to make the alpha component equal to the average of the RGB values.
        // This is needed when drawing sub-pixel antialiased text on translucent targets.
        if (img->format() == QImage::Format_RGB32
#if Q_BYTE_ORDER != Q_BIG_ENDIAN
            || !supportsBgra
#endif
        ) {
            for (int y = 0; y < maskHeight; ++y) {
                QRgb *src = reinterpret_cast<QRgb *>(img->scanLine(y));
                for (int x = 0; x < maskWidth; ++x) {
                    QRgb &rgb = src[x];

                    if (img->format() == QImage::Format_RGB32) {
                        int r = qRed(rgb);
                        int g = qGreen(rgb);
                        int b = qBlue(rgb);
                        int avg = (r + g + b + 1) / 3; // "+1" for rounding.
                        rgb = qRgba(r, g, b, avg);
                    }

#if Q_BYTE_ORDER != Q_BIG_ENDIAN
                    if (!supportsBgra) {
                        // swizzle the bits to accommodate for the RGBA upload.
                        rgb = ARGB2RGBA(rgb);
                        m_bgra = false;
                    }
#endif
                }
            }
        }
#if Q_BYTE_ORDER != Q_BIG_ENDIAN
        if (supportsBgra)
            m_bgra = true;
#endif
    }
}

void QSGRhiTextureGlyphCache::fillTexture(const Coord &c, glyph_t glyph, const QFixedPoint &subPixelPosition)
{
    QRhiTextureSubresourceUploadDescription subresDesc;
    QImage mask;

    if (!m_resizeWithTextureCopy) {
        QImageTextureGlyphCache::fillTexture(c, glyph, subPixelPosition);
        // Explicitly copy() here to avoid fillTexture detaching the *entire* image() when
        // it is still referenced by QRhiTextureSubresourceUploadDescription.
        mask = image().copy(QRect(c.x, c.y, c.w, c.h));
    } else {
        mask = textureMapForGlyph(glyph, subPixelPosition);
    }

    prepareGlyphImage(&mask);

    subresDesc.setImage(mask);
    subresDesc.setDestinationTopLeft(QPoint(c.x, c.y));
    m_uploads.append(QRhiTextureUploadEntry(0, 0, subresDesc));
}

void QSGRhiTextureGlyphCache::endFillTexture()
{
    if (m_uploads.isEmpty())
        return;

    if (!m_texture) {
        QRhiTexture::Format texFormat;
        if (m_format == QFontEngine::Format_A32 || m_format == QFontEngine::Format_ARGB)
            texFormat = m_bgra ? QRhiTexture::BGRA8 : QRhiTexture::RGBA8;
        else // should be R8, but there is the OpenGL ES 2.0 nonsense
            texFormat = QRhiTexture::RED_OR_ALPHA8;

        m_texture = createEmptyTexture(texFormat);
        if (!m_texture)
            return;
    }

    QRhiResourceUpdateBatch *resourceUpdates = m_rc->glyphCacheResourceUpdates();
    QRhiTextureUploadDescription desc;
    desc.setEntries(m_uploads.cbegin(), m_uploads.cend());
    resourceUpdates->uploadTexture(m_texture, desc);
    m_uploads.clear();
}

int QSGRhiTextureGlyphCache::glyphPadding() const
{
    if (m_format == QFontEngine::Format_Mono)
        return 8;
    else
        return 1;
}

int QSGRhiTextureGlyphCache::maxTextureWidth() const
{
    return m_rhi->resourceLimit(QRhi::TextureSizeMax);
}

int QSGRhiTextureGlyphCache::maxTextureHeight() const
{
    if (!m_resizeWithTextureCopy)
        return qMin(1024, m_rhi->resourceLimit(QRhi::TextureSizeMax));

    return m_rhi->resourceLimit(QRhi::TextureSizeMax);
}

void QSGRhiTextureGlyphCache::commitResourceUpdates(QRhiResourceUpdateBatch *mergeInto)
{
    if (QRhiResourceUpdateBatch *resourceUpdates = m_rc->maybeGlyphCacheResourceUpdates()) {
        mergeInto->merge(resourceUpdates);
        m_rc->resetGlyphCacheResources();
    }
}

bool QSGRhiTextureGlyphCache::eightBitFormatIsAlphaSwizzled() const
{
    // return true when the shaders for 8-bit formats need .a instead of .r
    // when sampling the texture
    return !m_rhi->isFeatureSupported(QRhi::RedOrAlpha8IsRed);
}

QT_END_NAMESPACE
