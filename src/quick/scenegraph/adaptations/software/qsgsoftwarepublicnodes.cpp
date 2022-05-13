// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgsoftwarepublicnodes_p.h"
#include "qsgsoftwarelayer_p.h"
#include "qsgsoftwarepixmaptexture_p.h"
#include "qsgsoftwareinternalimagenode_p.h"
#include <private/qsgplaintexture_p.h>

QT_BEGIN_NAMESPACE

QSGSoftwareRectangleNode::QSGSoftwareRectangleNode()
    : m_color(QColor(255, 255, 255))
{
    setMaterial((QSGMaterial*)1);
    setGeometry((QSGGeometry*)1);
}

void QSGSoftwareRectangleNode::paint(QPainter *painter)
{
    painter->fillRect(m_rect, m_color);
}

QSGSoftwareImageNode::QSGSoftwareImageNode()
    : m_texture(nullptr),
      m_owns(false),
      m_filtering(QSGTexture::None),
      m_transformMode(NoTransform),
      m_cachedMirroredPixmapIsDirty(false)
{
    setMaterial((QSGMaterial*)1);
    setGeometry((QSGGeometry*)1);
}

QSGSoftwareImageNode::~QSGSoftwareImageNode()
{
    if (m_owns)
        delete m_texture;
}

void QSGSoftwareImageNode::setTexture(QSGTexture *texture)
{
    if (m_owns)
        delete m_texture;

    m_texture = texture; markDirty(DirtyMaterial);
    m_cachedMirroredPixmapIsDirty = true;
}

void QSGSoftwareImageNode::setTextureCoordinatesTransform(QSGImageNode::TextureCoordinatesTransformMode transformNode)
{
    if (m_transformMode == transformNode)
        return;

    m_transformMode = transformNode;
    m_cachedMirroredPixmapIsDirty = true;

    markDirty(DirtyGeometry);
}

void QSGSoftwareImageNode::paint(QPainter *painter)
{
    if (m_cachedMirroredPixmapIsDirty)
        updateCachedMirroredPixmap();

    painter->setRenderHint(QPainter::SmoothPixmapTransform, (m_filtering == QSGTexture::Linear));
    // Disable antialiased clipping. It causes transformed tiles to have gaps.
    painter->setRenderHint(QPainter::Antialiasing, false);

    if (!m_cachedPixmap.isNull()) {
        painter->drawPixmap(m_rect, m_cachedPixmap, m_sourceRect);
    } else if (QSGSoftwarePixmapTexture *pt = qobject_cast<QSGSoftwarePixmapTexture *>(m_texture)) {
        const QPixmap &pm = pt->pixmap();
        painter->drawPixmap(m_rect, pm, m_sourceRect);
    } else if (QSGSoftwareLayer *pt = qobject_cast<QSGSoftwareLayer *>(m_texture)) {
        const QPixmap &pm = pt->pixmap();
        painter->drawPixmap(m_rect, pm, m_sourceRect);
    } else if (QSGPlainTexture *pt = qobject_cast<QSGPlainTexture *>(m_texture)) {
        const QImage &im = pt->image();
        painter->drawImage(m_rect, im, m_sourceRect);
    }
}

void QSGSoftwareImageNode::updateCachedMirroredPixmap()
{
    if (m_transformMode == NoTransform) {
        m_cachedPixmap = QPixmap();
    } else {
        if (QSGSoftwarePixmapTexture *pt = qobject_cast<QSGSoftwarePixmapTexture *>(m_texture)) {
            QTransform mirrorTransform;
            if (m_transformMode.testFlag(MirrorVertically))
                mirrorTransform = mirrorTransform.scale(1, -1);
            if (m_transformMode.testFlag(MirrorHorizontally))
                mirrorTransform = mirrorTransform.scale(-1, 1);
            m_cachedPixmap = pt->pixmap().transformed(mirrorTransform);
        } else if (QSGSoftwareLayer *pt = qobject_cast<QSGSoftwareLayer *>(m_texture)) {
            QTransform mirrorTransform;
            if (m_transformMode.testFlag(MirrorVertically))
                mirrorTransform = mirrorTransform.scale(1, -1);
            if (m_transformMode.testFlag(MirrorHorizontally))
                mirrorTransform = mirrorTransform.scale(-1, 1);
            m_cachedPixmap = pt->pixmap().transformed(mirrorTransform);
        } else if (QSGPlainTexture *pt = qobject_cast<QSGPlainTexture *>(m_texture)) {
            m_cachedPixmap = QPixmap::fromImage(pt->image().mirrored(m_transformMode.testFlag(MirrorHorizontally), m_transformMode.testFlag(MirrorVertically)));
        } else {
            m_cachedPixmap = QPixmap();
        }
    }

    m_cachedMirroredPixmapIsDirty = false;
}

QSGSoftwareNinePatchNode::QSGSoftwareNinePatchNode()
{
    setMaterial((QSGMaterial*)1);
    setGeometry((QSGGeometry*)1);
}

void QSGSoftwareNinePatchNode::setTexture(QSGTexture *texture)
{
    QSGSoftwarePixmapTexture *pt = qobject_cast<QSGSoftwarePixmapTexture*>(texture);
    if (!pt) {
        qWarning() << "Image used with invalid texture format.";
    } else {
        m_pixmap = pt->pixmap();
        markDirty(DirtyMaterial);
    }
    delete texture;
}

void QSGSoftwareNinePatchNode::setBounds(const QRectF &bounds)
{
    if (m_bounds == bounds)
        return;

    m_bounds = bounds;
    markDirty(DirtyGeometry);
}

void QSGSoftwareNinePatchNode::setDevicePixelRatio(qreal ratio)
{
    if (m_pixelRatio == ratio)
        return;

    m_pixelRatio = ratio;
    markDirty(DirtyGeometry);
}

void QSGSoftwareNinePatchNode::setPadding(qreal left, qreal top, qreal right, qreal bottom)
{
    QMargins margins(qRound(left), qRound(top), qRound(right), qRound(bottom));
    if (m_margins == margins)
        return;

    m_margins = QMargins(qRound(left), qRound(top), qRound(right), qRound(bottom));
    markDirty(DirtyGeometry);
}

void QSGSoftwareNinePatchNode::update()
{
}

void QSGSoftwareNinePatchNode::paint(QPainter *painter)
{
    // Disable antialiased clipping. It causes transformed tiles to have gaps.
    painter->setRenderHint(QPainter::Antialiasing, false);

    if (m_margins.isNull())
        painter->drawPixmap(m_bounds, m_pixmap, QRectF(0, 0, m_pixmap.width(), m_pixmap.height()));
    else
        QSGSoftwareHelpers::qDrawBorderPixmap(painter, m_bounds.toRect(), m_margins, m_pixmap, QRect(0, 0, m_pixmap.width(), m_pixmap.height()),
                                              m_margins, Qt::StretchTile, QSGSoftwareHelpers::QDrawBorderPixmap::DrawingHints{});
}

QRectF QSGSoftwareNinePatchNode::bounds() const
{
    return m_bounds;
}

QT_END_NAMESPACE
