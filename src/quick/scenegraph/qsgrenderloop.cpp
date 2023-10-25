// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgrenderloop_p.h"
#include "qsgthreadedrenderloop_p.h"
#include "qsgrhisupport_p.h"
#include <private/qquickanimatorcontroller_p.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QLibraryInfo>
#include <QtCore/private/qabstractanimation_p.h>

#include <QtGui/QOffscreenSurface>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <QPlatformSurfaceEvent>

#include <QtQml/private/qqmlglobal_p.h>

#include <QtQuick/QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <QtQuick/private/qsgrenderer_p.h>
#include <private/qquickprofiler_p.h>
#include <qtquick_tracepoints_p.h>

#include <private/qsgrhishadereffectnode_p.h>

#include <private/qsgdefaultrendercontext_p.h>

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

extern bool qsg_useConsistentTiming();

#define ENABLE_DEFAULT_BACKEND

Q_TRACE_POINT(qtquick, QSG_renderWindow_entry)
Q_TRACE_POINT(qtquick, QSG_renderWindow_exit)
Q_TRACE_POINT(qtquick, QSG_polishItems_entry)
Q_TRACE_POINT(qtquick, QSG_polishItems_exit)
Q_TRACE_POINT(qtquick, QSG_sync_entry)
Q_TRACE_POINT(qtquick, QSG_sync_exit)
Q_TRACE_POINT(qtquick, QSG_render_entry)
Q_TRACE_POINT(qtquick, QSG_render_exit)
Q_TRACE_POINT(qtquick, QSG_swap_entry)
Q_TRACE_POINT(qtquick, QSG_swap_exit)


/*
     - Uses one QRhi per window. (and so each window has its own QOpenGLContext or ID3D11Device(Context) etc.)
     - Animations are advanced using the standard timer (no custom animation
       driver is installed), so QML animations run as expected even when vsync
       throttling is broken.
 */

DEFINE_BOOL_CONFIG_OPTION(qmlNoThreadedRenderer, QML_BAD_GUI_RENDER_LOOP);
DEFINE_BOOL_CONFIG_OPTION(qmlForceThreadedRenderer, QML_FORCE_THREADED_RENDERER); // Might trigger graphics driver threading bugs, use at own risk

QSGRenderLoop *QSGRenderLoop::s_instance = nullptr;

QSGRenderLoop::~QSGRenderLoop()
{
}

void QSGRenderLoop::cleanup()
{
    if (!s_instance)
        return;
    for (QQuickWindow *w : s_instance->windows()) {
        QQuickWindowPrivate *wd = QQuickWindowPrivate::get(w);
        if (wd->windowManager == s_instance) {
           s_instance->windowDestroyed(w);
           wd->windowManager = nullptr;
        }
    }
    delete s_instance;
    s_instance = nullptr;
}

QSurface::SurfaceType QSGRenderLoop::windowSurfaceType() const
{
#ifdef ENABLE_DEFAULT_BACKEND
    return QSGRhiSupport::instance()->windowSurfaceType();
#else
    return QSurface::RasterSurface;
#endif
}

void QSGRenderLoop::postJob(QQuickWindow *window, QRunnable *job)
{
    Q_ASSERT(job);
#ifdef ENABLE_DEFAULT_BACKEND
    Q_ASSERT(window);
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    if (cd->rhi)
        cd->rhi->makeThreadLocalNativeContextCurrent();
    job->run();
#else
    Q_UNUSED(window);
    job->run();
#endif
    delete job;
}

#ifdef ENABLE_DEFAULT_BACKEND
class QSGGuiThreadRenderLoop : public QSGRenderLoop
{
    Q_OBJECT
public:
    QSGGuiThreadRenderLoop();
    ~QSGGuiThreadRenderLoop();

    void show(QQuickWindow *window) override;
    void hide(QQuickWindow *window) override;

    void windowDestroyed(QQuickWindow *window) override;

    void renderWindow(QQuickWindow *window);
    void exposureChanged(QQuickWindow *window) override;
    QImage grab(QQuickWindow *window) override;

    void maybeUpdate(QQuickWindow *window) override;
    void update(QQuickWindow *window) override { maybeUpdate(window); } // identical for this implementation.
    void handleUpdateRequest(QQuickWindow *) override;

