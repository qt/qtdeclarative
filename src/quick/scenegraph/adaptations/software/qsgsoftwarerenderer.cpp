// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsgsoftwarerenderer_p.h"

#include "qsgsoftwarerenderablenodeupdater_p.h"
#include "qsgsoftwarerenderlistbuilder_p.h"
#include "qsgsoftwarecontext_p.h"
#include "qsgsoftwarerenderablenode_p.h"

#include <QtGui/QPaintDevice>
#include <QtGui/QBackingStore>
#include <QElapsedTimer>

Q_STATIC_LOGGING_CATEGORY(lcRenderer, "qt.scenegraph.softwarecontext.renderer")

QT_BEGIN_NAMESPACE

QSGSoftwareRenderer::QSGSoftwareRenderer(QSGRenderContext *context)
    : QSGAbstractSoftwareRenderer(context)
    , m_paintDevice(nullptr)
    , m_backingStore(nullptr)
{
    // Check if the environment variable is set to force/disable partial updates
    static bool partialUpdateOverrideSet = false;
    static const int forcePartialUpdates = qEnvironmentVariableIntValue("QSG_SOFTWARE_RENDERER_FORCE_PARTIAL_UPDATES", &partialUpdateOverrideSet);
    if (partialUpdateOverrideSet) {
        if (forcePartialUpdates == 0)
            m_partialUpdateMode = DisablePartialUpdate;
        else
            m_partialUpdateMode = ForcePartialUpdate;
    }
}

QSGSoftwareRenderer::~QSGSoftwareRenderer()
{
}

void QSGSoftwareRenderer::setCurrentPaintDevice(QPaintDevice *device)
{
    m_paintDevice = device;
    m_backingStore = nullptr;
}

QPaintDevice *QSGSoftwareRenderer::currentPaintDevice() const
{
    return m_paintDevice;
}

void QSGSoftwareRenderer::setBackingStore(QBackingStore *backingStore)
{
    if (backingStore != m_backingStore)
        m_backingStoreSize = QSize();
    m_backingStore = backingStore;
    m_paintDevice = nullptr;
}

QRegion QSGSoftwareRenderer::flushRegion() const
{
    return m_flushRegion;
}

void QSGSoftwareRenderer::renderScene()
{
    QSGRenderer::renderScene();
}

void QSGSoftwareRenderer::render()
{
    if (!m_paintDevice && !m_backingStore && !m_rt.paintDevice)
        return;

    QPaintDevice *paintDevice = m_paintDevice ? m_paintDevice : m_rt.paintDevice;
    QSize paintSize;
    qreal paintDevicePixelRatio = 1.0;

    if (paintDevice) {
        paintSize = QSize(paintDevice->width(), paintDevice->height());
        paintDevicePixelRatio = paintDevice->devicePixelRatio();
    } else {
        // For HiDPI QBackingStores, the paint device is valid between calls to
        // beginPaint() and endPaint(). See: QTBUG-55875
        const QSize size = m_backingStore->size();
        const qreal windowDpr = m_backingStore->window()->devicePixelRatio();
        if (size != m_backingStoreSize || windowDpr != m_windowDevicePixelRatio) {
            m_backingStore->beginPaint(QRegion());

            const QPaintDevice *backingStorePaintDevice = m_backingStore->paintDevice();
            m_backingStorePaintSize = QSize(backingStorePaintDevice->width(),
                                            backingStorePaintDevice->height());
            m_backingStorePaintDevicePixelRatio = backingStorePaintDevice->devicePixelRatio();

            m_backingStore->endPaint();
            m_backingStoreSize = size;
            m_windowDevicePixelRatio = windowDpr;
        }
        paintSize = m_backingStorePaintSize;
        paintDevicePixelRatio = m_backingStorePaintDevicePixelRatio;
    }

    QElapsedTimer renderTimer;

    setBackgroundColor(clearColor());
    const QRect backgroundRect =
            QRectF(QPointF(), QSizeF(paintSize) / paintDevicePixelRatio).toAlignedRect();
    setBackgroundRect(backgroundRect, paintDevicePixelRatio);
    const bool disablePartialUpdates = m_partialUpdateMode == DisablePartialUpdate;

    // Force the whole render area to be marked as dirty, avoiding any partial update
    if (disablePartialUpdates)
        markDirty();

    // Build Renderlist
    // The renderlist is created by visiting each node in the tree and when a
    // renderable node is reach, we find the coorosponding RenderableNode object
    // and append it to the renderlist.  At this point the RenderableNode object
    // should not need any further updating so it is just a matter of appending
    // RenderableNodes
    renderTimer.start();
    buildRenderList();
    qint64 buildRenderListTime = renderTimer.restart();

    // Optimize Renderlist
    // This is a pass through the renderlist to determine what actually needs to
    // be painted.  Without this pass the renderlist will simply render each item
    // from back to front, with a high potential for overdraw.  It would also lead
    // to the entire window being flushed every frame.  The objective of the
    // optimization pass is to only paint dirty nodes that are not occuluded.  A
    // side effect of this is that additional nodes may need to be marked dirty to
    // force a repaint.  It is also important that any item that needs to be
    // repainted only paints what is needed, via the use of clip regions.
    const QRegion updateRegion = optimizeRenderList();
    qint64 optimizeRenderListTime = renderTimer.restart();

    // Nothing to paint: render nodes are the only ones that paint without a dirty region
    const auto &nodes = renderableNodes();
    if (updateRegion.isEmpty() && std::none_of(nodes.cbegin(), nodes.cend(), [](auto *node) {
            return node->type() == QSGSoftwareRenderableNode::RenderNode && node->isDirty();
        })) {
        for (auto *node : nodes)
            node->clearDirty();
        m_flushRegion = QRegion();
        qCDebug(lcRenderer) << "render: nothing to paint" << buildRenderListTime << optimizeRenderListTime;
        return;
    }

    // If Rendering to a backingstore, prepare it to be updated
    bool usingBackingStore = false;
    if (!paintDevice) {
        m_backingStore->beginPaint(updateRegion);
        paintDevice = m_backingStore->paintDevice();
        usingBackingStore = true;
    }

    QPainter painter(paintDevice);
    painter.setRenderHint(QPainter::Antialiasing);
    auto rc = static_cast<QSGSoftwareRenderContext *>(context());
    QPainter *prevPainter = rc->m_activePainter;
    rc->m_activePainter = &painter;

    // Render the contents Renderlist
    m_flushRegion = renderNodes(&painter);
    qint64 renderTime = renderTimer.elapsed();

    painter.end();
    if (usingBackingStore)
        m_backingStore->endPaint();

    rc->m_activePainter = prevPainter;
    qCDebug(lcRenderer) << "render" << m_flushRegion << buildRenderListTime << optimizeRenderListTime << renderTime;
}

QT_END_NAMESPACE
