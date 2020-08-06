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

#include "qsgrenderloop_p.h"
#include "qsgthreadedrenderloop_p.h"
#include "qsgwindowsrenderloop_p.h"
#include "qsgrhisupport_p.h"
#include <private/qquickanimatorcontroller_p.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QTime>
#include <QtCore/QLibraryInfo>
#include <QtCore/private/qabstractanimation_p.h>

#include <QtGui/QOffscreenSurface>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <QPlatformSurfaceEvent>

#include <QtQml/private/qqmlglobal_p.h>

#include <QtQuick/QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <QtQuick/private/qsgrenderer_p.h>
#include <private/qquickprofiler_p.h>
#include <qtquick_tracepoints_p.h>

#include <private/qsgrhishadereffectnode_p.h>

#if QT_CONFIG(opengl)
#include <QtGui/QOpenGLContext>
#if QT_CONFIG(quick_shadereffect)
#include <private/qquickopenglshadereffectnode_p.h>
#endif
#include <private/qsgdefaultrendercontext_p.h>
#endif

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

extern bool qsg_useConsistentTiming();
extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

// ### We do not yet support using Qt Quick with QRhi (and Vulkan, D3D
// or Metal) in -no-opengl builds as of Qt 5.14. This is due to to the
// widespread direct OpenGL usage all over the place in qsgdefault*
// and the related classes. To be cleaned up in Qt 6 when the direct
// GL code path is removed.

#if QT_CONFIG(opengl) /* || QT_CONFIG(vulkan) || defined(Q_OS_WIN) || defined(Q_OS_DARWIN) */

#define ENABLE_DEFAULT_BACKEND

/*
    expectations for this manager to work:
     - one opengl context to render multiple windows
     - OpenGL pipeline will not block for vsync in swap
     - OpenGL pipeline will block based on a full buffer queue.
     - Multiple screens can share the OpenGL context
     - Animations are advanced for all windows once per swap
 */

DEFINE_BOOL_CONFIG_OPTION(qmlNoThreadedRenderer, QML_BAD_GUI_RENDER_LOOP);
DEFINE_BOOL_CONFIG_OPTION(qmlForceThreadedRenderer, QML_FORCE_THREADED_RENDERER); // Might trigger graphics driver threading bugs, use at own risk
#endif

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

#ifdef ENABLE_DEFAULT_BACKEND
    QSGRhiSupport::instance()->cleanup();
    QSGRhiProfileConnection::instance()->cleanup();
#endif
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
    if (!QSGRhiSupport::instance()->isRhiEnabled()) {
        if (window->openglContext()) {
            window->openglContext()->makeCurrent(window);
            job->run();
        }
    } else {
        QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
        if (cd->rhi)
            cd->rhi->makeThreadLocalNativeContextCurrent();
        job->run();
    }
#else
    Q_UNUSED(window)
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
    QSGRenderContext *createRenderContext(QSGContext *) const override { return rc; }

    void releaseSwapchain(QQuickWindow *window);
    void handleDeviceLoss();

    bool eventFilter(QObject *watched, QEvent *event) override;

    struct WindowData {
        WindowData()
            : updatePending(false),
              grabOnly(false),
              rhiDeviceLost(false),
              rhiDoomed(false)
        { }
        bool updatePending : 1;
        bool grabOnly : 1;
        bool rhiDeviceLost : 1;
        bool rhiDoomed : 1;
    };

    QHash<QQuickWindow *, WindowData> m_windows;

    QOpenGLContext *gl = nullptr;
    QOffscreenSurface *offscreenSurface = nullptr;
    QRhi *rhi = nullptr;
    QSGContext *sg;
    QSGRenderContext *rc;

    QImage grabContent;
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
            if (rhiSupport->isRhiEnabled() && rhiSupport->rhiBackend() != QRhi::OpenGLES2) {
                loopType = ThreadedRenderLoop;
            } else {
                loopType = BasicRenderLoop;
#ifdef Q_OS_WIN
                // With desktop OpenGL (opengl32.dll), use threaded. Otherwise (ANGLE) use windows.
                if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL
                        && QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL))
                {
                    loopType = ThreadedRenderLoop;
                } else {
                    loopType = WindowsRenderLoop;
                }
