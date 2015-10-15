/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of Qt Quick 2d Renderer module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "softwarelayer.h"

#include "context.h"

SoftwareLayer::SoftwareLayer(QSGRenderContext *renderContext)
    : m_item(0)
    , m_context(renderContext)
    , m_renderer(0)
    , m_device_pixel_ratio(1)
    , m_mirrorHorizontal(false)
    , m_mirrorVertical(false)
    , m_live(true)
    , m_grab(true)
    , m_recursive(false)
    , m_dirtyTexture(true)
{

}

SoftwareLayer::~SoftwareLayer()
{
    invalidated();
}

int SoftwareLayer::textureId() const
{
    return 0;
}

QSize SoftwareLayer::textureSize() const
{
    return m_pixmap.size();
}

bool SoftwareLayer::hasAlphaChannel() const
{
    return m_pixmap.hasAlphaChannel();
}

bool SoftwareLayer::hasMipmaps() const
{
    return false;
}

void SoftwareLayer::bind()
{
}

bool SoftwareLayer::updateTexture()
{
    bool doGrab = (m_live || m_grab) && m_dirtyTexture;
    if (doGrab)
        grab();
    if (m_grab)
        emit scheduledUpdateCompleted();
    m_grab = false;
    return doGrab;
}

void SoftwareLayer::setItem(QSGNode *item)
{
    if (item == m_item)
        return;
    m_item = item;

    if (m_live && !m_item)
        m_pixmap = QPixmap();

    markDirtyTexture();
}

void SoftwareLayer::setRect(const QRectF &rect)
{
    if (rect == m_rect)
        return;
    m_rect = rect;
    markDirtyTexture();
}

void SoftwareLayer::setSize(const QSize &size)
{
    if (size == m_size)
        return;
    m_size = size;

    if (m_live && m_size.isNull())
        m_pixmap = QPixmap();

    markDirtyTexture();
}

void SoftwareLayer::scheduleUpdate()
{
    if (m_grab)
        return;
    m_grab = true;
    if (m_dirtyTexture) {
        emit updateRequested();
    }
}

QImage SoftwareLayer::toImage() const
{
    return m_pixmap.toImage();
}

void SoftwareLayer::setLive(bool live)
{
    if (live == m_live)
        return;
    m_live = live;

    if (m_live && (!m_item || m_size.isNull()))
        m_pixmap = QPixmap();

    markDirtyTexture();
}

void SoftwareLayer::setRecursive(bool recursive)
{
    m_recursive = recursive;
}

void SoftwareLayer::setFormat(GLenum)
{
}

void SoftwareLayer::setHasMipmaps(bool)
{
}

void SoftwareLayer::setDevicePixelRatio(qreal ratio)
{
    m_device_pixel_ratio = ratio;
}

void SoftwareLayer::setMirrorHorizontal(bool mirror)
{
    if (m_mirrorHorizontal == mirror)
        return;
    m_mirrorHorizontal = mirror;
    markDirtyTexture();
}

void SoftwareLayer::setMirrorVertical(bool mirror)
{
    if (m_mirrorVertical == mirror)
        return;
    m_mirrorVertical = mirror;
    markDirtyTexture();
}

void SoftwareLayer::markDirtyTexture()
{
    m_dirtyTexture = true;
    if (m_live || m_grab) {
        emit updateRequested();
    }
}

void SoftwareLayer::invalidated()
{
    delete m_renderer;
    m_renderer = 0;
}

void SoftwareLayer::grab()
{
    if (!m_item || m_size.isNull()) {
        m_pixmap = QPixmap();
        m_dirtyTexture = false;
        return;
    }
    QSGNode *root = m_item;
    while (root->firstChild() && root->type() != QSGNode::RootNodeType)
        root = root->firstChild();
    if (root->type() != QSGNode::RootNodeType)
        return;

    if (!m_renderer) {
        m_renderer = new SoftwareContext::PixmapRenderer(m_context);
        connect(m_renderer, SIGNAL(sceneGraphChanged()), this, SLOT(markDirtyTexture()));
    }
    m_renderer->setDevicePixelRatio(m_device_pixel_ratio);
    m_renderer->setRootNode(static_cast<QSGRootNode *>(root));

    if (m_pixmap.size() != m_size) {
        m_pixmap = QPixmap(m_size);
        m_pixmap.setDevicePixelRatio(m_device_pixel_ratio);
    }

    // Render texture.
    root->markDirty(QSGNode::DirtyForceUpdate); // Force matrix, clip and opacity update.
    m_renderer->nodeChanged(root, QSGNode::DirtyForceUpdate); // Force render list update.

    m_dirtyTexture = false;

    m_renderer->setDeviceRect(m_size);
    m_renderer->setViewportRect(m_size);
    QRect mirrored(m_mirrorHorizontal ? m_rect.right() * m_device_pixel_ratio : m_rect.left() * m_device_pixel_ratio,
                   m_mirrorVertical ? m_rect.top() * m_device_pixel_ratio : m_rect.bottom() * m_device_pixel_ratio,
                   m_mirrorHorizontal ? -m_rect.width() * m_device_pixel_ratio : m_rect.width() * m_device_pixel_ratio,
                   m_mirrorVertical ? m_rect.height() * m_device_pixel_ratio : -m_rect.height() * m_device_pixel_ratio);
    m_renderer->m_projectionRect = mirrored;
    m_renderer->setClearColor(Qt::transparent);

    m_renderer->renderScene();
    m_renderer->render(&m_pixmap);

    root->markDirty(QSGNode::DirtyForceUpdate); // Force matrix, clip, opacity and render list update.

    if (m_recursive)
        markDirtyTexture(); // Continuously update if 'live' and 'recursive'.
}
