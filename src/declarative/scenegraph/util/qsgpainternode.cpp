/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgpainternode_p.h"

#include "qsgpainteditem.h"
#include <private/qsgcontext_p.h>
#include <qglframebufferobject.h>
#include <qglfunctions.h>
#include <qmath.h>

QT_BEGIN_NAMESPACE

#define QT_MINIMUM_FBO_SIZE 64

static inline int qt_next_power_of_two(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    return v;
}

QSGPainterTexture::QSGPainterTexture()
    : QSGPlainTexture()
{

}

void QSGPainterTexture::bind()
{
    if (m_dirty_rect.isNull() || m_texture_id == 0) {
        QSGPlainTexture::bind();
    } else {
        glBindTexture(GL_TEXTURE_2D, m_texture_id);

        QImage subImage = m_image.copy(m_dirty_rect);

        int w = m_dirty_rect.width();
        int h = m_dirty_rect.height();

#ifdef QT_OPENGL_ES
        glTexSubImage2D(GL_TEXTURE_2D, 0, m_dirty_rect.x(), m_dirty_rect.y(), w, h,
                        GL_RGBA, GL_UNSIGNED_BYTE, subImage.constBits());
#else
        glTexSubImage2D(GL_TEXTURE_2D, 0, m_dirty_rect.x(), m_dirty_rect.y(), w, h,
                        GL_BGRA, GL_UNSIGNED_BYTE, subImage.constBits());
#endif

        if (m_has_mipmaps && !m_mipmaps_generated) {
            const QGLContext *ctx = QGLContext::currentContext();
            ctx->functions()->glGenerateMipmap(GL_TEXTURE_2D);
            m_mipmaps_generated = true;
        }

        m_dirty_texture = false;
        m_dirty_bind_options = false;
    }
    m_dirty_rect = QRect();
}

QSGPainterNode::QSGPainterNode(QSGPaintedItem *item)
    : QSGGeometryNode()
    , m_preferredRenderTarget(QSGPaintedItem::Image)
    , m_actualRenderTarget(QSGPaintedItem::Image)
    , m_item(item)
    , m_fbo(0)
    , m_multisampledFbo(0)
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
    , m_texture(0)
    , m_size(1, 1)
    , m_dirtyContents(false)
    , m_opaquePainting(false)
    , m_linear_filtering(false)
    , m_mipmapping(false)
    , m_smoothPainting(false)
    , m_extensionsChecked(false)
    , m_multisamplingSupported(false)
    , m_fillColor(Qt::transparent)
    , m_contentsScale(1.0)
    , m_dirtyGeometry(false)
    , m_dirtyRenderTarget(false)
    , m_dirtyTexture(false)
{
    setMaterial(&m_materialO);
    setOpaqueMaterial(&m_material);
    setGeometry(&m_geometry);
}

QSGPainterNode::~QSGPainterNode()
{
    delete m_texture;
    delete m_fbo;
    delete m_multisampledFbo;
}

void QSGPainterNode::paint()
{
    QRect dirtyRect = m_dirtyRect.isNull() ? QRect(0, 0, m_size.width(), m_size.height()) : m_dirtyRect;

    QPainter painter;
    if (m_actualRenderTarget == QSGPaintedItem::Image)
        painter.begin(&m_image);
    else if (m_multisampledFbo)
        painter.begin(m_multisampledFbo);
    else
        painter.begin(m_fbo);

    if (m_smoothPainting) {
        painter.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing
                               | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    }

    painter.scale(m_contentsScale, m_contentsScale);

    QRect sclip(qFloor(dirtyRect.x()/m_contentsScale),
                qFloor(dirtyRect.y()/m_contentsScale),
                qCeil(dirtyRect.width()/m_contentsScale+dirtyRect.x()/m_contentsScale-qFloor(dirtyRect.x()/m_contentsScale)),
                qCeil(dirtyRect.height()/m_contentsScale+dirtyRect.y()/m_contentsScale-qFloor(dirtyRect.y()/m_contentsScale)));

    if (!m_dirtyRect.isNull())
        painter.setClipRect(sclip);

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(sclip, m_fillColor);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    m_item->paint(&painter);
    painter.end();

    if (m_actualRenderTarget == QSGPaintedItem::Image) {
        m_texture->setImage(m_image);
        m_texture->setDirtyRect(dirtyRect);
    } else if (m_multisampledFbo) {
        QGLFramebufferObject::blitFramebuffer(m_fbo, dirtyRect, m_multisampledFbo, dirtyRect);
    }

    m_dirtyRect = QRect();
}

