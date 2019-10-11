/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qsgrhiatlastexture_p.h"

#include <QtCore/QVarLengthArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QtMath>

#include <QtGui/QWindow>

#include <private/qqmlglobal_p.h>
#include <private/qquickprofiler_p.h>
#include <private/qsgdefaultrendercontext_p.h>
#include <private/qsgtexture_p.h>

#include <qtquick_tracepoints_p.h>

#if 0
#include <private/qsgcompressedtexture_p.h>
#include <private/qsgcompressedatlastexture_p.h>
#endif

QT_BEGIN_NAMESPACE

int qt_sg_envInt(const char *name, int defaultValue);

static QElapsedTimer qsg_renderer_timer;

//DEFINE_BOOL_CONFIG_OPTION(qsgEnableCompressedAtlas, QSG_ENABLE_COMPRESSED_ATLAS)

namespace QSGRhiAtlasTexture
{

Manager::Manager(QSGDefaultRenderContext *rc, const QSize &surfacePixelSize, QSurface *maybeSurface)
    : m_rc(rc)
    , m_rhi(rc->rhi())
{
    const int maxSize = m_rhi->resourceLimit(QRhi::TextureSizeMax);
    int w = qMin(maxSize, qt_sg_envInt("QSG_ATLAS_WIDTH", qMax(512U, qNextPowerOfTwo(surfacePixelSize.width() - 1))));
    int h = qMin(maxSize, qt_sg_envInt("QSG_ATLAS_HEIGHT", qMax(512U, qNextPowerOfTwo(surfacePixelSize.height() - 1))));

    if (maybeSurface && maybeSurface->surfaceClass() == QSurface::Window) {
        QWindow *window = static_cast<QWindow *>(maybeSurface);
        // Coverwindows, optimize for memory rather than speed
        if ((window->type() & Qt::CoverWindow) == Qt::CoverWindow) {
            w /= 2;
            h /= 2;
        }
    }

    m_atlas_size_limit = qt_sg_envInt("QSG_ATLAS_SIZE_LIMIT", qMax(w, h) / 2);
    m_atlas_size = QSize(w, h);

    qCDebug(QSG_LOG_INFO, "rhi texture atlas dimensions: %dx%d", w, h);
}

Manager::~Manager()
{
    Q_ASSERT(m_atlas == nullptr);
    Q_ASSERT(m_atlases.isEmpty());
}

void Manager::invalidate()
{
    if (m_atlas) {
        m_atlas->invalidate();
        m_atlas->deleteLater();
        m_atlas = nullptr;
    }

 #if 0
    QHash<unsigned int, QSGCompressedAtlasTexture::Atlas*>::iterator i = m_atlases.begin();
    while (i != m_atlases.end()) {
        i.value()->invalidate();
        i.value()->deleteLater();
        ++i;
    }
    m_atlases.clear();
#endif
}

QSGTexture *Manager::create(const QImage &image, bool hasAlphaChannel)
{
    Texture *t = nullptr;
    if (image.width() < m_atlas_size_limit && image.height() < m_atlas_size_limit) {
        if (!m_atlas)
            m_atlas = new Atlas(m_rc, m_atlas_size);
        t = m_atlas->create(image);
        if (t && !hasAlphaChannel && t->hasAlphaChannel())
            t->setHasAlphaChannel(false);
    }
    return t;
}

QSGTexture *Manager::create(const QSGCompressedTextureFactory *factory)
{
    Q_UNUSED(factory);
    return nullptr;
    // ###

#if 0
    QSGTexture *t = nullptr;
    if (!qsgEnableCompressedAtlas() || !factory->m_textureData.isValid())
        return t;

    // TODO: further abstract the atlas and remove this restriction
    unsigned int format = factory->m_textureData.glInternalFormat();
    switch (format) {
    case QOpenGLTexture::RGB8_ETC1:
    case QOpenGLTexture::RGB8_ETC2:
    case QOpenGLTexture::RGBA8_ETC2_EAC:
    case QOpenGLTexture::RGB8_PunchThrough_Alpha1_ETC2:
        break;
    default:
        return t;
    }

    QSize size = factory->m_textureData.size();
    if (size.width() < m_atlas_size_limit && size.height() < m_atlas_size_limit) {
        QHash<unsigned int, QSGCompressedAtlasTexture::Atlas*>::iterator i = m_atlases.find(format);
        if (i == m_atlases.end())
            i = m_atlases.insert(format, new QSGCompressedAtlasTexture::Atlas(m_atlas_size, format));
        // must be multiple of 4
        QSize paddedSize(((size.width() + 3) / 4) * 4, ((size.height() + 3) / 4) * 4);
        QByteArray data = factory->m_textureData.data();
        t = i.value()->create(data, factory->m_textureData.dataLength(), factory->m_textureData.dataOffset(), size, paddedSize);
    }
#endif
}

AtlasBase::AtlasBase(QSGDefaultRenderContext *rc, const QSize &size)
    : m_rc(rc)
    , m_rhi(rc->rhi())
    , m_allocator(size)
    , m_size(size)
{
}

AtlasBase::~AtlasBase()
{
    Q_ASSERT(!m_texture);
}

void AtlasBase::invalidate()
{
    delete m_texture;
    m_texture = nullptr;
}

void AtlasBase::updateRhiTexture(QRhiResourceUpdateBatch *resourceUpdates)
{
    if (!m_allocated) {
        m_allocated = true;
        if (!generateTexture()) {
            qWarning("QSGTextureAtlas: Failed to create texture");
            return;
        }
    }

    for (TextureBase *t : m_pending_uploads) {
        // ### this profiling is all wrong, the real work is done elsewhere
        bool profileFrames = QSG_LOG_TIME_TEXTURE().isDebugEnabled();
        if (profileFrames)
            qsg_renderer_timer.start();

        Q_TRACE_SCOPE(QSG_texture_prepare);
        Q_QUICK_SG_PROFILE_START(QQuickProfiler::SceneGraphTexturePrepare);

        // Skip bind, convert, swizzle; they're irrelevant
        Q_QUICK_SG_PROFILE_SKIP(QQuickProfiler::SceneGraphTexturePrepare,
                                QQuickProfiler::SceneGraphTexturePrepareStart, 3);
        Q_TRACE(QSG_texture_upload_entry);

        enqueueTextureUpload(t, resourceUpdates);

        Q_TRACE(QSG_texture_upload_exit);
        Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphTexturePrepare,
                                  QQuickProfiler::SceneGraphTexturePrepareUpload);

        // Skip mipmap; unused
        Q_QUICK_SG_PROFILE_SKIP(QQuickProfiler::SceneGraphTexturePrepare,
                                QQuickProfiler::SceneGraphTexturePrepareUpload, 1);
        Q_QUICK_SG_PROFILE_REPORT(QQuickProfiler::SceneGraphTexturePrepare,
                                  QQuickProfiler::SceneGraphTexturePrepareMipmap);
    }

