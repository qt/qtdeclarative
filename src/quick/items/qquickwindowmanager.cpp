/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickwindowmanager_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTime>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/private/qabstractanimation_p.h>

#include <QtGui/QOpenGLContext>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

#include <QtQml/private/qqmlglobal_p.h>

#include <QtQuick/QQuickCanvas>
#include <QtQuick/private/qquickcanvas_p.h>
#include <QtQuick/private/qsgcontext_p.h>

QT_BEGIN_NAMESPACE

const QEvent::Type QEvent_Sync = QEvent::Type(QEvent::User);
const QEvent::Type QEvent_DeferredUpdate = QEvent::Type(QEvent::User + 1);


#define QQUICK_CANVAS_TIMING
#ifdef QQUICK_CANVAS_TIMING
static bool qquick_canvas_timing = !qgetenv("QML_CANVAS_TIMING").isEmpty();
static QTime threadTimer;
static int syncTime;
static int renderTime;
static int swapTime;
#endif

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);



/*!
    expectations for this manager to work:
     - one opengl context to render multiple windows
     - OpenGL pipeline will not block for vsync in swap
     - OpenGL pipeline will block based on a full buffer queue.
     - Multiple screens can share the OpenGL context
     - Animations are advanced for all windows once per swap
 */

/*
  Threaded Rendering
  ==================

  The threaded rendering uses a number of different variables to track potential
  states used to handle resizing, initial paint, grabbing and driving animations
  while ALWAYS keeping the GL context in the rendering thread and keeping the
  overhead of normal one-shot paints and vblank driven animations at a minimum.

  Resize, initial show and grab suffer slightly in this model as they are locked
  to the rendering in the rendering thread, but this is a necessary evil for
  the system to work.

  Variables that are used:

  Private::animationRunning: This is true while the animations are running, and only
  written to inside locks.

  RenderThread::isGuiLocked: This is used to indicate that the GUI thread owns the
  lock. This variable is an integer to allow for recursive calls to lockInGui()
  without using a recursive mutex. See isPostingSyncEvent.

  RenderThread::isPostingSyncEvent: This variable is set in the render thread just
  before the sync event is sent to the GUI thread. It is used to avoid deadlocks
  in the case where render thread waits while waiting for GUI to pick up the sync
  event and GUI thread gets a resizeEvent, the initial paintEvent or a grab.
  When this happens, we use the
  exhaustSyncEvent() function to do the sync right there and mark the coming
  sync event to be discarded. There can only ever be one sync incoming.

  RenderThread::isRenderBlock: This variable is true when animations are not
  running and the render thread has gone to sleep, waiting for more to do.

  RenderThread::isExternalUpdatePending: This variable is set to false when
  a new render pass is started and to true in maybeUpdate(). It is an
  indication to the render thread that another render pass needs to take
  place, rather than the render thread going to sleep after completing its swap.

  RenderThread::doGrab: This variable is set by the grab() function and
  tells the renderer to do a grab after rendering is complete and before
  swapping happens.

  RenderThread::shouldExit: This variable is used to determine if the render
  thread should do a nother pass. It is typically set as a result of show()
  and unset as a result of hide() or during shutdown()

  RenderThread::hasExited: Used by the GUI thread to synchronize the shutdown
  after shouldExit has been set to true.
 */

DEFINE_BOOL_CONFIG_OPTION(qmlNoThreadedRenderer, QML_BAD_GUI_RENDER_LOOP);
DEFINE_BOOL_CONFIG_OPTION(qmlForceThreadedRenderer, QML_FORCE_THREADED_RENDERER); // Might trigger graphics driver threading bugs, use at own risk

//#define THREAD_DEBUG

QQuickWindowManager::~QQuickWindowManager()
{
}

class QQuickRenderThreadSingleContextWindowManager : public QThread, public QQuickWindowManager
{
    Q_OBJECT
public:
    QQuickRenderThreadSingleContextWindowManager()
        : sg(QSGContext::createDefaultContext())
        , gl(0)
        , animationTimer(-1)
        , allowMainThreadProcessingFlag(false)
        , isGuiLocked(0)
        , animationRunning(false)
        , isPostingSyncEvent(false)
        , isRenderBlocked(false)
        , isExternalUpdatePending(false)
        , syncAlreadyHappened(false)
        , inSync(false)
        , shouldExit(false)
        , hasExited(false)
        , isDeferredUpdatePosted(false)
        , canvasToGrab(0)
    {
        sg->moveToThread(this);

        animationDriver = sg->createAnimationDriver(this);
        animationDriver->install();
        connect(animationDriver, SIGNAL(started()), this, SLOT(animationStarted()));
        connect(animationDriver, SIGNAL(stopped()), this, SLOT(animationStopped()));
    }

