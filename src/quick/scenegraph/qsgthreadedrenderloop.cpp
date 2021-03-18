/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
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


#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QAnimationDriver>
#include <QtCore/QQueue>
#include <QtCore/QTime>

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/QOffscreenSurface>

#include <qpa/qwindowsysteminterface.h>

#include <QtQuick/QQuickWindow>
#include <private/qquickwindow_p.h>

#include <QtQuick/private/qsgrenderer_p.h>

#include "qsgthreadedrenderloop_p.h"
#include "qsgrhisupport_p.h"
#include <private/qquickanimatorcontroller_p.h>

#include <private/qquickprofiler_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>
#include <private/qqmldebugconnector_p.h>

#if QT_CONFIG(quick_shadereffect)
#include <private/qquickopenglshadereffectnode_p.h>
#endif
#include <private/qsgrhishadereffectnode_p.h>
#include <private/qsgdefaultrendercontext_p.h>

#include <qtquick_tracepoints_p.h>

/*
   Overall design:

   There are two classes here. QSGThreadedRenderLoop and
   QSGRenderThread. All communication between the two is based on
   event passing and we have a number of custom events.

   In this implementation, the render thread is never blocked and the
   GUI thread will initiate a polishAndSync which will block and wait
   for the render thread to pick it up and release the block only
   after the render thread is done syncing. The reason for this
   is:

   1. Clear blocking paradigm. We only have one real "block" point
   (polishAndSync()) and all blocking is initiated by GUI and picked
   up by Render at specific times based on events. This makes the
   execution deterministic.

   2. Render does not have to interact with GUI. This is done so that
   the render thread can run its own animation system which stays
   alive even when the GUI thread is blocked doing i/o, object
   instantiation, QPainter-painting or any other non-trivial task.

   ---

   There is one thread per window and one opengl context per thread.

   ---

   The render thread has affinity to the GUI thread until a window
   is shown. From that moment and until the window is destroyed, it
   will have affinity to the render thread. (moved back at the end
   of run for cleanup).

   ---

   The render loop is active while any window is exposed. All visible
   windows are tracked, but only exposed windows are actually added to
   the render thread and rendered. That means that if all windows are
   obscured, we might end up cleaning up the SG and GL context (if all
   windows have disabled persistency). Especially for multiprocess,
   low-end systems, this should be quite important.

 */

QT_BEGIN_NAMESPACE

#define QSG_RT_PAD "                    (RT) %s"

static inline int qsgrl_animation_interval() {
    qreal refreshRate = QGuiApplication::primaryScreen()->refreshRate();
    // To work around that some platforms wrongfully return 0 or something
    // bogus for refreshrate
    if (refreshRate < 1)
        return 16;
    return int(1000 / refreshRate);
}


static QElapsedTimer threadTimer;
static qint64 syncTime;
static qint64 renderTime;
static qint64 sinceLastTime;

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

// RL: Render Loop
// RT: Render Thread

// Passed from the RL to the RT when a window is removed obscured and
// should be removed from the render loop.
const QEvent::Type WM_Obscure           = QEvent::Type(QEvent::User + 1);

// Passed from the RL to RT when GUI has been locked, waiting for sync
// (updatePaintNode())
const QEvent::Type WM_RequestSync       = QEvent::Type(QEvent::User + 2);

// Passed by the RL to the RT to free up maybe release SG and GL contexts
// if no windows are rendering.
const QEvent::Type WM_TryRelease        = QEvent::Type(QEvent::User + 4);

// Passed by the RL to the RT when a QQuickWindow::grabWindow() is
// called.
const QEvent::Type WM_Grab              = QEvent::Type(QEvent::User + 5);

// Passed by the window when there is a render job to run
const QEvent::Type WM_PostJob           = QEvent::Type(QEvent::User + 6);

// When using the QRhi this is sent upon PlatformSurfaceAboutToBeDestroyed from
// the event filter installed on the QQuickWindow.
const QEvent::Type WM_ReleaseSwapchain  = QEvent::Type(QEvent::User + 7);

template <typename T> T *windowFor(const QList<T> &list, QQuickWindow *window)
{
    for (int i=0; i<list.size(); ++i) {
        const T &t = list.at(i);
        if (t.window == window)
            return const_cast<T *>(&t);
    }
    return nullptr;
}


class WMWindowEvent : public QEvent
{
public:
    WMWindowEvent(QQuickWindow *c, QEvent::Type type) : QEvent(type), window(c) { }
    QQuickWindow *window;
};

class WMTryReleaseEvent : public WMWindowEvent
{
public:
    WMTryReleaseEvent(QQuickWindow *win, bool destroy, bool needsFallbackSurface)
        : WMWindowEvent(win, WM_TryRelease)
        , inDestructor(destroy)
        , needsFallback(needsFallbackSurface)
    {}

    bool inDestructor;
    bool needsFallback;
};

class WMSyncEvent : public WMWindowEvent
{
public:
    WMSyncEvent(QQuickWindow *c, bool inExpose, bool force)
        : WMWindowEvent(c, WM_RequestSync)
        , size(c->size())
        , dpr(float(c->effectiveDevicePixelRatio()))
        , syncInExpose(inExpose)
        , forceRenderPass(force)
    {}
    QSize size;
    float dpr;
    bool syncInExpose;
    bool forceRenderPass;
};


class WMGrabEvent : public WMWindowEvent
{
public:
    WMGrabEvent(QQuickWindow *c, QImage *result) : WMWindowEvent(c, WM_Grab), image(result) {}
    QImage *image;
};

class WMJobEvent : public WMWindowEvent
{
public:
    WMJobEvent(QQuickWindow *c, QRunnable *postedJob)
        : WMWindowEvent(c, WM_PostJob), job(postedJob) {}
    ~WMJobEvent() { delete job; }
    QRunnable *job;
};

class WMReleaseSwapchainEvent : public WMWindowEvent
{
public:
    WMReleaseSwapchainEvent(QQuickWindow *c) : WMWindowEvent(c, WM_ReleaseSwapchain) { }
};

class QSGRenderThreadEventQueue : public QQueue<QEvent *>
{
public:
    QSGRenderThreadEventQueue()
        : waiting(false)
    {
    }

    void addEvent(QEvent *e) {
        mutex.lock();
        enqueue(e);
        if (waiting)
            condition.wakeOne();
        mutex.unlock();
    }

    QEvent *takeEvent(bool wait) {
        mutex.lock();
        if (size() == 0 && wait) {
            waiting = true;
            condition.wait(&mutex);
            waiting = false;
        }
        QEvent *e = dequeue();
        mutex.unlock();
        return e;
    }

    bool hasMoreEvents() {
        mutex.lock();
        bool has = !isEmpty();
        mutex.unlock();
        return has;
    }

private:
    QMutex mutex;
    QWaitCondition condition;
    bool waiting;
};


