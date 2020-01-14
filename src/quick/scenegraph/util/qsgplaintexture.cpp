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

#include "qsgplaintexture_p.h"
#include <QtQuick/private/qsgcontext_p.h>
#include <qmath.h>
#include <private/qquickprofiler_p.h>
#include <private/qqmlglobal_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpa/qplatformnativeinterface.h>
#if QT_CONFIG(opengl)
# include <QtGui/qopenglcontext.h>
# include <QtGui/qopenglfunctions.h>
# include <QtGui/private/qopengltextureuploader_p.h>
# include <private/qsgdefaultrendercontext_p.h>
#endif
#include <QtGui/private/qrhi_p.h>

#include <qtquick_tracepoints_p.h>

#if QT_CONFIG(opengl)
static QElapsedTimer qsg_renderer_timer;
#endif

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

QT_BEGIN_NAMESPACE

QSGPlainTexture::QSGPlainTexture()
    : QSGTexture(*(new QSGPlainTexturePrivate))
    , m_texture_id(0)
    , m_texture(nullptr)
    , m_has_alpha(false)
    , m_dirty_texture(false)
    , m_dirty_bind_options(false)
    , m_owns_texture(true)
    , m_mipmaps_generated(false)
    , m_retain_image(false)
    , m_mipmap_warned(false)
{
}

QSGPlainTexture::QSGPlainTexture(QSGPlainTexturePrivate &dd)
    : QSGTexture(dd)
    , m_texture_id(0)
    , m_texture(nullptr)
    , m_has_alpha(false)
    , m_dirty_texture(false)
    , m_dirty_bind_options(false)
    , m_owns_texture(true)
    , m_mipmaps_generated(false)
    , m_retain_image(false)
    , m_mipmap_warned(false)
{
}

QSGPlainTexture::~QSGPlainTexture()
{
#if QT_CONFIG(opengl)
    if (m_texture_id && m_owns_texture && QOpenGLContext::currentContext())
        QOpenGLContext::currentContext()->functions()->glDeleteTextures(1, &m_texture_id);
#endif
    if (m_texture && m_owns_texture)
        delete m_texture;
}

void QSGPlainTexture::setImage(const QImage &image)
{
    m_image = image;
    m_texture_size = image.size();
    m_has_alpha = image.hasAlphaChannel();
    m_dirty_texture = true;
    m_dirty_bind_options = true;
    m_mipmaps_generated = false;
 }

int QSGPlainTexture::textureId() const // legacy (GL-only)
{
    if (m_dirty_texture) {
        if (m_image.isNull()) {
            // The actual texture and id will be updated/deleted in a later bind()
            // or ~QSGPlainTexture so just keep it minimal here.
            return 0;
        } else if (m_texture_id == 0){
#if QT_CONFIG(opengl)
            // Generate a texture id for use later and return it.
            QOpenGLContext::currentContext()->functions()->glGenTextures(1, &const_cast<QSGPlainTexture *>(this)->m_texture_id);
#endif
            return m_texture_id;
        }
    }
    return m_texture_id;
}

void QSGPlainTexture::setTextureId(int id) // legacy (GL-only)
{
#if QT_CONFIG(opengl)
    if (m_texture_id && m_owns_texture)
        QOpenGLContext::currentContext()->functions()->glDeleteTextures(1, &m_texture_id);
#endif

    m_texture_id = id;
    m_dirty_texture = false;
    m_dirty_bind_options = true;
    m_image = QImage();
    m_mipmaps_generated = false;
}

