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

#include "qsgd3d12layer_p.h"
#include "qsgd3d12rendercontext_p.h"
#include "qsgd3d12engine_p.h"
#include "qsgd3d12renderer_p.h"

QT_BEGIN_NAMESPACE

// NOTE: Avoid categorized logging. It is slow.

#define DECLARE_DEBUG_VAR(variable) \
    static bool debug_ ## variable() \
    { static bool value = qgetenv("QSG_RENDERER_DEBUG").contains(QT_STRINGIFY(variable)); return value; }

DECLARE_DEBUG_VAR(render)

QSGD3D12Layer::QSGD3D12Layer(QSGD3D12RenderContext *rc)
    : m_rc(rc)
{
    if (Q_UNLIKELY(debug_render()))
        qDebug("new layer %p", this);
}

QSGD3D12Layer::~QSGD3D12Layer()
{
    if (Q_UNLIKELY(debug_render()))
        qDebug("destroying layer %p", this);

    cleanup();
}

// QSGTexture

int QSGD3D12Layer::textureId() const
{
    return m_rt; // not a texture id per se but will do
}

QSize QSGD3D12Layer::textureSize() const
{
    return m_size;
}

bool QSGD3D12Layer::hasAlphaChannel() const
{
    return true;
}

bool QSGD3D12Layer::hasMipmaps() const
{
    // mipmapped layers are not supported for now
    return false;
}

QRectF QSGD3D12Layer::normalizedTextureSubRect() const
{
    // (0, 0) is the top-left corner, unlike OpenGL. Hence the inversion of
    // m_mirrorVertical, as opposed to the GL version.
    return QRectF(m_mirrorHorizontal ? 1 : 0,
                  m_mirrorVertical ? 1 : 0,
                  m_mirrorHorizontal ? -1 : 1,
                  m_mirrorVertical ? -1 : 1);
}

void QSGD3D12Layer::bind()
{
    if (Q_UNLIKELY(debug_render()))
        qDebug("layer %p bind rt=%u", this, m_rt);

    // ###

    m_rc->engine()->activateRenderTargetAsTexture(m_rt);
}

// QSGDynamicTexture

bool QSGD3D12Layer::updateTexture()
{
    if (Q_UNLIKELY(debug_render()))
        qDebug("layer %p updateTexture", this);

    const bool doUpdate = (m_live || m_updateContentPending) && m_dirtyTexture;

    if (doUpdate)
        updateContent();

    if (m_updateContentPending) {
        m_updateContentPending = false;
        emit scheduledUpdateCompleted();
    }

    return doUpdate;
}

// QSGLayer

void QSGD3D12Layer::setItem(QSGNode *item)
{
    if (m_item == item)
        return;

    if (m_live && !item)
        resetRenderTarget();

    m_item = item;
    markDirtyTexture();
}

void QSGD3D12Layer::setRect(const QRectF &rect)
{
    if (m_rect == rect)
        return;

    m_rect = rect;
    markDirtyTexture();
}

void QSGD3D12Layer::setSize(const QSize &size)
{
    if (m_size == size)
        return;

    if (m_live && size.isNull())
        resetRenderTarget();

    m_size = size;
    markDirtyTexture();
}

void QSGD3D12Layer::scheduleUpdate()
{
    if (m_updateContentPending)
        return;

    if (Q_UNLIKELY(debug_render()))
        qDebug("layer %p scheduleUpdate", this);

    m_updateContentPending = true;

    if (m_dirtyTexture)
        emit updateRequested();
}

QImage QSGD3D12Layer::toImage() const
{
    // ### figure out something for item grab support
    return QImage();
}

void QSGD3D12Layer::setLive(bool live)
{
    if (m_live == live)
        return;

    if (live && (!m_item || m_size.isNull()))
        resetRenderTarget();

    m_live = live;
    markDirtyTexture();
}

void QSGD3D12Layer::setRecursive(bool recursive)
{
    m_recursive = recursive;
}

void QSGD3D12Layer::setFormat(uint format)
{
    Q_UNUSED(format);
}

void QSGD3D12Layer::setHasMipmaps(bool mipmap)
{
    // mipmapped layers are not supported for now
    Q_UNUSED(mipmap);
}

void QSGD3D12Layer::setDevicePixelRatio(qreal ratio)
{
    m_dpr = ratio;
}

void QSGD3D12Layer::setMirrorHorizontal(bool mirror)
{
    m_mirrorHorizontal = mirror;
}

void QSGD3D12Layer::setMirrorVertical(bool mirror)
{
    m_mirrorVertical = mirror;
}

void QSGD3D12Layer::markDirtyTexture()
{
    if (Q_UNLIKELY(debug_render()))
        qDebug("layer %p markDirtyTexture", this);

    m_dirtyTexture = true;

    if (m_live || m_updateContentPending)
        emit updateRequested();
}

void QSGD3D12Layer::invalidated()
{
    cleanup();
}

void QSGD3D12Layer::cleanup()
{
    if (!m_renderer && !m_rt)
        return;

    if (Q_UNLIKELY(debug_render()))
        qDebug("layer %p cleanup renderer=%p rt=%u", this, m_renderer, m_rt);

    delete m_renderer;
    m_renderer = nullptr;

    resetRenderTarget();
}

void QSGD3D12Layer::resetRenderTarget()
{
    if (!m_rt)
        return;

    if (Q_UNLIKELY(debug_render()))
        qDebug("layer %p resetRenderTarget rt=%u", this, m_rt);

    m_rc->engine()->releaseRenderTarget(m_rt);
    m_rt = 0;
}

void QSGD3D12Layer::updateContent()
{
    if (Q_UNLIKELY(debug_render()))
        qDebug("layer %p updateContent", this);

    if (!m_item || m_size.isNull()) {
        resetRenderTarget();
        m_dirtyTexture = false;
        return;
    }

    QSGNode *root = m_item;
    while (root->firstChild() && root->type() != QSGNode::RootNodeType)
        root = root->firstChild();

    if (root->type() != QSGNode::RootNodeType)
        return;

    if (!m_renderer) {
        m_renderer = m_rc->createRenderer();
        static_cast<QSGD3D12Renderer *>(m_renderer)->turnToLayerRenderer();
        connect(m_renderer, &QSGRenderer::sceneGraphChanged, this, &QSGD3D12Layer::markDirtyTexture);
    }

    m_renderer->setDevicePixelRatio(m_dpr);
    m_renderer->setRootNode(static_cast<QSGRootNode *>(root));

    if (!m_rt || m_rtSize != m_size) {
        // ### recursive, multisample

        if (m_rt)
            resetRenderTarget();

        m_rt = m_rc->engine()->genRenderTarget();
        m_rtSize = m_size;
        m_rc->engine()->createRenderTarget(m_rt, m_rtSize, QVector4D(0, 0, 0, 0), 0);
    }

    m_dirtyTexture = false;

    m_renderer->setDeviceRect(m_size);
    m_renderer->setViewportRect(m_size);
    QRectF mirrored(m_mirrorHorizontal ? m_rect.right() : m_rect.left(),
                    m_mirrorVertical ? m_rect.bottom() : m_rect.top(),
                    m_mirrorHorizontal ? -m_rect.width() : m_rect.width(),
                    m_mirrorVertical ? -m_rect.height() : m_rect.height());
    m_renderer->setProjectionMatrixToRect(mirrored);
    m_renderer->setClearColor(Qt::transparent);

    // ### recursive, multisample
    m_renderer->renderScene(m_rt);

    if (m_recursive)
        markDirtyTexture(); // Continuously update if 'live' and 'recursive'.
}

QT_END_NAMESPACE
