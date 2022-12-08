// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgsoftwarerenderloop_p.h"

#include "qsgsoftwarecontext_p.h"

#include <QtCore/QCoreApplication>

#include <private/qquickwindow_p.h>
#include <private/qquickitem_p.h>
#include <QElapsedTimer>
#include <private/qquickanimatorcontroller_p.h>
#include <private/qquickprofiler_p.h>
#include <private/qsgsoftwarerenderer_p.h>
#include <qpa/qplatformbackingstore.h>

#include <QtGui/QBackingStore>

#include <qtquick_tracepoints_p.h>

QT_BEGIN_NAMESPACE

QSGSoftwareRenderLoop::QSGSoftwareRenderLoop()
{
    sg = new QSGSoftwareContext();
    rc = sg->createRenderContext();
}

QSGSoftwareRenderLoop::~QSGSoftwareRenderLoop()
{
    delete rc;
    delete sg;
}

void QSGSoftwareRenderLoop::show(QQuickWindow *window)
{
    WindowData data;
    data.updatePending = false;
    data.grabOnly = false;
    m_windows[window] = data;

    if (m_backingStores[window] == nullptr) {
        m_backingStores[window] = new QBackingStore(window);
    }

    maybeUpdate(window);
}

void QSGSoftwareRenderLoop::hide(QQuickWindow *window)
{
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    cd->fireAboutToStop();
}

void QSGSoftwareRenderLoop::windowDestroyed(QQuickWindow *window)
{
    m_windows.remove(window);
    delete m_backingStores[window];
    m_backingStores.remove(window);
    hide(window);

    QQuickWindowPrivate *d = QQuickWindowPrivate::get(window);
    d->cleanupNodesOnShutdown();

    if (m_windows.size() == 0) {
        rc->invalidate();
    }

    d->animationController.reset();
}

