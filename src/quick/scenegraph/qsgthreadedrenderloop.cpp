/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

#include <QtQuick/QQuickWindow>
#include <private/qquickwindow_p.h>

#include <QtQuick/private/qsgrenderer_p.h>

#include "qsgthreadedrenderloop_p.h"
#include <private/qquickanimatorcontroller_p.h>

#include <private/qquickprofiler_p.h>
#include <private/qqmldebugservice_p.h>

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


// #define QSG_RENDER_LOOP_DEBUG

#if defined (QSG_RENDER_LOOP_DEBUG)
QElapsedTimer qsgrl_timer;
#  define QSG_RT_DEBUG(MSG)       qDebug("(%6d) line=%4d - win=%10p):                       Render: %s", (int) qsgrl_timer.elapsed(), __LINE__, window, MSG);
#  define QSG_GUI_DEBUG(WIN, MSG) qDebug("(%6d) line=%4d - win=%10p): Gui: %s", (int) qsgrl_timer.elapsed(), __LINE__, WIN, MSG);
#else
#  define QSG_RT_DEBUG(MSG)
#  define QSG_GUI_DEBUG(WIN,MSG)
#endif


static int get_env_int(const char *name, int defaultValue)
{
    QByteArray content = qgetenv(name);

    bool ok = false;
    int value = content.toInt(&ok);
    return ok ? value : defaultValue;
}


static inline int qsgrl_animation_interval() {
    qreal refreshRate = QGuiApplication::primaryScreen()->refreshRate();
    // To work around that some platforms wrongfully return 0 or something
    // bogus for refreshrate
    if (refreshRate < 1)
        return 16;
    return int(1000 / refreshRate);
}


#ifndef QSG_NO_RENDER_TIMING
static bool qsg_render_timing = !qgetenv("QSG_RENDER_TIMING").isEmpty();
static QElapsedTimer threadTimer;
static qint64 syncTime;
static qint64 renderTime;
static qint64 sinceLastTime;
#endif

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

// RL: Render Loop
// RT: Render Thread

// Passed from the RL to the RT when a window is rendeirng on screen
// and should be added to the render loop.
const QEvent::Type WM_Expose            = QEvent::Type(QEvent::User + 1);

// Passed from the RL to the RT when a window is removed obscured and
// should be removed from the render loop.
const QEvent::Type WM_Obscure           = QEvent::Type(QEvent::User + 2);

// Passed from the RL to itself to initiate a polishAndSync() call.
//const QEvent::Type WM_LockAndSync       = QEvent::Type(QEvent::User + 3); // not used for now

// Passed from the RL to RT when GUI has been locked, waiting for sync
// (updatePaintNode())
const QEvent::Type WM_RequestSync       = QEvent::Type(QEvent::User + 4);

// Passed by the RT to itself to trigger another render pass. This is
// typically a result of QQuickWindow::update().
const QEvent::Type WM_RequestRepaint    = QEvent::Type(QEvent::User + 5);

// Passed by the RL to the RT to free up maybe release SG and GL contexts
// if no windows are rendering.
const QEvent::Type WM_TryRelease        = QEvent::Type(QEvent::User + 7);

// Passed by the RL to the RT when a QQuickWindow::grabWindow() is
// called.
const QEvent::Type WM_Grab              = QEvent::Type(QEvent::User + 9);

// Passed by RL to RT when polish fails and we need to reset the expose sycle.
const QEvent::Type WM_ResetExposeCycle = QEvent::Type(QEvent::User + 10);

template <typename T> T *windowFor(const QList<T> list, QQuickWindow *window)
{
    for (int i=0; i<list.size(); ++i) {
        const T &t = list.at(i);
        if (t.window == window)
            return const_cast<T *>(&t);
    }
    return 0;
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
    WMTryReleaseEvent(QQuickWindow *win, bool destroy, QOffscreenSurface *fallback)
        : WMWindowEvent(win, WM_TryRelease)
        , inDestructor(destroy)
        , fallbackSurface(fallback)
    {}

    bool inDestructor;
    QOffscreenSurface *fallbackSurface;
};

class WMExposeEvent : public WMWindowEvent
{
public:
    WMExposeEvent(QQuickWindow *c) : WMWindowEvent(c, WM_Expose), size(c->size()) { }
    QSize size;
};


