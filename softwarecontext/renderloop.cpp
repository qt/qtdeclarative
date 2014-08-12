/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt SceneGraph Raster Add-on.
**
** $QT_BEGIN_LICENSE$
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
** $QT_END_LICENSE$
**
****************************************************************************/
#include "renderloop.h"

#include "context.h"
#include <private/qquickwindow_p.h>
#include <QElapsedTimer>
#include <private/qquickprofiler_p.h>

// Used for very high-level info about the renderering and gl context
// Includes GL_VERSION, type of render loop, atlas size, etc.
Q_LOGGING_CATEGORY(QSG_LOG_INFO,                "qt.scenegraph.info")

// Used to debug the renderloop logic. Primarily useful for platform integrators
// and when investigating the render loop logic.
Q_LOGGING_CATEGORY(QSG_LOG_RENDERLOOP,          "qt.scenegraph.renderloop")


// GLSL shader compilation
Q_LOGGING_CATEGORY(QSG_LOG_TIME_COMPILATION,    "qt.scenegraph.time.compilation")

// polish, animations, sync, render and swap in the render loop
Q_LOGGING_CATEGORY(QSG_LOG_TIME_RENDERLOOP,     "qt.scenegraph.time.renderloop")

// Texture uploads and swizzling
Q_LOGGING_CATEGORY(QSG_LOG_TIME_TEXTURE,        "qt.scenegraph.time.texture")

// Glyph preparation (only for distance fields atm)
Q_LOGGING_CATEGORY(QSG_LOG_TIME_GLYPH,          "qt.scenegraph.time.glyph")

// Timing inside the renderer base class
Q_LOGGING_CATEGORY(QSG_LOG_TIME_RENDERER,       "qt.scenegraph.time.renderer")

RenderLoop::RenderLoop()
    : eventPending(false)
{
    sg = QSGContext::createDefaultContext();
    rc = sg->createRenderContext();
}

RenderLoop::~RenderLoop()
{
    delete rc;
    delete sg;
}

void RenderLoop::show(QQuickWindow *window)
{
    WindowData data;
    data.updatePending = false;
    data.grabOnly = false;
    m_windows[window] = data;

    maybeUpdate(window);
}

void RenderLoop::hide(QQuickWindow *window)
{
    if (!m_windows.contains(window))
        return;

    m_windows.remove(window);
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    cd->fireAboutToStop();
    cd->cleanupNodesOnShutdown();

    if (m_windows.size() == 0) {
        if (!cd->persistentSceneGraph) {
            rc->invalidate();
            QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        }
    }
}

void RenderLoop::windowDestroyed(QQuickWindow *window)
{
    hide(window);
    if (m_windows.size() == 0) {
        rc->invalidate();
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    }
}

void RenderLoop::renderWindow(QQuickWindow *window)
{
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    if (!cd->isRenderable() || !m_windows.contains(window))
        return;

    WindowData &data = const_cast<WindowData &>(m_windows[window]);

    // ### create QPainter and set up pointer to current window/painter
    static_cast<SoftwareContext::RenderContext*>(cd->context)->currentWindow = window;

    bool alsoSwap = data.updatePending;
    data.updatePending = false;

    if (!data.grabOnly) {
        cd->flushDelayedTouchEvent();
        // Event delivery/processing triggered the window to be deleted or stop rendering.
        if (!m_windows.contains(window))
            return;
    }
    QElapsedTimer renderTimer;
    qint64 renderTime = 0, syncTime = 0, polishTime = 0;
    bool profileFrames = QSG_LOG_TIME_RENDERLOOP().isDebugEnabled() || QQuickProfiler::enabled;
    if (profileFrames)
        renderTimer.start();

    cd->polishItems();

    if (profileFrames) {
        polishTime = renderTimer.nsecsElapsed();
        Q_QUICK_SG_PROFILE(QQuickProfiler::SceneGraphPolishFrame, (polishTime));
    }

    emit window->afterAnimating();

    cd->syncSceneGraph();

    if (profileFrames)
        syncTime = renderTimer.nsecsElapsed();

    cd->renderSceneGraph(window->size());

    if (profileFrames)
        renderTime = renderTimer.nsecsElapsed();

    if (data.grabOnly) {
        // #### grabContent = qt_gl_read_framebuffer(window->size() * window->devicePixelRatio(), false, false);
        data.grabOnly = false;
    }

    if (alsoSwap && window->isVisible()) {
//        ### gl->swapBuffers(window);
        cd->fireFrameSwapped();
    }

    qint64 swapTime = 0;
    if (profileFrames)
        swapTime = renderTimer.nsecsElapsed();

    if (QSG_LOG_TIME_RENDERLOOP().isDebugEnabled()) {
        static QTime lastFrameTime = QTime::currentTime();
        qCDebug(QSG_LOG_TIME_RENDERLOOP,
                "Frame rendered with 'basic' renderloop in %dms, polish=%d, sync=%d, render=%d, swap=%d, frameDelta=%d",
                int(swapTime / 1000000),
                int(polishTime / 1000000),
                int((syncTime - polishTime) / 1000000),
                int((renderTime - syncTime) / 1000000),
                int((swapTime - renderTime) / 10000000),
                int(lastFrameTime.msecsTo(QTime::currentTime())));
        lastFrameTime = QTime::currentTime();
    }

    Q_QUICK_SG_PROFILE(QQuickProfiler::SceneGraphRenderLoopFrame, (
            syncTime - polishTime,
            renderTime - syncTime,
            swapTime - renderTime));

    // Might have been set during syncSceneGraph()
    if (data.updatePending)
        maybeUpdate(window);
}

void RenderLoop::exposureChanged(QQuickWindow *window)
{
    if (window->isExposed()) {
        m_windows[window].updatePending = true;
        renderWindow(window);
    }
}

QImage RenderLoop::grab(QQuickWindow *window)
{
    if (!m_windows.contains(window))
        return QImage();

    m_windows[window].grabOnly = true;

    renderWindow(window);

    QImage grabbed = grabContent;
    grabContent = QImage();
    return grabbed;
}



void RenderLoop::maybeUpdate(QQuickWindow *window)
{
    if (!m_windows.contains(window))
        return;

    m_windows[window].updatePending = true;

    if (!eventPending) {
        const int exhaust_delay = 5;
        m_update_timer = startTimer(exhaust_delay, Qt::PreciseTimer);
        eventPending = true;
    }
}

QSurface::SurfaceType RenderLoop::windowSurfaceType() const
{
    return QSurface::RasterSurface;
}



QSGContext *RenderLoop::sceneGraphContext() const
{
    return sg;
}


bool RenderLoop::event(QEvent *e)
{
    if (e->type() == QEvent::Timer) {
        eventPending = false;
        killTimer(m_update_timer);
        m_update_timer = 0;
        for (QHash<QQuickWindow *, WindowData>::const_iterator it = m_windows.constBegin();
             it != m_windows.constEnd(); ++it) {
            const WindowData &data = it.value();
            if (data.updatePending)
                renderWindow(it.key());
        }
        return true;
    }
    return QObject::event(e);
}
