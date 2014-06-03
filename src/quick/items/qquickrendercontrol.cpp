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
#include <QtQuick/private/qsgcontext_p.h>
#include <private/qqmlprofilerservice_p.h>
#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

class QQuickRenderControlPrivate : public QObjectPrivate
{
public:
    QQuickRenderControlPrivate()
        : window(0)
    {
        sg = QSGContext::createDefaultContext();
        rc = new QSGRenderContext(sg);
    }

    ~QQuickRenderControlPrivate()
     {
         delete rc;
         delete sg;
     }

    QQuickWindow *window;
    QSGContext *sg;
    QSGRenderContext *rc;
};

/*!
  \class QQuickRenderControl
  \brief The QQuickRenderControl class provides a mechanism for rendering the Qt Quick scenegraph.

  \internal

  \inmodule QtQuick
*/

QQuickRenderControl::QQuickRenderControl()
    : QObject(*(new QQuickRenderControlPrivate), 0)
{
}

QQuickRenderControl::~QQuickRenderControl()
{
}

void QQuickRenderControl::windowDestroyed()
{
    Q_D(QQuickRenderControl);
    if (d->window == 0) {
        d->rc->invalidate();
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    }
}

void QQuickRenderControl::initialize(QOpenGLContext *gl)
{
    Q_D(QQuickRenderControl);
    if (!d->window)
        return;

    // It is the caller's responsiblity to make a context/surface current.
    // It cannot be done here since the surface to use may not be the
    // surface belonging to window. In fact window may not have a native
    // window/surface at all.

    d->rc->initialize(gl);
}

void QQuickRenderControl::invalidate()
{
    Q_D(QQuickRenderControl);
    d->rc->invalidate();
}

/*!
  This function should be called as late as possible before
  sync(). In a threaded scenario, rendering can happen in parallel with this function.
 */
void QQuickRenderControl::polishItems()
{
    Q_D(QQuickRenderControl);
    if (!d->window)
        return;

    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(d->window);
    cd->polishItems();
}

/*!
  Synchronize GUI and scenegraph. Returns true if the scene graph was changed.

  This function is a synchronization point. Rendering can not happen in parallel.
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
  Stop rendering and release resources. This function is typically
  called when the window is hidden. Requires a current context.
 */
void QQuickRenderControl::stop()
{
    Q_D(QQuickRenderControl);
    if (!d->window)
        return;

    QQuickWindowPrivate *cd = QQuickWindowPrivate::get(d->window);
    cd->fireAboutToStop();
    cd->cleanupNodesOnShutdown();

    if (!cd->persistentSceneGraph) {
        d->rc->invalidate();
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    }
}

/*!
  Render the scenegraph using the current context.
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


QImage QQuickRenderControl::grab()
{
    Q_D(QQuickRenderControl);
    if (!d->window)
        return QImage();

    render();
    QImage grabContent = qt_gl_read_framebuffer(d->window->size() * d->window->devicePixelRatio(), false, false);
    return grabContent;
}

QSGContext *QQuickRenderControl::sceneGraphContext() const
{
    Q_D(const QQuickRenderControl);
    return d->sg;
}

QSGRenderContext *QQuickRenderControl::renderContext(QSGContext *) const
{
    Q_D(const QQuickRenderControl);
    return d->rc;
}

void QQuickRenderControl::setWindow(QQuickWindow *window)
{
    Q_D(QQuickRenderControl);
    d->window = window;
}

/*!
  Returns the offscreen window.
 */

QQuickWindow *QQuickRenderControl::window() const
{
    Q_D(const QQuickRenderControl);
    return d->window;
}

/*!
  Create an offscreen QQuickWindow for this render control,
  unless the render control already has a window().

  Returns the offscreen window if one is created, otherwise returns null.
  The caller takes ownership of the window, and is responsible for deleting it.
 */
QQuickWindow *QQuickRenderControl::createOffscreenWindow()
{
    Q_D(QQuickRenderControl);
    if (!d->window)
        return new QQuickWindow(this);
    return 0;
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