    m_pending_uploads.clear();
}

void AtlasBase::remove(TextureBase *t)
{
    QRect atlasRect = t->atlasSubRect();
    m_allocator.deallocate(atlasRect);
    m_pending_uploads.removeOne(t);
}

Atlas::Atlas(QSGDefaultRenderContext *rc, const QSize &size)
    : AtlasBase(rc, size)
{
    // use RGBA texture internally as that is the only one guaranteed to be always supported
    m_format = QRhiTexture::RGBA8;

    m_debug_overlay = qt_sg_envInt("QSG_ATLAS_OVERLAY", 0);

    // images smaller than this will retain their QImage.
    // by default no images are retained (favoring memory)
    // set to a very large value to retain all images (allowing quick removal from the atlas)
    m_atlas_transient_image_threshold = qt_sg_envInt("QSG_ATLAS_TRANSIENT_IMAGE_THRESHOLD", 0);
}

Atlas::~Atlas()
{
}

Texture *Atlas::create(const QImage &image)
{
    // No need to lock, as manager already locked it.
    QRect rect = m_allocator.allocate(QSize(image.width() + 2, image.height() + 2));
    if (rect.width() > 0 && rect.height() > 0) {
        Texture *t = new Texture(this, rect, image);
        m_pending_uploads << t;
        return t;
    }
    return nullptr;
}

bool Atlas::generateTexture()
{
    m_texture = m_rhi->newTexture(m_format, m_size, 1, QRhiTexture::UsedAsTransferSource);
    if (!m_texture)
        return false;

    if (!m_texture->build()) {
        delete m_texture;
        m_texture = nullptr;
        return false;
    }

    return true;
}