    void releaseResources(QQuickWindow *) override;

    QAnimationDriver *animationDriver() const override { return nullptr; }

    QSGContext *sceneGraphContext() const override;
    QSGRenderContext *createRenderContext(QSGContext *) const override;

    void releaseSwapchain(QQuickWindow *window);
    void handleDeviceLoss();

    bool eventFilter(QObject *watched, QEvent *event) override;

    struct WindowData {
        WindowData()
            : updatePending(false),
              rhiDeviceLost(false),
              rhiDoomed(false)
        { }
        QRhi *rhi = nullptr;
        bool ownRhi = true;
        QSGRenderContext *rc = nullptr;
        QElapsedTimer timeBetweenRenders;
        int sampleCount = 1;
        bool updatePending : 1;
        bool rhiDeviceLost : 1;
        bool rhiDoomed : 1;
    };

    bool ensureRhi(QQuickWindow *window, WindowData &data);

    QHash<QQuickWindow *, WindowData> m_windows;

    QOffscreenSurface *offscreenSurface = nullptr;
    QSGContext *sg;
    mutable QSet<QSGRenderContext *> pendingRenderContexts;

    bool m_inPolish = false;
};
#endif

QSGRenderLoop *QSGRenderLoop::instance()
{
    if (!s_instance) {

        QSGRhiSupport::checkEnvQSgInfo();

        s_instance = QSGContext::createWindowManager();
#ifdef ENABLE_DEFAULT_BACKEND
        if (!s_instance) {
            QSGRhiSupport *rhiSupport = QSGRhiSupport::instance();

            QSGRenderLoopType loopType;
            if (rhiSupport->rhiBackend() != QRhi::OpenGLES2) {
                loopType = ThreadedRenderLoop;
            } else {
                if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL))
                    loopType = ThreadedRenderLoop;
                else
                    loopType = BasicRenderLoop;
            }

            switch (rhiSupport->rhiBackend()) {
            case QRhi::Null:
                loopType = BasicRenderLoop;
                break;

            case QRhi::D3D11:
                // The threaded loop's model may not be suitable for DXGI
                // due to the possibility of having the main thread (with
                // the Windows message pump) blocked while issuing a
                // Present on the render thread. However, according to the
                // docs this can be a problem for fullscreen swapchains
                // only. So leave threaded enabled by default for now and
                // revisit later if there are problems.
                break;

            default:
                break;
            }

            // The environment variables can always override. This is good
            // because in some situations it makes perfect sense to try out a
            // render loop that is otherwise disabled by default.

            if (qmlNoThreadedRenderer())
                loopType = BasicRenderLoop;
            else if (qmlForceThreadedRenderer())
                loopType = ThreadedRenderLoop;

            if (Q_UNLIKELY(qEnvironmentVariableIsSet("QSG_RENDER_LOOP"))) {
                const QByteArray loopName = qgetenv("QSG_RENDER_LOOP");
                if (loopName == "windows") {
                    qWarning("The 'windows' render loop is no longer supported. Using 'basic' instead.");
                    loopType = BasicRenderLoop;
                } else if (loopName == "basic") {
                    loopType = BasicRenderLoop;
                } else if (loopName == "threaded") {
                    loopType = ThreadedRenderLoop;
                }
            }

            switch (loopType) {
#if QT_CONFIG(thread)
            case ThreadedRenderLoop:
                qCDebug(QSG_LOG_INFO, "threaded render loop");
                s_instance = new QSGThreadedRenderLoop();
                break;
#endif
            default:
                qCDebug(QSG_LOG_INFO, "basic render loop");
                s_instance = new QSGGuiThreadRenderLoop();
                break;
            }
        }
#endif
        qAddPostRoutine(QSGRenderLoop::cleanup);
    }

    return s_instance;
}

void QSGRenderLoop::setInstance(QSGRenderLoop *instance)
{
    Q_ASSERT(!s_instance);
    s_instance = instance;
}

