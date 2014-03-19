/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "qquickrendercontrol.h"
#include "qquickrendercontrol_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QTime>
#include <QtCore/private/qabstractanimation_p.h>

#include <QtGui/QOpenGLContext>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

#include <QtQml/private/qqmlglobal_p.h>

#include <QtQuick/QQuickWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <private/qqmlprofilerservice_p.h>
#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

/*!
  \class QQuickRenderControl

  \brief The QQuickRenderControl class provides a mechanism for rendering the Qt
  Quick scenegraph onto an offscreen render target in a fully
  application-controlled manner.

  \since 5.4

  QQuickWindow and QQuickView and their associated internal render loops render
  the Qt Quick scene onto a native window. In some cases, for example when
  integrating with 3rd party OpenGL renderers, it might be beneficial to get the
  scene into a texture that can then be used in arbitrary ways by the external
  rendering engine. QQuickRenderControl makes this possible in a hardware
  accelerated manner, unlike the performance-wise limited alternative of using
  QQuickWindow::grabWindow()

  When using a QQuickRenderControl, the QQuickWindow does not have to be shown
  or even created at all. This means there will not be an underlying native
  window for it. Instead, the QQuickWindow instance is associated with the
  render control, using the overload of the QQuickWindow constructor, and an
  OpenGL framebuffer object by calling QQuickWindow::setRenderTarget().

  Management of the context and framebuffer object is up to the application. The
  context that will be used by Qt Quick must be created before calling
  initialize(). The creation of the framebuffer object can be deferred, see
  below. Qt 5.4 introduces the ability for QOpenGLContext to adopt existing
  native contexts. Together with QQuickRenderControl this makes it possible to
  create a QOpenGLContext that shares with an external rendering engine's
  existing context. This new QOpenGLContext can then be used to render the Qt
  Quick scene into a texture that is accessible by the other engine's context
  too.

  Loading and instantiation of the QML components happen by using a
  QQmlEngine. Once the root object is created, it will need to be parented to
  the QQuickWindow's contentItem().

  Applications will usually have to connect to 4 important signals:

  \list

  \li QQuickWindow::sceneGraphInitialized() Emitted at some point after calling
  QQuickRenderControl::initialize(). Upon this signal, the application is
  expected to create its framebuffer object and associate it with the
  QQuickWindow.

  \li QQuickWindow::sceneGraphInvalidated() When the scenegraph resources are
  released, the framebuffer object can be destroyed too.

  \li QQuickRenderControl::renderRequested() Indicates that the scene has to be
  rendered by calling render(). After making the context current, applications
  are expected to call render().

  \li QQuickRenderControl::sceneChanged() Inidcates that the scene has changed
  meaning that, before rendering, polishing and synchronizing is also necessary.

  \endlist

  To send events, for example mouse or keyboard events, to the scene, use
  QCoreApplication::sendEvent() with the QQuickWindow instance as the receiver.

  \inmodule QtQuick
*/

QSGContext *QQuickRenderControlPrivate::sg = 0;

QQuickRenderControlPrivate::QQuickRenderControlPrivate()
    : initialized(0),
      window(0)
{
    if (!sg) {
        qAddPostRoutine(cleanup);
        sg = QSGContext::createDefaultContext();
    }
    rc = new QSGRenderContext(sg);
}

QQuickRenderControlPrivate::~QQuickRenderControlPrivate()
{
    delete rc;
}

void QQuickRenderControlPrivate::cleanup()
{
    delete sg;
    sg = 0;
}

QQuickRenderControl::QQuickRenderControl(QObject *parent)
    : QObject(*(new QQuickRenderControlPrivate), parent)
{
}

/*!
  Destroys the instance. Releases all scenegraph resources.

  \sa stop()
 */
QQuickRenderControl::~QQuickRenderControl()
{
    Q_D(QQuickRenderControl);

    stop();

    if (d->window)
        QQuickWindowPrivate::get(d->window)->renderControl = 0;
}

void QQuickRenderControlPrivate::windowDestroyed()
{
    if (window == 0) {
        rc->invalidate();
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    }
}

/*!
  Initializes the scene graph resources. The context \a gl has to
  be the current context.
 */