void Atlas::enqueueTextureUpload(TextureBase *t, QRhiResourceUpdateBatch *resourceUpdates)
{
    Texture *tex = static_cast<Texture *>(t);
    const QRect &r = tex->atlasSubRect();
    QImage image = tex->image();

    if (image.isNull())
        return;

    if (image.format() != QImage::Format_RGBA8888_Premultiplied)
        image = std::move(image).convertToFormat(QImage::Format_RGBA8888_Premultiplied);

    if (m_debug_overlay) {
        QPainter p(&image);
        p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
        p.fillRect(0, 0, image.width(), image.height(), QBrush(QColor::fromRgbF(0, 1, 1, 0.5)));
    }

    const int iw = image.width();
    const int ih = image.height();
    const int bpl = image.bytesPerLine() / 4;
    QVarLengthArray<quint32, 1024> tmpBits(qMax(iw + 2, ih + 2));
    const int tmpBitsSize = tmpBits.size() * 4;
    const quint32 *src = reinterpret_cast<const quint32 *>(image.constBits());
    quint32 *dst = tmpBits.data();
    QVarLengthArray<QRhiTextureUploadEntry, 5> entries;

    // top row, padding corners
    dst[0] = src[0];
    memcpy(dst + 1, src, iw * sizeof(quint32));
    dst[1 + iw] = src[iw - 1];
    {
        QRhiTextureSubresourceUploadDescription subresDesc(dst, tmpBitsSize);
        subresDesc.setDestinationTopLeft(QPoint(r.x(), r.y()));
        subresDesc.setSourceSize(QSize(iw + 2, 1));
        entries.append(QRhiTextureUploadEntry(0, 0, subresDesc));
    }

    // bottom row, padded corners
    const quint32 *lastRow = src + bpl * (ih - 1);
    dst[0] = lastRow[0];
    memcpy(dst + 1, lastRow, iw * sizeof(quint32));
    dst[1 + iw] = lastRow[iw - 1];
    {
        QRhiTextureSubresourceUploadDescription subresDesc(dst, tmpBitsSize);
        subresDesc.setDestinationTopLeft(QPoint(r.x(), r.y() + ih + 1));
        subresDesc.setSourceSize(QSize(iw + 2, 1));
        entries.append(QRhiTextureUploadEntry(0, 0, subresDesc));
    }

    // left column
    for (int i = 0; i < ih; ++i)
        dst[i] = src[i * bpl];
    {
        QRhiTextureSubresourceUploadDescription subresDesc(dst, tmpBitsSize);
        subresDesc.setDestinationTopLeft(QPoint(r.x(), r.y() + 1));
        subresDesc.setSourceSize(QSize(1, ih));
        entries.append(QRhiTextureUploadEntry(0, 0, subresDesc));
    }


    // right column
    for (int i = 0; i < ih; ++i)
        dst[i] = src[i * bpl + iw - 1];
    {
        QRhiTextureSubresourceUploadDescription subresDesc(dst, tmpBitsSize);
        subresDesc.setDestinationTopLeft(QPoint(r.x() + iw + 1, r.y() + 1));
        subresDesc.setSourceSize(QSize(1, ih));
        entries.append(QRhiTextureUploadEntry(0, 0, subresDesc));
    }

    // Inner part of the image....
    if (bpl != iw) {
        int sy = r.y() + 1;
        int ey = sy + r.height() - 2;
        entries.reserve(4 + (ey - sy));
        for (int y = sy; y < ey; ++y) {
            QRhiTextureSubresourceUploadDescription subresDesc(src, image.bytesPerLine());
            subresDesc.setDestinationTopLeft(QPoint(r.x() + 1, y));
            subresDesc.setSourceSize(QSize(r.width() - 2, 1));
            entries.append(QRhiTextureUploadEntry(0, 0, subresDesc));
            src += bpl;
        }
    } else {
        QRhiTextureSubresourceUploadDescription subresDesc(src, image.sizeInBytes());
        subresDesc.setDestinationTopLeft(QPoint(r.x() + 1, r.y() + 1));
        subresDesc.setSourceSize(QSize(r.width() - 2, r.height() - 2));
        entries.append(QRhiTextureUploadEntry(0, 0, subresDesc));
    }

    QRhiTextureUploadDescription desc;
    desc.setEntries(entries.cbegin(), entries.cend());
    resourceUpdates->uploadTexture(m_texture, desc);

    const QSize textureSize = t->textureSize();
    if (textureSize.width() > m_atlas_transient_image_threshold || textureSize.height() > m_atlas_transient_image_threshold)
        tex->releaseImage();

    qCDebug(QSG_LOG_TIME_TEXTURE, "atlastexture upload enqueued in: %lldms (%dx%d)",
            qsg_renderer_timer.elapsed(),
            t->textureSize().width(),
            t->textureSize().height());
}