class QSGRenderThread : public QThread
{
    Q_OBJECT
public:
    QSGRenderThread(QSGThreadedRenderLoop *w, QSGRenderContext *renderContext)
        : wm(w)
        , gl(nullptr)
        , enableRhi(false)
        , rhi(nullptr)
        , offscreenSurface(nullptr)
        , animatorDriver(nullptr)
        , pendingUpdate(0)
        , sleeping(false)
        , syncResultedInChanges(false)
        , active(false)
        , window(nullptr)
        , stopEventProcessing(false)
    {
        sgrc = static_cast<QSGDefaultRenderContext *>(renderContext);
#if defined(Q_OS_QNX) && defined(Q_PROCESSOR_X86)
        // The SDP 6.6.0 x86 MESA driver requires a larger stack than the default.
        setStackSize(1024 * 1024);
#endif
        vsyncDelta = qsgrl_animation_interval();
    }

    ~QSGRenderThread()
    {
        delete sgrc;
        delete offscreenSurface;
    }

    void invalidateGraphics(QQuickWindow *window, bool inDestructor, QOffscreenSurface *backupSurface);

    bool event(QEvent *) override;
    void run() override;

    void syncAndRender(QImage *grabImage = nullptr);
    void sync(bool inExpose, bool inGrab);

    void requestRepaint()
    {
        if (sleeping)
            stopEventProcessing = true;
        if (window)
            pendingUpdate |= RepaintRequest;
    }

    void processEventsAndWaitForMore();
    void processEvents();
    void postEvent(QEvent *e);

public slots:
    void sceneGraphChanged() {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "sceneGraphChanged");
        syncResultedInChanges = true;
    }

public:
    enum UpdateRequest {
        SyncRequest         = 0x01,
        RepaintRequest      = 0x02,
        ExposeRequest       = 0x04 | RepaintRequest | SyncRequest
    };

    void ensureRhi();
    void handleDeviceLoss();

    QSGThreadedRenderLoop *wm;
    QOpenGLContext *gl;
    bool enableRhi;
    QRhi *rhi;
    QSGDefaultRenderContext *sgrc;
    QOffscreenSurface *offscreenSurface;

    QAnimationDriver *animatorDriver;

    uint pendingUpdate;
    bool sleeping;
    bool syncResultedInChanges;

    volatile bool active;

    float vsyncDelta;

    QMutex mutex;
    QWaitCondition waitCondition;

    QElapsedTimer m_timer;

    QQuickWindow *window; // Will be 0 when window is not exposed
    QSize windowSize;
    float dpr = 1;
    int rhiSampleCount = 1;
    bool rhiDeviceLost = false;
    bool rhiDoomed = false;
    bool guiNotifiedAboutRhiFailure = false;

    // Local event queue stuff...
    bool stopEventProcessing;
    QSGRenderThreadEventQueue eventQueue;
};

bool QSGRenderThread::event(QEvent *e)
{
    switch ((int) e->type()) {

    case WM_Obscure: {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "WM_Obscure");

        Q_ASSERT(!window || window == static_cast<WMWindowEvent *>(e)->window);

        mutex.lock();
        if (window) {
            QQuickWindowPrivate::get(window)->fireAboutToStop();
            qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- window removed");
            window = nullptr;
        }
        waitCondition.wakeOne();
        mutex.unlock();

        return true; }

    case WM_RequestSync: {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "WM_RequestSync");
        WMSyncEvent *se = static_cast<WMSyncEvent *>(e);
        if (sleeping)
            stopEventProcessing = true;
        window = se->window;
        windowSize = se->size;
        dpr = se->dpr;

        pendingUpdate |= SyncRequest;
        if (se->syncInExpose) {
            qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- triggered from expose");
            pendingUpdate |= ExposeRequest;
        }
        if (se->forceRenderPass) {
            qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- repaint regardless");
            pendingUpdate |= RepaintRequest;
        }
        return true; }

    case WM_TryRelease: {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "WM_TryRelease");
        mutex.lock();
        wm->m_lockedForSync = true;
        WMTryReleaseEvent *wme = static_cast<WMTryReleaseEvent *>(e);
        if (!window || wme->inDestructor) {
            qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- setting exit flag and invalidating OpenGL");
            invalidateGraphics(wme->window, wme->inDestructor, wme->needsFallback ? offscreenSurface : nullptr);
            active = gl || rhi;
            Q_ASSERT_X(!wme->inDestructor || !active, "QSGRenderThread::invalidateGraphics()", "Thread's active state is not set to false when shutting down");
            if (sleeping)
                stopEventProcessing = true;
        } else {
            qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- not releasing because window is still active");
            if (window) {
                QQuickWindowPrivate *d = QQuickWindowPrivate::get(window);
                if (d->renderer) {
                    qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- requesting renderer to release cached resources");
                    d->renderer->releaseCachedResources();
                }
            }
        }
        waitCondition.wakeOne();
        wm->m_lockedForSync = false;
        mutex.unlock();
        return true;
    }

    case WM_Grab: {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "WM_Grab");
        WMGrabEvent *ce = static_cast<WMGrabEvent *>(e);
        Q_ASSERT(ce->window);
        Q_ASSERT(ce->window == window || !window);
        mutex.lock();
        if (ce->window) {
            const bool alpha = ce->window->format().alphaBufferSize() > 0 && ce->window->color().alpha() != 255;
            const QSize readbackSize = windowSize * ce->window->effectiveDevicePixelRatio();
            if (rhi) {
                rhi->makeThreadLocalNativeContextCurrent();
                syncAndRender(ce->image);
            } else {
                gl->makeCurrent(ce->window);

                qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- sync scene graph");
                QQuickWindowPrivate *d = QQuickWindowPrivate::get(ce->window);
                d->syncSceneGraph();
                sgrc->endSync();

                qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- rendering scene graph");
                QQuickWindowPrivate::get(ce->window)->renderSceneGraph(ce->window->size());

                qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- grabbing result");
                *ce->image = qt_gl_read_framebuffer(readbackSize, alpha, alpha);
            }
            ce->image->setDevicePixelRatio(ce->window->effectiveDevicePixelRatio());
        }
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- waking gui to handle result");
        waitCondition.wakeOne();
        mutex.unlock();
        return true;
    }

    case WM_PostJob: {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "WM_PostJob");
        WMJobEvent *ce = static_cast<WMJobEvent *>(e);
        Q_ASSERT(ce->window == window);
        if (window) {
            if (rhi)
                rhi->makeThreadLocalNativeContextCurrent();
            else
                gl->makeCurrent(window);
            ce->job->run();
            delete ce->job;
            ce->job = nullptr;
            qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- job done");
        }
        return true;
    }

    case WM_ReleaseSwapchain: {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "WM_ReleaseSwapchain");
        WMReleaseSwapchainEvent *ce = static_cast<WMReleaseSwapchainEvent *>(e);
        // forget about 'window' here that may be null when already unexposed
        Q_ASSERT(ce->window);
        mutex.lock();
        if (ce->window) {
            wm->releaseSwapchain(ce->window);
            qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- swapchain released");
        }
        waitCondition.wakeOne();
        mutex.unlock();
        return true;
    }

    default:
        break;
    }
    return QThread::event(e);
}