class WMGrabEvent : public WMWindowEvent
{
public:
    WMGrabEvent(QQuickWindow *c, QImage *result) : WMWindowEvent(c, WM_Grab), image(result) {}
    QImage *image;
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
        , gl(0)
        , sgrc(renderContext)
        , animatorDriver(0)
        , pendingUpdate(0)
        , sleeping(false)
        , syncResultedInChanges(false)
        , exposeCycle(NoExpose)
        , active(false)
        , window(0)
        , stopEventProcessing(false)
    {
#if defined(Q_OS_QNX) && !defined(Q_OS_BLACKBERRY) && defined(Q_PROCESSOR_X86)
        // The SDP 6.6.0 x86 MESA driver requires a larger stack than the default.
        setStackSize(1024 * 1024);
#endif
        vsyncDelta = qsgrl_animation_interval();
    }

    ~QSGRenderThread()
    {
        delete sgrc;
    }

    void invalidateOpenGL(QQuickWindow *window, bool inDestructor, QOffscreenSurface *backupSurface);
    void initializeOpenGL();

    bool event(QEvent *);
    void run();

    void syncAndRender();
    void sync();

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
        QSG_RT_DEBUG("sceneGraphChanged()");
        syncResultedInChanges = true;
    }

public:
    enum UpdateRequest {
        SyncRequest         = 0x01,
        RepaintRequest      = 0x02
    };

    enum ExposeCycle {
        NoExpose,
        ExposePendingSync,
        ExposePendingSwap
    };

    QSGThreadedRenderLoop *wm;
    QOpenGLContext *gl;
    QSGRenderContext *sgrc;

    QAnimationDriver *animatorDriver;

    uint pendingUpdate;
    bool sleeping;
    bool syncResultedInChanges;
    ExposeCycle exposeCycle;

    volatile bool active;

    float vsyncDelta;

    QMutex mutex;
    QWaitCondition waitCondition;

    QElapsedTimer m_timer;

    QQuickWindow *window; // Will be 0 when window is not exposed
    QSize windowSize;

    // Local event queue stuff...
    bool stopEventProcessing;
    QSGRenderThreadEventQueue eventQueue;
};

bool QSGRenderThread::event(QEvent *e)
{
    switch ((int) e->type()) {

    case WM_Expose: {
        QSG_RT_DEBUG("WM_Expose");
        WMExposeEvent *se = static_cast<WMExposeEvent *>(e);
        Q_ASSERT(!window || window == se->window);
        windowSize = se->size;
        window = se->window;
        Q_ASSERT(exposeCycle == NoExpose);
        exposeCycle = ExposePendingSync;
        return true; }

    case WM_ResetExposeCycle:
        QSG_RT_DEBUG("WM_ResetExposeCycle");
        exposeCycle = NoExpose;
        return true;

    case WM_Obscure: {
        QSG_RT_DEBUG("WM_Obscure");

        Q_ASSERT(!window || window == static_cast<WMWindowEvent *>(e)->window);

        mutex.lock();
        if (window) {
            QQuickWindowPrivate::get(window)->fireAboutToStop();
            QSG_RT_DEBUG(" - removed window...");
            window = 0;
        }
        waitCondition.wakeOne();
        mutex.unlock();

        return true; }

    case WM_RequestSync:
        QSG_RT_DEBUG("WM_RequestSync");
        if (sleeping)
            stopEventProcessing = true;
        if (window)
            pendingUpdate |= SyncRequest;
        if (exposeCycle == ExposePendingSync) {
            pendingUpdate |= RepaintRequest;
            exposeCycle = ExposePendingSwap;
        }
        return true;

    case WM_TryRelease: {
        QSG_RT_DEBUG("WM_TryRelease");
        mutex.lock();
        wm->m_locked = true;
        WMTryReleaseEvent *wme = static_cast<WMTryReleaseEvent *>(e);
        if (!window || wme->inDestructor) {
            QSG_RT_DEBUG(" - setting exit flag and invalidating GL");
            invalidateOpenGL(wme->window, wme->inDestructor, wme->fallbackSurface);
            active = gl;
            if (sleeping)
                stopEventProcessing = true;
        } else {
            QSG_RT_DEBUG(" - not releasing anything because we have active windows...");
        }
        waitCondition.wakeOne();
        wm->m_locked = false;
        mutex.unlock();
        return true;
    }

    case WM_Grab: {
        QSG_RT_DEBUG("WM_Grab");
        WMGrabEvent *ce = static_cast<WMGrabEvent *>(e);
        Q_ASSERT(ce->window == window);
        mutex.lock();
        if (window) {
            gl->makeCurrent(window);

            QSG_RT_DEBUG(" - syncing scene graph");
            QQuickWindowPrivate *d = QQuickWindowPrivate::get(window);
            d->syncSceneGraph();

            QSG_RT_DEBUG(" - rendering scene graph");
            QQuickWindowPrivate::get(window)->renderSceneGraph(windowSize);

            QSG_RT_DEBUG(" - grabbing result...");
            *ce->image = qt_gl_read_framebuffer(windowSize * window->devicePixelRatio(), false, false);
        }
        QSG_RT_DEBUG(" - waking gui to handle grab result");
        waitCondition.wakeOne();
        mutex.unlock();
        return true;
    }

    case WM_RequestRepaint:
        // When GUI posts this event, it is followed by a polishAndSync, so we mustn't
        // exit the event loop yet.
        pendingUpdate |= RepaintRequest;
        break;

    default:
        break;
    }
    return QThread::event(e);
}