void QSGRenderLoop::handleContextCreationFailure(QQuickWindow *window)
{
    QString translatedMessage;
    QString untranslatedMessage;
    QQuickWindowPrivate::rhiCreationFailureMessage(QSGRhiSupport::instance()->rhiBackendName(),
                                                   &translatedMessage,
                                                   &untranslatedMessage);
    // If there is a slot connected to the error signal, emit it and leave it to
    // the application to do something with the message. If nothing is connected,
    // show a message on our own and terminate.
    const bool signalEmitted =
        QQuickWindowPrivate::get(window)->emitError(QQuickWindow::ContextNotAvailable,
                                                    translatedMessage);
#if defined(Q_OS_WIN)
    if (!signalEmitted && !QLibraryInfo::isDebugBuild() && !GetConsoleWindow()) {
        MessageBox(0, (LPCTSTR) translatedMessage.utf16(),
                   (LPCTSTR)(QCoreApplication::applicationName().utf16()),
                   MB_OK | MB_ICONERROR);
    }
#endif // Q_OS_WIN
    if (!signalEmitted)
        qFatal("%s", qPrintable(untranslatedMessage));
}

#ifdef ENABLE_DEFAULT_BACKEND
QSGGuiThreadRenderLoop::QSGGuiThreadRenderLoop()
{
    if (qsg_useConsistentTiming()) {
        QUnifiedTimer::instance(true)->setConsistentTiming(true);
        qCDebug(QSG_LOG_INFO, "using fixed animation steps");
    }

    sg = QSGContext::createDefaultContext();
}

QSGGuiThreadRenderLoop::~QSGGuiThreadRenderLoop()
{
    qDeleteAll(pendingRenderContexts);
    delete sg;
}

void QSGGuiThreadRenderLoop::show(QQuickWindow *window)
{
    if (!m_windows.contains(window))
        m_windows.insert(window, {});

    m_windows[window].timeBetweenRenders.start();
    maybeUpdate(window);
}

void QSGGuiThreadRenderLoop::hide(QQuickWindow *window)
{
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    cd->fireAboutToStop();
    auto it = m_windows.find(window);
    if (it != m_windows.end())
        it->updatePending = false;
}

void QSGGuiThreadRenderLoop::windowDestroyed(QQuickWindow *window)
{
    hide(window);

    WindowData data = m_windows.value(window, {});
    m_windows.remove(window);

    QQuickWindowPrivate *d = QQuickWindowPrivate::get(window);

    if (data.rhi) {
        // Direct OpenGL calls in user code need a current context, like
        // when rendering; ensure this (no-op when not running on GL).
        // Also works when there is no handle() anymore.
        data.rhi->makeThreadLocalNativeContextCurrent();
    }

    if (d->swapchain) {
        if (window->handle()) {
            // We get here when exiting via QCoreApplication::quit() instead of
            // through QWindow::close().
            releaseSwapchain(window);
        } else {
            qWarning("QSGGuiThreadRenderLoop cleanup with QQuickWindow %p swapchain %p still alive, this should not happen.",
                     window, d->swapchain);
        }
    }

    d->cleanupNodesOnShutdown();

#if QT_CONFIG(quick_shadereffect)
    QSGRhiShaderEffectNode::resetMaterialTypeCache(window);
#endif

    if (data.rc) {
        data.rc->invalidate();
        delete data.rc;
    }

    if (data.ownRhi)
        QSGRhiSupport::instance()->destroyRhi(data.rhi, d->graphicsConfig);

    d->rhi = nullptr;

    d->animationController.reset();

    if (m_windows.isEmpty()) {
        delete offscreenSurface;
        offscreenSurface = nullptr;
    }
}

void QSGGuiThreadRenderLoop::handleDeviceLoss()
{
    qWarning("Graphics device lost, cleaning up scenegraph and releasing RHIs");

    for (auto it = m_windows.begin(), itEnd = m_windows.end(); it != itEnd; ++it) {
        if (!it->rhi || !it->rhi->isDeviceLost())
            return;

        QQuickWindowPrivate::get(it.key())->cleanupNodesOnShutdown();

        if (it->rc)
            it->rc->invalidate();

        releaseSwapchain(it.key());
        it->rhiDeviceLost = true;

        if (it->ownRhi)
            QSGRhiSupport::instance()->destroyRhi(it->rhi, {});
        it->rhi = nullptr;
    }
}

void QSGGuiThreadRenderLoop::releaseSwapchain(QQuickWindow *window)
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

