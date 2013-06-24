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

#include "qsgrenderloop_p.h"
#include "qsgthreadedrenderloop_p.h"
#include "qsgwindowsrenderloop_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTime>
#include <QtCore/private/qabstractanimation_p.h>

#include <QtGui/QOpenGLContext>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

#include <QtQml/private/qqmlglobal_p.h>

#include <QtQuick/QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <private/qqmlprofilerservice_p.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qsg_render_timing, QSG_RENDER_TIMING)

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

/*!
    expectations for this manager to work:
     - one opengl context to render multiple windows
     - OpenGL pipeline will not block for vsync in swap
     - OpenGL pipeline will block based on a full buffer queue.
     - Multiple screens can share the OpenGL context
     - Animations are advanced for all windows once per swap
 */

DEFINE_BOOL_CONFIG_OPTION(qmlNoThreadedRenderer, QML_BAD_GUI_RENDER_LOOP);
DEFINE_BOOL_CONFIG_OPTION(qmlForceThreadedRenderer, QML_FORCE_THREADED_RENDERER); // Might trigger graphics driver threading bugs, use at own risk

QSGRenderLoop *QSGRenderLoop::s_instance = 0;

QSGRenderLoop::~QSGRenderLoop()
{
}

class QSGGuiThreadRenderLoop : public QSGRenderLoop
{
    Q_OBJECT
public:
    QSGGuiThreadRenderLoop();

    void show(QQuickWindow *window);
    void hide(QQuickWindow *window);

    void windowDestroyed(QQuickWindow *window);

    void initializeGL();
    void renderWindow(QQuickWindow *window);
    void exposureChanged(QQuickWindow *window);
    QImage grab(QQuickWindow *window);

    void maybeUpdate(QQuickWindow *window);
    void update(QQuickWindow *window) { maybeUpdate(window); } // identical for this implementation.

    void releaseResources(QQuickWindow *) { }

    QAnimationDriver *animationDriver() const { return 0; }

    QSGContext *sceneGraphContext() const;

    bool event(QEvent *);

    struct WindowData {
        bool updatePending : 1;
        bool grabOnly : 1;
    };

    QHash<QQuickWindow *, WindowData> m_windows;

    QOpenGLContext *gl;
    QSGContext *sg;

    QImage grabContent;

    bool eventPending;
};


QSGRenderLoop *QSGRenderLoop::instance()
{
    if (!s_instance) {

        s_instance = QSGContext::createWindowManager();

        bool bufferQueuing = QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::BufferQueueingOpenGL);

        // Enable fixed animation steps...
        QByteArray fixed = qgetenv("QML_FIXED_ANIMATION_STEP");
        bool fixedAnimationSteps = bufferQueuing;
        if (fixed == "no")
            fixedAnimationSteps = false;
        else if (fixed.length())
            fixedAnimationSteps = true;
        if (fixedAnimationSteps)
            QUnifiedTimer::instance(true)->setConsistentTiming(true);

        if (!s_instance) {

            enum RenderLoopType {
                BasicRenderLoop,
                ThreadedRenderLoop,
                WindowsRenderLoop
            };

            RenderLoopType loopType = BasicRenderLoop;

#ifdef Q_OS_WIN
            loopType = WindowsRenderLoop;
#else
            if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL))
                loopType = ThreadedRenderLoop;
#endif
            if (qmlNoThreadedRenderer())
                loopType = BasicRenderLoop;
            else if (qmlForceThreadedRenderer())
                loopType = ThreadedRenderLoop;

            const QByteArray loopName = qgetenv("QSG_RENDER_LOOP");
            if (loopName == QByteArrayLiteral("windows"))
                loopType = WindowsRenderLoop;
            else if (loopName == QByteArrayLiteral("basic"))
                loopType = BasicRenderLoop;
            else if (loopName == QByteArrayLiteral("threaded"))
                loopType = ThreadedRenderLoop;

            switch (loopType) {
            case ThreadedRenderLoop:
                s_instance = new QSGThreadedRenderLoop();
                break;
            case WindowsRenderLoop:
                s_instance = new QSGWindowsRenderLoop();
                break;
            default:
                s_instance = new QSGGuiThreadRenderLoop();
                break;
            }
        }
    }
    return s_instance;
}

void QSGRenderLoop::setInstance(QSGRenderLoop *instance)
{
    Q_ASSERT(!s_instance);
    s_instance = instance;
}