#else
                if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL))
                    loopType = ThreadedRenderLoop;
#endif
            }

            if (rhiSupport->isRhiEnabled()) {
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

                // no 'windows' because that's not yet ported to the rhi
                if (loopType == WindowsRenderLoop)
                    loopType = BasicRenderLoop;
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
                if (loopName == "windows")
                    loopType = WindowsRenderLoop;
                else if (loopName == "basic")
                    loopType = BasicRenderLoop;
                else if (loopName == "threaded")
                    loopType = ThreadedRenderLoop;
            }

            switch (loopType) {
#if QT_CONFIG(thread)
            case ThreadedRenderLoop:
                qCDebug(QSG_LOG_INFO, "threaded render loop");
                s_instance = new QSGThreadedRenderLoop();
                break;
#endif
            case WindowsRenderLoop:
                qCDebug(QSG_LOG_INFO, "windows render loop");
                s_instance = new QSGWindowsRenderLoop();
                break;
            default:
                qCDebug(QSG_LOG_INFO, "QSG: basic render loop");
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
    if (QSGRhiSupport::instance()->isRhiEnabled()) {
        QQuickWindowPrivate::rhiCreationFailureMessage(QSGRhiSupport::instance()->rhiBackendName(),
                                                       &translatedMessage,
                                                       &untranslatedMessage);
    } else {
        QQuickWindowPrivate::contextCreationFailureMessage(window->requestedFormat(),
                                                           &translatedMessage,
                                                           &untranslatedMessage);
    }
    // If there is a slot connected to the error signal, emit it and leave it to
    // the application to do something with the message. If nothing is connected,
    // show a message on our own and terminate.
    const bool signalEmitted =
        QQuickWindowPrivate::get(window)->emitError(QQuickWindow::ContextNotAvailable,
                                                    translatedMessage);
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    if (!signalEmitted && !QLibraryInfo::isDebugBuild() && !GetConsoleWindow()) {
        MessageBox(0, (LPCTSTR) translatedMessage.utf16(),
                   (LPCTSTR)(QCoreApplication::applicationName().utf16()),
                   MB_OK | MB_ICONERROR);
    }
#endif // Q_OS_WIN && !Q_OS_WINRT
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
    rc = sg->createRenderContext();
}

QSGGuiThreadRenderLoop::~QSGGuiThreadRenderLoop()
{
    delete rc;
    delete sg;
}

void QSGGuiThreadRenderLoop::show(QQuickWindow *window)
{
    m_windows[window] = WindowData();

    maybeUpdate(window);
}

void QSGGuiThreadRenderLoop::hide(QQuickWindow *window)
{
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    cd->fireAboutToStop();
    if (m_windows.contains(window))
        m_windows[window].updatePending = false;
}

void QSGGuiThreadRenderLoop::windowDestroyed(QQuickWindow *window)
{
    m_windows.remove(window);
    hide(window);
    QQuickWindowPrivate *d = QQuickWindowPrivate::get(window);

    bool current = false;
    if (gl || rhi) {
        if (rhi) {
            // Direct OpenGL calls in user code need a current context, like
            // when rendering; ensure this (no-op when not running on GL).
            // Also works when there is no handle() anymore.
            rhi->makeThreadLocalNativeContextCurrent();
            current = true;
        } else {
            QSurface *surface = window;
            // There may be no platform window if the window got closed.
            if (!window->handle())
                surface = offscreenSurface;
            current = gl->makeCurrent(surface);
        }
        if (Q_UNLIKELY(!current))
            qCDebug(QSG_LOG_RENDERLOOP, "cleanup without an OpenGL context");
    }

#if QT_CONFIG(quick_shadereffect)
    QSGRhiShaderEffectNode::cleanupMaterialTypeCache();
#if QT_CONFIG(opengl)
    QQuickOpenGLShaderEffectMaterial::cleanupMaterialCache();
#endif
#endif

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
    if (m_windows.size() == 0) {
        rc->invalidate();
        d->rhi = nullptr;
        delete rhi;
        rhi = nullptr;
        delete gl;
        gl = nullptr;
        delete offscreenSurface;
        offscreenSurface = nullptr;
    } else if (gl && window == gl->surface() && current) {
        if (!rhi)
            gl->doneCurrent();
    }

    d->animationController.reset();
}