void QSGRenderThread::invalidateOpenGL(QQuickWindow *window, bool inDestructor, QOffscreenSurface *fallback)
{
    QSG_RT_DEBUG("invalidateOpenGL()");

    if (!gl)
        return;

    if (!window) {
        qWarning("QSGThreadedRenderLoop:QSGRenderThread: no window to make current...");
        return;
    }


    bool wipeSG = inDestructor || !window->isPersistentSceneGraph();
    bool wipeGL = inDestructor || (wipeSG && !window->isPersistentOpenGLContext());

    bool current = gl->makeCurrent(fallback ? static_cast<QSurface *>(fallback) : static_cast<QSurface *>(window));
    if (!current) {
#ifndef QT_NO_DEBUG
        qWarning() << "Scene Graph failed to acquire GL context during cleanup";
#endif
        return;
    }

    // The canvas nodes must be cleaned up regardless if we are in the destructor..
    if (wipeSG) {
        QQuickWindowPrivate *dd = QQuickWindowPrivate::get(window);
        dd->cleanupNodesOnShutdown();
    } else {
        QSG_RT_DEBUG(" - persistent SG, avoiding cleanup");
        gl->doneCurrent();
        return;
    }

    sgrc->invalidate();
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    gl->doneCurrent();
    QSG_RT_DEBUG(" - invalidated scenegraph..");

    if (wipeGL) {
        delete gl;
        gl = 0;
        QSG_RT_DEBUG(" - invalidated OpenGL");
    } else {
        QSG_RT_DEBUG(" - persistent GL, avoiding cleanup");
    }
}

/*!
    Enters the mutex lock to make sure GUI is blocking and performs
    sync, then wakes GUI.
 */
void QSGRenderThread::sync()
{
    QSG_RT_DEBUG("sync()");
    mutex.lock();

    Q_ASSERT_X(wm->m_locked, "QSGRenderThread::sync()", "sync triggered on bad terms as gui is not already locked...");

    bool current = false;
    if (windowSize.width() > 0 && windowSize.height() > 0)
        current = gl->makeCurrent(window);
    if (current) {
        QQuickWindowPrivate *d = QQuickWindowPrivate::get(window);
        bool hadRenderer = d->renderer != 0;
        // If the scene graph was touched since the last sync() make sure it sends the
        // changed signal.
        if (d->renderer)
            d->renderer->clearChangedFlag();
        d->syncSceneGraph();
        if (!hadRenderer && d->renderer) {
            QSG_RT_DEBUG(" - renderer was created, hooking up changed signal");
            syncResultedInChanges = true;
            connect(d->renderer, SIGNAL(sceneGraphChanged()), this, SLOT(sceneGraphChanged()), Qt::DirectConnection);
        }

        // Process deferred deletes now, directly after the sync as
        // deleteLater on the GUI must now also have resulted in SG changes
        // and the delete is a safe operation.
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    } else {
        QSG_RT_DEBUG(" - window has bad size, waiting...");
    }

    waitCondition.wakeOne();
    mutex.unlock();
}