bool QSGGuiThreadRenderLoop::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::PlatformSurface:
        // this is the proper time to tear down the swapchain (while the native window and surface are still around)
        if (static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            QQuickWindow *w = qobject_cast<QQuickWindow *>(watched);
            if (w)
                releaseSwapchain(w);
            // keep this filter on the window - needed for uncommon but valid
            // sequences of calls like window->destroy(); window->show();
        }
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

bool QSGGuiThreadRenderLoop::ensureRhi(QQuickWindow *window, WindowData &data)
{
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    QSGRhiSupport *rhiSupport = QSGRhiSupport::instance();
    bool ok = data.rhi != nullptr;

    if (!data.rhi) {
        // This block below handles both the initial QRhi initialization and
        // also the subsequent reinitialization attempts after a device lost
        // (reset) situation.

        if (data.rhiDoomed) // no repeated attempts if the initial attempt failed
            return false;

        if (!offscreenSurface)
            offscreenSurface = rhiSupport->maybeCreateOffscreenSurface(window);

        QSGRhiSupport::RhiCreateResult rhiResult = rhiSupport->createRhi(window, offscreenSurface);
        data.rhi = rhiResult.rhi;
        data.ownRhi = rhiResult.own;

        if (data.rhi) {
            data.rhiDeviceLost = false;

            ok = true;
            // We need to guarantee that sceneGraphInitialized is
            // emitted with a context current, if running with OpenGL.
            data.rhi->makeThreadLocalNativeContextCurrent();

            // The sample count cannot vary between windows as we use the same
            // rendercontext for all of them. Decide it here and now.
            data.sampleCount = rhiSupport->chooseSampleCountForWindowWithRhi(window, data.rhi);

            cd->rhi = data.rhi; // set this early in case something hooked up to rc initialized() accesses it

            QSGDefaultRenderContext::InitParams rcParams;
            rcParams.rhi = data.rhi;
            rcParams.sampleCount = data.sampleCount;
            rcParams.initialSurfacePixelSize = window->size() * window->effectiveDevicePixelRatio();
            rcParams.maybeSurface = window;
            cd->context->initialize(&rcParams);
        } else {
            if (!data.rhiDeviceLost) {
                data.rhiDoomed = true;
                handleContextCreationFailure(window);
            }
            // otherwise no error, just return false so that we will retry on a subsequent rendering attempt
        }
    }

    if (data.rhi && !cd->swapchain) {
        // if it's not the first window then the rhi is not yet stored to
        // QQuickWindowPrivate, do it now
        cd->rhi = data.rhi;

        // Unlike the threaded render loop, we use the same rhi for all windows
        // and so createRhi() is called only once. Certain initialization may
        // need to be done on a per window basis still, so make sure it is done.
        rhiSupport->prepareWindowForRhi(window);

        QRhiSwapChain::Flags flags = QRhiSwapChain::UsedAsTransferSource; // may be used in a grab
        const QSurfaceFormat requestedFormat = window->requestedFormat();

        // QQ is always premul alpha. Decide based on alphaBufferSize in
        // requestedFormat(). (the platform plugin can override format() but
        // what matters here is what the application wanted, hence using the
        // requested one)
        const bool alpha = requestedFormat.alphaBufferSize() > 0;
        if (alpha)
            flags |= QRhiSwapChain::SurfaceHasPreMulAlpha;

        // Request NoVSync if swap interval was set to 0 (either by the app or
        // by QSG_NO_VSYNC). What this means in practice is another question,
        // but at least we tried.
        if (requestedFormat.swapInterval() == 0) {
            qCDebug(QSG_LOG_INFO, "Swap interval is 0, attempting to disable vsync when presenting.");
            flags |= QRhiSwapChain::NoVSync;
        }

        cd->swapchain = data.rhi->newSwapChain();
        static bool depthBufferEnabled = qEnvironmentVariableIsEmpty("QSG_NO_DEPTH_BUFFER");
        if (depthBufferEnabled) {
            cd->depthStencilForSwapchain = data.rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil,
                                                                     QSize(),
                                                                     data.sampleCount,
                                                                     QRhiRenderBuffer::UsedWithSwapChainOnly);
            cd->swapchain->setDepthStencil(cd->depthStencilForSwapchain);
        }
        cd->swapchain->setWindow(window);
        rhiSupport->applySwapChainFormat(cd->swapchain, window);
        qCDebug(QSG_LOG_INFO, "MSAA sample count for the swapchain is %d. Alpha channel requested = %s",
                data.sampleCount, alpha ? "yes" : "no");
        cd->swapchain->setSampleCount(data.sampleCount);
        cd->swapchain->setFlags(flags);
        cd->rpDescForSwapchain = cd->swapchain->newCompatibleRenderPassDescriptor();
        cd->swapchain->setRenderPassDescriptor(cd->rpDescForSwapchain);

        window->installEventFilter(this);
    }

    if (!data.rc) {
        QSGRenderContext *rc = cd->context;
        pendingRenderContexts.remove(rc);
        data.rc = rc;
        if (!data.rc)
            qWarning("No QSGRenderContext for window %p, this should not happen", window);
    }

    return ok;
}