void QSGPainterNode::update()
{
    if (m_dirtyRenderTarget)
        updateRenderTarget();
    if (m_dirtyGeometry)
        updateGeometry();
    if (m_dirtyTexture)
        updateTexture();

    if (m_dirtyContents)
        paint();

    m_dirtyGeometry = false;
    m_dirtyRenderTarget = false;
    m_dirtyTexture = false;
    m_dirtyContents = false;
}

void QSGPainterNode::updateTexture()
{
    m_texture->setHasMipmaps(m_mipmapping);
    m_texture->setHasAlphaChannel(!m_opaquePainting);
    m_material.setTexture(m_texture);
    m_materialO.setTexture(m_texture);

    markDirty(DirtyMaterial);
}

void QSGPainterNode::updateGeometry()
{
    QRectF source;
    if (m_actualRenderTarget == QSGPaintedItem::Image)
        source = QRectF(0, 0, 1, 1);
    else
        source = QRectF(0, 1, qreal(m_size.width()) / m_fboSize.width(), qreal(-m_size.height()) / m_fboSize.height());
    QSGGeometry::updateTexturedRectGeometry(&m_geometry,
                                            QRectF(0, 0, m_size.width(), m_size.height()),
                                            source);
    markDirty(DirtyGeometry);
}

void QSGPainterNode::updateRenderTarget()
{
    if (!m_extensionsChecked) {
        QList<QByteArray> extensions = QByteArray((const char *)glGetString(GL_EXTENSIONS)).split(' ');
        m_multisamplingSupported = extensions.contains("GL_EXT_framebuffer_multisample")
                && extensions.contains("GL_EXT_framebuffer_blit");
        m_extensionsChecked = true;
    }

    m_dirtyContents = true;

    QSGPaintedItem::RenderTarget oldTarget = m_actualRenderTarget;
    if (m_preferredRenderTarget == QSGPaintedItem::Image) {
        m_actualRenderTarget = QSGPaintedItem::Image;
    } else {
        if (!m_multisamplingSupported && m_smoothPainting)
            m_actualRenderTarget = QSGPaintedItem::Image;
        else
            m_actualRenderTarget = QSGPaintedItem::FramebufferObject;
    }
    if (oldTarget != m_actualRenderTarget) {
        m_image = QImage();
        delete m_fbo;
        delete m_multisampledFbo;
        m_fbo = m_multisampledFbo = 0;
    }

    if (m_actualRenderTarget == QSGPaintedItem::FramebufferObject) {
        const QGLContext *ctx = QGLContext::currentContext();
        if (m_fbo && !m_dirtyGeometry && (!ctx->format().sampleBuffers() || !m_multisamplingSupported))
            return;

        if (m_fboSize.isEmpty())
            updateFBOSize();

        delete m_fbo;
        delete m_multisampledFbo;
        m_fbo = m_multisampledFbo = 0;

        if (m_smoothPainting && ctx->format().sampleBuffers() && m_multisamplingSupported) {
            {
                QGLFramebufferObjectFormat format;
                format.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
                format.setSamples(ctx->format().samples());
                m_multisampledFbo = new QGLFramebufferObject(m_fboSize, format);
            }
            {
                QGLFramebufferObjectFormat format;
                format.setAttachment(QGLFramebufferObject::NoAttachment);
                m_fbo = new QGLFramebufferObject(m_fboSize, format);
            }
        } else {
            QGLFramebufferObjectFormat format;
            format.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
            m_fbo = new QGLFramebufferObject(m_fboSize, format);
        }
    } else {
        if (!m_image.isNull() && !m_dirtyGeometry)
            return;

        m_image = QImage(m_size, QImage::Format_ARGB32_Premultiplied);
        m_image.fill(Qt::transparent);
    }

    QSGPainterTexture *texture = new QSGPainterTexture;
    if (m_actualRenderTarget == QSGPaintedItem::Image) {
        texture->setOwnsTexture(true);
        texture->setTextureSize(m_size);
    } else {
        texture->setTextureId(m_fbo->texture());
        texture->setOwnsTexture(false);
        texture->setTextureSize(m_fboSize);
    }

    if (m_texture)
        delete m_texture;

    texture->setTextureSize(m_size);
    m_texture = texture;
}