void QSGRenderThread::syncAndRender()
{
#ifndef QSG_NO_RENDER_TIMING
    bool profileFrames = qsg_render_timing || QQuickProfiler::enabled;
    if (profileFrames) {
        sinceLastTime = threadTimer.nsecsElapsed();
        threadTimer.start();
    }
#endif
    QElapsedTimer waitTimer;
    waitTimer.start();

    QSG_RT_DEBUG("syncAndRender()");

    syncResultedInChanges = false;

    bool repaintRequested = pendingUpdate & RepaintRequest;
    bool syncRequested = pendingUpdate & SyncRequest;
    pendingUpdate = 0;

    if (syncRequested) {
        QSG_RT_DEBUG(" - update pending, doing sync");
        sync();
    }

    if (!syncResultedInChanges && !(repaintRequested)) {
        QSG_RT_DEBUG(" - no changes, rendering aborted");
        int waitTime = vsyncDelta - (int) waitTimer.elapsed();
        if (waitTime > 0)
            msleep(waitTime);
        return;
    }

#ifndef QSG_NO_RENDER_TIMING
    if (profileFrames)
        syncTime = threadTimer.nsecsElapsed();
#endif
    QSG_RT_DEBUG(" - rendering starting");

    QQuickWindowPrivate *d = QQuickWindowPrivate::get(window);

    if (animatorDriver->isRunning()) {
        d->animationController->lock();
        animatorDriver->advance();
        d->animationController->unlock();
    }

    bool current = false;
    if (d->renderer && windowSize.width() > 0 && windowSize.height() > 0)
        current = gl->makeCurrent(window);
    if (current) {
        d->renderSceneGraph(windowSize);
#ifndef QSG_NO_RENDER_TIMING
        if (profileFrames)
            renderTime = threadTimer.nsecsElapsed();
#endif
        gl->swapBuffers(window);
        d->fireFrameSwapped();
    } else {
        QSG_RT_DEBUG(" - Window not yet ready, skipping render...");
    }

    QSG_RT_DEBUG(" - rendering done");

    // Though it would be more correct to put this block directly after
    // fireFrameSwapped in the if (current) branch above, we don't do
    // that to avoid blocking the GUI thread in the case where it
    // has started rendering with a bad window, causing makeCurrent to
    // fail or if the window has a bad size.
    mutex.lock();
    if (exposeCycle == ExposePendingSwap) {
        QSG_RT_DEBUG(" - waking GUI after expose");
        exposeCycle = NoExpose;
        waitCondition.wakeOne();
    }
    mutex.unlock();

#ifndef QSG_NO_RENDER_TIMING
        if (qsg_render_timing)
            qDebug("Render Thread: window=%p, framedelta=%d, sync=%d, first render=%d, after final swap=%d",
                   window,
                   int(sinceLastTime/1000000),
                   int(syncTime/1000000),
                   int((renderTime - syncTime)/1000000),
                   int(threadTimer.elapsed() - renderTime/1000000));

        Q_QUICK_SG_PROFILE1(QQuickProfiler::SceneGraphRenderLoopFrame, (
                syncTime,
                renderTime - syncTime,
                threadTimer.nsecsElapsed() - renderTime));
#endif
}



void QSGRenderThread::postEvent(QEvent *e)
{
    eventQueue.addEvent(e);
}



void QSGRenderThread::processEvents()
{
    QSG_RT_DEBUG("processEvents()");
    while (eventQueue.hasMoreEvents()) {
        QEvent *e = eventQueue.takeEvent(false);
        event(e);
        delete e;
    }
    QSG_RT_DEBUG(" - done with processEvents()");
}

void QSGRenderThread::processEventsAndWaitForMore()
{
    QSG_RT_DEBUG("processEventsAndWaitForMore()");
    stopEventProcessing = false;
    while (!stopEventProcessing) {
        QEvent *e = eventQueue.takeEvent(true);
        event(e);
        delete e;
    }
    QSG_RT_DEBUG(" - done with processEventsAndWaitForMore()");
}