void QSGRenderThread::invalidateGraphics(QQuickWindow *window, bool inDestructor, QOffscreenSurface *fallback)
{
    qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "invalidateGraphics()");

    if (!gl && !rhi)
        return;

    if (!window) {
        qCWarning(QSG_LOG_RENDERLOOP, "QSGThreadedRenderLoop:QSGRenderThread: no window to make current...");
        return;
    }


    bool wipeSG = inDestructor || !window->isPersistentSceneGraph();
    bool wipeGL = inDestructor || (wipeSG && !window->isPersistentOpenGLContext());

    bool current = true;
    if (gl)
        current = gl->makeCurrent(fallback ? static_cast<QSurface *>(fallback) : static_cast<QSurface *>(window));
    else if (rhi)
        rhi->makeThreadLocalNativeContextCurrent();

    if (Q_UNLIKELY(!current)) {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- cleanup without an OpenGL context");
    }

    QQuickWindowPrivate *dd = QQuickWindowPrivate::get(window);

#if QT_CONFIG(quick_shadereffect)
    QSGRhiShaderEffectNode::cleanupMaterialTypeCache();
#if QT_CONFIG(opengl)
    if (current)
        QQuickOpenGLShaderEffectMaterial::cleanupMaterialCache();
#endif
#endif

    // The canvas nodes must be cleaned up regardless if we are in the destructor..
    if (wipeSG) {
        dd->cleanupNodesOnShutdown();
    } else {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- persistent SG, avoiding cleanup");
        if (current && gl)
            gl->doneCurrent();
        return;
    }

    sgrc->invalidate();
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    if (inDestructor)
        dd->animationController.reset();
    if (current && gl)
        gl->doneCurrent();
    qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- invalidating scene graph");

    if (wipeGL) {
        if (dd->swapchain) {
            if (window->handle()) {
                // We get here when exiting via QCoreApplication::quit() instead of
                // through QWindow::close().
                wm->releaseSwapchain(window);
            } else {
                qWarning("QSGThreadedRenderLoop cleanup with QQuickWindow %p swapchain %p still alive, this should not happen.",
                         window, dd->swapchain);
            }
        }
        delete gl;
        gl = nullptr;
        delete rhi;
        rhi = nullptr;
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- invalidated OpenGL");
    } else {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- persistent GL, avoiding cleanup");
    }
}

/*
    Enters the mutex lock to make sure GUI is blocking and performs
    sync, then wakes GUI.
 */
void QSGRenderThread::sync(bool inExpose, bool inGrab)
{
    qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "sync()");
    if (!inGrab)
        mutex.lock();

    Q_ASSERT_X(wm->m_lockedForSync, "QSGRenderThread::sync()", "sync triggered on bad terms as gui is not already locked...");

    bool current = true;
    if (gl) {
        if (windowSize.width() > 0 && windowSize.height() > 0)
            current = gl->makeCurrent(window);
        else
            current = false;
        // Check for context loss.
        if (!current && !gl->isValid()) {
            QQuickWindowPrivate::get(window)->cleanupNodesOnShutdown();
            sgrc->invalidate();
            current = gl->create() && gl->makeCurrent(window);
            if (current) {
                QSGDefaultRenderContext::InitParams rcParams;
                rcParams.sampleCount = qMax(1, gl->format().samples());
                rcParams.openGLContext = gl;
                rcParams.initialSurfacePixelSize = windowSize * qreal(dpr);
                rcParams.maybeSurface = window;
                sgrc->initialize(&rcParams);
            }
        }
    } else if (rhi) {
        // With the rhi making the (OpenGL) context current serves only one
        // purpose: to enable external OpenGL rendering connected to one of
        // the QQuickWindow signals (beforeSynchronizing, beforeRendering,
        // etc.) to function like it did on the direct OpenGL path. For our
        // own rendering this call would not be necessary.
        rhi->makeThreadLocalNativeContextCurrent();
    } else {
        current = false;
    }
    if (current) {
        QQuickWindowPrivate *d = QQuickWindowPrivate::get(window);
        bool hadRenderer = d->renderer != nullptr;
        // If the scene graph was touched since the last sync() make sure it sends the
        // changed signal.
        if (d->renderer)
            d->renderer->clearChangedFlag();
        d->syncSceneGraph();
        sgrc->endSync();
        if (!hadRenderer && d->renderer) {
            qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- renderer was created");
            syncResultedInChanges = true;
            connect(d->renderer, SIGNAL(sceneGraphChanged()), this, SLOT(sceneGraphChanged()), Qt::DirectConnection);
        }

        // Process deferred deletes now, directly after the sync as
        // deleteLater on the GUI must now also have resulted in SG changes
        // and the delete is a safe operation.
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    } else {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- window has bad size, sync aborted");
    }

    // Two special cases: For grabs we do not care about blocking the gui
    // (main) thread. When this is from an expose, we will keep locked until
    // the frame is rendered (submitted), so in that case waking happens later
    // in syncAndRender(). Otherwise, wake now and let the main thread go on
    // while we render.
    if (!inExpose && !inGrab) {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- sync complete, waking Gui");
        waitCondition.wakeOne();
        mutex.unlock();
    }
}

void QSGRenderThread::handleDeviceLoss()
{
    if (!rhi || !rhi->isDeviceLost())
        return;

    qWarning("Graphics device lost, cleaning up scenegraph and releasing RHI");
    QQuickWindowPrivate::get(window)->cleanupNodesOnShutdown();
    sgrc->invalidate();
    wm->releaseSwapchain(window);
    rhiDeviceLost = true;
    delete rhi;
    rhi = nullptr;
}