void QSGGuiThreadRenderLoop::handleDeviceLoss()
{
    if (!rhi || !rhi->isDeviceLost())
        return;

    qWarning("Graphics device lost, cleaning up scenegraph and releasing RHI");

    for (auto it = m_windows.constBegin(), itEnd = m_windows.constEnd(); it != itEnd; ++it)
        QQuickWindowPrivate::get(it.key())->cleanupNodesOnShutdown();

    rc->invalidate();

    for (auto it = m_windows.begin(), itEnd = m_windows.end(); it != itEnd; ++it) {
        releaseSwapchain(it.key());
        it->rhiDeviceLost = true;
    }

    delete rhi;
    rhi = nullptr;
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
            if (w) {
                releaseSwapchain(w);
                w->removeEventFilter(this);
            }
        }
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

void QSGGuiThreadRenderLoop::renderWindow(QQuickWindow *window)
{
    if (!m_windows.contains(window))
        return;

    WindowData &data = const_cast<WindowData &>(m_windows[window]);
    bool alsoSwap = data.updatePending;
    data.updatePending = false;

    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    if (!cd->isRenderable())
        return;

    bool current = false;
    QSGRhiSupport *rhiSupport = QSGRhiSupport::instance();
    int rhiSampleCount = 1;
    const bool enableRhi = rhiSupport->isRhiEnabled();

    if (enableRhi && !rhi) {
        // This block below handles both the initial QRhi initialization and
        // also the subsequent reinitialization attempts after a device lost
        // (reset) situation.

        if (data.rhiDoomed) // no repeated attempts if the initial attempt failed
            return;

        if (!offscreenSurface)
            offscreenSurface = rhiSupport->maybeCreateOffscreenSurface(window);

        rhi = rhiSupport->createRhi(window, offscreenSurface);

        if (rhi) {
            if (rhiSupport->isProfilingRequested())
                QSGRhiProfileConnection::instance()->initialize(rhi);

            data.rhiDeviceLost = false;

            current = true;
            rhi->makeThreadLocalNativeContextCurrent();

            // The sample count cannot vary between windows as we use the same
            // rendercontext for all of them. Decide it here and now.
            rhiSampleCount = rhiSupport->chooseSampleCountForWindowWithRhi(window, rhi);

            cd->rhi = rhi; // set this early in case something hooked up to rc initialized() accesses it

            QSGDefaultRenderContext::InitParams rcParams;
            rcParams.rhi = rhi;
            rcParams.sampleCount = rhiSampleCount;
            rcParams.initialSurfacePixelSize = window->size() * window->effectiveDevicePixelRatio();
            rcParams.maybeSurface = window;
            cd->context->initialize(&rcParams);
        } else {
            if (!data.rhiDeviceLost) {
                data.rhiDoomed = true;
                handleContextCreationFailure(window);
            }
            // otherwise no error, will retry on a subsequent rendering attempt
        }
    } else if (!enableRhi && !gl) {
        gl = new QOpenGLContext();
        gl->setFormat(window->requestedFormat());
        gl->setScreen(window->screen());
        if (qt_gl_global_share_context())
            gl->setShareContext(qt_gl_global_share_context());
        if (!gl->create()) {
            delete gl;
            gl = nullptr;
            handleContextCreationFailure(window);
        } else {
            if (!offscreenSurface) {
                offscreenSurface = new QOffscreenSurface;
                offscreenSurface->setFormat(gl->format());
                offscreenSurface->create();
            }
            cd->fireOpenGLContextCreated(gl);
            current = gl->makeCurrent(window);
        }
        if (current) {
            QSGDefaultRenderContext::InitParams rcParams;
            rcParams.sampleCount = qMax(1, gl->format().samples());
            rcParams.openGLContext = gl;
            rcParams.initialSurfacePixelSize = window->size() * window->effectiveDevicePixelRatio();
            rcParams.maybeSurface = window;
            cd->context->initialize(&rcParams);
        }
    } else {
        if (rhi) {
            current = true;
            // With the rhi making the (OpenGL) context current serves only one
            // purpose: to enable external OpenGL rendering connected to one of
            // the QQuickWindow signals (beforeSynchronizing, beforeRendering,
            // etc.) to function like it did on the direct OpenGL path. For our
            // own rendering this call would not be necessary.
            rhi->makeThreadLocalNativeContextCurrent();
        } else {
            current = gl->makeCurrent(window);
        }
    }

    if (enableRhi && rhi && !cd->swapchain) {
        // if it's not the first window then the rhi is not yet stored to
        // QQuickWindowPrivate, do it now
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
        qCDebug(QSG_LOG_INFO, "MSAA sample count for the swapchain is %d. Alpha channel requested = %s",
                rhiSampleCount, alpha ? "yes" : "no");
        cd->swapchain->setSampleCount(rhiSampleCount);
        cd->swapchain->setFlags(flags);
        cd->rpDescForSwapchain = cd->swapchain->newCompatibleRenderPassDescriptor();
        cd->swapchain->setRenderPassDescriptor(cd->rpDescForSwapchain);

        window->installEventFilter(this);
    }

    bool lastDirtyWindow = true;
    auto i = m_windows.constBegin();
    while (i != m_windows.constEnd()) {
        if (i.value().updatePending) {
            lastDirtyWindow = false;
            break;
        }
        i++;
    }

    // Check for context loss. (legacy GL only)
    if (!current && !rhi && !gl->isValid()) {
        for (auto it = m_windows.constBegin() ; it != m_windows.constEnd(); it++) {
            QQuickWindowPrivate *windowPrivate = QQuickWindowPrivate::get(it.key());
            windowPrivate->cleanupNodesOnShutdown();
        }
        rc->invalidate();
        current = gl->create() && gl->makeCurrent(window);
        if (current) {
            QSGDefaultRenderContext::InitParams rcParams;
            rcParams.sampleCount = qMax(1, gl->format().samples());
            rcParams.openGLContext = gl;
            rcParams.initialSurfacePixelSize = window->size() * window->effectiveDevicePixelRatio();
            rcParams.maybeSurface = window;
            rc->initialize(&rcParams);
        }
    }

    if (!current)
        return;

    if (!data.grabOnly) {
        cd->flushFrameSynchronousEvents();
        // Event delivery/processing triggered the window to be deleted or stop rendering.
        if (!m_windows.contains(window))
            return;
    }

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
    bool profileFrames = QSG_LOG_TIME_RENDERLOOP().isDebugEnabled();
    if (profileFrames)
        renderTimer.start();
    Q_TRACE(QSG_polishItems_entry);
    Q_QUICK_SG_PROFILE_START(QQuickProfiler::SceneGraphPolishFrame);

    cd->polishItems();

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

            cd->hasActiveSwapchain = cd->swapchain->buildOrResize();
            if (!cd->hasActiveSwapchain && rhi->isDeviceLost()) {
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

        Q_ASSERT(rhi == cd->rhi);
        // ### the flag should only be set when the app requests it, but there's no way to do that right now
        QRhi::BeginFrameFlags frameFlags = QRhi::ExternalContentsInPass;
        QRhi::FrameOpResult frameResult = rhi->beginFrame(cd->swapchain, frameFlags);
        if (frameResult != QRhi::FrameOpSuccess) {
            if (frameResult == QRhi::FrameOpDeviceLost)
                handleDeviceLoss();
            else if (frameResult == QRhi::FrameOpError)
                qWarning("Failed to start frame");
            // out of date is not worth warning about - it may happen even during resizing on some platforms
            return;
        }
    }

    cd->syncSceneGraph();
    if (lastDirtyWindow)
        rc->endSync();

    if (profileFrames)
        syncTime = renderTimer.nsecsElapsed();

    Q_TRACE(QSG_sync_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRenderLoopFrame,
                              QQuickProfiler::SceneGraphRenderLoopSync);
    Q_TRACE(QSG_render_entry);

    cd->renderSceneGraph(window->size(), effectiveOutputSize);

    if (profileFrames)
        renderTime = renderTimer.nsecsElapsed();
    Q_TRACE(QSG_render_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRenderLoopFrame,
                              QQuickProfiler::SceneGraphRenderLoopRender);
    Q_TRACE(QSG_swap_entry);

    if (data.grabOnly) {
        const bool alpha = window->format().alphaBufferSize() > 0 && window->color().alpha() != 255;
        if (cd->swapchain)
            grabContent = rhiSupport->grabAndBlockInCurrentFrame(rhi, cd->swapchain);
        else
            grabContent = qt_gl_read_framebuffer(window->size() * window->effectiveDevicePixelRatio(), alpha, alpha);
        grabContent.setDevicePixelRatio(window->effectiveDevicePixelRatio());
        data.grabOnly = false;
    }

    const bool needsPresent = alsoSwap && window->isVisible();
    if (cd->swapchain) {
        QRhi::EndFrameFlags flags;
        if (!needsPresent)
            flags |= QRhi::SkipPresent;
        QRhi::FrameOpResult frameResult = rhi->endFrame(cd->swapchain, flags);
        if (frameResult != QRhi::FrameOpSuccess) {
            if (frameResult == QRhi::FrameOpDeviceLost)
                handleDeviceLoss();
            else if (frameResult == QRhi::FrameOpError)
                qWarning("Failed to end frame");
        }
    } else if (needsPresent) {
        if (!cd->customRenderStage || !cd->customRenderStage->swap())
            gl->swapBuffers(window);
    }
    if (needsPresent)
        cd->fireFrameSwapped();

    qint64 swapTime = 0;
    if (profileFrames)
        swapTime = renderTimer.nsecsElapsed();

    Q_TRACE(QSG_swap_exit);
    Q_QUICK_SG_PROFILE_END(QQuickProfiler::SceneGraphRenderLoopFrame,
                           QQuickProfiler::SceneGraphRenderLoopSwap);

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

    QSGRhiProfileConnection::instance()->send(rhi);

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

    if (window->isExposed() && (!rhi || !wd->hasActiveSwapchain || wd->hasRenderableSwapchain)) {
        m_windows[window].updatePending = true;
        renderWindow(window);
    }
}

