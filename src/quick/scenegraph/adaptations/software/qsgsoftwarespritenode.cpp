// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgsoftwarespritenode_p.h"
#include "qsgsoftwarepixmaptexture_p.h"
#include <QtGui/QPainter>

QT_BEGIN_NAMESPACE

QSGSoftwareSpriteNode::QSGSoftwareSpriteNode()
{
    setMaterial((QSGMaterial*)1);
    setGeometry((QSGGeometry*)1);
}

QSGSoftwareSpriteNode::~QSGSoftwareSpriteNode()
{
    delete m_texture;
}

void QSGSoftwareSpriteNode::setTexture(QSGTexture *texture)
{
    m_texture = qobject_cast<QSGSoftwarePixmapTexture*>(texture);
    markDirty(DirtyMaterial);
}

void QSGSoftwareSpriteNode::setTime(float time)
{
    if (m_time != time) {
        m_time = time;
        markDirty(DirtyMaterial);
    }
}

void QSGSoftwareSpriteNode::setSourceA(const QPoint &source)
{
    if (m_sourceA != source) {
        m_sourceA = source;
        markDirty(DirtyMaterial);
    }
}

void QSGSoftwareSpriteNode::setSourceB(const QPoint &source)
{
    if (m_sourceB != source) {
        m_sourceB = source;
        markDirty(DirtyMaterial);
    }
}

void QSGSoftwareSpriteNode::setSpriteSize(const QSize &size)
{
    if (m_spriteSize != size) {
        m_spriteSize = size;
        markDirty(DirtyMaterial);
    }
}

void QSGSoftwareSpriteNode::setSheetSize(const QSize &size)
{
    if (m_sheetSize != size) {
        m_sheetSize = size;
        markDirty(DirtyMaterial);
    }
}

void QSGSoftwareSpriteNode::setSize(const QSizeF &size)
{
    if (m_size != size) {
        m_size = size;
        markDirty(DirtyGeometry);
    }
}

void QSGSoftwareSpriteNode::setFiltering(QSGTexture::Filtering filtering)
{
    Q_UNUSED(filtering);
}

void QSGSoftwareSpriteNode::update()
{
}

void QSGSoftwareSpriteNode::paint(QPainter *painter)
{
    //Get the pixmap handle from the texture
    if (!m_texture)
        return;

    const QPixmap &pixmap = m_texture->pixmap();

    // XXX try to do some kind of interpolation between sourceA and sourceB using time
    painter->drawPixmap(QRectF(0, 0, m_size.width(), m_size.height()),
                        pixmap,
                        QRectF(m_sourceA * pixmap.devicePixelRatio(), m_spriteSize * pixmap.devicePixelRatio()));
}

bool QSGSoftwareSpriteNode::isOpaque() const
{
    return false;
}

QRectF QSGSoftwareSpriteNode::rect() const
{
    return QRectF(0, 0, m_size.width(), m_size.height());
}

QT_END_NAMESPACE