    QSGContext *sceneGraphContext() const { return sg; }

    void releaseResources() { }

    void show(QQuickCanvas *canvas);
    void hide(QQuickCanvas *canvas);

    void canvasDestroyed(QQuickCanvas *canvas);

    void exposureChanged(QQuickCanvas *canvas);
    QImage grab(QQuickCanvas *canvas);
    void resize(QQuickCanvas *canvas, const QSize &size);
    void handleDeferredUpdate();
    void maybeUpdate(QQuickCanvas *canvas);
    void update(QQuickCanvas *canvas) { maybeUpdate(canvas); } // identical for this implementation
    void wakeup();

    void startRendering();
    void stopRendering();

    void exhaustSyncEvent();
    void sync(bool guiAlreadyLocked);

    void initialize();

    volatile bool *allowMainThreadProcessing() { return &allowMainThreadProcessingFlag; }

    bool event(QEvent *);

    inline void lock() { mutex.lock(); }
    inline void unlock() { mutex.unlock(); }
    inline void wait() { condition.wait(&mutex); }
    inline void wake() { condition.wakeOne(); }
    void lockInGui();
    void unlockInGui();

    void run();

    QQuickCanvas *masterCanvas() {
        QQuickCanvas *win = 0;
        for (QHash<QQuickCanvas *, CanvasData *>::const_iterator it = m_rendered_windows.constBegin();
            it != m_rendered_windows.constEnd() && !win; ++it) {
            if (it.value()->isVisible)
                win = it.key();
        }
        return win;
    }

public slots:
    void animationStarted();
    void animationStopped();
    void canvasVisibilityChanged();

private:
    void handleAddedWindows();
    void handleAddedWindow(QQuickCanvas *canvas);
    void handleRemovedWindows(bool clearGLContext = true);

    QSGContext *sg;
    QOpenGLContext *gl;
    QAnimationDriver *animationDriver;
    int animationTimer;

    QMutex mutex;
    QWaitCondition condition;

    volatile bool allowMainThreadProcessingFlag;

    int isGuiLocked;
    uint animationRunning: 1;
    uint isPostingSyncEvent : 1;
    uint isRenderBlocked : 1;
    uint isExternalUpdatePending : 1;
    uint syncAlreadyHappened : 1;
    uint inSync : 1;
    uint shouldExit : 1;
    uint hasExited : 1;
    uint isDeferredUpdatePosted : 1;

    QQuickCanvas *canvasToGrab;
    QImage grabContent;

    struct CanvasData {
        QSize renderedSize;
        QSize windowSize;
        QSize viewportSize;

        uint sizeWasChanged : 1;
        uint isVisible : 1;
    };

    QHash<QQuickCanvas *, CanvasData *> m_rendered_windows;

    struct CanvasTracker {
        QQuickCanvas *canvas;
        uint isVisible : 1;
        uint toBeRemoved : 1;
    };

    QList<CanvasTracker> m_tracked_windows;

    QList<QQuickCanvas *> m_removed_windows;
    QList<QQuickCanvas *> m_added_windows;
};


class QQuickTrivialWindowManager : public QObject, public QQuickWindowManager
{
public:
    QQuickTrivialWindowManager();

    void show(QQuickCanvas *canvas);
    void hide(QQuickCanvas *canvas);

    void canvasDestroyed(QQuickCanvas *canvas);

    void initializeGL();
    void renderCanvas(QQuickCanvas *canvas);
    void exposureChanged(QQuickCanvas *canvas);
    QImage grab(QQuickCanvas *canvas);
    void resize(QQuickCanvas *canvas, const QSize &size);
    void wakeup();

    void maybeUpdate(QQuickCanvas *canvas);
    void update(QQuickCanvas *canvas) { maybeUpdate(canvas); } // identical for this implementation.

    void releaseResources() { }

    volatile bool *allowMainThreadProcessing();

    QSGContext *sceneGraphContext() const;

    bool event(QEvent *);

    struct CanvasData {
        bool updatePending : 1;
        bool grabOnly : 1;
    };

    QHash<QQuickCanvas *, CanvasData> m_windows;

    QOpenGLContext *gl;
    QSGContext *sg;

    QImage grabContent;

    bool eventPending;
};


