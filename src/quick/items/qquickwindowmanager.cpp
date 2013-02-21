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

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qquick_render_timing, QML_RENDER_TIMING)

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

QQuickWindowManager *QQuickWindowManager::s_instance = 0;

QQuickWindowManager::~QQuickWindowManager()
{
}

class QQuickTrivialWindowManager : public QObject, public QQuickWindowManager
{
    Q_OBJECT
public:
    QQuickTrivialWindowManager();

    void show(QQuickWindow *window);
    void hide(QQuickWindow *window);

    void windowDestroyed(QQuickWindow *window);

    void initializeGL();
    void renderWindow(QQuickWindow *window);
    void exposureChanged(QQuickWindow *window);
    QImage grab(QQuickWindow *window);
    void resize(QQuickWindow *window, const QSize &size);

    void maybeUpdate(QQuickWindow *window);
    void update(QQuickWindow *window) { maybeUpdate(window); } // identical for this implementation.

    void releaseResources() { }

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


QQuickWindowManager *QQuickWindowManager::instance()
{
    if (!s_instance) {

        s_instance = QSGContext::createWindowManager();

        bool bufferQueuing = QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::BufferQueueingOpenGL);
#ifdef Q_OS_WIN
        bool fancy = false; // QTBUG-28037
#else
        bool fancy = QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL);
#endif
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

        if (!s_instance) {
            s_instance = fancy
                    ? (QQuickWindowManager*) new QQuickRenderThreadSingleContextWindowManager
                    : (QQuickWindowManager*) new QQuickTrivialWindowManager;
        }
    }
    return s_instance;
}

void QQuickWindowManager::setInstance(QQuickWindowManager *instance)
{
    Q_ASSERT(!s_instance);
    s_instance = instance;
}

QQuickTrivialWindowManager::QQuickTrivialWindowManager()
    : gl(0)
    , eventPending(false)
{
    sg = QSGContext::createDefaultContext();
}


void QQuickTrivialWindowManager::show(QQuickWindow *window)
{
    WindowData data;
    data.updatePending = false;
    data.grabOnly = false;
    m_windows[window] = data;

    maybeUpdate(window);
}

void QQuickTrivialWindowManager::hide(QQuickWindow *window)
{
    if (!m_windows.contains(window))
        return;

    m_windows.remove(window);
    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    cd->cleanupNodesOnShutdown();

    if (m_windows.size() == 0) {
        sg->invalidate();
        delete gl;
        gl = 0;
    }
}

void QQuickTrivialWindowManager::windowDestroyed(QQuickWindow *window)
{
    hide(window);
}

void QQuickTrivialWindowManager::renderWindow(QQuickWindow *window)
{
    bool renderWithoutShowing = QQuickWindowPrivate::get(window)->renderWithoutShowing;
    if ((!window->isExposed() && !renderWithoutShowing) || !m_windows.contains(window))
        return;

    WindowData &data = const_cast<WindowData &>(m_windows[window]);

    QQuickWindow *masterWindow = 0;
    if (!window->isVisible() && !renderWithoutShowing) {
        // Find a "proper surface" to bind...
        for (QHash<QQuickWindow *, WindowData>::const_iterator it = m_windows.constBegin();
             it != m_windows.constEnd() && !masterWindow; ++it) {
            if (it.key()->isVisible())
                masterWindow = it.key();
        }
    } else {
        masterWindow = window;
    }

    if (!masterWindow)
        return;

    if (!QQuickWindowPrivate::get(masterWindow)->isRenderable()) {
        qWarning().nospace()
            << "Unable to find a renderable master window "
            << masterWindow << "when trying to render"
            << window << " (" << window->geometry() << ").";
        return;
    }

    if (!gl) {
        gl = new QOpenGLContext();
        gl->setFormat(masterWindow->requestedFormat());
        gl->create();
        if (!gl->makeCurrent(masterWindow))
            qWarning("QQuickWindow: makeCurrent() failed...");
        sg->initialize(gl);
    } else {
        gl->makeCurrent(masterWindow);
    }

    bool alsoSwap = data.updatePending;
    data.updatePending = false;

    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(window);
    cd->polishItems();

    int renderTime = 0, syncTime = 0;
    QTime renderTimer;
    if (qquick_render_timing())
        renderTimer.start();

    cd->syncSceneGraph();

    if (qquick_render_timing())
        syncTime = renderTimer.elapsed();

    cd->renderSceneGraph(window->size());

    if (qquick_render_timing())
        renderTime = renderTimer.elapsed() - syncTime;

    if (data.grabOnly) {
        grabContent = qt_gl_read_framebuffer(window->size(), false, false);
        data.grabOnly = false;
    }

    if (alsoSwap && window->isVisible()) {
        gl->swapBuffers(window);
        cd->fireFrameSwapped();
    }

    if (qquick_render_timing()) {
        static QTime lastFrameTime = QTime::currentTime();
        const int swapTime = renderTimer.elapsed() - renderTime - syncTime;
        qDebug() << "- Breakdown of frame time; sync:" << syncTime
                 << "ms render:" << renderTime << "ms swap:" << swapTime
                 << "ms total:" << swapTime + renderTime + syncTime
                 << "ms time since last frame:" << (lastFrameTime.msecsTo(QTime::currentTime()))
                 << "ms";
        lastFrameTime = QTime::currentTime();
    }

    // Might have been set during syncSceneGraph()
    if (data.updatePending)
        maybeUpdate(window);
}

void QQuickTrivialWindowManager::exposureChanged(QQuickWindow *window)
{
    if (window->isExposed())
        maybeUpdate(window);
}

QImage QQuickTrivialWindowManager::grab(QQuickWindow *window)
{
    if (!m_windows.contains(window))
        return QImage();

    m_windows[window].grabOnly = true;

    renderWindow(window);

    QImage grabbed = grabContent;
    grabContent = QImage();
    return grabbed;
}



void QQuickTrivialWindowManager::resize(QQuickWindow *, const QSize &)
{
}



void QQuickTrivialWindowManager::maybeUpdate(QQuickWindow *window)
{
    if (!m_windows.contains(window))
        return;

    m_windows[window].updatePending = true;

    if (!eventPending) {
        QCoreApplication::postEvent(this, new QEvent(QEvent::User));
        eventPending = true;
    }
}



QSGContext *QQuickTrivialWindowManager::sceneGraphContext() const
{
    return sg;
}


bool QQuickTrivialWindowManager::event(QEvent *e)
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

#include "qquickwindowmanager.moc"

QT_END_NAMESPACE