QSGGuiThreadRenderLoop::QSGGuiThreadRenderLoop()
    : gl(0)
    , eventPending(false)
{
    sg = QSGContext::createDefaultContext();
}


void QSGGuiThreadRenderLoop::show(QQuickWindow *window)
{
    WindowData data;
    data.updatePending = false;
    data.grabOnly = false;
    m_windows[window] = data;

    maybeUpdate(window);
}

void QSGGuiThreadRenderLoop::hide(QQuickWindow *window)
{
    if (!m_windows.contains(window))
        return;

    m_windows.remove(window);
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    cd->cleanupNodesOnShutdown();

    if (m_windows.size() == 0) {
        if (!cd->persistentSceneGraph) {
            sg->invalidate();
            if (!cd->persistentGLContext) {
                delete gl;
                gl = 0;
            }
        }
    }
}

void QSGGuiThreadRenderLoop::windowDestroyed(QQuickWindow *window)
{
    hide(window);
    if (m_windows.size() == 0) {
        sg->invalidate();
        delete gl;
        gl = 0;
    }
}

void QSGGuiThreadRenderLoop::renderWindow(QQuickWindow *window)
{
    if (!QQuickWindowPrivate::get(window)->isRenderable() || !m_windows.contains(window))
        return;

    WindowData &data = const_cast<WindowData &>(m_windows[window]);

    bool current = false;

    if (!gl) {
        gl = new QOpenGLContext();
        gl->setFormat(window->requestedFormat());
        if (!gl->create()) {
            delete gl;
            gl = 0;
        }
        current = gl->makeCurrent(window);
        if (current)
            sg->initialize(gl);
    } else {
        current = gl->makeCurrent(window);
    }

    bool alsoSwap = data.updatePending;
    data.updatePending = false;

    if (!current)
        return;

    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    cd->polishItems();

    qint64 renderTime = 0, syncTime = 0;
    QElapsedTimer renderTimer;
    bool profileFrames = qsg_render_timing()  || QQmlProfilerService::enabled;
    if (profileFrames)
        renderTimer.start();

    cd->syncSceneGraph();

    if (profileFrames)
        syncTime = renderTimer.nsecsElapsed();

    cd->renderSceneGraph(window->size());

    if (profileFrames)
        renderTime = renderTimer.nsecsElapsed() - syncTime;

    if (data.grabOnly) {
        grabContent = qt_gl_read_framebuffer(window->size(), false, false);
        data.grabOnly = false;
    }

    if (alsoSwap && window->isVisible()) {
        gl->swapBuffers(window);
        cd->fireFrameSwapped();
    }

    qint64 swapTime = 0;
    if (profileFrames) {
        swapTime = renderTimer.nsecsElapsed() - renderTime - syncTime;
    }

    if (qsg_render_timing()) {
        static QTime lastFrameTime = QTime::currentTime();
        qDebug() << "- Breakdown of frame time; sync:" << syncTime/1000000
                 << "ms render:" << renderTime/1000000 << "ms swap:" << swapTime/1000000
                 << "ms total:" << (swapTime + renderTime + syncTime)/1000000
                 << "ms time since last frame:" << (lastFrameTime.msecsTo(QTime::currentTime()))
                 << "ms";
        lastFrameTime = QTime::currentTime();
    }

    if (QQmlProfilerService::enabled) {
        QQmlProfilerService::sceneGraphFrame(
                    QQmlProfilerService::SceneGraphRenderLoopFrame,
                    syncTime,
                    renderTime,
                    swapTime);
    }

    // Might have been set during syncSceneGraph()
    if (data.updatePending)
        maybeUpdate(window);
}

void QSGGuiThreadRenderLoop::exposureChanged(QQuickWindow *window)
{
    if (window->isExposed()) {
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
    if (!m_windows.contains(window))
        return;

    m_windows[window].updatePending = true;

    if (!eventPending) {
        QCoreApplication::postEvent(this, new QEvent(QEvent::User));
        eventPending = true;
    }
}



QSGContext *QSGGuiThreadRenderLoop::sceneGraphContext() const
{
    return sg;
}


bool QSGGuiThreadRenderLoop::event(QEvent *e)
{
    if (e->type() == QEvent::User) {
        eventPending = false;
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

#include "qsgrenderloop.moc"

QT_END_NAMESPACE