QQuickWindowManager *QQuickWindowManager::instance()
{
    static QQuickWindowManager *theInstance;

    if (!theInstance) {

        theInstance = QSGContext::createWindowManager();

        bool bufferQueuing = QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::BufferQueueingOpenGL);
        bool fancy = bufferQueuing
            && QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL);
        if (qmlNoThreadedRenderer())
            fancy = false;
        else if (qmlForceThreadedRenderer())
            fancy = true;

        // Enable fixed animation steps...
        QByteArray fixed = qgetenv("QML_FIXED_ANIMATION_STEP");
        bool fixedAnimationSteps = bufferQueuing;
        if (fixed == "no")
            fixedAnimationSteps = false;
        else if (fixed.length())
            fixedAnimationSteps = true;
        if (fixedAnimationSteps)
            QUnifiedTimer::instance(true)->setConsistentTiming(true);

        if (!theInstance) {
            theInstance = fancy
                    ? (QQuickWindowManager*) new QQuickRenderThreadSingleContextWindowManager
                    : (QQuickWindowManager*) new QQuickTrivialWindowManager;
        }
    }
    return theInstance;
}





void QQuickRenderThreadSingleContextWindowManager::initialize()
{
    Q_ASSERT(m_rendered_windows.size());

    QQuickCanvas *win = masterCanvas();
    if (!win)
        return;

    gl = new QOpenGLContext();
    // Pick up the surface format from one of them
    gl->setFormat(win->requestedFormat());
    gl->create();
    if (!gl->makeCurrent(win))
        qWarning("QQuickCanvas: makeCurrent() failed...");

    Q_ASSERT(!sg->isReady());
    sg->initialize(gl);
}


/*!
    This function is called when the canvas is created to register the canvas with
    the window manager.

    Called on GUI Thread.
 */

void QQuickRenderThreadSingleContextWindowManager::show(QQuickCanvas *canvas)
{
#ifdef THREAD_DEBUG
    printf("GUI: Canvas added to windowing system, %p, %dx%d\n", canvas, canvas->width(), canvas->height());
#endif

    CanvasTracker tracker;
    tracker.canvas = canvas;
    tracker.isVisible = false;
    tracker.toBeRemoved = false;
    m_tracked_windows << tracker;

    connect(canvas, SIGNAL(widthChanged(int)), this, SLOT(canvasVisibilityChanged()), Qt::DirectConnection);
    connect(canvas, SIGNAL(heightChanged(int)), this, SLOT(canvasVisibilityChanged()), Qt::DirectConnection);

    canvasVisibilityChanged();
}


void QQuickRenderThreadSingleContextWindowManager::handleAddedWindow(QQuickCanvas *canvas)
{
#ifdef THREAD_DEBUG
    printf("                RenderThread: adding canvas: %p\n", canvas);
#endif

    CanvasData *data = new CanvasData;
    data->sizeWasChanged = false;
    data->windowSize = canvas->size();
    data->isVisible = canvas->isVisible();
    m_rendered_windows[canvas] = data;

    isExternalUpdatePending = true;
}


/*!
    Called on Render Thread
 */
void QQuickRenderThreadSingleContextWindowManager::handleAddedWindows()
{
#ifdef THREAD_DEBUG
    printf("                RenderThread: about to add %d\n", m_added_windows.size());
#endif

    while (m_added_windows.size()) {
        QQuickCanvas *canvas = m_added_windows.takeLast();
        handleAddedWindow(canvas);
    }
}


/*!
    Called on the GUI Thread, from the canvas' destructor
 */

void QQuickRenderThreadSingleContextWindowManager::canvasDestroyed(QQuickCanvas *canvas)
{
#ifdef THREAD_DEBUG
    printf("GUI: Canvas destroyed: %p\n", canvas);
#endif

    hide(canvas);
}


/*!
    Called on GUI Thread
 */

void QQuickRenderThreadSingleContextWindowManager::hide(QQuickCanvas *canvas)
{
#ifdef THREAD_DEBUG
    printf("GUI: Canvas hidden: %p\n", canvas);
#endif

    int position = -1;
    for (int i=0; i<m_tracked_windows.size(); ++i) {
        if (m_tracked_windows.at(i).canvas == canvas) {
            m_tracked_windows[i].toBeRemoved = true;
            position = i;
            break;
        }
    }

    if (position >= 0) {
        disconnect(canvas, SIGNAL(widthChanged(int)), this, SLOT(canvasVisibilityChanged()));
        disconnect(canvas, SIGNAL(heightChanged(int)), this, SLOT(canvasVisibilityChanged()));
        canvasVisibilityChanged();
        m_tracked_windows.removeAt(position);
    }

#ifdef THREAD_DEBUG
    printf("GUI: Canvas removal completed... %p\n", canvas);
#endif
}