void QSGPainterNode::updateFBOSize()
{
    int fboWidth = qMax(QT_MINIMUM_FBO_SIZE, qt_next_power_of_two(m_size.width()));
    int fboHeight = qMax(QT_MINIMUM_FBO_SIZE, qt_next_power_of_two(m_size.height()));
    m_fboSize = QSize(fboWidth, fboHeight);
}

void QSGPainterNode::setPreferredRenderTarget(QSGPaintedItem::RenderTarget target)
{
    if (m_preferredRenderTarget == target)
        return;

    m_preferredRenderTarget = target;

    m_dirtyRenderTarget = true;
    m_dirtyGeometry = true;
    m_dirtyTexture = true;
}

void QSGPainterNode::setSize(const QSize &size)
{
    if (size == m_size)
        return;

    m_size = size;
    updateFBOSize();

    if (m_fbo)
        m_dirtyRenderTarget = m_fbo->size() != m_fboSize || m_dirtyRenderTarget;
    else
        m_dirtyRenderTarget = true;
    m_dirtyGeometry = true;
    m_dirtyTexture = true;
}

void QSGPainterNode::setDirty(bool d, const QRect &dirtyRect)
{
    m_dirtyContents = d;
    m_dirtyRect = dirtyRect;

    if (m_mipmapping)
        m_dirtyTexture = true;

    markDirty(DirtyMaterial);
}

void QSGPainterNode::setOpaquePainting(bool opaque)
{
    if (opaque == m_opaquePainting)
        return;

    m_opaquePainting = opaque;
    m_dirtyTexture = true;
}

void QSGPainterNode::setLinearFiltering(bool linearFiltering)
{
    if (linearFiltering == m_linear_filtering)
        return;

    m_linear_filtering = linearFiltering;

    m_material.setFiltering(linearFiltering ? QSGTexture::Linear : QSGTexture::Nearest);
    m_materialO.setFiltering(linearFiltering ? QSGTexture::Linear : QSGTexture::Nearest);
    markDirty(DirtyMaterial);
}

void QSGPainterNode::setMipmapping(bool mipmapping)
{
    if (mipmapping == m_mipmapping)
        return;

    m_mipmapping = mipmapping;
    m_material.setMipmapFiltering(mipmapping ? QSGTexture::Linear : QSGTexture::None);
    m_materialO.setMipmapFiltering(mipmapping ? QSGTexture::Linear : QSGTexture::None);
    m_dirtyTexture = true;
}

void QSGPainterNode::setSmoothPainting(bool s)
{
    if (s == m_smoothPainting)
        return;

    m_smoothPainting = s;
    m_dirtyRenderTarget = true;
}

void QSGPainterNode::setFillColor(const QColor &c)
{
    if (c == m_fillColor)
        return;

    m_fillColor = c;
    markDirty(DirtyMaterial);
}

void QSGPainterNode::setContentsScale(qreal s)
{
    if (s == m_contentsScale)
        return;

    m_contentsScale = s;
    markDirty(DirtyMaterial);
}

QImage QSGPainterNode::toImage() const
{
    if (m_actualRenderTarget == QSGPaintedItem::Image)
        return m_image;
    else
        return m_fbo->toImage();
}

QT_END_NAMESPACE