void QSGGuiThreadRenderLoop::renderWindow(QQuickWindow *window)
{
    auto winDataIt = m_windows.find(window);
    if (winDataIt == m_windows.end())
        return;

    WindowData &data(*winDataIt);
    bool alsoSwap = data.updatePending;
    data.updatePending = false;

    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    if (!cd->isRenderable())
        return;

    if (!cd->updatesEnabled)
        return;

    if (!ensureRhi(window, data))
        return;

    bool lastDirtyWindow = true;
    for (auto it = m_windows.cbegin(), end = m_windows.cend(); it != end; ++it) {
        if (it->updatePending) {
            lastDirtyWindow = false;
            break;
        }
    }

    cd->deliveryAgentPrivate()->flushFrameSynchronousEvents(window);
    // Event delivery/processing triggered the window to be deleted or stop rendering.
    if (!m_windows.contains(window))
        return;

    QSize effectiveOutputSize; // always prefer what the surface tells us, not the QWindow
    if (cd->swapchain) {
        effectiveOutputSize = cd->swapchain->surfacePixelSize();
        // An update request could still be delivered right before we get an
        // unexpose. With Vulkan on Windows for example attempting to render
        // leads to failures at this stage since the surface size is already 0.
        if (effectiveOutputSize.isEmpty())
            return;
    }

    Q_TRACE_SCOPE(QSG_renderWindow);
    QElapsedTimer renderTimer;
    qint64 renderTime = 0, syncTime = 0, polishTime = 0;
    const bool profileFrames = QSG_LOG_TIME_RENDERLOOP().isDebugEnabled();
    if (profileFrames)
        renderTimer.start();
    Q_TRACE(QSG_polishItems_entry);
    Q_QUICK_SG_PROFILE_START(QQuickProfiler::SceneGraphPolishFrame);

    m_inPolish = true;
    cd->polishItems();
    m_inPolish = false;

    if (profileFrames)
        polishTime = renderTimer.nsecsElapsed();

    Q_TRACE(QSG_polishItems_exit);
    Q_QUICK_SG_PROFILE_SWITCH(QQuickProfiler::SceneGraphPolishFrame,
                              QQuickProfiler::SceneGraphRenderLoopFrame,
                              QQuickProfiler::SceneGraphPolishPolish);
    Q_TRACE(QSG_sync_entry);

    emit window->afterAnimating();

    // Begin the frame before syncing -> sync is where we may invoke
    // updatePaintNode() on the items and they may want to do resource updates.
    // Also relevant for applications that connect to the before/afterSynchronizing
    // signals and want to do graphics stuff already there.
    if (cd->swapchain) {
        Q_ASSERT(!effectiveOutputSize.isEmpty());
        const QSize previousOutputSize = cd->swapchain->currentPixelSize();
        if (previousOutputSize != effectiveOutputSize || cd->swapchainJustBecameRenderable) {
            if (cd->swapchainJustBecameRenderable)
                qCDebug(QSG_LOG_RENDERLOOP, "just became exposed");

            cd->hasActiveSwapchain = cd->swapchain->createOrResize();
            if (!cd->hasActiveSwapchain && data.rhi->isDeviceLost()) {
                handleDeviceLoss();
                return;
            }

            cd->swapchainJustBecameRenderable = false;
            cd->hasRenderableSwapchain = cd->hasActiveSwapchain;

            if (cd->hasActiveSwapchain) {
                // surface size atomicity: now that buildOrResize() succeeded,
                // query the size that was used in there by the swapchain, and
                // that is the size we will use while preparing the next frame.
                effectiveOutputSize = cd->swapchain->currentPixelSize();
                qCDebug(QSG_LOG_RENDERLOOP) << "rhi swapchain size" << effectiveOutputSize;
            } else {
                qWarning("Failed to build or resize swapchain");
            }
        }

        emit window->beforeFrameBegin();

        Q_ASSERT(data.rhi == cd->rhi);
        QRhi::FrameOpResult frameResult = data.rhi->beginFrame(cd->swapchain);
        if (frameResult != QRhi::FrameOpSuccess) {
            if (frameResult == QRhi::FrameOpDeviceLost)
                handleDeviceLoss();
            else if (frameResult == QRhi::FrameOpError)
                qWarning("Failed to start frame");
            // out of date is not worth warning about - it may happen even during resizing on some platforms
            emit window->afterFrameEnd();
            return;
        }
    }

    // Enable external OpenGL rendering connected to one of the
    // QQuickWindow signals (beforeSynchronizing, beforeRendering,
    // etc.) to function like it did on the direct OpenGL path,
    // i.e. ensure there is a context current, just in case.
    data.rhi->makeThreadLocalNativeContextCurrent();

    cd->syncSceneGraph();
    if (lastDirtyWindow)
        data.rc->endSync();

    if (profileFrames)
        syncTime = renderTimer.nsecsElapsed();

    Q_TRACE(QSG_sync_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRenderLoopFrame,
                              QQuickProfiler::SceneGraphRenderLoopSync);
    Q_TRACE(QSG_render_entry);

    cd->renderSceneGraph();

    if (profileFrames)
        renderTime = renderTimer.nsecsElapsed();
    Q_TRACE(QSG_render_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRenderLoopFrame,
                              QQuickProfiler::SceneGraphRenderLoopRender);
    Q_TRACE(QSG_swap_entry);

    const bool needsPresent = alsoSwap && window->isVisible();
    double lastCompletedGpuTime = 0;
    if (cd->swapchain) {
        QRhi::EndFrameFlags flags;
        if (!needsPresent)
            flags |= QRhi::SkipPresent;
        QRhi::FrameOpResult frameResult = data.rhi->endFrame(cd->swapchain, flags);
        if (frameResult != QRhi::FrameOpSuccess) {
            if (frameResult == QRhi::FrameOpDeviceLost)
                handleDeviceLoss();
            else if (frameResult == QRhi::FrameOpError)
                qWarning("Failed to end frame");
        } else {
            lastCompletedGpuTime = cd->swapchain->currentFrameCommandBuffer()->lastCompletedGpuTime();
        }
    }
    if (needsPresent)
        cd->fireFrameSwapped();

    emit window->afterFrameEnd();

    qint64 swapTime = 0;
    if (profileFrames)
        swapTime = renderTimer.nsecsElapsed();

    Q_TRACE(QSG_swap_exit);
    Q_QUICK_SG_PROFILE_END(QQuickProfiler::SceneGraphRenderLoopFrame,
                           QQuickProfiler::SceneGraphRenderLoopSwap);

    if (profileFrames) {
        qCDebug(QSG_LOG_TIME_RENDERLOOP,
                "[window %p][gui thread] syncAndRender: frame rendered in %dms, polish=%d, sync=%d, render=%d, swap=%d, perWindowFrameDelta=%d",
                window,
                int(swapTime / 1000000),
                int(polishTime / 1000000),
                int((syncTime - polishTime) / 1000000),
                int((renderTime - syncTime) / 1000000),
                int((swapTime - renderTime) / 1000000),
                int(data.timeBetweenRenders.restart()));
        if (!qFuzzyIsNull(lastCompletedGpuTime) && cd->graphicsConfig.timestampsEnabled()) {
            qCDebug(QSG_LOG_TIME_RENDERLOOP, "[window %p][gui thread] syncAndRender: last retrieved GPU frame time was %.4f ms",
                    window,
                    lastCompletedGpuTime * 1000.0);
        }
    }

    // Might have been set during syncSceneGraph()
    if (data.updatePending)
        maybeUpdate(window);
}