void QSGRenderThread::syncAndRender(QImage *grabImage)
{
    bool profileFrames = QSG_LOG_TIME_RENDERLOOP().isDebugEnabled();
    if (profileFrames) {
        sinceLastTime = threadTimer.nsecsElapsed();
        threadTimer.start();
    }
    Q_TRACE_SCOPE(QSG_syncAndRender);
    Q_QUICK_SG_PROFILE_START(QQuickProfiler::SceneGraphRenderLoopFrame);
    Q_TRACE(QSG_sync_entry);

    QElapsedTimer waitTimer;
    waitTimer.start();

    qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "syncAndRender()");

    syncResultedInChanges = false;
    QQuickWindowPrivate *d = QQuickWindowPrivate::get(window);

    const bool repaintRequested = (pendingUpdate & RepaintRequest) || d->customRenderStage || grabImage;
    const bool syncRequested = (pendingUpdate & SyncRequest) || grabImage;
    const bool exposeRequested = (pendingUpdate & ExposeRequest) == ExposeRequest;
    const bool grabRequested = grabImage != nullptr;
    if (!grabRequested)
        pendingUpdate = 0;

    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    // Begin the frame before syncing -> sync is where we may invoke
    // updatePaintNode() on the items and they may want to do resource updates.
    // Also relevant for applications that connect to the before/afterSynchronizing
    // signals and want to do graphics stuff already there.
    if (cd->swapchain && windowSize.width() > 0 && windowSize.height() > 0) {
        // always prefer what the surface tells us, not the QWindow
        const QSize effectiveOutputSize = cd->swapchain->surfacePixelSize();
        // An update request could still be delivered right before we get an
        // unexpose. With Vulkan on Windows for example attempting to render
        // leads to failures at this stage since the surface size is already 0.
        if (effectiveOutputSize.isEmpty())
            return;

        const QSize previousOutputSize = cd->swapchain->currentPixelSize();
        if (previousOutputSize != effectiveOutputSize || cd->swapchainJustBecameRenderable) {
            if (cd->swapchainJustBecameRenderable)
                qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "just became exposed");

            cd->hasActiveSwapchain = cd->swapchain->buildOrResize();
            if (!cd->hasActiveSwapchain && rhi->isDeviceLost()) {
                handleDeviceLoss();
                QCoreApplication::postEvent(window, new QEvent(QEvent::Type(QQuickWindowPrivate::FullUpdateRequest)));
                return;
            }

            cd->swapchainJustBecameRenderable = false;
            cd->hasRenderableSwapchain = cd->hasActiveSwapchain;

            if (!cd->hasActiveSwapchain)
                qWarning("Failed to build or resize swapchain");
            else
                qCDebug(QSG_LOG_RENDERLOOP) << "rhi swapchain size" << cd->swapchain->currentPixelSize();
        }

        Q_ASSERT(rhi == cd->rhi);
        // ### the flag should only be set when the app requests it, but there's no way to do that right now
        QRhi::BeginFrameFlags frameFlags = QRhi::ExternalContentsInPass;
        QRhi::FrameOpResult frameResult = rhi->beginFrame(cd->swapchain, frameFlags);
        if (frameResult != QRhi::FrameOpSuccess) {
            if (frameResult == QRhi::FrameOpDeviceLost)
                handleDeviceLoss();
            else if (frameResult == QRhi::FrameOpError)
                qWarning("Failed to start frame");
            // try again later
            if (frameResult == QRhi::FrameOpDeviceLost || frameResult == QRhi::FrameOpSwapChainOutOfDate)
                QCoreApplication::postEvent(window, new QEvent(QEvent::Type(QQuickWindowPrivate::FullUpdateRequest)));
            // Before returning we need to ensure the same wake up logic that
            // would have happened if beginFrame() had suceeded.
            if (syncRequested && !grabRequested) {
                // Lock like sync() would do. Note that exposeRequested always includes syncRequested.
                qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- bailing out due to failed beginFrame, wake Gui");
                mutex.lock();
                // Go ahead with waking because we will return right after this.
                waitCondition.wakeOne();
                mutex.unlock();
            }
            return;
        }
    }

    if (syncRequested) {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- updatePending, doing sync");
        sync(exposeRequested, grabRequested);
    }
#ifndef QSG_NO_RENDER_TIMING
    if (profileFrames)
        syncTime = threadTimer.nsecsElapsed();
#endif
    Q_TRACE(QSG_sync_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRenderLoopFrame,
                              QQuickProfiler::SceneGraphRenderLoopSync);

    if (!syncResultedInChanges
            && !repaintRequested
            && !(pendingUpdate & RepaintRequest) // may have been set in sync()
            && sgrc->isValid()
            && !grabRequested
            && (gl || (rhi && !rhi->isRecordingFrame())))
    {
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- no changes, render aborted");
        int waitTime = vsyncDelta - (int) waitTimer.elapsed();
        if (waitTime > 0)
            msleep(waitTime);
        return;
    }

    qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- rendering started");

    Q_TRACE(QSG_render_entry);

    // RepaintRequest may have been set in pendingUpdate in an
    // updatePaintNode() invoked from sync(). We are about to do a repaint
    // right now, so reset the flag. (bits other than RepaintRequest cannot
    // be set in pendingUpdate at this point)
    if (!grabRequested)
        pendingUpdate = 0;

    if (animatorDriver->isRunning() && !grabRequested) {
        d->animationController->lock();
        animatorDriver->advance();
        d->animationController->unlock();
    }

    bool current = true;
    if (d->renderer && windowSize.width() > 0 && windowSize.height() > 0 && (gl || rhi)) {
        if (gl)
            current = gl->makeCurrent(window);
        else
            rhi->makeThreadLocalNativeContextCurrent();
    } else {
        current = false;
    }
    // Check for context loss (GL, RHI case handled after the beginFrame() above)
    if (gl) {
        if (!current && !gl->isValid()) {
            // Cannot do anything here because gui is not locked. Request a new
            // sync+render round on the gui thread and let the sync handle it.
            QCoreApplication::postEvent(window, new QEvent(QEvent::Type(QQuickWindowPrivate::FullUpdateRequest)));
        }
    }
    if (current) {
        d->renderSceneGraph(windowSize, rhi ? cd->swapchain->currentPixelSize() : QSize());

        if (profileFrames)
            renderTime = threadTimer.nsecsElapsed();
        Q_TRACE(QSG_render_exit);
        Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRenderLoopFrame,
                                  QQuickProfiler::SceneGraphRenderLoopRender);
        Q_TRACE(QSG_swap_entry);

        // With the rhi grabs can only be done by adding a readback and then
        // blocking in a real frame. The legacy GL path never gets here with
        // grabs as it rather invokes sync/render directly without going
        // through syncAndRender().
        if (grabRequested) {
            Q_ASSERT(rhi && !gl && cd->swapchain);
            *grabImage = QSGRhiSupport::instance()->grabAndBlockInCurrentFrame(rhi, cd->swapchain);
        }

        if (cd->swapchain) {
            QRhi::EndFrameFlags flags;
            if (grabRequested)
                flags |= QRhi::SkipPresent;
            QRhi::FrameOpResult frameResult = rhi->endFrame(cd->swapchain, flags);
            if (frameResult != QRhi::FrameOpSuccess) {
                if (frameResult == QRhi::FrameOpDeviceLost)
                    handleDeviceLoss();
                else if (frameResult == QRhi::FrameOpError)
                    qWarning("Failed to end frame");
                if (frameResult == QRhi::FrameOpDeviceLost || frameResult == QRhi::FrameOpSwapChainOutOfDate)
                    QCoreApplication::postEvent(window, new QEvent(QEvent::Type(QQuickWindowPrivate::FullUpdateRequest)));
            }
        } else {
            if (!cd->customRenderStage || !cd->customRenderStage->swap())
                gl->swapBuffers(window);
        }

        if (!grabRequested)
            d->fireFrameSwapped();
    } else {
        Q_TRACE(QSG_render_exit);
        Q_QUICK_SG_PROFILE_SKIP(QQuickProfiler::SceneGraphRenderLoopFrame,
                                QQuickProfiler::SceneGraphRenderLoopSync, 1);
        Q_TRACE(QSG_swap_entry);
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- window not ready, skipping render");
    }

    qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- rendering done");

    // Though it would be more correct to put this block directly after
    // fireFrameSwapped in the if (current) branch above, we don't do
    // that to avoid blocking the GUI thread in the case where it
    // has started rendering with a bad window, causing makeCurrent to
    // fail or if the window has a bad size.
    if (exposeRequested) {
        // With expose sync() did not wake gui, do it now.
        qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "- wake Gui after expose");
        waitCondition.wakeOne();
        mutex.unlock();
    }

    qCDebug(QSG_LOG_TIME_RENDERLOOP,
            "Frame rendered with 'threaded' renderloop in %dms, sync=%d, render=%d, swap=%d - (on render thread)",
            int(threadTimer.elapsed()),
            int((syncTime/1000000)),
            int((renderTime - syncTime) / 1000000),
            int(threadTimer.elapsed() - renderTime / 1000000));

    Q_TRACE(QSG_swap_exit);
    Q_QUICK_SG_PROFILE_END(QQuickProfiler::SceneGraphRenderLoopFrame,
                           QQuickProfiler::SceneGraphRenderLoopSwap);

    QSGRhiProfileConnection::instance()->send(rhi);
}