/*!
    Called on Render Thread
 */
void QQuickRenderThreadSingleContextWindowManager::handleRemovedWindows(bool clearGLContext)
{
#ifdef THREAD_DEBUG
    printf("                RenderThread: about to remove %d\n", m_removed_windows.size());
#endif

    bool removedAnything = false;
    while (m_removed_windows.size()) {
        QQuickCanvas *canvas = m_removed_windows.takeLast();
#ifdef THREAD_DEBUG
    printf("                RenderThread: removing %p\n", canvas);
#endif

        QQuickCanvasPrivate::get(canvas)->cleanupNodesOnShutdown();
        delete m_rendered_windows.take(canvas);
        removedAnything = true;
    }

    // If a window is removed because it has been hidden it will take with it
    // the gl context (at least on Mac) if bound, so disconnect the gl context
    // from anything
    if (removedAnything && clearGLContext)
        gl->doneCurrent();
}



/*!
    Called on GUI Thread
 */

void QQuickRenderThreadSingleContextWindowManager::canvasVisibilityChanged()
{
    bool anyoneShowing = false;
    QList<QQuickCanvas *> toAdd, toRemove;

    // Not optimal, but also not frequently used...
    for (int i=0; i<m_tracked_windows.size(); ++i) {
        CanvasTracker &t = const_cast<CanvasTracker &>(m_tracked_windows.at(i));
        QQuickCanvas *win = t.canvas;

        Q_ASSERT(win->isVisible() || QQuickCanvasPrivate::get(win)->renderWithoutShowing || t.toBeRemoved);
        bool canvasVisible = win->width() > 0 && win->height() > 0;
        anyoneShowing |= (canvasVisible && !t.toBeRemoved);

        if ((!canvasVisible && t.isVisible) || t.toBeRemoved) {
            toRemove << win;
        } else if (canvasVisible && !t.isVisible) {
            toAdd << win;
        }
        t.isVisible = canvasVisible;
    }

    if (isRunning()) {
        if (!anyoneShowing) {
            stopRendering();
        } else {
            lockInGui();
            exhaustSyncEvent();
            m_added_windows << toAdd;
            m_removed_windows << toRemove;
            while (isRunning() && (m_added_windows.size() || m_removed_windows.size())) {
                if (isRenderBlocked)
                    wake();
                wait();
            }
            unlockInGui();
        }

    } else if (anyoneShowing) {
        Q_ASSERT(toRemove.isEmpty()); // since loop is not running, nothing is showing now
        for (int i=0; i<toAdd.size(); ++i)
            handleAddedWindow(toAdd.at(i));
        startRendering();
    }

}