void QSGSoftwareRenderLoop::renderWindow(QQuickWindow *window, bool isNewExpose)
{
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    if (!m_windows.contains(window))
        return;

    WindowData &data = const_cast<WindowData &>(m_windows[window]);

    //If were not in grabOnly mode, dont render a non-renderable window
    if (!data.grabOnly && !cd->isRenderable())
        return;

    //Resize the backing store if necessary
    if (m_backingStores[window]->size() != window->size()) {
        m_backingStores[window]->resize(window->size());
    }

    // ### create QPainter and set up pointer to current window/painter
    QSGSoftwareRenderContext *ctx = static_cast<QSGSoftwareRenderContext*>(cd->context);
    ctx->initializeIfNeeded();

    bool alsoSwap = data.updatePending;
    data.updatePending = false;

    if (!data.grabOnly) {
        cd->deliveryAgentPrivate()->flushFrameSynchronousEvents(window);
        // Event delivery/processing triggered the window to be deleted or stop rendering.
        if (!m_windows.contains(window))
            return;
    }

    Q_TRACE_SCOPE(QSG_renderWindow)
    QElapsedTimer renderTimer;
    qint64 renderTime = 0, syncTime = 0, polishTime = 0;
    bool profileFrames = QSG_RASTER_LOG_TIME_RENDERLOOP().isDebugEnabled();
    if (profileFrames)
        renderTimer.start();
    Q_QUICK_SG_PROFILE_START(QQuickProfiler::SceneGraphPolishFrame);
    Q_TRACE(QSG_polishItems_entry);

    cd->polishItems();

    if (profileFrames)
        polishTime = renderTimer.nsecsElapsed();
    Q_TRACE(QSG_polishItems_exit);
    Q_QUICK_SG_PROFILE_SWITCH(QQuickProfiler::SceneGraphPolishFrame,
                              QQuickProfiler::SceneGraphRenderLoopFrame,
                              QQuickProfiler::SceneGraphPolishPolish);
    Q_TRACE(QSG_sync_entry);

    emit window->afterAnimating();

    emit window->beforeFrameBegin();

    cd->syncSceneGraph();
    rc->endSync();

    if (profileFrames)
        syncTime = renderTimer.nsecsElapsed();
    Q_TRACE(QSG_sync_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRenderLoopFrame,
                              QQuickProfiler::SceneGraphRenderLoopSync);
    Q_TRACE(QSG_render_entry);

    //Tell the renderer about the windows backing store
    auto softwareRenderer = static_cast<QSGSoftwareRenderer*>(cd->renderer);
    if (softwareRenderer)
        softwareRenderer->setBackingStore(m_backingStores[window]);

    cd->renderSceneGraph();

    if (profileFrames)
        renderTime = renderTimer.nsecsElapsed();
    Q_TRACE(QSG_render_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRenderLoopFrame,
                              QQuickProfiler::SceneGraphRenderLoopRender);
    Q_TRACE(QSG_swap_entry);

    if (data.grabOnly) {
        grabContent = m_backingStores[window]->handle()->toImage();
        data.grabOnly = false;
    }

    if (alsoSwap && window->isVisible()) {
        //Flush backingstore to window
        if (!isNewExpose)
            m_backingStores[window]->flush(softwareRenderer->flushRegion());
        else
            m_backingStores[window]->flush(QRegion(QRect(QPoint(0,0), window->size())));
        cd->fireFrameSwapped();
    }

    emit window->afterFrameEnd();

    qint64 swapTime = 0;
    if (profileFrames)
        swapTime = renderTimer.nsecsElapsed();
    Q_TRACE(QSG_swap_exit);
    Q_QUICK_SG_PROFILE_END(QQuickProfiler::SceneGraphRenderLoopFrame,
                           QQuickProfiler::SceneGraphRenderLoopSwap);

    if (QSG_RASTER_LOG_TIME_RENDERLOOP().isDebugEnabled()) {
        static QTime lastFrameTime = QTime::currentTime();
        qCDebug(QSG_RASTER_LOG_TIME_RENDERLOOP,
                "Frame rendered with 'software' renderloop in %dms, polish=%d, sync=%d, render=%d, swap=%d, frameDelta=%d",
                int(swapTime / 1000000),
                int(polishTime / 1000000),
                int((syncTime - polishTime) / 1000000),
                int((renderTime - syncTime) / 1000000),
                int((swapTime - renderTime) / 1000000),
                int(lastFrameTime.msecsTo(QTime::currentTime())));
        lastFrameTime = QTime::currentTime();
    }

    // Might have been set during syncSceneGraph()
    if (data.updatePending)
        maybeUpdate(window);
}

void QSGSoftwareRenderLoop::exposureChanged(QQuickWindow *window)
{
    if (window->isExposed()) {
        m_windows[window].updatePending = true;
        renderWindow(window, true);
    }
}

QImage QSGSoftwareRenderLoop::grab(QQuickWindow *window)
{
    //If the window was never shown, create a new backing store
    if (!m_backingStores.contains(window)) {
        m_backingStores[window] = new QBackingStore(window);
        // Call create on window to make sure platform window is created
        window->create();
    }

    //If there is no WindowData, add one
    if (!m_windows.contains(window)) {
        WindowData data;
        data.updatePending = false;
        m_windows[window] = data;
    }

    m_windows[window].grabOnly = true;

    renderWindow(window);

    QImage grabbed = grabContent;
    grabbed.detach();
    grabContent = QImage();
    return grabbed;
}



void QSGSoftwareRenderLoop::maybeUpdate(QQuickWindow *window)
{
    if (!m_windows.contains(window))
        return;

    m_windows[window].updatePending = true;
    window->requestUpdate();
}

QSurface::SurfaceType QSGSoftwareRenderLoop::windowSurfaceType() const
{
    return QSurface::RasterSurface;
}



QSGContext *QSGSoftwareRenderLoop::sceneGraphContext() const
{
    return sg;
}


void QSGSoftwareRenderLoop::handleUpdateRequest(QQuickWindow *window)
{
    renderWindow(window);
}

QT_END_NAMESPACE

#include "moc_qsgsoftwarerenderloop_p.cpp"