void QSGRenderThread::postEvent(QEvent *e)
{
    eventQueue.addEvent(e);
}



void QSGRenderThread::processEvents()
{
    qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "--- begin processEvents()");
    while (eventQueue.hasMoreEvents()) {
        QEvent *e = eventQueue.takeEvent(false);
        event(e);
        delete e;
    }
    qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "--- done processEvents()");
}

void QSGRenderThread::processEventsAndWaitForMore()
{
    qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "--- begin processEventsAndWaitForMore()");
    stopEventProcessing = false;
    while (!stopEventProcessing) {
        QEvent *e = eventQueue.takeEvent(true);
        event(e);
        delete e;
    }
    qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "--- done processEventsAndWaitForMore()");
}

void QSGRenderThread::ensureRhi()
{
    if (!rhi) {
        if (rhiDoomed) // no repeated attempts if the initial attempt failed
            return;
        QSGRhiSupport *rhiSupport = QSGRhiSupport::instance();
        rhi = rhiSupport->createRhi(window, offscreenSurface);
        if (rhi) {
            rhiDeviceLost = false;
            rhiSampleCount = rhiSupport->chooseSampleCountForWindowWithRhi(window, rhi);
            if (rhiSupport->isProfilingRequested())
                QSGRhiProfileConnection::instance()->initialize(rhi); // ### this breaks down with multiple windows
        } else {
            if (!rhiDeviceLost) {
                rhiDoomed = true;
                qWarning("Failed to create QRhi on the render thread; scenegraph is not functional");
            }
            // otherwise no error, will retry on a subsequent rendering attempt
            return;
        }
    }
    if (!sgrc->rhi() && windowSize.width() > 0 && windowSize.height() > 0) {
        rhi->makeThreadLocalNativeContextCurrent();
        QSGDefaultRenderContext::InitParams rcParams;
        rcParams.rhi = rhi;
        rcParams.sampleCount = rhiSampleCount;
        rcParams.openGLContext = nullptr;
        rcParams.initialSurfacePixelSize = windowSize * qreal(dpr);
        rcParams.maybeSurface = window;
        sgrc->initialize(&rcParams);
    }
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    if (rhi && !cd->swapchain) {
        cd->rhi = rhi;
        QRhiSwapChain::Flags flags = QRhiSwapChain::UsedAsTransferSource; // may be used in a grab
        // QQ is always premul alpha. Decide based on alphaBufferSize in
        // requestedFormat(). (the platform plugin can override format() but
        // what matters here is what the application wanted, hence using the
        // requested one)
        const bool alpha = window->requestedFormat().alphaBufferSize() > 0;
        if (alpha)
            flags |= QRhiSwapChain::SurfaceHasPreMulAlpha;
        cd->swapchain = rhi->newSwapChain();
        cd->depthStencilForSwapchain = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil,
                                                            QSize(),
                                                            rhiSampleCount,
                                                            QRhiRenderBuffer::UsedWithSwapChainOnly);
        cd->swapchain->setWindow(window);
        cd->swapchain->setDepthStencil(cd->depthStencilForSwapchain);
        qCDebug(QSG_LOG_INFO, "MSAA sample count for the swapchain is %d. Alpha channel requested = %s.",
                rhiSampleCount, alpha ? "yes" : "no");
        cd->swapchain->setSampleCount(rhiSampleCount);
        cd->swapchain->setFlags(flags);
        cd->rpDescForSwapchain = cd->swapchain->newCompatibleRenderPassDescriptor();
        cd->swapchain->setRenderPassDescriptor(cd->rpDescForSwapchain);
    }
}

void QSGRenderThread::run()
{
    qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "run()");
    animatorDriver = sgrc->sceneGraphContext()->createAnimationDriver(nullptr);
    animatorDriver->install();
    if (QQmlDebugConnector::service<QQmlProfilerService>())
        QQuickProfiler::registerAnimationCallback();

    while (active) {
#ifdef Q_OS_DARWIN
        QMacAutoReleasePool frameReleasePool;
#endif

        if (window) {
            if (enableRhi) {

                ensureRhi();

                // We absolutely have to syncAndRender() here, even when QRhi
                // failed to initialize otherwise the gui thread will be left
                // in a blocked state. It is up to syncAndRender() to
                // gracefully skip all graphics stuff when rhi is null.

                syncAndRender();

                // Now we can do something about rhi init failures. (reinit
                // failure after device reset does not count)
                if (rhiDoomed && !guiNotifiedAboutRhiFailure) {
                    guiNotifiedAboutRhiFailure = true;
                    QEvent *e = new QEvent(QEvent::Type(QQuickWindowPrivate::TriggerContextCreationFailure));
                    QCoreApplication::postEvent(window, e);
                }

            } else {
                if (!sgrc->openglContext() && windowSize.width() > 0 && windowSize.height() > 0 && gl->makeCurrent(window)) {
                    QSGDefaultRenderContext::InitParams rcParams;
                    rcParams.sampleCount = qMax(1, gl->format().samples());
                    rcParams.openGLContext = gl;
                    rcParams.initialSurfacePixelSize = windowSize * qreal(dpr);
                    rcParams.maybeSurface = window;
                    sgrc->initialize(&rcParams);
                }
                syncAndRender();
            }
        }

        processEvents();
        QCoreApplication::processEvents();

        if (active && (pendingUpdate == 0 || !window)) {
            qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "done drawing, sleep...");
            sleeping = true;
            processEventsAndWaitForMore();
            sleeping = false;
        }
    }

    Q_ASSERT_X(!gl && !rhi, "QSGRenderThread::run()", "The graphics context should be cleaned up before exiting the render thread...");

    qCDebug(QSG_LOG_RENDERLOOP, QSG_RT_PAD, "run() completed");

    delete animatorDriver;
    animatorDriver = nullptr;

    sgrc->moveToThread(wm->thread());
    moveToThread(wm->thread());
}

QSGThreadedRenderLoop::QSGThreadedRenderLoop()
    : sg(QSGContext::createDefaultContext())
    , m_animation_timer(0)
{
#if defined(QSG_RENDER_LOOP_DEBUG)
    qsgrl_timer.start();
#endif

    m_animation_driver = sg->createAnimationDriver(this);

    connect(m_animation_driver, SIGNAL(started()), this, SLOT(animationStarted()));
    connect(m_animation_driver, SIGNAL(stopped()), this, SLOT(animationStopped()));

    m_animation_driver->install();
}

QSGThreadedRenderLoop::~QSGThreadedRenderLoop()
{
    qDeleteAll(pendingRenderContexts);
    delete sg;
}

QSGRenderContext *QSGThreadedRenderLoop::createRenderContext(QSGContext *sg) const
{
    auto context = sg->createRenderContext();
    pendingRenderContexts.insert(context);
    return context;
}

void QSGThreadedRenderLoop::maybePostPolishRequest(Window *w)
{
    w->window->requestUpdate();
}

QAnimationDriver *QSGThreadedRenderLoop::animationDriver() const
{
    return m_animation_driver;
}

QSGContext *QSGThreadedRenderLoop::sceneGraphContext() const
{
    return sg;
}