void QQuickRenderThreadSingleContextWindowManager::run()
{
#ifdef THREAD_DEBUG
    printf("QML Rendering Thread Started\n");
#endif

    lock();
    Q_ASSERT(!gl);
    initialize();
    // Wake GUI as it is waiting for the GL context to have appeared, as
    // an indication that the render thread is now running.
    wake();
    unlock();

    if (!gl)
        return;

    while (!shouldExit) {
        lock();

#ifdef THREAD_DEBUG
        printf("                RenderThread: *** NEW FRAME ***\n");
#endif

        isExternalUpdatePending = false;
        handleAddedWindows();

        if (!isGuiLocked) {
            isPostingSyncEvent = true;

#ifdef THREAD_DEBUG
            printf("                RenderThread: aquired sync lock...\n");
#endif
            allowMainThreadProcessingFlag = false;
            QCoreApplication::postEvent(this, new QEvent(QEvent_Sync));

#ifdef THREAD_DEBUG
            printf("                RenderThread: going to sleep...\n");
#endif
            wake(); // In case the event got through all the way to wait() before this thread got to wait.
            wait();


            isPostingSyncEvent = false;
        }

#ifdef THREAD_DEBUG
        printf("                RenderThread: Doing locked sync\n");
#endif
#ifdef QQUICK_CANVAS_TIMING
        if (qquick_canvas_timing)
            threadTimer.start();
#endif
        inSync = true;
        for (QHash<QQuickCanvas *, CanvasData *>::const_iterator it = m_rendered_windows.constBegin();
             it != m_rendered_windows.constEnd(); ++it) {
            QQuickCanvas *canvas = it.key();

#ifdef THREAD_DEBUG
            printf("                RenderThread: Syncing canvas: %p\n", canvas);
#endif

            CanvasData *canvasData = it.value();
            QQuickCanvasPrivate *canvasPrivate = QQuickCanvasPrivate::get(canvas);

            Q_ASSERT(canvasData->windowSize.width() > 0 && canvasData->windowSize.height() > 0);

            if (!canvasData->isVisible)
                gl->makeCurrent(masterCanvas());
            else
                gl->makeCurrent(canvas);

            if (canvasData->viewportSize != canvasData->windowSize) {
#ifdef THREAD_DEBUG
                printf("                RenderThread: --- window has changed size...\n");
#endif
                canvasData->viewportSize = canvasData->windowSize;
                canvasData->sizeWasChanged = true;
                glViewport(0, 0, canvasData->viewportSize.width(), canvasData->viewportSize.height());
            }

            canvasPrivate->syncSceneGraph();
        }
        inSync = false;

        // Wake GUI after sync to let it continue animating and event processing.
        allowMainThreadProcessingFlag = true;
        wake();
        unlock();
#ifdef THREAD_DEBUG
        printf("                RenderThread: sync done\n");
#endif
#ifdef QQUICK_CANVAS_TIMING
        if (qquick_canvas_timing)
            syncTime = threadTimer.elapsed();
#endif

        for (QHash<QQuickCanvas *, CanvasData *>::const_iterator it = m_rendered_windows.constBegin();
             it != m_rendered_windows.constEnd(); ++it) {
            QQuickCanvas *canvas = it.key();
            CanvasData *canvasData = it.value();
            QQuickCanvasPrivate *canvasPrivate = QQuickCanvasPrivate::get(canvas);

#ifdef THREAD_DEBUG
            printf("                RenderThread: Rendering canvas %p\n", canvas);
#endif

            Q_ASSERT(canvasData->windowSize.width() > 0 && canvasData->windowSize.height() > 0);

#ifdef THREAD_DEBUG
            printf("                RenderThread: --- rendering at size %dx%d\n",
                   canvasData->viewportSize.width(), canvasData->viewportSize.height()
                   );
#endif

            // We only need to re-makeCurrent when we have multiple surfaces.
            if (m_rendered_windows.size() > 1)
                gl->makeCurrent(canvas);

            canvasPrivate->renderSceneGraph(canvasData->viewportSize);
#ifdef QQUICK_CANVAS_TIMING
            if (qquick_canvas_timing)
                renderTime = threadTimer.elapsed() - syncTime;
#endif

            // The content of the target buffer is undefined after swap() so grab needs
            // to happen before swap();
            if (canvas == canvasToGrab) {
#ifdef THREAD_DEBUG
                printf("                RenderThread: --- grabbing...\n");
#endif
                grabContent = qt_gl_read_framebuffer(canvasData->windowSize, false, false);
                canvasToGrab = 0;
            }

#ifdef THREAD_DEBUG
            printf("                RenderThread: --- wait for swap...\n");
#endif

            if (canvasData->isVisible)
                gl->swapBuffers(canvas);

            canvasPrivate->fireFrameSwapped();
#ifdef THREAD_DEBUG
            printf("                RenderThread: --- swap complete...\n");
#endif

        }

#ifdef QQUICK_CANVAS_TIMING
            if (qquick_canvas_timing) {
                swapTime = threadTimer.elapsed() - renderTime;
                qDebug() << "- Breakdown of frame time; sync:" << syncTime
                         << "ms render:" << renderTime << "ms swap:" << swapTime
                         << "ms total:" << swapTime + renderTime << "ms";
            }
#endif

        lock();

        handleRemovedWindows();

        // Update sizes...
        for (QHash<QQuickCanvas *, CanvasData *>::const_iterator it = m_rendered_windows.constBegin();
             it != m_rendered_windows.constEnd(); ++it) {
            CanvasData *canvasData = it.value();
            if (canvasData->sizeWasChanged) {
                canvasData->renderedSize = canvasData->viewportSize;
                canvasData->sizeWasChanged = false;
            }
        }


        // Wake the GUI thread now that rendering is complete, to signal that painting
        // is done, resizing is done or grabbing is completed. For grabbing, we're
        // signalling this much later than needed (we could have done it before swap)
        // but we don't want to lock an extra time.
        wake();

        if (!animationRunning && !isExternalUpdatePending && !shouldExit && !canvasToGrab) {
#ifdef THREAD_DEBUG
            printf("                RenderThread: nothing to do, going to sleep...\n");
#endif
            isRenderBlocked = true;
            wait();
            isRenderBlocked = false;
        }

        unlock();

        QCoreApplication::processEvents();

        // Process any "deleteLater" objects...
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    }

#ifdef THREAD_DEBUG
    printf("                RenderThread: deleting all outstanding nodes\n");
#endif

    m_removed_windows << m_rendered_windows.keys();
    handleRemovedWindows(false);

    sg->invalidate();

    gl->doneCurrent();
    delete gl;
    gl = 0;

#ifdef THREAD_DEBUG
    printf("                RenderThread: render loop exited... Good Night!\n");
#endif

    lock();
    hasExited = true;

#ifdef THREAD_DEBUG
    printf("                RenderThread: waking GUI for final sleep..\n");
#endif
    wake();
    unlock();

#ifdef THREAD_DEBUG
    printf("                RenderThread: All done...\n");
#endif
}

