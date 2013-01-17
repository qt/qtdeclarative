/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qquickwindowmanager_p.h"
#include "qquickthreadedwindowmanager_p.h"

#include <QtCore/QTime>
#include <QtCore/QDebug>

#include <QtGui/QOpenGLContext>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

#include <QtQml/private/qqmlglobal_p.h>

#include <QtQuick/QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>

QT_BEGIN_NAMESPACE

//#define THREAD_DEBUG
extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

const QEvent::Type QEvent_Sync = QEvent::Type(QEvent::User);
const QEvent::Type QEvent_DeferredUpdate = QEvent::Type(QEvent::User + 1);

#define QQUICK_RENDER_TIMING
#ifdef QQUICK_RENDER_TIMING
DEFINE_BOOL_CONFIG_OPTION(qquick_render_timing, QML_RENDER_TIMING)
static QTime threadTimer;
static int syncTime;
static int renderTime;
static int swapTime;
#endif


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


void QQuickRenderThreadSingleContextWindowManager::initialize()
{
    Q_ASSERT(m_rendered_windows.size());

    QQuickWindow *win = 0;
    for (QHash<QQuickWindow *, WindowData *>::const_iterator it = m_rendered_windows.constBegin();
        it != m_rendered_windows.constEnd() && !win; ++it) {
        if (QQuickWindowPrivate::get(it.key())->isRenderable())
            win = it.key();
    }
    if (!win)
        return;

    gl = new QOpenGLContext();
    // Pick up the surface format from one of them
    gl->setFormat(win->requestedFormat());
    gl->create();
    if (!gl->makeCurrent(win))
        qWarning("QQuickWindow: makeCurrent() failed...");

    Q_ASSERT(!sg->isReady());
    sg->initialize(gl);
}


/*!
    This function is called when the window is created to register the window with
    the window manager.

    Called on GUI Thread.
 */

void QQuickRenderThreadSingleContextWindowManager::show(QQuickWindow *window)
{
#ifdef THREAD_DEBUG
    printf("GUI: Window added to windowing system, %p, %dx%d\n", window, window->width(), window->height());
#endif

    WindowTracker tracker;
    tracker.window = window;
    tracker.isVisible = false;
    tracker.toBeRemoved = false;
    m_tracked_windows << tracker;

    connect(window, SIGNAL(widthChanged(int)), this, SLOT(windowVisibilityChanged()), Qt::DirectConnection);
    connect(window, SIGNAL(heightChanged(int)), this, SLOT(windowVisibilityChanged()), Qt::DirectConnection);

    windowVisibilityChanged();
}


void QQuickRenderThreadSingleContextWindowManager::handleAddedWindow(QQuickWindow *window)
{
#ifdef THREAD_DEBUG
    printf("                RenderThread: adding window: %p\n", window);
#endif

    WindowData *data = new WindowData;
    data->sizeWasChanged = false;
    data->windowSize = window->size();
    data->isVisible = window->isVisible();
    data->isRenderable = QQuickWindowPrivate::get(window)->isRenderable();
    m_rendered_windows[window] = data;

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
        QQuickWindow *window = m_added_windows.takeLast();
        handleAddedWindow(window);
    }
}


/*!
    Called on the GUI Thread, from the window' destructor
 */

void QQuickRenderThreadSingleContextWindowManager::windowDestroyed(QQuickWindow *window)
{
#ifdef THREAD_DEBUG
    printf("GUI: Window destroyed: %p\n", window);
#endif

    hide(window);
}


/*!
    Called on GUI Thread
 */