void QSGRenderThread::run()
{
    QSG_RT_DEBUG("run()");
    animatorDriver = sgrc->sceneGraphContext()->createAnimationDriver(0);
    animatorDriver->install();
    QUnifiedTimer::instance(true)->setConsistentTiming(QSGRenderLoop::useConsistentTiming());
    if (QQmlDebugService::isDebuggingEnabled())
        QQuickProfiler::registerAnimationCallback();

    while (active) {

        if (window) {
            if (!sgrc->openglContext() && windowSize.width() > 0 && windowSize.height() > 0 && gl->makeCurrent(window))
                sgrc->initialize(gl);
            syncAndRender();
        }

        processEvents();
        QCoreApplication::processEvents();

        if (active && (pendingUpdate == 0 || !window)) {
            QSG_RT_DEBUG("enter event loop (going to sleep)");
            sleeping = true;
            processEventsAndWaitForMore();
            sleeping = false;
        }
    }

    Q_ASSERT_X(!gl, "QSGRenderThread::run()", "The OpenGL context should be cleaned up before exiting the render thread...");

    QSG_RT_DEBUG("run() completed...");

    delete animatorDriver;
    animatorDriver = 0;

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

    m_exhaust_delay = get_env_int("QML_EXHAUST_DELAY", 5);

    connect(m_animation_driver, SIGNAL(started()), this, SLOT(animationStarted()));
    connect(m_animation_driver, SIGNAL(stopped()), this, SLOT(animationStopped()));

    m_animation_driver->install();
    QSG_GUI_DEBUG((void *) 0, "QSGThreadedRenderLoop() created");
}

QSGRenderContext *QSGThreadedRenderLoop::createRenderContext(QSGContext *sg) const
{
    return sg->createRenderContext();
}

void QSGThreadedRenderLoop::maybePostPolishRequest(Window *w)
{
    if (w->timerId == 0) {
        QSG_GUI_DEBUG(w->window, " - posting update");
        w->timerId = startTimer(m_exhaust_delay, Qt::PreciseTimer);
    }
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
    QSG_GUI_DEBUG((void *) 0, "animationStarted()");
    startOrStopAnimationTimer();

    for (int i=0; i<m_windows.size(); ++i)
        maybePostPolishRequest(const_cast<Window *>(&m_windows.at(i)));
}

void QSGThreadedRenderLoop::animationStopped()
{
    QSG_GUI_DEBUG((void *) 0, "animationStopped()");
    startOrStopAnimationTimer();
}


void QSGThreadedRenderLoop::startOrStopAnimationTimer()
{
    int exposedWindows = 0;
    Window *theOne = 0;
    for (int i=0; i<m_windows.size(); ++i) {
        Window &w = m_windows[i];
        if (w.window->isVisible() && w.window->isExposed()) {
            ++exposedWindows;
            theOne = &w;
        }
    }

    if (m_animation_timer != 0 && (exposedWindows == 1 || !m_animation_driver->isRunning())) {
        killTimer(m_animation_timer);
        m_animation_timer = 0;
        // If animations are running, make sure we keep on animating
        if (m_animation_driver->isRunning())
            maybePostPolishRequest(theOne);

    } else if (m_animation_timer == 0 && exposedWindows != 1 && m_animation_driver->isRunning()) {
        m_animation_timer = startTimer(qsgrl_animation_interval());
    }
}

/*
    Adds this window to the list of tracked windows in this window
    manager. show() does not trigger rendering to start, that happens
    in expose.
 */

void QSGThreadedRenderLoop::show(QQuickWindow *window)
{
    QSG_GUI_DEBUG(window, "show()");

    if (Window *w = windowFor(m_windows, window)) {
        /* Safeguard ourselves against misbehaving platform plugins.
         *
         * When being shown, the window should not be exposed as the
         * platform plugin is only told to show after we send the show
         * event. If we are already shown at this time and we don't have
         * an active rendering thread we don't trust the plugin to send
         * us another expose event, so make this explicit call to
         * handleExposure.
         *
         * REF: QTCREATORBUG-10699
         */
        if (window->isExposed() && (!w->thread || !w->thread->window))
            handleExposure(w);
        return;
    }

    QSG_GUI_DEBUG(window, " - now tracking new window");

    Window win;
    win.window = window;
    win.actualWindowFormat = window->format();
    win.thread = new QSGRenderThread(this, QQuickWindowPrivate::get(window)->context);
    win.timerId = 0;
    win.updateDuringSync = false;
    m_windows << win;
}