bool QQuickRenderThreadSingleContextWindowManager::event(QEvent *e)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());

    if (e->type() == QEvent_Sync) {

        // If all canvases have been hidden, ignore the event
        if (!isRunning())
            return true;

        if (!syncAlreadyHappened)
            sync(false);

        syncAlreadyHappened = false;

        if (animationRunning) {
#ifdef THREAD_DEBUG
            printf("GUI: Advancing animations...\n");
#endif

            animationDriver->advance();

#ifdef THREAD_DEBUG
            printf("GUI: Animations advanced...\n");
#endif
        }

        return true;
    } else if (e->type() == QEvent_DeferredUpdate) {
        handleDeferredUpdate();

    } else if (e->type() == QEvent::Timer) {
#ifdef THREAD_DEBUG
            printf("GUI: Animations advanced via timer...\n");
#endif
        animationDriver->advance();
    }

    return QThread::event(e);
}



void QQuickRenderThreadSingleContextWindowManager::exhaustSyncEvent()
{
    if (isPostingSyncEvent) {
        sync(true);
        syncAlreadyHappened = true;
    }
}



void QQuickRenderThreadSingleContextWindowManager::sync(bool guiAlreadyLocked)
{
#ifdef THREAD_DEBUG
    printf("GUI: sync - %s\n", guiAlreadyLocked ? "outside event" : "inside event");
#endif
    if (!guiAlreadyLocked)
        lockInGui();

    for (QHash<QQuickCanvas *, CanvasData *>::const_iterator it = m_rendered_windows.constBegin();
         it != m_rendered_windows.constEnd(); ++it) {
        QQuickCanvasPrivate::get(it.key())->polishItems();
    }

    wake();
    wait();

    if (!guiAlreadyLocked)
        unlockInGui();
}




/*!
    Acquires the mutex for the GUI thread. The function uses the isGuiLocked
    variable to keep track of how many recursion levels the gui is locked with.
    We only actually acquire the mutex for the first level to avoid deadlocking
    ourselves.
 */

void QQuickRenderThreadSingleContextWindowManager::lockInGui()
{
    if (++isGuiLocked == 1)
        lock();

#ifdef THREAD_DEBUG
    printf("GUI: aquired lock... level=%d\n", isGuiLocked);
#endif
}



void QQuickRenderThreadSingleContextWindowManager::unlockInGui()
{
#ifdef THREAD_DEBUG
    printf("GUI: releasing lock... level=%d\n", isGuiLocked);
#endif

    if (--isGuiLocked == 0)
        unlock();
}




void QQuickRenderThreadSingleContextWindowManager::animationStarted()
{
#ifdef THREAD_DEBUG
    printf("GUI: animationStarted()\n");
#endif

    if (!isRunning()) {
        animationTimer = startTimer(1000/60);
        return;
    }

    lockInGui();

    animationRunning = true;

    if (isRenderBlocked)
        wake();

    unlockInGui();
}



void QQuickRenderThreadSingleContextWindowManager::animationStopped()
{
#ifdef THREAD_DEBUG
    printf("GUI: animationStopped()...\n");
#endif

    if (!isRunning()) {
        killTimer(animationTimer);
        animationTimer = -1;
        return;
    }

    lockInGui();
    animationRunning = false;
    unlockInGui();
}


void QQuickRenderThreadSingleContextWindowManager::exposureChanged(QQuickCanvas *canvas)
{
    Q_UNUSED(canvas);
#ifdef THREAD_DEBUG
    printf("GUI: exposure changed: %p\n", canvas);
#endif

    if (canvas->isExposed())
        maybeUpdate(canvas);

#ifdef THREAD_DEBUG
    printf("GUI: exposure changed done: %p\n", canvas);
#endif
}