TextureBase::TextureBase(AtlasBase *atlas, const QRect &textureRect)
    : QSGTexture(*(new TextureBasePrivate))
    , m_allocated_rect(textureRect)
    , m_atlas(atlas)
{
}

TextureBase::~TextureBase()
{
    m_atlas->remove(this);
}

QRhiResourceUpdateBatch *TextureBase::workResourceUpdateBatch() const
{
    Q_D(const TextureBase);
    return d->workResourceUpdateBatch;
}

int TextureBasePrivate::comparisonKey() const
{
    Q_Q(const TextureBase);

    // We need special care here: a typical comparisonKey() implementation
    // returns a unique result when there is no underlying texture yet. This is
    // not quite ideal for atlasing however since textures with the same atlas
    // should be considered equal regardless of the state of the underlying
    // graphics resources.

    // base the comparison on the atlas ptr; this way textures for the same
    // atlas are considered equal
    return int(qintptr(q->m_atlas));
}

QRhiTexture *TextureBasePrivate::rhiTexture() const
{
    Q_Q(const TextureBase);
    return q->m_atlas->m_texture;
}

void TextureBasePrivate::updateRhiTexture(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_Q(TextureBase);
#ifdef QT_NO_DEBUG
    Q_UNUSED(rhi);
#endif
    Q_ASSERT(rhi == q->m_atlas->m_rhi);
    q->m_atlas->updateRhiTexture(resourceUpdates);
}

Texture::Texture(Atlas *atlas, const QRect &textureRect, const QImage &image)
    : TextureBase(atlas, textureRect)
    , m_image(image)
    , m_has_alpha(image.hasAlphaChannel())
{
    float w = atlas->size().width();
    float h = atlas->size().height();
    QRect nopad = atlasSubRectWithoutPadding();
    m_texture_coords_rect = QRectF(nopad.x() / w,
                                   nopad.y() / h,
                                   nopad.width() / w,
                                   nopad.height() / h);
}

Texture::~Texture()
{
    if (m_nonatlas_texture)
        delete m_nonatlas_texture;
}

QSGTexture *Texture::removedFromAtlas() const
{
    if (!m_nonatlas_texture) {
        m_nonatlas_texture = new QSGPlainTexture;
        if (!m_image.isNull()) {
            m_nonatlas_texture->setImage(m_image);
            m_nonatlas_texture->setFiltering(filtering());
        } else {
            QSGDefaultRenderContext *rc = m_atlas->renderContext();
            QRhi *rhi = m_atlas->rhi();
            Q_ASSERT(rhi->isRecordingFrame());
            const QRect r = atlasSubRectWithoutPadding();

            QRhiTexture *extractTex = rhi->newTexture(m_atlas->texture()->format(), r.size());
            if (extractTex->build()) {
                bool ownResUpd = false;
                QRhiResourceUpdateBatch *resUpd = workResourceUpdateBatch(); // ### Qt 6: should be an arg to this function
                if (!resUpd) {
                    ownResUpd = true;
                    resUpd = rhi->nextResourceUpdateBatch();
                }
                QRhiTextureCopyDescription desc;
                desc.setSourceTopLeft(r.topLeft());
                desc.setPixelSize(r.size());
                resUpd->copyTexture(extractTex, m_atlas->texture(), desc);
                if (ownResUpd)
                    rc->currentFrameCommandBuffer()->resourceUpdate(resUpd);
            }

            m_nonatlas_texture->setTexture(extractTex);
            m_nonatlas_texture->setOwnsTexture(true);
            m_nonatlas_texture->setHasAlphaChannel(m_has_alpha);
            m_nonatlas_texture->setTextureSize(r.size());
        }
    }

    m_nonatlas_texture->setMipmapFiltering(mipmapFiltering());
    m_nonatlas_texture->setFiltering(filtering());
    return m_nonatlas_texture;
}

}

QT_END_NAMESPACE

#include "moc_qsgrhiatlastexture_p.cpp"