/*
    Removes this window from the list of tracked windowes in this
    window manager. hide() will trigger obscure, which in turn will
    stop rendering.
 */

void QSGThreadedRenderLoop::hide(QQuickWindow *window)
{
    QSG_GUI_DEBUG(window, "hide()");

    if (window->isExposed())
        handleObscurity(windowFor(m_windows, window));

    releaseResources(window);
}


/*!
    If the window is first hide it, then perform a complete cleanup
    with releaseResources which will take down the GL context and
    exit the rendering thread.
 */
void QSGThreadedRenderLoop::windowDestroyed(QQuickWindow *window)
{
    QSG_GUI_DEBUG(window, "windowDestroyed()");

    if (window->isVisible())
        hide(window);
    releaseResources(window, true);

    for (int i=0; i<m_windows.size(); ++i) {
        if (m_windows.at(i).window == window) {
            QSGRenderThread *thread = m_windows.at(i).thread;
            while (thread->isRunning())
                QThread::yieldCurrentThread();
            Q_ASSERT(thread->thread() == QThread::currentThread());
            delete thread;
            m_windows.removeAt(i);
            break;
        }
    }

    QSG_GUI_DEBUG(window, " - done with windowDestroyed()");
}


void QSGThreadedRenderLoop::exposureChanged(QQuickWindow *window)
{
    QSG_GUI_DEBUG(window, "exposureChanged()");
    Window *w = windowFor(m_windows, window);
    if (!w)
        return;

    if (window->isExposed()) {
        handleExposure(w);
    } else {
        handleObscurity(w);
    }
}

/*!
    Will post an event to the render thread that this window should
    start to render.
 */
void QSGThreadedRenderLoop::handleExposure(Window *w)
{
    QSG_GUI_DEBUG(w->window, "handleExposure");

    if (w->window->width() <= 0 || w->window->height() <= 0
            || !w->window->geometry().intersects(w->window->screen()->availableGeometry())) {
#ifndef QT_NO_DEBUG
        qWarning("QSGThreadedRenderLoop: expose event received for window with invalid geometry.");
#endif
    }

    // Because we are going to bind a GL context to it, make sure it
    // is created.
    if (!w->window->handle())
        w->window->create();

    // Start render thread if it is not running
    if (!w->thread->isRunning()) {

        QSG_GUI_DEBUG(w->window, " - starting render thread...");

        if (!w->thread->gl) {
            w->thread->gl = new QOpenGLContext();
            if (QOpenGLContextPrivate::globalShareContext())
                w->thread->gl->setShareContext(QOpenGLContextPrivate::globalShareContext());
            w->thread->gl->setFormat(w->window->requestedFormat());
            if (!w->thread->gl->create()) {
                const bool isEs = w->thread->gl->isOpenGLES();
                delete w->thread->gl;
                w->thread->gl = 0;
                handleContextCreationFailure(w->window, isEs);
                return;
            }

            QQuickWindowPrivate::get(w->window)->fireOpenGLContextCreated(w->thread->gl);

            w->thread->gl->moveToThread(w->thread);
            QSG_GUI_DEBUG(w->window, " - OpenGL context created...");
        }

        QQuickAnimatorController *controller = QQuickWindowPrivate::get(w->window)->animationController;
        if (controller->thread() != w->thread)
            controller->moveToThread(w->thread);

        w->thread->active = true;
        if (w->thread->thread() == QThread::currentThread()) {
            w->thread->sgrc->moveToThread(w->thread);
            w->thread->moveToThread(w->thread);
        }
        w->thread->start();

    } else {
        QSG_GUI_DEBUG(w->window, " - render thread already running");
    }

    w->thread->postEvent(new WMExposeEvent(w->window));
    bool synced = polishAndSync(w);

    if (synced) {
        w->thread->mutex.lock();
        if (w->thread->exposeCycle != QSGRenderThread::NoExpose) {
            QSG_GUI_DEBUG(w->window, " - waiting for swap to complete...");
            w->thread->waitCondition.wait(&w->thread->mutex);
        }
        Q_ASSERT(w->thread->exposeCycle == QSGRenderThread::NoExpose);
        w->thread->mutex.unlock();
    } else {
        w->thread->postEvent(new QEvent(WM_ResetExposeCycle));
    }
    QSG_GUI_DEBUG(w->window, " - handleExposure completed...");

    startOrStopAnimationTimer();
}