void QQuickRenderThreadSingleContextWindowManager::resize(QQuickCanvas *canvas, const QSize &size)
{
#ifdef THREAD_DEBUG
    printf("GUI: Resize Event: %p = %dx%d\n", canvas, size.width(), size.height());
#endif

    // If the rendering thread is not running we do not need to do anything.
    // Also if the canvas is being resized to an invalid size, it will be removed
    // by the canvasVisibilityChanged slot as result of width/heightcChanged()
    if (!isRunning() || size.width() <= 0 || size.height() <= 0)
        return;

    lockInGui();
    exhaustSyncEvent();

    CanvasData *canvasData = m_rendered_windows.value(canvas);
    if (canvasData) {
        canvasData->windowSize = size;
        while (isRunning() && canvasData->renderedSize != size && size.width() > 0 && size.height() > 0) {
            if (isRenderBlocked)
                wake();
            wait();
        }
    }
    unlockInGui();

#ifdef THREAD_DEBUG
    printf("GUI: Resize done: %p\n", canvas);
#endif
}



void QQuickRenderThreadSingleContextWindowManager::startRendering()
{
#ifdef THREAD_DEBUG
    printf("GUI: Starting Render Thread\n");
#endif
    hasExited = false;
    shouldExit = false;
    isGuiLocked = 0;
    isPostingSyncEvent = false;
    syncAlreadyHappened = false;
    inSync = false;

    lockInGui();
    animationRunning = animationDriver->isRunning();
    start(); // Start the render thread...
    wait();
    unlockInGui();

    // Animations will now be driven from the rendering thread.
    if (animationTimer >= 0) {
        killTimer(animationTimer);
        animationTimer = -1;
    }


}



void QQuickRenderThreadSingleContextWindowManager::stopRendering()
{
#ifdef THREAD_DEBUG
    printf("GUI: stopping render thread\n");
#endif

    lockInGui();
    exhaustSyncEvent();
    shouldExit = true;

    if (isRenderBlocked) {
#ifdef THREAD_DEBUG
        printf("GUI: waking up render thread\n");
#endif
        wake();
    }

    while (!hasExited) {
#ifdef THREAD_DEBUG
        printf("GUI: waiting for render thread to have exited..\n");
#endif
        wait();
    }

    unlockInGui();

#ifdef THREAD_DEBUG
    printf("GUI: waiting for render thread to terminate..\n");
#endif
    // Actually wait for the thread to terminate.  Otherwise we can delete it
    // too early and crash.
    QThread::wait();

#ifdef THREAD_DEBUG
    printf("GUI: thread has terminated and we're all good..\n");
#endif

    // Activate timer to keep animations running
    if (animationDriver->isRunning())
        animationTimer = startTimer(1000/60);
}



QImage QQuickRenderThreadSingleContextWindowManager::grab(QQuickCanvas *canvas)
{
    if (!isRunning())
        return QImage();

    if (QThread::currentThread() != qApp->thread()) {
        qWarning("QQuickCanvas::grabFrameBuffer: can only be called from the GUI thread");
        return QImage();
    }

#ifdef THREAD_DEBUG
    printf("GUI: doing a pixelwise grab..\n");
#endif

    lockInGui();
    exhaustSyncEvent();

    canvasToGrab = canvas;
    while (isRunning() && canvasToGrab) {
        if (isRenderBlocked)
            wake();
        wait();
    }

    QImage grabbed = grabContent;
    grabContent = QImage();

    unlockInGui();

    return grabbed;
}


void QQuickRenderThreadSingleContextWindowManager::handleDeferredUpdate()
{
#ifdef THREAD_DEBUG
    printf("GUI: handling update to ourselves...\n");
#endif

    isDeferredUpdatePosted = false;

    lockInGui();
    isExternalUpdatePending = true;
    if (isRenderBlocked)
        wake();
    unlockInGui();
}

void QQuickRenderThreadSingleContextWindowManager::maybeUpdate(QQuickCanvas *)
{
    Q_ASSERT_X(QThread::currentThread() == QCoreApplication::instance()->thread() || inSync,
               "QQuickCanvas::update",
               "Function can only be called from GUI thread or during QQuickItem::updatePaintNode()");

    if (inSync) {
        isExternalUpdatePending = true;

    } else if (!isDeferredUpdatePosted) {
#ifdef THREAD_DEBUG
        printf("GUI: posting update to ourselves...\n");
#endif
        isDeferredUpdatePosted = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent_DeferredUpdate));
    }

}

void QQuickRenderThreadSingleContextWindowManager::wakeup()
{
    lockInGui();
    isExternalUpdatePending = true;
    if (isRenderBlocked)
        wake();
    unlockInGui();
}