bool QSGThreadedRenderLoop::anyoneShowing() const
{
    for (int i=0; i<m_windows.size(); ++i) {
        QQuickWindow *c = m_windows.at(i).window;
        if (c->isVisible() && c->isExposed())
            return true;
    }
    return false;
}

bool QSGThreadedRenderLoop::interleaveIncubation() const
{
    return m_animation_driver->isRunning() && anyoneShowing();
}

void QSGThreadedRenderLoop::animationStarted()
{
    qCDebug(QSG_LOG_RENDERLOOP, "- animationStarted()");
    startOrStopAnimationTimer();

    for (int i=0; i<m_windows.size(); ++i)
        maybePostPolishRequest(const_cast<Window *>(&m_windows.at(i)));
}

void QSGThreadedRenderLoop::animationStopped()
{
    qCDebug(QSG_LOG_RENDERLOOP, "- animationStopped()");
    startOrStopAnimationTimer();
}


void QSGThreadedRenderLoop::startOrStopAnimationTimer()
{
    int exposedWindows = 0;
    const Window *theOne = nullptr;
    for (int i=0; i<m_windows.size(); ++i) {
        const Window &w = m_windows.at(i);
        if (w.window->isVisible() && w.window->isExposed()) {
            ++exposedWindows;
            theOne = &w;
        }
    }

    if (m_animation_timer != 0 && (exposedWindows == 1 || !m_animation_driver->isRunning())) {
        qCDebug(QSG_LOG_RENDERLOOP, "*** Stopping animation timer");
        killTimer(m_animation_timer);
        m_animation_timer = 0;
        // If animations are running, make sure we keep on animating
        if (m_animation_driver->isRunning())
            maybePostPolishRequest(const_cast<Window *>(theOne));
    } else if (m_animation_timer == 0 && exposedWindows != 1 && m_animation_driver->isRunning()) {
        qCDebug(QSG_LOG_RENDERLOOP, "*** Starting animation timer");
        m_animation_timer = startTimer(qsgrl_animation_interval());
    }
}

/*
    Removes this window from the list of tracked windowes in this
    window manager. hide() will trigger obscure, which in turn will
    stop rendering.

    This function will be called during QWindow::close() which will
    also destroy the QPlatformWindow so it is important that this
    triggers handleObscurity() and that rendering for that window
    is fully done and over with by the time this function exits.
 */

void QSGThreadedRenderLoop::hide(QQuickWindow *window)
{
    qCDebug(QSG_LOG_RENDERLOOP) << "hide()" << window;

    if (window->isExposed())
        handleObscurity(windowFor(m_windows, window));

    releaseResources(window);
}


/*
    If the window is first hide it, then perform a complete cleanup
    with releaseResources which will take down the GL context and
    exit the rendering thread.
 */
void QSGThreadedRenderLoop::windowDestroyed(QQuickWindow *window)
{
    qCDebug(QSG_LOG_RENDERLOOP) << "begin windowDestroyed()" << window;

    Window *w = windowFor(m_windows, window);
    if (!w)
        return;

    handleObscurity(w);
    releaseResources(w, true);

    QSGRenderThread *thread = w->thread;
    while (thread->isRunning())
        QThread::yieldCurrentThread();
    Q_ASSERT(thread->thread() == QThread::currentThread());
    delete thread;

    for (int i=0; i<m_windows.size(); ++i) {
        if (m_windows.at(i).window == window) {
            m_windows.removeAt(i);
            break;
        }
    }

    // Now that we altered the window list, we may need to stop the animation
    // timer even if we didn't via handleObscurity. This covers the case where
    // we destroy a visible & exposed QQuickWindow.
    startOrStopAnimationTimer();

    qCDebug(QSG_LOG_RENDERLOOP) << "done windowDestroyed()" << window;
}

void QSGThreadedRenderLoop::releaseSwapchain(QQuickWindow *window)
{
    QQuickWindowPrivate *wd = QQuickWindowPrivate::get(window);
    delete wd->rpDescForSwapchain;
    wd->rpDescForSwapchain = nullptr;
    delete wd->swapchain;
    wd->swapchain = nullptr;
    delete wd->depthStencilForSwapchain;
    wd->depthStencilForSwapchain = nullptr;
    wd->hasActiveSwapchain = wd->hasRenderableSwapchain = wd->swapchainJustBecameRenderable = false;
}

void QSGThreadedRenderLoop::exposureChanged(QQuickWindow *window)
{
    qCDebug(QSG_LOG_RENDERLOOP) << "exposureChanged()" << window;

    // This is tricker than used to be. We want to detect having an empty
    // surface size (which may be the case even when window->size() is
    // non-empty, on some platforms with some graphics APIs!) as well as the
    // case when the window just became "newly exposed" (e.g. after a
    // minimize-restore on Windows, or when switching between fully obscured -
    // not fully obscured on macOS)
    QQuickWindowPrivate *wd = QQuickWindowPrivate::get(window);
    if (!window->isExposed())
        wd->hasRenderableSwapchain = false;

    bool skipThisExpose = false;
    if (window->isExposed() && wd->hasActiveSwapchain && wd->swapchain->surfacePixelSize().isEmpty()) {
        wd->hasRenderableSwapchain = false;
        skipThisExpose = true;
    }

    if (window->isExposed() && !wd->hasRenderableSwapchain && wd->hasActiveSwapchain
            && !wd->swapchain->surfacePixelSize().isEmpty())
    {
        wd->hasRenderableSwapchain = true;
        wd->swapchainJustBecameRenderable = true;
    }

    if (window->isExposed()) {
        if (!skipThisExpose)
            handleExposure(window);
    } else {
        Window *w = windowFor(m_windows, window);
        if (w)
            handleObscurity(w);
    }
}

/*
    Will post an event to the render thread that this window should
    start to render.
 */
