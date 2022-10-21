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

#include "qsgdefaultpainternode_p.h"

#include <QtQuick/private/qquickpainteditem_p.h>

#include <QtQuick/private/qsgdefaultrendercontext_p.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <qmath.h>
#include <qpainter.h>

QT_BEGIN_NAMESPACE

#define QT_MINIMUM_DYNAMIC_FBO_SIZE 64U

QSGPainterTexture::QSGPainterTexture()
    : QSGPlainTexture(*(new QSGPlainTexturePrivate(this)))
{
    m_retain_image = true;
}

void QSGPainterTexture::commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    if (!m_dirty_rect.isNull()) {
        setImage(m_image);
        m_dirty_rect = QRect();
    }
    QSGPlainTexture::commitTextureOperations(rhi, resourceUpdates);
}

QSGDefaultPainterNode::QSGDefaultPainterNode(QQuickPaintedItem *item)
    : QSGPainterNode()
    , m_preferredRenderTarget(QQuickPaintedItem::Image)
    , m_actualRenderTarget(QQuickPaintedItem::Image)
    , m_item(item)
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
    , m_texture(nullptr)
    , m_fillColor(Qt::transparent)
    , m_contentsScale(1.0)
    , m_dirtyContents(false)
    , m_opaquePainting(false)
    , m_linear_filtering(false)
    , m_mipmapping(false)
    , m_smoothPainting(false)
    , m_multisamplingSupported(false)
    , m_fastFBOResizing(false)
    , m_dirtyGeometry(false)
    , m_dirtyRenderTarget(false)
    , m_dirtyTexture(false)
{
    Q_UNUSED(m_multisamplingSupported);
    m_context = static_cast<QSGDefaultRenderContext *>(static_cast<QQuickPaintedItemPrivate *>(QObjectPrivate::get(item))->sceneGraphRenderContext());

    setMaterial(&m_materialO);
    setOpaqueMaterial(&m_material);
    setGeometry(&m_geometry);

#ifdef QSG_RUNTIME_DESCRIPTION
    qsgnode_set_description(this, QString::fromLatin1("QQuickPaintedItem(%1):%2").arg(QString::fromLatin1(item->metaObject()->className())).arg(item->objectName()));
#endif
}

QSGDefaultPainterNode::~QSGDefaultPainterNode()
{
    delete m_texture;
}

void QSGDefaultPainterNode::paint()
{
    QRect dirtyRect = m_dirtyRect.isNull() ? QRect(0, 0, m_size.width(), m_size.height()) : m_dirtyRect;

    QPainter painter;
    Q_ASSERT(m_actualRenderTarget == QQuickPaintedItem::Image);
    if (m_image.isNull())
        return;
    painter.begin(&m_image);

    if (m_smoothPainting) {
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    }

    QRect clipRect;
    QRect dirtyTextureRect;

    if (m_contentsScale == 1) {
        qreal scaleX = m_textureSize.width() / (qreal) m_size.width();
        qreal scaleY = m_textureSize.height() / (qreal) m_size.height();
        painter.scale(scaleX, scaleY);
        clipRect = dirtyRect;
        dirtyTextureRect = QRectF(dirtyRect.x() * scaleX,
                                  dirtyRect.y() * scaleY,
                                  dirtyRect.width() * scaleX,
                                  dirtyRect.height() * scaleY).toAlignedRect();
    } else {
        painter.scale(m_contentsScale, m_contentsScale);
        QRect sclip(qFloor(dirtyRect.x()/m_contentsScale),
                    qFloor(dirtyRect.y()/m_contentsScale),
                    qCeil(dirtyRect.width()/m_contentsScale+dirtyRect.x()/m_contentsScale-qFloor(dirtyRect.x()/m_contentsScale)),
                    qCeil(dirtyRect.height()/m_contentsScale+dirtyRect.y()/m_contentsScale-qFloor(dirtyRect.y()/m_contentsScale)));
        clipRect = sclip;
        dirtyTextureRect = dirtyRect;
    }

    // only clip if we were originally updating only a subrect
    if (!m_dirtyRect.isNull()) {
        painter.setClipRect(clipRect);
    }

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(clipRect, m_fillColor);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    m_item->paint(&painter);
    painter.end();

    m_texture->setImage(m_image);
    m_texture->setDirtyRect(dirtyTextureRect);

    m_dirtyRect = QRect();
}

void QSGDefaultPainterNode::update()
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