QQuickTrivialWindowManager::QQuickTrivialWindowManager()
    : gl(0)
    , eventPending(false)
{
    sg = QSGContext::createDefaultContext();
}


void QQuickTrivialWindowManager::show(QQuickCanvas *canvas)
{
    CanvasData data;
    data.updatePending = false;
    data.grabOnly = false;
    m_windows[canvas] = data;

    maybeUpdate(canvas);
}

void QQuickTrivialWindowManager::hide(QQuickCanvas *canvas)
{
    if (!m_windows.contains(canvas))
        return;

    m_windows.remove(canvas);
    QQuickCanvasPrivate *cd = QQuickCanvasPrivate::get(canvas);
    cd->cleanupNodesOnShutdown();

    if (m_windows.size() == 0) {
        sg->invalidate();
        delete gl;
        gl = 0;
    }
}

void QQuickTrivialWindowManager::canvasDestroyed(QQuickCanvas *canvas)
{
    hide(canvas);
}

void QQuickTrivialWindowManager::renderCanvas(QQuickCanvas *canvas)
{
    if (!canvas->isExposed() || !m_windows.contains(canvas))
        return;

    CanvasData &data = const_cast<CanvasData &>(m_windows[canvas]);

    QQuickCanvas *masterCanvas = 0;
    if (!canvas->isVisible()) {
        // Find a "proper surface" to bind...
        for (QHash<QQuickCanvas *, CanvasData>::const_iterator it = m_windows.constBegin();
             it != m_windows.constEnd() && !masterCanvas; ++it) {
            if (it.key()->isVisible())
                masterCanvas = it.key();
        }
    } else {
        masterCanvas = canvas;
    }

    if (!masterCanvas)
        return;

    if (!gl) {
        gl = new QOpenGLContext();
        gl->setFormat(masterCanvas->requestedFormat());
        gl->create();
        if (!gl->makeCurrent(masterCanvas))
            qWarning("QQuickCanvas: makeCurrent() failed...");
        sg->initialize(gl);
    } else {
        gl->makeCurrent(masterCanvas);
    }

    bool alsoSwap = data.updatePending;
    data.updatePending = false;

    QQuickCanvasPrivate *cd = QQuickCanvasPrivate::get(canvas);
    cd->polishItems();
    cd->syncSceneGraph();
    cd->renderSceneGraph(canvas->size());

    if (data.grabOnly) {
        grabContent = qt_gl_read_framebuffer(canvas->size(), false, false);
        data.grabOnly = false;
    }

    if (alsoSwap && canvas->isVisible()) {
        gl->swapBuffers(canvas);
        cd->fireFrameSwapped();
    }

    // Might have been set during syncSceneGraph()
    if (data.updatePending)
        maybeUpdate(canvas);
}

void QQuickTrivialWindowManager::exposureChanged(QQuickCanvas *canvas)
{
    if (canvas->isExposed())
        maybeUpdate(canvas);
}

QImage QQuickTrivialWindowManager::grab(QQuickCanvas *canvas)
{
    if (!m_windows.contains(canvas))
        return QImage();

    m_windows[canvas].grabOnly = true;

    renderCanvas(canvas);

    QImage grabbed = grabContent;
    grabContent = QImage();
    return grabbed;
}



void QQuickTrivialWindowManager::resize(QQuickCanvas *, const QSize &)
{
}



void QQuickTrivialWindowManager::maybeUpdate(QQuickCanvas *canvas)
{
    if (!m_windows.contains(canvas))
        return;

    m_windows[canvas].updatePending = true;

    if (!eventPending) {
        QCoreApplication::postEvent(this, new QEvent(QEvent::User));
        eventPending = true;
    }
}

void QQuickTrivialWindowManager::wakeup()
{
}

volatile bool *QQuickTrivialWindowManager::allowMainThreadProcessing()
{
    return 0;
}



QSGContext *QQuickTrivialWindowManager::sceneGraphContext() const
{
    return sg;
}


bool QQuickTrivialWindowManager::event(QEvent *e)
{
    if (e->type() == QEvent::User) {
        eventPending = false;
        for (QHash<QQuickCanvas *, CanvasData>::const_iterator it = m_windows.constBegin();
             it != m_windows.constEnd(); ++it) {
            const CanvasData &data = it.value();
            if (data.updatePending)
                renderCanvas(it.key());
        }
        return true;
    }
    return QObject::event(e);
}

#include "qquickwindowmanager.moc"

QT_END_NAMESPACE