void QSGGuiThreadRenderLoop::exposureChanged(QQuickWindow *window)
{
    QQuickWindowPrivate *wd = QQuickWindowPrivate::get(window);

    // This is tricker than used to be. We want to detect having an empty
    // surface size (which may be the case even when window->size() is
    // non-empty, on some platforms with some graphics APIs!) as well as the
    // case when the window just became "newly exposed" (e.g. after a
    // minimize-restore on Windows, or when switching between fully obscured -
    // not fully obscured on macOS)

    if (!window->isExposed() || (wd->hasActiveSwapchain && wd->swapchain->surfacePixelSize().isEmpty()))
        wd->hasRenderableSwapchain = false;

    if (window->isExposed() && !wd->hasRenderableSwapchain && wd->hasActiveSwapchain
            && !wd->swapchain->surfacePixelSize().isEmpty())
    {
        wd->hasRenderableSwapchain = true;
        wd->swapchainJustBecameRenderable = true;
    }

    auto winDataIt = m_windows.find(window);
    if (winDataIt != m_windows.end()) {
        if (window->isExposed() && (!winDataIt->rhi || !wd->hasActiveSwapchain || wd->hasRenderableSwapchain)) {
            winDataIt->updatePending = true;
            renderWindow(window);
        }
    }
}