void QSGThreadedRenderLoop::handleExposure(QQuickWindow *window)
{
    qCDebug(QSG_LOG_RENDERLOOP) << "handleExposure()" << window;

    Window *w = windowFor(m_windows, window);
    if (!w) {
        qCDebug(QSG_LOG_RENDERLOOP, "- adding window to list");
        Window win;
        win.window = window;
        win.actualWindowFormat = window->format();
        auto renderContext = QQuickWindowPrivate::get(window)->context;
        // The thread assumes ownership, so we don't need to delete it later.
        pendingRenderContexts.remove(renderContext);
        win.thread = new QSGRenderThread(this, renderContext);
        win.updateDuringSync = false;
        win.forceRenderPass = true; // also covered by polishAndSync(inExpose=true), but doesn't hurt
        m_windows << win;
        w = &m_windows.last();
    }

    // set this early as we'll be rendering shortly anyway and this avoids
    // specialcasing exposure in polishAndSync.
    w->thread->window = window;

    if (w->window->width() <= 0 || w->window->height() <= 0
        || (w->window->isTopLevel() && !w->window->geometry().intersects(w->window->screen()->availableGeometry()))) {
#ifndef QT_NO_DEBUG
        qWarning().noquote().nospace() << "QSGThreadedRenderLoop: expose event received for window "
            << w->window << " with invalid geometry: " << w->window->geometry()
            << " on " << w->window->screen();
#endif
    }

    // Because we are going to bind a GL context to it, make sure it
    // is created.
    if (!w->window->handle())
        w->window->create();

    // Start render thread if it is not running
    if (!w->thread->isRunning()) {
        qCDebug(QSG_LOG_RENDERLOOP, "- starting render thread");

        w->thread->enableRhi = QSGRhiSupport::instance()->isRhiEnabled();
        if (w->thread->enableRhi) {
            if (!w->thread->rhi) {
                QSGRhiSupport *rhiSupport = QSGRhiSupport::instance();
                w->thread->offscreenSurface = rhiSupport->maybeCreateOffscreenSurface(window);
                window->installEventFilter(this);
            }
        } else {
            if (!w->thread->gl) {
                w->thread->gl = new QOpenGLContext();
                if (qt_gl_global_share_context())
                    w->thread->gl->setShareContext(qt_gl_global_share_context());
                w->thread->gl->setFormat(w->window->requestedFormat());
                w->thread->gl->setScreen(w->window->screen());
                if (!w->thread->gl->create()) {
                    delete w->thread->gl;
                    w->thread->gl = nullptr;
                    handleContextCreationFailure(w->window);
                    return;
                }

                QQuickWindowPrivate::get(w->window)->fireOpenGLContextCreated(w->thread->gl);

                w->thread->gl->moveToThread(w->thread);
                if (!w->thread->gl->shareContext())
                    w->thread->gl->shareGroup()->moveToThread(w->thread);
                qCDebug(QSG_LOG_RENDERLOOP, "- OpenGL context created");

                w->thread->offscreenSurface = new QOffscreenSurface();
                w->thread->offscreenSurface->setFormat(w->actualWindowFormat);
                w->thread->offscreenSurface->create();
            }
        }

        QQuickAnimatorController *controller
                = QQuickWindowPrivate::get(w->window)->animationController.get();
        if (controller->thread() != w->thread)
            controller->moveToThread(w->thread);

        w->thread->active = true;
        if (w->thread->thread() == QThread::currentThread()) {
            w->thread->sgrc->moveToThread(w->thread);
            w->thread->moveToThread(w->thread);
        }
        w->thread->start();
        if (!w->thread->isRunning())
            qFatal("Render thread failed to start, aborting application.");

    } else {
        qCDebug(QSG_LOG_RENDERLOOP, "- render thread already running");
    }

    polishAndSync(w, true);
    qCDebug(QSG_LOG_RENDERLOOP, "- done with handleExposure()");

    startOrStopAnimationTimer();
}

/*
    This function posts an event to the render thread to remove the window
    from the list of windowses to render.

    It also starts up the non-vsync animation tick if no more windows
    are showing.
 */
void QSGThreadedRenderLoop::handleObscurity(Window *w)
{
    qCDebug(QSG_LOG_RENDERLOOP) << "handleObscurity()" << w->window;
    if (w->thread->isRunning()) {
        w->thread->mutex.lock();
        w->thread->postEvent(new WMWindowEvent(w->window, WM_Obscure));
        w->thread->waitCondition.wait(&w->thread->mutex);
        w->thread->mutex.unlock();
    }
    startOrStopAnimationTimer();
}

bool QSGThreadedRenderLoop::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::PlatformSurface:
        // this is the proper time to tear down the swapchain (while the native window and surface are still around)
        if (static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            QQuickWindow *window = qobject_cast<QQuickWindow *>(watched);
            if (window) {
                Window *w = windowFor(m_windows, window);
                if (w) {
                    w->thread->mutex.lock();
                    w->thread->postEvent(new WMReleaseSwapchainEvent(window));
                    w->thread->waitCondition.wait(&w->thread->mutex);
                    w->thread->mutex.unlock();
                }
                window->removeEventFilter(this);
            }
        }
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

void QSGThreadedRenderLoop::handleUpdateRequest(QQuickWindow *window)
{
    qCDebug(QSG_LOG_RENDERLOOP, "- polish and sync update request");
    Window *w = windowFor(m_windows, window);
    if (w)
        polishAndSync(w);
}

void QSGThreadedRenderLoop::maybeUpdate(QQuickWindow *window)
{
    Window *w = windowFor(m_windows, window);
    if (w)
        maybeUpdate(w);
}

/*
    Called whenever the QML scene has changed. Will post an event to
    ourselves that a sync is needed.
 */
void QSGThreadedRenderLoop::maybeUpdate(Window *w)
{
    if (!QCoreApplication::instance())
        return;

    if (!w || !w->thread->isRunning())
        return;

    QThread *current = QThread::currentThread();
    if (current == w->thread && w->thread->rhi && w->thread->rhi->isDeviceLost())
        return;
    if (current != QCoreApplication::instance()->thread() && (current != w->thread || !m_lockedForSync)) {
        qWarning() << "Updates can only be scheduled from GUI thread or from QQuickItem::updatePaintNode()";
        return;
    }

    qCDebug(QSG_LOG_RENDERLOOP) << "update from item" << w->window;

    // Call this function from the Gui thread later as startTimer cannot be
    // called from the render thread.
    if (current == w->thread) {
        qCDebug(QSG_LOG_RENDERLOOP, "- on render thread");
        w->updateDuringSync = true;
        return;
    }

    maybePostPolishRequest(w);
}

/*
    Called when the QQuickWindow should be explicitly repainted. This function
    can also be called on the render thread when the GUI thread is blocked to
    keep render thread animations alive.
 */
void QSGThreadedRenderLoop::update(QQuickWindow *window)
{
    Window *w = windowFor(m_windows, window);
    if (!w)
        return;

    if (w->thread == QThread::currentThread()) {
        qCDebug(QSG_LOG_RENDERLOOP) << "update on window - on render thread" << w->window;
        w->thread->requestRepaint();
        return;
    }

    qCDebug(QSG_LOG_RENDERLOOP) << "update on window" << w->window;
    // We set forceRenderPass because we want to make sure the QQuickWindow
    // actually does a full render pass after the next sync.
    w->forceRenderPass = true;
    maybeUpdate(w);
}


void QSGThreadedRenderLoop::releaseResources(QQuickWindow *window)
{
    Window *w = windowFor(m_windows, window);
    if (w)
        releaseResources(w, false);
}

/*
 * Release resources will post an event to the render thread to
 * free up the SG and GL resources and exists the render thread.
 */
void QSGThreadedRenderLoop::releaseResources(Window *w, bool inDestructor)
{
    qCDebug(QSG_LOG_RENDERLOOP) << "releaseResources()" << (inDestructor ? "in destructor" : "in api-call") << w->window;

    w->thread->mutex.lock();
    if (w->thread->isRunning() && w->thread->active) {
        QQuickWindow *window = w->window;

        // The platform window might have been destroyed before
        // hide/release/windowDestroyed is called, so we may need to have a
        // fallback surface to perform the cleanup of the scene graph and the
        // OpenGL resources. QOffscreenSurface must be created on the GUI
        // thread so that is done for us already.

        qCDebug(QSG_LOG_RENDERLOOP, "- posting release request to render thread");
        w->thread->postEvent(new WMTryReleaseEvent(window, inDestructor, window->handle() == nullptr));
        w->thread->waitCondition.wait(&w->thread->mutex);

        // Avoid a shutdown race condition.
        // If SG is invalidated and 'active' becomes false, the thread's run()
        // method will exit. handleExposure() relies on QThread::isRunning() (because it
        // potentially needs to start the thread again) and our mutex cannot be used to
        // track the thread stopping, so we wait a few nanoseconds extra so the thread
        // can exit properly.
        if (!w->thread->active) {
            qCDebug(QSG_LOG_RENDERLOOP) << " - waiting for render thread to exit" << w->window;
            w->thread->wait();
            qCDebug(QSG_LOG_RENDERLOOP) << " - render thread finished" << w->window;
        }
    }
    w->thread->mutex.unlock();
}