/*!
    This function posts an event to the render thread to remove the window
    from the list of windowses to render.

    It also starts up the non-vsync animation tick if no more windows
    are showing.
 */
void QSGThreadedRenderLoop::handleObscurity(Window *w)
{
    QSG_GUI_DEBUG(w->window, "handleObscurity");
    if (w->thread->isRunning()) {
        w->thread->mutex.lock();
        w->thread->postEvent(new WMWindowEvent(w->window, WM_Obscure));
        w->thread->waitCondition.wait(&w->thread->mutex);
        w->thread->mutex.unlock();
    }

    startOrStopAnimationTimer();
}


void QSGThreadedRenderLoop::maybeUpdate(QQuickWindow *window)
{
    Window *w = windowFor(m_windows, window);
    if (w)
        maybeUpdate(w);
}

/*!
    Called whenever the QML scene has changed. Will post an event to
    ourselves that a sync is needed.
 */
void QSGThreadedRenderLoop::maybeUpdate(Window *w)
{
    if (!QCoreApplication::instance())
        return;

    Q_ASSERT_X(QThread::currentThread() == QCoreApplication::instance()->thread() || m_locked,
               "QQuickItem::update()",
               "Function can only be called from GUI thread or during QQuickItem::updatePaintNode()");

    QSG_GUI_DEBUG(w->window, "maybeUpdate...");
    if (!w || !w->thread->isRunning()) {
        return;
    }

    // Call this function from the Gui thread later as startTimer cannot be
    // called from the render thread.
    if (QThread::currentThread() == w->thread) {
        QSG_GUI_DEBUG(w->window, " - on render thread, will update later..");
        w->updateDuringSync = true;
        return;
    }

    maybePostPolishRequest(w);
}

/*!
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
        QSG_RT_DEBUG("QQuickWindow::update called on render thread");
        w->thread->requestRepaint();
        return;
    }

    QSG_GUI_DEBUG(w->window, "update called");
    w->thread->postEvent(new QEvent(WM_RequestRepaint));
    maybeUpdate(w);
}



/*!
 * Release resources will post an event to the render thread to
 * free up the SG and GL resources and exists the render thread.
 */
void QSGThreadedRenderLoop::releaseResources(QQuickWindow *window, bool inDestructor)
{
    QSG_GUI_DEBUG(window, "releaseResources requested...");

    Window *w = windowFor(m_windows, window);
    if (!w)
        return;

    w->thread->mutex.lock();
    if (w->thread->isRunning() && w->thread->active) {

        // The platform window might have been destroyed before
        // hide/release/windowDestroyed is called, so we need to have a
        // fallback surface to perform the cleanup of the scene graph
        // and the OpenGL resources.
        // QOffscreenSurface must be created on the GUI thread, so we
        // create it here and pass it on to QSGRenderThread::invalidateGL()
        QOffscreenSurface *fallback = 0;
        if (!window->handle()) {
            QSG_GUI_DEBUG(w->window, " - using fallback surface");
            fallback = new QOffscreenSurface();
            fallback->setFormat(w->actualWindowFormat);
            fallback->create();
        }

        QSG_GUI_DEBUG(w->window, " - posting release request to render thread");
        w->thread->postEvent(new WMTryReleaseEvent(window, inDestructor, fallback));
        w->thread->waitCondition.wait(&w->thread->mutex);

        delete fallback;
    }
    w->thread->mutex.unlock();
}


/* Calls polish on all items, then requests synchronization with the render thread
 * and blocks until that is complete. Returns false if it aborted; otherwise true.
 */