QImage QSGGuiThreadRenderLoop::grab(QQuickWindow *window)
{
    auto winDataIt = m_windows.find(window);
    if (winDataIt == m_windows.end())
        return QImage();

    if (!ensureRhi(window, *winDataIt))
        return QImage();

    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    m_inPolish = true;
    cd->polishItems();
    m_inPolish = false;

    // The assumption is that the swapchain is usable since on expose we do a
    // renderWindow() so one cannot get to grab() without having done at least
    // one on-screen frame.
    cd->rhi->beginFrame(cd->swapchain);
    cd->rhi->makeThreadLocalNativeContextCurrent(); // for custom GL rendering before/during/after sync
    cd->syncSceneGraph();
    cd->renderSceneGraph();
    QImage image = QSGRhiSupport::instance()->grabAndBlockInCurrentFrame(cd->rhi, cd->swapchain->currentFrameCommandBuffer());
    cd->rhi->endFrame(cd->swapchain, QRhi::SkipPresent);

    image.setDevicePixelRatio(window->effectiveDevicePixelRatio());
    return image;
}

void QSGGuiThreadRenderLoop::maybeUpdate(QQuickWindow *window)
{
    auto winDataIt = m_windows.find(window);
    if (winDataIt == m_windows.end())
        return;

    // Even if the window is not renderable,
    // renderWindow() called on different window
    // should not delete QSGTexture's
    // from this unrenderable window.
    winDataIt->updatePending = true;

    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    if (!cd->isRenderable())
        return;

    // An updatePolish() implementation may call update() to get the QQuickItem
    // dirtied. That's fine but it also leads to calling this function.
    // Requesting another update is a waste then since the updatePolish() call
    // will be followed up with a round of sync and render.
    if (m_inPolish)
        return;

    window->requestUpdate();
}

QSGContext *QSGGuiThreadRenderLoop::sceneGraphContext() const
{
    return sg;
}

QSGRenderContext *QSGGuiThreadRenderLoop::createRenderContext(QSGContext *sg) const
{
    QSGRenderContext *rc = sg->createRenderContext();
    pendingRenderContexts.insert(rc);
    return rc;
}

void QSGGuiThreadRenderLoop::releaseResources(QQuickWindow *w)
{
    // No full invalidation of the rendercontext, just clear some caches.
    QQuickWindowPrivate *d = QQuickWindowPrivate::get(w);
    emit d->context->releaseCachedResourcesRequested();
    if (d->renderer)
        d->renderer->releaseCachedResources();
#if QT_CONFIG(quick_shadereffect)
    QSGRhiShaderEffectNode::garbageCollectMaterialTypeCache(w);
#endif
}

void QSGGuiThreadRenderLoop::handleUpdateRequest(QQuickWindow *window)
{
    renderWindow(window);
}

#endif // ENABLE_DEFAULT_BACKEND

QT_END_NAMESPACE

#include "qsgrenderloop.moc"
#include "moc_qsgrenderloop_p.cpp"
