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

#include "qsgd3d12renderloop_p.h"
#include "qsgd3d12engine_p.h"
#include "qsgd3d12context_p.h"
#include "qsgd3d12rendercontext_p.h"
#include <private/qquickwindow_p.h>
#include <private/qquickprofiler_p.h>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE

QSGD3D12RenderLoop::QSGD3D12RenderLoop()
{
    qDebug("new d3d12 render loop");
    sg = new QSGD3D12Context;
    rc = new QSGD3D12RenderContext(sg);
}

QSGD3D12RenderLoop::~QSGD3D12RenderLoop()
{
    delete rc;
    delete sg;
}

void QSGD3D12RenderLoop::show(QQuickWindow *window)
{
    qDebug() << "show" << window;

    WindowData data;
    data.engine = new QSGD3D12Engine;
    m_windows[window] = data;

    data.engine->attachToWindow(window);

    maybeUpdate(window);
}

void QSGD3D12RenderLoop::hide(QQuickWindow *window)
{
    qDebug() << "hide" << window;

    QQuickWindowPrivate *wd = QQuickWindowPrivate::get(window);
    wd->fireAboutToStop();
}

void QSGD3D12RenderLoop::resize(QQuickWindow *window)
{
    if (!window->isExposed() || window->size().isEmpty())
        return;

    qDebug() << "resize" << window;

    WindowData &data(m_windows[window]);
    if (data.engine)
        data.engine->resize();
}

void QSGD3D12RenderLoop::windowDestroyed(QQuickWindow *window)
{
    qDebug() << "window destroyed" << window;

    WindowData &data(m_windows[window]);
    delete data.engine;
    m_windows.remove(window);

    hide(window);

    QQuickWindowPrivate *wd = QQuickWindowPrivate::get(window);
    wd->cleanupNodesOnShutdown();

    if (m_windows.isEmpty()) {
        rc->invalidate();
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    }
}

void QSGD3D12RenderLoop::exposureChanged(QQuickWindow *window)
{
    qDebug() << "exposure changed" << window;

    if (window->isExposed()) {
        m_windows[window].updatePending = true;
        renderWindow(window);
    }
}

QImage QSGD3D12RenderLoop::grab(QQuickWindow *window)
{
    Q_UNUSED(window);
    Q_UNREACHABLE();
    return QImage();
}

void QSGD3D12RenderLoop::update(QQuickWindow *window)
{
    //qDebug() << "update" << window;

    if (!m_windows.contains(window))
        return;

    m_windows[window].updatePending = true;
    window->requestUpdate();
}

void QSGD3D12RenderLoop::maybeUpdate(QQuickWindow *window)
{
    //qDebug() << "maybeUpdate" << window;

    update(window);
}

// called in response to window->requestUpdate()
void QSGD3D12RenderLoop::handleUpdateRequest(QQuickWindow *window)
{
    qDebug() << "handleUpdateRequest" << window;

    renderWindow(window);
}

QAnimationDriver *QSGD3D12RenderLoop::animationDriver() const
{
    return nullptr;
}

QSGContext *QSGD3D12RenderLoop::sceneGraphContext() const
{
    return sg;
}

QSGRenderContext *QSGD3D12RenderLoop::createRenderContext(QSGContext *) const
{
    return rc;
}

void QSGD3D12RenderLoop::releaseResources(QQuickWindow *window)
{
    qDebug() << "releaseResources" << window;
}

void QSGD3D12RenderLoop::postJob(QQuickWindow *window, QRunnable *job)
{
    Q_UNUSED(window);
    Q_UNUSED(job);
    Q_UNREACHABLE();
}

QSurface::SurfaceType QSGD3D12RenderLoop::windowSurfaceType() const
{
    return QSurface::OpenGLSurface;
}

void QSGD3D12RenderLoop::renderWindow(QQuickWindow *window)
{
    qDebug() << "renderWindow" << window;

    QQuickWindowPrivate *wd = QQuickWindowPrivate::get(window);
    if (!wd->isRenderable() || !m_windows.contains(window))
        return;

    WindowData &data(m_windows[window]);

    const bool needsSwap = data.updatePending;
    data.updatePending = false;

    if (!data.grabOnly) {
        wd->flushDelayedTouchEvent();
        if (!m_windows.contains(window))
            return;
    }

    rc->setEngine(data.engine);

    QElapsedTimer renderTimer;
    qint64 renderTime = 0, syncTime = 0, polishTime = 0;
    const bool profileFrames = QSG_LOG_TIME_RENDERLOOP().isDebugEnabled();
    if (profileFrames)
        renderTimer.start();
    Q_QUICK_SG_PROFILE_START(QQuickProfiler::SceneGraphPolishFrame);

    wd->polishItems();

    if (profileFrames)
        polishTime = renderTimer.nsecsElapsed();
    Q_QUICK_SG_PROFILE_SWITCH(QQuickProfiler::SceneGraphPolishFrame,
                              QQuickProfiler::SceneGraphRenderLoopFrame);

    emit window->afterAnimating();

    wd->syncSceneGraph();

    if (profileFrames)
        syncTime = renderTimer.nsecsElapsed();
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRenderLoopFrame);

    wd->renderSceneGraph(window->size());

    if (profileFrames)
        renderTime = renderTimer.nsecsElapsed();
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRenderLoopFrame);

    if (data.grabOnly) {
        Q_UNREACHABLE();
        data.grabOnly = false;
    }

    if (needsSwap && window->isVisible()) {
        data.engine->present();
        data.engine->waitGPU();
        wd->fireFrameSwapped();
    } else {
        data.engine->waitGPU();
    }

    qint64 swapTime = 0;
    if (profileFrames)
        swapTime = renderTimer.nsecsElapsed();
    Q_QUICK_SG_PROFILE_END(QQuickProfiler::SceneGraphRenderLoopFrame);

    if (QSG_LOG_TIME_RENDERLOOP().isDebugEnabled()) {
        static QTime lastFrameTime = QTime::currentTime();
        qCDebug(QSG_LOG_TIME_RENDERLOOP,
                "Frame rendered with 'd3d12' renderloop in %dms, polish=%d, sync=%d, render=%d, swap=%d, frameDelta=%d",
                int(swapTime / 1000000),
                int(polishTime / 1000000),
                int((syncTime - polishTime) / 1000000),
                int((renderTime - syncTime) / 1000000),
                int((swapTime - renderTime) / 10000000),
                int(lastFrameTime.msecsTo(QTime::currentTime())));
        lastFrameTime = QTime::currentTime();
    }

    rc->setEngine(nullptr);

    // Might have been set during syncSceneGraph()
    if (data.updatePending)
        maybeUpdate(window);
}

QT_END_NAMESPACE