void QSGPlainTexture::bind() // legacy (GL-only)
{
#if QT_CONFIG(opengl)
    Q_TRACE_SCOPE(QSG_texture_prepare);
    QOpenGLContext *context = QOpenGLContext::currentContext();
    QOpenGLFunctions *funcs = context->functions();
    if (!m_dirty_texture) {
        Q_TRACE_SCOPE(QSG_texture_bind);
        funcs->glBindTexture(GL_TEXTURE_2D, m_texture_id);
        if (mipmapFiltering() != QSGTexture::None && !m_mipmaps_generated) {
            funcs->glGenerateMipmap(GL_TEXTURE_2D);
            m_mipmaps_generated = true;
        }
        updateBindOptions(m_dirty_bind_options);
        m_dirty_bind_options = false;
        return;
    }

    m_dirty_texture = false;

    bool profileFrames = QSG_LOG_TIME_TEXTURE().isDebugEnabled();
    if (profileFrames)
        qsg_renderer_timer.start();
    Q_QUICK_SG_PROFILE_START_SYNCHRONIZED(QQuickProfiler::SceneGraphTexturePrepare,
                                          QQuickProfiler::SceneGraphTextureDeletion);


    if (m_image.isNull()) {
        if (m_texture_id && m_owns_texture) {
            Q_TRACE_SCOPE(QSG_texture_delete);
            funcs->glDeleteTextures(1, &m_texture_id);
            qCDebug(QSG_LOG_TIME_TEXTURE, "plain texture deleted in %dms - %dx%d",
                    (int) qsg_renderer_timer.elapsed(),
                    m_texture_size.width(),
                    m_texture_size.height());
            Q_QUICK_SG_PROFILE_END(QQuickProfiler::SceneGraphTextureDeletion,
                                   QQuickProfiler::SceneGraphTextureDeletionDelete);
        }
        m_texture_id = 0;
        m_texture_size = QSize();
        m_has_alpha = false;

        return;
    }

    Q_TRACE(QSG_texture_bind_entry);

    if (m_texture_id == 0)
        funcs->glGenTextures(1, &m_texture_id);
    funcs->glBindTexture(GL_TEXTURE_2D, m_texture_id);

    qint64 bindTime = 0;
    if (profileFrames)
        bindTime = qsg_renderer_timer.nsecsElapsed();
    Q_TRACE(QSG_texture_bind_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphTexturePrepare,
                              QQuickProfiler::SceneGraphTexturePrepareBind);
    Q_TRACE(QSG_texture_upload_entry);

    // ### TODO: check for out-of-memory situations...

    QOpenGLTextureUploader::BindOptions options = QOpenGLTextureUploader::PremultipliedAlphaBindOption;

    // Downscale the texture to fit inside the max texture limit if it is too big.
    // It would be better if the image was already downscaled to the right size,
    // but this information is not always available at that time, so as a last
    // resort we can do it here. Texture coordinates are normalized, so it
    // won't cause any problems and actual texture sizes will be written
    // based on QSGTexture::textureSize which is updated after this, so that
    // should be ok.
    int max;
    if (auto rc = QSGDefaultRenderContext::from(context))
        max = rc->maxTextureSize();
    else
        funcs->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);

    m_texture_size = m_texture_size.boundedTo(QSize(max, max));

    // Scale to a power of two size if mipmapping is requested and the
    // texture is npot and npot textures are not properly supported.
    if (mipmapFiltering() != QSGTexture::None
        && !funcs->hasOpenGLFeature(QOpenGLFunctions::NPOTTextures)) {
        options |= QOpenGLTextureUploader::PowerOfTwoBindOption;
    }

    updateBindOptions(m_dirty_bind_options);

    QOpenGLTextureUploader::textureImage(GL_TEXTURE_2D, m_image, options, QSize(max, max));

    qint64 uploadTime = 0;
    if (profileFrames)
        uploadTime = qsg_renderer_timer.nsecsElapsed();
    Q_TRACE(QSG_texture_upload_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphTexturePrepare,
                              QQuickProfiler::SceneGraphTexturePrepareUpload);
    Q_TRACE(QSG_texture_mipmap_entry);

    if (mipmapFiltering() != QSGTexture::None) {
        funcs->glGenerateMipmap(GL_TEXTURE_2D);
        m_mipmaps_generated = true;
    }

    qint64 mipmapTime = 0;
    if (profileFrames) {
        mipmapTime = qsg_renderer_timer.nsecsElapsed();
        qCDebug(QSG_LOG_TIME_TEXTURE,
                "plain texture uploaded in: %dms (%dx%d), bind=%d, upload=%d, mipmap=%d%s",
                int(mipmapTime / 1000000),
                m_texture_size.width(), m_texture_size.height(),
                int(bindTime / 1000000),
                int((uploadTime - bindTime)/1000000),
                int((mipmapTime - uploadTime)/1000000),
                m_texture_size != m_image.size() ? " (scaled to GL_MAX_TEXTURE_SIZE)" : "");
    }
    Q_TRACE(QSG_texture_mipmap_exit);
    Q_QUICK_SG_PROFILE_END(QQuickProfiler::SceneGraphTexturePrepare,
                           QQuickProfiler::SceneGraphTexturePrepareMipmap);

    m_texture_rect = QRectF(0, 0, 1, 1);

    m_dirty_bind_options = false;
    if (!m_retain_image)
        m_image = QImage();
#endif
}

void QSGPlainTexture::setTexture(QRhiTexture *texture) // RHI only
{
    if (m_texture && m_owns_texture && m_texture != texture)
        delete m_texture;

    m_texture = texture;
    m_dirty_texture = false;
    m_dirty_bind_options = true;
    m_image = QImage();
    m_mipmaps_generated = false;
}

void QSGPlainTexture::setTextureFromNativeObject(QRhi *rhi, QQuickWindow::NativeObjectType type,
                                                 const void *nativeObjectPtr, int nativeLayout,
                                                 const QSize &size, bool mipmap)
{
    Q_UNUSED(type);

    QRhiTexture::Flags flags;
    if (mipmap)
        flags |= QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips;

    QRhiTexture *t = rhi->newTexture(QRhiTexture::RGBA8, size, 1, flags);

    // ownership of the native object is never taken
    t->buildFrom({nativeObjectPtr, nativeLayout});

    setTexture(t);
}

