// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgopenvgpainternode.h"
#include "qsgopenvgtexture.h"
#include <private/qsgcontext_p.h>
#include <qmath.h>

#include <QtGui/QPainter>

QT_BEGIN_NAMESPACE

QSGOpenVGPainterNode::QSGOpenVGPainterNode(QQuickPaintedItem *item)
    : m_preferredRenderTarget(QQuickPaintedItem::Image)
    , m_item(item)
    , m_texture(nullptr)
    , m_dirtyContents(false)
    , m_opaquePainting(false)
    , m_linear_filtering(false)
    , m_smoothPainting(false)
    , m_fillColor(Qt::transparent)
    , m_contentsScale(1.0)
    , m_dirtyGeometry(false)
{
    // Set Dummy material and geometry to avoid asserts
    setMaterial((QSGMaterial*)1);
    setGeometry((QSGGeometry*)1);
}

QSGOpenVGPainterNode::~QSGOpenVGPainterNode()
{
    delete m_texture;
}

void QSGOpenVGPainterNode::setPreferredRenderTarget(QQuickPaintedItem::RenderTarget)
{
}

void QSGOpenVGPainterNode::setSize(const QSize &size)
{
    if (size == m_size)
        return;

    m_size = size;

    m_dirtyGeometry = true;
}

void QSGOpenVGPainterNode::setDirty(const QRect &dirtyRect)
{
    m_dirtyContents = true;
    m_dirtyRect = dirtyRect;
    markDirty(DirtyMaterial);
}

void QSGOpenVGPainterNode::setOpaquePainting(bool opaque)
{
    if (opaque == m_opaquePainting)
        return;

    m_opaquePainting = opaque;
}

void QSGOpenVGPainterNode::setLinearFiltering(bool linearFiltering)
{
    if (linearFiltering == m_linear_filtering)
        return;

    m_linear_filtering = linearFiltering;
}

void QSGOpenVGPainterNode::setMipmapping(bool)
{

}

void QSGOpenVGPainterNode::setSmoothPainting(bool s)
{
    if (s == m_smoothPainting)
        return;

    m_smoothPainting = s;
}

void QSGOpenVGPainterNode::setFillColor(const QColor &c)
{
    if (c == m_fillColor)
        return;

    m_fillColor = c;
    markDirty(DirtyMaterial);
}

void QSGOpenVGPainterNode::setContentsScale(qreal s)
{
    if (s == m_contentsScale)
        return;

    m_contentsScale = s;
    markDirty(DirtyMaterial);
}

void QSGOpenVGPainterNode::setFastFBOResizing(bool)
{
}

void QSGOpenVGPainterNode::setTextureSize(const QSize &size)
{
    if (size == m_textureSize)
        return;

    m_textureSize = size;
    m_dirtyGeometry = true;
}

QImage QSGOpenVGPainterNode::toImage() const
{
    return m_image;
}

void QSGOpenVGPainterNode::update()
{
    if (m_dirtyGeometry) {
        if (!m_opaquePainting)
            m_image = QImage(m_size, QImage::Format_ARGB32_Premultiplied);
        else
            m_image = QImage(m_size, QImage::Format_RGB32);
    }

    if (m_dirtyContents)
        paint();

    m_dirtyGeometry = false;
    m_dirtyContents = false;
}

QSGTexture *QSGOpenVGPainterNode::texture() const
{
    return m_texture;
}

void QSGOpenVGPainterNode::render()
{
    if (!m_texture)
        return;

    // Set Draw Mode
    if (opacity() < 1.0) {
        //Transparent
        vgSetPaint(opacityPaint(), VG_FILL_PATH);
        vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_MULTIPLY);
    } else {
        vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
    }

    if (m_linear_filtering)
        vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_BETTER);
    else
        vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_NONANTIALIASED);

    // Set Transform
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
    vgLoadMatrix(transform().constData());

    vgDrawImage(static_cast<VGImage>(m_texture->comparisonKey()));
}

void QSGOpenVGPainterNode::paint()
{
    QRect dirtyRect = m_dirtyRect.isNull() ? QRect(0, 0, m_size.width(), m_size.height()) : m_dirtyRect;

    QPainter painter;

    painter.begin(&m_image);
    if (m_smoothPainting) {
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    }

    QRect clipRect;

    if (m_contentsScale == 1) {
        qreal scaleX = m_textureSize.width() / (qreal) m_size.width();
        qreal scaleY = m_textureSize.height() / (qreal) m_size.height();
        painter.scale(scaleX, scaleY);
        clipRect = dirtyRect;
    } else {
        painter.scale(m_contentsScale, m_contentsScale);

        QRect sclip(qFloor(dirtyRect.x()/m_contentsScale),
                    qFloor(dirtyRect.y()/m_contentsScale),
                    qCeil(dirtyRect.width()/m_contentsScale+dirtyRect.x()/m_contentsScale-qFloor(dirtyRect.x()/m_contentsScale)),
                    qCeil(dirtyRect.height()/m_contentsScale+dirtyRect.y()/m_contentsScale-qFloor(dirtyRect.y()/m_contentsScale)));

        clipRect = sclip;
    }

    if (!m_dirtyRect.isNull())
        painter.setClipRect(clipRect);

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(clipRect, m_fillColor);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    m_item->paint(&painter);
    painter.end();

    m_dirtyRect = QRect();

    if (m_texture)
        delete m_texture;

    uint textureFlags = m_opaquePainting ? 0 : QSGRenderContext::CreateTexture_Alpha;
    m_texture = new QSGOpenVGTexture(m_image, textureFlags);
}

QT_END_NAMESPACE