/* Calls polish on all items, then requests synchronization with the render thread
 * and blocks until that is complete. Returns false if it aborted; otherwise true.
 */
void QSGThreadedRenderLoop::polishAndSync(Window *w, bool inExpose)
{
    qCDebug(QSG_LOG_RENDERLOOP) << "polishAndSync" << (inExpose ? "(in expose)" : "(normal)") << w->window;

    QQuickWindow *window = w->window;
    if (!w->thread || !w->thread->window) {
        qCDebug(QSG_LOG_RENDERLOOP, "- not exposed, abort");
        return;
    }

    // Flush pending touch events.
    QQuickWindowPrivate::get(window)->flushFrameSynchronousEvents();
    // The delivery of the event might have caused the window to stop rendering
    w = windowFor(m_windows, window);
    if (!w || !w->thread || !w->thread->window) {
        qCDebug(QSG_LOG_RENDERLOOP, "- removed after event flushing, abort");
        return;
    }

    Q_TRACE_SCOPE(QSG_polishAndSync);
    QElapsedTimer timer;
    qint64 polishTime = 0;
    qint64 waitTime = 0;
    qint64 syncTime = 0;
    bool profileFrames = QSG_LOG_TIME_RENDERLOOP().isDebugEnabled();
    if (profileFrames)
        timer.start();
    Q_QUICK_SG_PROFILE_START(QQuickProfiler::SceneGraphPolishAndSync);
    Q_TRACE(QSG_polishItems_entry);

    QQuickWindowPrivate *d = QQuickWindowPrivate::get(window);
    d->polishItems();

    if (profileFrames)
        polishTime = timer.nsecsElapsed();
    Q_TRACE(QSG_polishItems_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphPolishAndSync,
                              QQuickProfiler::SceneGraphPolishAndSyncPolish);
    Q_TRACE(QSG_wait_entry);

    w->updateDuringSync = false;

    emit window->afterAnimating();

    qCDebug(QSG_LOG_RENDERLOOP, "- lock for sync");
    w->thread->mutex.lock();
    m_lockedForSync = true;
    w->thread->postEvent(new WMSyncEvent(window, inExpose, w->forceRenderPass));
    w->forceRenderPass = false;

    qCDebug(QSG_LOG_RENDERLOOP, "- wait for sync");
    if (profileFrames)
        waitTime = timer.nsecsElapsed();
    Q_TRACE(QSG_wait_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphPolishAndSync,
                              QQuickProfiler::SceneGraphPolishAndSyncWait);
    Q_TRACE(QSG_sync_entry);

    w->thread->waitCondition.wait(&w->thread->mutex);
    m_lockedForSync = false;
    w->thread->mutex.unlock();
    qCDebug(QSG_LOG_RENDERLOOP, "- unlock after sync");

    if (profileFrames)
        syncTime = timer.nsecsElapsed();
    Q_TRACE(QSG_sync_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphPolishAndSync,
                              QQuickProfiler::SceneGraphPolishAndSyncSync);
    Q_TRACE(QSG_animations_entry);

    if (m_animation_timer == 0 && m_animation_driver->isRunning()) {
        qCDebug(QSG_LOG_RENDERLOOP, "- advancing animations");
        m_animation_driver->advance();
        qCDebug(QSG_LOG_RENDERLOOP, "- animations done..");
        // We need to trigger another sync to keep animations running...
        maybePostPolishRequest(w);
        emit timeToIncubate();
    } else if (w->updateDuringSync) {
        maybePostPolishRequest(w);
    }

    qCDebug(QSG_LOG_TIME_RENDERLOOP()).nospace()
            << "Frame prepared with 'threaded' renderloop"
            << ", polish=" << (polishTime / 1000000)
            << ", lock=" << (waitTime - polishTime) / 1000000
            << ", blockedForSync=" << (syncTime - waitTime) / 1000000
            << ", animations=" << (timer.nsecsElapsed() - syncTime) / 1000000
            << " - (on Gui thread) " << window;

    Q_TRACE(QSG_animations_exit);
    Q_QUICK_SG_PROFILE_END(QQuickProfiler::SceneGraphPolishAndSync,
                           QQuickProfiler::SceneGraphPolishAndSyncAnimations);
}

bool QSGThreadedRenderLoop::event(QEvent *e)
{
    switch ((int) e->type()) {

    case QEvent::Timer: {
        QTimerEvent *te = static_cast<QTimerEvent *>(e);
        if (te->timerId() == m_animation_timer) {
            qCDebug(QSG_LOG_RENDERLOOP, "- ticking non-visual timer");
            m_animation_driver->advance();
            emit timeToIncubate();
            return true;
        }
    }

    default:
        break;
    }

    return QObject::event(e);
}



/*
    Locks down GUI and performs a grab the scene graph, then returns the result.

    Since the QML scene could have changed since the last time it was rendered,
    we need to polish and sync the scene graph. This might seem superfluous, but
     - QML changes could have triggered deleteLater() which could have removed
       textures or other objects from the scene graph, causing render to crash.
     - Autotests rely on grab(), setProperty(), grab(), compare behavior.
 */

QImage QSGThreadedRenderLoop::grab(QQuickWindow *window)
{
    qCDebug(QSG_LOG_RENDERLOOP) << "grab()" << window;

    Window *w = windowFor(m_windows, window);
    Q_ASSERT(w);

    if (!w->thread->isRunning())
        return QImage();

    if (!window->handle())
        window->create();

    qCDebug(QSG_LOG_RENDERLOOP, "- polishing items");
    QQuickWindowPrivate *d = QQuickWindowPrivate::get(window);
    d->polishItems();

    QImage result;
    w->thread->mutex.lock();
    m_lockedForSync = true;
    qCDebug(QSG_LOG_RENDERLOOP, "- posting grab event");
    w->thread->postEvent(new WMGrabEvent(window, &result));
    w->thread->waitCondition.wait(&w->thread->mutex);
    m_lockedForSync = false;
    w->thread->mutex.unlock();

    qCDebug(QSG_LOG_RENDERLOOP, "- grab complete");

    return result;
}

/*
 * Posts a new job event to the render thread.
 * Returns true if posting succeeded.
 */
void QSGThreadedRenderLoop::postJob(QQuickWindow *window, QRunnable *job)
{
    Window *w = windowFor(m_windows, window);
    if (w && w->thread && w->thread->window)
        w->thread->postEvent(new WMJobEvent(window, job));
    else
        delete job;
}

#include "qsgthreadedrenderloop.moc"
#include "moc_qsgthreadedrenderloop_p.cpp"

QT_END_NAMESPACE