int QSGPlainTexturePrivate::comparisonKey() const
{
    Q_Q(const QSGPlainTexture);

    // not textureId() as that would create an id when not yet done - that's not wanted here
    if (q->m_texture_id)
        return q->m_texture_id;

    if (q->m_texture)
        return int(qintptr(q->m_texture));

    // two textures (and so materials) with not-yet-created texture underneath are never equal
    return int(qintptr(q));
}

QRhiTexture *QSGPlainTexturePrivate::rhiTexture() const
{
    Q_Q(const QSGPlainTexture);
    return q->m_texture;
}

void QSGPlainTexturePrivate::updateRhiTexture(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_Q(QSGPlainTexture);

    const bool hasMipMaps = q->mipmapFiltering() != QSGTexture::None;
    const bool mipmappingChanged = q->m_texture && ((hasMipMaps && !q->m_texture->flags().testFlag(QRhiTexture::MipMapped)) // did not have it before
            || (!hasMipMaps && q->m_texture->flags().testFlag(QRhiTexture::MipMapped))); // does not have it anymore

    if (!q->m_dirty_texture) {
        if (!q->m_texture)
            return;
        if (q->m_texture && !mipmappingChanged) {
            if (hasMipMaps && !q->m_mipmaps_generated) {
                resourceUpdates->generateMips(q->m_texture);
                q->m_mipmaps_generated = true;
            }
            return;
        }
    }

    if (q->m_image.isNull()) {
        if (!q->m_dirty_texture && mipmappingChanged) {
            // Full Mipmap Panic!
            if (!q->m_mipmap_warned) {
                qWarning("QSGPlainTexture: Mipmap settings changed without having image data available. "
                         "Call setImage() again or enable m_retain_image. "
                         "Falling back to previous mipmap filtering mode.");
                q->m_mipmap_warned = true;
            }
            // leave the texture valid and rather ignore the mipmap mode change attempt
            q->setMipmapFiltering(m_last_mipmap_filter);
            return;
        }

        if (q->m_texture && q->m_owns_texture)
            delete q->m_texture;

        q->m_texture = nullptr;
        q->m_texture_size = QSize();
        q->m_has_alpha = false;

        q->m_dirty_texture = false;
        return;
    }

    q->m_dirty_texture = false;

    QImage tmp;
    bool bgra = false;
    bool needsConvert = false;
    if (q->m_image.format() == QImage::Format_RGB32 || q->m_image.format() == QImage::Format_ARGB32_Premultiplied) {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        if (rhi->isTextureFormatSupported(QRhiTexture::BGRA8)) {
            tmp = q->m_image;
            bgra = true;
        } else {
            needsConvert = true;
        }
#else
        needsConvert = true;
#endif
    } else if (q->m_image.format() == QImage::Format_RGBX8888 || q->m_image.format() == QImage::Format_RGBA8888_Premultiplied) {
        tmp = q->m_image;
    } else {
        needsConvert = true;
    }

    if (needsConvert)
        tmp = q->m_image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);

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
        q->m_texture_size = tmp.size();
    }

    if ((q->mipmapFiltering() != QSGTexture::None
            || q->horizontalWrapMode() != QSGTexture::ClampToEdge
            || q->verticalWrapMode() != QSGTexture::ClampToEdge)
            && !rhi->isFeatureSupported(QRhi::NPOTTextureRepeat))
    {
        const int w = qNextPowerOfTwo(tmp.width() - 1);
        const int h = qNextPowerOfTwo(tmp.height() - 1);
        if (tmp.width() != w || tmp.height() != h) {
            tmp = tmp.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            q->m_texture_size = tmp.size();
        }
    }

    bool needsRebuild = q->m_texture && q->m_texture->pixelSize() != q->m_texture_size;

    if (mipmappingChanged) {
        QRhiTexture::Flags f = q->m_texture->flags();
        f.setFlag(QRhiTexture::MipMapped, hasMipMaps);
        f.setFlag(QRhiTexture::UsedWithGenerateMips, hasMipMaps);
        q->m_texture->setFlags(f);
        needsRebuild = true;
    }

    if (!q->m_texture) {
        QRhiTexture::Flags f;
        if (hasMipMaps)
            f |= QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips;

        q->m_texture = rhi->newTexture(bgra ? QRhiTexture::BGRA8 : QRhiTexture::RGBA8, q->m_texture_size, 1, f);
        needsRebuild = true;
    }

    if (needsRebuild) {
        if (!q->m_texture->build()) {
            qWarning("Failed to build texture for QSGPlainTexture (size %dx%d)",
                     q->m_texture_size.width(), q->m_texture_size.height());
            return;
        }
    }

    if (tmp.width() * 4 != tmp.bytesPerLine())
        tmp = tmp.copy();

    resourceUpdates->uploadTexture(q->m_texture, tmp);

    if (hasMipMaps) {
        resourceUpdates->generateMips(q->m_texture);
        q->m_mipmaps_generated = true;
    }

    m_last_mipmap_filter = q->mipmapFiltering();
    q->m_texture_rect = QRectF(0, 0, 1, 1);

    if (!q->m_retain_image)
        q->m_image = QImage();
}

QT_END_NAMESPACE