void QQuickRenderControl::initialize(QOpenGLContext *gl)
{
    Q_D(QQuickRenderControl);

    if (!d->window) {
        qWarning("QQuickRenderControl::initialize called with no associated window");
        return;
    }

    if (QOpenGLContext::currentContext() != gl) {
        qWarning("QQuickRenderControl::initialize called with incorrect current context");
        return;
    }

    // It is the caller's responsiblity to make a context/surface current.
    // It cannot be done here since the surface to use may not be the
    // surface belonging to window. In fact window may not have a native
    // window/surface at all.

    d->rc->initialize(gl);

    d->initialized = true;
}

/*!
  This function should be called as late as possible before
  sync(). In a threaded scenario, rendering can happen in parallel
  with this function.
 */
void QQuickRenderControl::polishItems()
{
    Q_D(QQuickRenderControl);
    if (!d->window)
        return;

    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(d->window);
    cd->flushDelayedTouchEvent();
    if (!d->window)
        return;
    cd->polishItems();
}

/*!
  This function is used to synchronize the QML scene with the rendering scene
  graph.

  If a dedicated render thread is used, the GUI thread should be blocked for the
  duration of this call.

  \return \e true if the synchronization changed the scene graph.
 */
bool QQuickRenderControl::sync()
{
    Q_D(QQuickRenderControl);
    if (!d->window)
        return false;

    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(d->window);
    cd->syncSceneGraph();

    // TODO: find out if the sync actually caused a scenegraph update.
    return true;
}

/*!
  Stop rendering and release resources. Requires a current context.

  This is the equivalent of the cleanup operations that happen with a
  real QQuickWindow when the window becomes hidden.

  This function takes QQuickWindow::persistentSceneGraph() into
  account, meaning that context-specific resources are not released
  when persistency is enabled.

  This function is called from the destructor. Therefore there will
  typically be no need to call it directly. Pay attention however to
  the fact that this requires the context, that was passed to
  initialize(), to be the current one at the time of destroying the
  QQuickRenderControl instance.

  Once stop() has been called, it is possible to reuse the
  QQuickRenderControl instance by calling initialize() again.
 */
void QQuickRenderControl::stop()
{
    Q_D(QQuickRenderControl);
    if (!d->initialized)
        return;

    if (!d->window)
        return;

    if (!QOpenGLContext::currentContext())
        return;

    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(d->window);
    cd->fireAboutToStop();
    cd->cleanupNodesOnShutdown();

    if (!cd->persistentSceneGraph) {
        d->rc->invalidate();
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    }

    d->initialized = false;
}

/*!
  Renders the scenegraph using the current context.
 */
void QQuickRenderControl::render()
{
    Q_D(QQuickRenderControl);
    if (!d->window)
        return;

    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(d->window);
    cd->renderSceneGraph(d->window->size());
}

/*!
    \fn void QQuickRenderControl::renderRequested()

    This signal is emitted when the scene graph needs to be rendered. It is not necessary to call sync().
*/

/*!
    \fn void QQuickRenderControl::sceneChanged()

    This signal is emitted when the scene graph is updated, meaning that
    polishItems() and sync() needs to be called. If sync() returns
    true, then render() needs to be called.
*/

/*!
  Grabs the contents of the scene and returns it as an image.

  \note Requires the context to be current.
 */
QImage QQuickRenderControl::grab()
{
    Q_D(QQuickRenderControl);
    if (!d->window)
        return QImage();

    render();
    QImage grabContent = qt_gl_read_framebuffer(d->window->size() * d->window->devicePixelRatio(), false, false);
    return grabContent;
}

void QQuickRenderControlPrivate::update()
{
    Q_Q(QQuickRenderControl);
    emit q->renderRequested();
}

void QQuickRenderControlPrivate::maybeUpdate()
{
    Q_Q(QQuickRenderControl);
    emit q->sceneChanged();
}

/*!
  \fn QWindow *QQuickRenderControl::renderWindow(QPoint *offset)

  Reimplemented in subclasses to return the real window this render control
  is rendering into.

  If \a offset in non-null, it is set to the offset of the control
  inside the window.
*/

/*!
  Returns the real window that \a win is being rendered to, if any.

  If \a offset in non-null, it is set to the offset of the rendering
  inside its window.

 */
QWindow *QQuickRenderControl::renderWindowFor(QQuickWindow *win, QPoint *offset)
{
    if (!win)
        return 0;
    QQuickRenderControl *rc = QQuickWindowPrivate::get(win)->renderControl;
    if (rc)
        return rc->renderWindow(offset);
    return 0;
}

QT_END_NAMESPACE