void QSGDefaultPainterNode::updateTexture()
{
    m_texture->setHasAlphaChannel(!m_opaquePainting);
    m_material.setTexture(m_texture);
    m_materialO.setTexture(m_texture);

    markDirty(DirtyMaterial);
}

void QSGDefaultPainterNode::updateGeometry()
{
    QRectF source(0, 0, 1, 1);
    QRectF dest(0, 0, m_size.width(), m_size.height());
    if (m_actualRenderTarget == QQuickPaintedItem::InvertedYFramebufferObject)
        dest = QRectF(QPointF(0, m_size.height()), QPointF(m_size.width(), 0));
    QSGGeometry::updateTexturedRectGeometry(&m_geometry,
                                            dest,
                                            source);
    markDirty(DirtyGeometry);
}

void QSGDefaultPainterNode::updateRenderTarget()
{
    m_dirtyContents = true;

    m_actualRenderTarget = QQuickPaintedItem::Image;
    if (!m_image.isNull() && !m_dirtyGeometry)
        return;

    m_image = QImage(m_textureSize, QImage::Format_ARGB32_Premultiplied);
    m_image.fill(Qt::transparent);

    if (!m_texture) {
        m_texture = new QSGPainterTexture;
        m_texture->setOwnsTexture(true);
    }
    m_texture->setTextureSize(m_textureSize);
}

void QSGDefaultPainterNode::setPreferredRenderTarget(QQuickPaintedItem::RenderTarget target)
{
    if (m_preferredRenderTarget == target)
        return;

    m_preferredRenderTarget = target;

    m_dirtyRenderTarget = true;
    m_dirtyGeometry = true;
    m_dirtyTexture = true;
}

void QSGDefaultPainterNode::setSize(const QSize &size)
{
    if (size == m_size)
        return;

    m_size = size;
    m_dirtyGeometry = true;
}

void QSGDefaultPainterNode::setTextureSize(const QSize &size)
{
    if (size == m_textureSize)
        return;

    m_textureSize = size;
    m_dirtyRenderTarget = true;
    m_dirtyGeometry = true;
    m_dirtyTexture = true;
}

void QSGDefaultPainterNode::setDirty(const QRect &dirtyRect)
{
    m_dirtyContents = true;
    m_dirtyRect = dirtyRect;

    if (m_mipmapping)
        m_dirtyTexture = true;

    markDirty(DirtyMaterial);
}

void QSGDefaultPainterNode::setOpaquePainting(bool opaque)
{
    if (opaque == m_opaquePainting)
        return;

    m_opaquePainting = opaque;
    m_dirtyTexture = true;
}

void QSGDefaultPainterNode::setLinearFiltering(bool linearFiltering)
{
    if (linearFiltering == m_linear_filtering)
        return;

    m_linear_filtering = linearFiltering;

    m_material.setFiltering(linearFiltering ? QSGTexture::Linear : QSGTexture::Nearest);
    m_materialO.setFiltering(linearFiltering ? QSGTexture::Linear : QSGTexture::Nearest);
    markDirty(DirtyMaterial);
}

void QSGDefaultPainterNode::setMipmapping(bool mipmapping)
{
    if (mipmapping == m_mipmapping)
        return;

    m_mipmapping = mipmapping;
    m_material.setMipmapFiltering(mipmapping ? QSGTexture::Linear : QSGTexture::None);
    m_materialO.setMipmapFiltering(mipmapping ? QSGTexture::Linear : QSGTexture::None);
    m_dirtyTexture = true;
}

void QSGDefaultPainterNode::setSmoothPainting(bool s)
{
    if (s == m_smoothPainting)
        return;

    m_smoothPainting = s;
    m_dirtyRenderTarget = true;
}

void QSGDefaultPainterNode::setFillColor(const QColor &c)
{
    if (c == m_fillColor)
        return;

    m_fillColor = c;
    markDirty(DirtyMaterial);
}

void QSGDefaultPainterNode::setContentsScale(qreal s)
{
    if (s == m_contentsScale)
        return;

    m_contentsScale = s;
    markDirty(DirtyMaterial);
}

void QSGDefaultPainterNode::setFastFBOResizing(bool fastResizing)
{
    if (m_fastFBOResizing == fastResizing)
        return;

    m_fastFBOResizing = fastResizing;
}

QImage QSGDefaultPainterNode::toImage() const
{
    Q_ASSERT(m_actualRenderTarget == QQuickPaintedItem::Image);
    return m_image;
}

QT_END_NAMESPACE