void QQuickRenderThreadSingleContextWindowManager::hide(QQuickWindow *window)
{
#ifdef THREAD_DEBUG
    printf("GUI: Window hidden: %p\n", window);
#endif

    int position = -1;
    for (int i=0; i<m_tracked_windows.size(); ++i) {
        if (m_tracked_windows.at(i).window == window) {
            m_tracked_windows[i].toBeRemoved = true;
            position = i;
            break;
        }
    }

    if (position >= 0) {
        disconnect(window, SIGNAL(widthChanged(int)), this, SLOT(windowVisibilityChanged()));
        disconnect(window, SIGNAL(heightChanged(int)), this, SLOT(windowVisibilityChanged()));
        windowVisibilityChanged();
        m_tracked_windows.removeAt(position);
    }

#ifdef THREAD_DEBUG
    printf("GUI: Window removal completed... %p\n", window);
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
        QQuickWindow *window = m_removed_windows.takeLast();
#ifdef THREAD_DEBUG
    printf("                RenderThread: removing %p\n", window);
#endif

        QQuickWindowPrivate::get(window)->cleanupNodesOnShutdown();
        delete m_rendered_windows.take(window);
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

void QQuickRenderThreadSingleContextWindowManager::windowVisibilityChanged()
{
    bool anyoneShowing = false;
    QList<QQuickWindow *> toAdd, toRemove;

    // Not optimal, but also not frequently used...
    for (int i=0; i<m_tracked_windows.size(); ++i) {
        WindowTracker &t = const_cast<WindowTracker &>(m_tracked_windows.at(i));
        QQuickWindow *win = t.window;

        Q_ASSERT(win->isVisible() || QQuickWindowPrivate::get(win)->renderWithoutShowing || t.toBeRemoved);
        bool windowVisible = win->width() > 0 && win->height() > 0;
        anyoneShowing |= (windowVisible && !t.toBeRemoved);

        if ((!windowVisible && t.isVisible) || t.toBeRemoved) {
            toRemove << win;
        } else if (windowVisible && !t.isVisible) {
            toAdd << win;
        }
        t.isVisible = windowVisible;
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
            printf("                RenderThread: acquired sync lock...\n");
#endif
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
#ifdef QQUICK_RENDER_TIMING
        if (qquick_render_timing())
            threadTimer.start();
#endif
        inSync = true;
        for (QHash<QQuickWindow *, WindowData *>::const_iterator it = m_rendered_windows.constBegin();
             it != m_rendered_windows.constEnd(); ++it) {
            QQuickWindow *window = it.key();

#ifdef THREAD_DEBUG
            printf("                RenderThread: Syncing window: %p\n", window);
#endif

            WindowData *windowData = it.value();
            QQuickWindowPrivate *windowPrivate = QQuickWindowPrivate::get(window);

            windowData->isRenderable = windowPrivate->isRenderable();

            if (windowData->isRenderable) {
                gl->makeCurrent(window);

                if (windowData->viewportSize != windowData->windowSize) {
#ifdef THREAD_DEBUG
                    printf("                RenderThread: --- window has changed size...\n");
#endif
                    windowData->viewportSize = windowData->windowSize;
                    windowData->sizeWasChanged = true;
                    glViewport(0, 0, windowData->viewportSize.width(), windowData->viewportSize.height());
                }

                windowPrivate->syncSceneGraph();
            }
        }
        inSync = false;

        // Wake GUI after sync to let it continue animating and event processing.
        wake();
        unlock();
#ifdef THREAD_DEBUG
        printf("                RenderThread: sync done\n");
#endif
#ifdef QQUICK_RENDER_TIMING
        if (qquick_render_timing())
            syncTime = threadTimer.elapsed();
#endif

        for (QHash<QQuickWindow *, WindowData *>::const_iterator it = m_rendered_windows.constBegin();
             it != m_rendered_windows.constEnd(); ++it) {
            QQuickWindow *window = it.key();
            WindowData *windowData = it.value();
            QQuickWindowPrivate *windowPrivate = QQuickWindowPrivate::get(window);

            if (!windowData->isRenderable)
                continue;

#ifdef THREAD_DEBUG
            printf("                RenderThread: Rendering window %p\n", window);
#endif

            Q_ASSERT(windowData->windowSize.width() > 0 && windowData->windowSize.height() > 0);

#ifdef THREAD_DEBUG
            printf("                RenderThread: --- rendering at size %dx%d\n",
                   windowData->viewportSize.width(), windowData->viewportSize.height()
                   );
#endif

            // We only need to re-makeCurrent when we have multiple surfaces.
            if (m_rendered_windows.size() > 1)
                gl->makeCurrent(window);

            windowPrivate->renderSceneGraph(windowData->viewportSize);
#ifdef QQUICK_RENDER_TIMING
            if (qquick_render_timing())
                renderTime = threadTimer.elapsed() - syncTime;
#endif

            // The content of the target buffer is undefined after swap() so grab needs
            // to happen before swap();
            if (window == windowToGrab) {
#ifdef THREAD_DEBUG
                printf("                RenderThread: --- grabbing...\n");
#endif
                grabContent = qt_gl_read_framebuffer(windowData->windowSize, false, false);
                windowToGrab = 0;
            }

#ifdef THREAD_DEBUG
            printf("                RenderThread: --- wait for swap...\n");
#endif

            if (windowData->isVisible && window->isExposed())
                gl->swapBuffers(window);

            windowPrivate->fireFrameSwapped();
#ifdef THREAD_DEBUG
            printf("                RenderThread: --- swap complete...\n");
#endif

        }

#ifdef QQUICK_RENDER_TIMING
            if (qquick_render_timing()) {
                static QTime lastFrameTime = QTime::currentTime();
                swapTime = threadTimer.elapsed() - renderTime - syncTime;
                qDebug() << "- Breakdown of frame time; sync:" << syncTime
                         << "ms render:" << renderTime << "ms swap:" << swapTime
                         << "ms total:" << swapTime + renderTime + syncTime
                         << "ms time since last frame:" << (lastFrameTime.msecsTo(QTime::currentTime()))
                         << "ms";
                lastFrameTime = QTime::currentTime();
            }
#endif

        lock();

        handleRemovedWindows();

        // Update sizes...
        for (QHash<QQuickWindow *, WindowData *>::const_iterator it = m_rendered_windows.constBegin();
             it != m_rendered_windows.constEnd(); ++it) {
            WindowData *windowData = it.value();
            if (windowData->sizeWasChanged) {
                windowData->renderedSize = windowData->viewportSize;
                windowData->sizeWasChanged = false;
            }
        }


        // Wake the GUI thread now that rendering is complete, to signal that painting
        // is done, resizing is done or grabbing is completed. For grabbing, we're
        // signalling this much later than needed (we could have done it before swap)
        // but we don't want to lock an extra time.
        wake();

        if (!animationRunning && !isExternalUpdatePending && !shouldExit && !windowToGrab) {
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

        // If all windowes have been hidden, ignore the event
        if (!isRunning())
            return true;

        if (!syncAlreadyHappened)
            sync(false);

        syncAlreadyHappened = false;

        if (animationRunning) {
#ifdef THREAD_DEBUG
            printf("GUI: Advancing animations...\n");
#endif

            animDriver->advance();

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
        animDriver->advance();
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

    for (QHash<QQuickWindow *, WindowData *>::const_iterator it = m_rendered_windows.constBegin();
         it != m_rendered_windows.constEnd(); ++it) {
        QQuickWindowPrivate::get(it.key())->polishItems();
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
    printf("GUI: acquired lock... level=%d\n", isGuiLocked);
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


void QQuickRenderThreadSingleContextWindowManager::exposureChanged(QQuickWindow *window)
{
    Q_UNUSED(window);
#ifdef THREAD_DEBUG
    printf("GUI: exposure changed: %p\n", window);
#endif

    if (window->isExposed())
        maybeUpdate(window);

#ifdef THREAD_DEBUG
    printf("GUI: exposure changed done: %p\n", window);
#endif
}



void QQuickRenderThreadSingleContextWindowManager::resize(QQuickWindow *window, const QSize &size)
{
#ifdef THREAD_DEBUG
    printf("GUI: Resize Event: %p = %dx%d\n", window, size.width(), size.height());
#endif

    // If the rendering thread is not running we do not need to do anything.
    // Also if the window is being resized to an invalid size, it will be removed
    // by the windowVisibilityChanged slot as result of width/heightcChanged()
    if (!isRunning() || size.width() <= 0 || size.height() <= 0)
        return;

    lockInGui();
    exhaustSyncEvent();

    WindowData *windowData = m_rendered_windows.value(window);
    if (windowData) {
        windowData->windowSize = size;
        while (isRunning() && windowData->renderedSize != size && size.width() > 0 && size.height() > 0) {
            if (isRenderBlocked)
                wake();
            wait();
        }
    }
    unlockInGui();

#ifdef THREAD_DEBUG
    printf("GUI: Resize done: %p\n", window);
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
    animationRunning = animDriver->isRunning();
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
    if (animDriver->isRunning())
        animationTimer = startTimer(1000/60);
}



QImage QQuickRenderThreadSingleContextWindowManager::grab(QQuickWindow *window)
{
    if (!isRunning())
        return QImage();

    if (QThread::currentThread() != qApp->thread()) {
        qWarning("QQuickWindow::grabFrameBuffer: can only be called from the GUI thread");
        return QImage();
    } else if (window->size().width() <= 0 || window->size().height() <= 0 ) {
        qWarning("QQuickWindow::grabFrameBuffer: Can't grab a Window with size %dx%d", window->size().width(), window->size().height());
        return QImage();
    }

#ifdef THREAD_DEBUG
    printf("GUI: doing a pixelwise grab..\n");
#endif

    lockInGui();
    exhaustSyncEvent();

    windowToGrab = window;
    while (isRunning() && windowToGrab) {
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

void QQuickRenderThreadSingleContextWindowManager::maybeUpdate(QQuickWindow *)
{
    Q_ASSERT_X(QThread::currentThread() == QCoreApplication::instance()->thread() || inSync,
               "QQuickWindow::update",
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

QT_END_NAMESPACE