QImage QSGGuiThreadRenderLoop::grab(QQuickWindow *window)
{
    if (!m_windows.contains(window))
        return QImage();

    m_windows[window].grabOnly = true;

    renderWindow(window);

    QImage grabbed = grabContent;
    grabContent = QImage();
    return grabbed;
}

void QSGGuiThreadRenderLoop::maybeUpdate(QQuickWindow *window)
{
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    if (!m_windows.contains(window))
        return;

    // Even if the window is not renderable,
    // renderWindow() called on different window
    // should not delete QSGTexture's
    // from this unrenderable window.
    m_windows[window].updatePending = true;

    if (!cd->isRenderable())
        return;

    window->requestUpdate();
}

QSGContext *QSGGuiThreadRenderLoop::sceneGraphContext() const
{
    return sg;
}

void QSGGuiThreadRenderLoop::releaseResources(QQuickWindow *w)
{
    // No full invalidation of the rendercontext, just clear some caches.
    QQuickWindowPrivate *d = QQuickWindowPrivate::get(w);
    if (d->renderer)
        d->renderer->releaseCachedResources();
}

void QSGGuiThreadRenderLoop::handleUpdateRequest(QQuickWindow *window)
{
    renderWindow(window);
}

#endif // ENABLE_DEFAULT_BACKEND

#include "qsgrenderloop.moc"
#include "moc_qsgrenderloop_p.cpp"

QT_END_NAMESPACE