bool QSGThreadedRenderLoop::polishAndSync(Window *w)
{
    QSG_GUI_DEBUG(w->window, "polishAndSync()");

    if (!w->window->isExposed() || !w->window->isVisible() || w->window->size().isEmpty()) {
        QSG_GUI_DEBUG(w->window, " - not exposed, aborting...");
        killTimer(w->timerId);
        w->timerId = 0;
        return false;
    }


#ifndef QSG_NO_RENDER_TIMING
    QElapsedTimer timer;
    qint64 polishTime = 0;
    qint64 waitTime = 0;
    qint64 syncTime = 0;
    bool profileFrames = qsg_render_timing  || QQuickProfiler::enabled;
    if (profileFrames)
        timer.start();
#endif

    QQuickWindowPrivate *d = QQuickWindowPrivate::get(w->window);
    d->polishItems();

#ifndef QSG_NO_RENDER_TIMING
    if (profileFrames)
        polishTime = timer.nsecsElapsed();
#endif

    w->updateDuringSync = false;

    emit w->window->afterAnimating();

    QSG_GUI_DEBUG(w->window, " - lock for sync...");
    w->thread->mutex.lock();
    m_locked = true;
    w->thread->postEvent(new QEvent(WM_RequestSync));

    QSG_GUI_DEBUG(w->window, " - wait for sync...");
#ifndef QSG_NO_RENDER_TIMING
    if (profileFrames)
        waitTime = timer.nsecsElapsed();
#endif
    w->thread->waitCondition.wait(&w->thread->mutex);
    m_locked = false;
    w->thread->mutex.unlock();
    QSG_GUI_DEBUG(w->window, " - unlocked after sync...");

#ifndef QSG_NO_RENDER_TIMING
    if (profileFrames)
        syncTime = timer.nsecsElapsed();
#endif

    killTimer(w->timerId);
    w->timerId = 0;

    if (m_animation_timer == 0 && m_animation_driver->isRunning()) {
        QSG_GUI_DEBUG(w->window, " - animations advancing");
        m_animation_driver->advance();
        QSG_GUI_DEBUG(w->window, " - animations done");
        // We need to trigger another sync to keep animations running...
        maybePostPolishRequest(w);
        emit timeToIncubate();
    } else if (w->updateDuringSync) {
        maybePostPolishRequest(w);
    }


#ifndef QSG_NO_RENDER_TIMING
    if (qsg_render_timing)
        qDebug(" - Gui Thread: window=%p, polish=%d, lock=%d, block/sync=%d -- animations=%d",
               w->window,
               int(polishTime/1000000),
               int((waitTime - polishTime)/1000000),
               int((syncTime - waitTime)/1000000),
               int((timer.nsecsElapsed() - syncTime)/1000000));

    Q_QUICK_SG_PROFILE1(QQuickProfiler::SceneGraphPolishAndSync, (
            polishTime,
            waitTime - polishTime,
            syncTime - waitTime,
            timer.nsecsElapsed() - syncTime));
#endif

    return true;
}

bool QSGThreadedRenderLoop::event(QEvent *e)
{
    switch ((int) e->type()) {

    case QEvent::Timer: {
        QTimerEvent *te = static_cast<QTimerEvent *>(e);
        if (te->timerId() == m_animation_timer) {
            QSG_GUI_DEBUG((void *) 0, "QEvent::Timer -> non-visual animation");
            m_animation_driver->advance();
            emit timeToIncubate();
        } else {
            QSG_GUI_DEBUG((void *) 0, "QEvent::Timer -> Polish & Sync");
            Window *w = 0;
            for (int i=0; i<m_windows.size(); ++i) {
                if (m_windows.at(i).timerId == te->timerId()) {
                    w = const_cast<Window *>(&m_windows.at(i));
                    break;
                }
            }
            if (w)
                polishAndSync(w);
            else
                killTimer(te->timerId());
        }
        return true;
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
    QSG_GUI_DEBUG(window, "grab");

    Window *w = windowFor(m_windows, window);
    Q_ASSERT(w);

    if (!w->thread->isRunning())
        return QImage();

    if (!window->handle())
        window->create();

    QSG_GUI_DEBUG(w->window, " - polishing items...");
    QQuickWindowPrivate *d = QQuickWindowPrivate::get(window);
    d->polishItems();

    QImage result;
    w->thread->mutex.lock();
    QSG_GUI_DEBUG(w->window, " - locking, posting grab event");
    w->thread->postEvent(new WMGrabEvent(window, &result));
    w->thread->waitCondition.wait(&w->thread->mutex);
    QSG_GUI_DEBUG(w->window, " - locking, grab done, unlocking");
    w->thread->mutex.unlock();

    QSG_GUI_DEBUG(w->window, " - grab complete");

    return result;
}


#include "qsgthreadedrenderloop.moc"

QT_END_NAMESPACE
