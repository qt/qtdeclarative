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

#include "qquickwidget.h"
#include "qquickwidget_p.h"

#include "private/qquickwindow_p.h"
#include "private/qquickitem_p.h"
#include "private/qquickitemchangelistener_p.h"
#include "private/qquickrendercontrol_p.h"

#include <private/qquickprofiler_p.h>
#include <private/qqmlinspectorservice_p.h>
#include <private/qqmlmemoryprofiler_p.h>

#include <QtQml/qqmlengine.h>
#include <private/qqmlengine_p.h>
#include <QtCore/qbasictimer.h>

QT_BEGIN_NAMESPACE

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

void QQuickWidgetPrivate::init(QQmlEngine* e)
{
    Q_Q(QQuickWidget);

    setRenderToTexture();
    engine = e;

    if (engine.isNull())
        engine = new QQmlEngine(q);

    if (!engine.data()->incubationController())
        engine.data()->setIncubationController(offscreenWindow->incubationController());

    if (QQmlDebugService::isDebuggingEnabled())
        QQmlInspectorService::instance()->addView(q);

    QWidget::connect(offscreenWindow, SIGNAL(sceneGraphInitialized()), q, SLOT(createFramebufferObject()));
    QWidget::connect(offscreenWindow, SIGNAL(sceneGraphInvalidated()), q, SLOT(destroyFramebufferObject()));
    QObject::connect(renderControl, SIGNAL(renderRequested()), q, SLOT(triggerUpdate()));
    QObject::connect(renderControl, SIGNAL(sceneChanged()), q, SLOT(triggerUpdate()));
}

QQuickWidgetPrivate::QQuickWidgetPrivate()
    : root(0)
    , component(0)
    , fbo(0)
    , context(0)
    , resizeMode(QQuickWidget::SizeViewToRootObject)
    , initialSize(0,0)
    , updateTimer(0)
    , eventPending(false)
    , updatePending(false)
{
    renderControl = new QQuickRenderControl;
    offscreenWindow = new QQuickWindow(renderControl);
    offscreenWindow->setTitle(QString::fromLatin1("Offscreen"));
    // Do not call create() on offscreenWindow.
}

QQuickWidgetPrivate::~QQuickWidgetPrivate()
{
    if (QQmlDebugService::isDebuggingEnabled())
        QQmlInspectorService::instance()->removeView(q_func());
    delete offscreenWindow;
    delete renderControl;
    delete fbo;
}

void QQuickWidgetPrivate::execute()
{
    Q_Q(QQuickWidget);
    if (!engine) {
        qWarning() << "QQuickWidget: invalid qml engine.";
        return;
    }

    if (root) {
        delete root;
        root = 0;
    }
    if (component) {
        delete component;
        component = 0;
    }
    if (!source.isEmpty()) {
        QML_MEMORY_SCOPE_URL(engine.data()->baseUrl().resolved(source));
        component = new QQmlComponent(engine.data(), source, q);
        if (!component->isLoading()) {
            q->continueExecute();
        } else {
            QObject::connect(component, SIGNAL(statusChanged(QQmlComponent::Status)),
                             q, SLOT(continueExecute()));
        }
    }
}

void QQuickWidgetPrivate::itemGeometryChanged(QQuickItem *resizeItem, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_Q(QQuickWidget);
    if (resizeItem == root && resizeMode == QQuickWidget::SizeViewToRootObject) {
        // wait for both width and height to be changed
        resizetimer.start(0,q);
    }
    QQuickItemChangeListener::itemGeometryChanged(resizeItem, newGeometry, oldGeometry);
}

void QQuickWidgetPrivate::renderSceneGraph()
{
    Q_Q(QQuickWidget);
    updatePending = false;

    QOpenGLContext *context = offscreenWindow->openglContext();
    if (!context) {
        qWarning("QQuickWidget: render scenegraph with no context");
        return;
    }

    Q_ASSERT(q->window()->windowHandle()->handle());
    context->makeCurrent(q->window()->windowHandle());
    renderControl->polishItems();
    renderControl->sync();
    renderControl->render();
    glFlush();
    context->doneCurrent();
    q->update();
}

/*!
    \class QQuickWidget
    \since 5.3
    \brief The QQuickWidget class provides a widget for displaying a Qt Quick user interface.

    \inmodule QtQuick

    This is a convenience wrapper for QQuickWindow which will automatically load and display a QML
    scene when given the URL of the main source file. Alternatively, you can instantiate your own
    objects using QQmlComponent and place them in a manually setup QQuickWidget.

    Typical usage:

    \code
    QQuickWidget *view = new QQuickWidget;
    view->setSource(QUrl::fromLocalFile("myqmlfile.qml"));
    view->show();
    \endcode

    To receive errors related to loading and executing QML with QQuickWidget,
    you can connect to the statusChanged() signal and monitor for QQuickWidget::Error.
    The errors are available via QQuickWidget::errors().

    QQuickWidget also manages sizing of the view and root object.  By default, the \l resizeMode
    is SizeViewToRootObject, which will load the component and resize it to the
    size of the view.  Alternatively the resizeMode may be set to SizeRootObjectToView which
    will resize the view to the size of the root object.

    \note QQuickWidget is an alternative to using QQuickView and QWidget::createWindowContainer().
    The restrictions on stacking order do not apply, making QQuickWidget the more flexible
    alternative behaving more like an ordinary widget. This comes at the expense of
    performance. Unlike QQuickWindow and QQuickView, QQuickWidget involves rendering into OpenGL
    framebuffer objects. This will naturally carry a minor performance hit.

    \note Using QQuickWidget disables the threaded render loop on all platforms. This means that
    some of the benefits of threaded rendering, for example Animator classes and vsync driven
    animations, will not be available.

    \sa {qtqml-cppintegration-exposecppattributes.html}{Exposing Attributes of C++ Types to QML}
*/


/*! \fn void QQuickWidget::statusChanged(QQuickWidget::Status status)
    This signal is emitted when the component's current \a status changes.
*/

/*!
  Constructs a QQuickWidget with the given \a parent.
  The default value of \a parent is 0.

*/
QQuickWidget::QQuickWidget(QWidget *parent)
: QWidget(*(new QQuickWidgetPrivate), parent, 0)
{
    setMouseTracking(true);
    d_func()->init();
}

/*!
  Constructs a QQuickWidget with the given QML \a source and \a parent.
  The default value of \a parent is 0.

*/
QQuickWidget::QQuickWidget(const QUrl &source, QWidget *parent)
: QWidget(*(new QQuickWidgetPrivate), parent, 0)
{
    setMouseTracking(true);
    d_func()->init();
    setSource(source);
}

/*!
  Constructs a QQuickWidget with the given QML \a engine and \a parent.

  Note: In this case, the QQuickWidget does not own the given \a engine object;
  it is the caller's responsibility to destroy the engine. If the \a engine is deleted
  before the view, status() will return QQuickWidget::Error.

  \sa Status, status(), errors()
*/
QQuickWidget::QQuickWidget(QQmlEngine* engine, QWidget *parent)
    : QWidget(*(new QQuickWidgetPrivate), parent, 0)
{
    setMouseTracking(true);
    Q_ASSERT(engine);
    d_func()->init(engine);
}

/*!
  Destroys the QQuickWidget.
*/
QQuickWidget::~QQuickWidget()
{
    // Ensure that the component is destroyed before the engine; the engine may
    // be a child of the QQuickWidgetPrivate, and will be destroyed by its dtor
    Q_D(QQuickWidget);
    delete d->root;
    d->root = 0;
}

/*!
  \property QQuickWidget::source
  \brief The URL of the source of the QML component.

  Ensure that the URL provided is full and correct, in particular, use
  \l QUrl::fromLocalFile() when loading a file from the local filesystem.

  Note that setting a source URL will result in the QML component being
  instantiated, even if the URL is unchanged from the current value.
*/

/*!
    Sets the source to the \a url, loads the QML component and instantiates it.

    Ensure that the URL provided is full and correct, in particular, use
    \l QUrl::fromLocalFile() when loading a file from the local filesystem.

    Calling this method multiple times with the same url will result
    in the QML component being reinstantiated.
 */
void QQuickWidget::setSource(const QUrl& url)
{
    Q_D(QQuickWidget);
    d->source = url;
    d->execute();
}

/*!
    \internal

    Set the source \a url, \a component and content \a item (root of the QML object hierarchy) directly.
 */
void QQuickWidget::setContent(const QUrl& url, QQmlComponent *component, QObject* item)
{
    Q_D(QQuickWidget);
    d->source = url;
    d->component = component;

    if (d->component && d->component->isError()) {
        QList<QQmlError> errorList = d->component->errors();
        foreach (const QQmlError &error, errorList) {
            QMessageLogger(error.url().toString().toLatin1().constData(), error.line(), 0).warning()
                    << error;
        }
        emit statusChanged(status());
        return;
    }

    d->setRootObject(item);
    emit statusChanged(status());
}

/*!
  Returns the source URL, if set.

  \sa setSource()
 */
QUrl QQuickWidget::source() const
{
    Q_D(const QQuickWidget);
    return d->source;
}

/*!
  Returns a pointer to the QQmlEngine used for instantiating
  QML Components.
 */
QQmlEngine* QQuickWidget::engine() const
{
    Q_D(const QQuickWidget);
    return d->engine ? const_cast<QQmlEngine *>(d->engine.data()) : 0;
}

/*!
  This function returns the root of the context hierarchy.  Each QML
  component is instantiated in a QQmlContext.  QQmlContext's are
  essential for passing data to QML components.  In QML, contexts are
  arranged hierarchically and this hierarchy is managed by the
  QQmlEngine.
 */
QQmlContext* QQuickWidget::rootContext() const
{
    Q_D(const QQuickWidget);
    return d->engine ? d->engine.data()->rootContext() : 0;
}

/*!
    \enum QQuickWidget::Status
    Specifies the loading status of the QQuickWidget.

    \value Null This QQuickWidget has no source set.
    \value Ready This QQuickWidget has loaded and created the QML component.
    \value Loading This QQuickWidget is loading network data.
    \value Error One or more errors has occurred. Call errors() to retrieve a list
           of errors.
*/

/*! \enum QQuickWidget::ResizeMode

  This enum specifies how to resize the view.

  \value SizeViewToRootObject The view resizes with the root item in the QML.
  \value SizeRootObjectToView The view will automatically resize the root item to the size of the view.
*/

/*!
    \property QQuickWidget::status
    The component's current \l{QQuickWidget::Status} {status}.
*/

QQuickWidget::Status QQuickWidget::status() const
{
    Q_D(const QQuickWidget);
    if (!d->engine)
        return QQuickWidget::Error;

    if (!d->component)
        return QQuickWidget::Null;

    return QQuickWidget::Status(d->component->status());
}

/*!
    Return the list of errors that occurred during the last compile or create
    operation.  When the status is not Error, an empty list is returned.
*/
QList<QQmlError> QQuickWidget::errors() const
{
    Q_D(const QQuickWidget);
    QList<QQmlError> errs;

    if (d->component)
        errs = d->component->errors();

    if (!d->engine) {
        QQmlError error;
        error.setDescription(QLatin1String("QQuickWidget: invalid qml engine."));
        errs << error;
    }

    return errs;
}

/*!
    \property QQuickWidget::resizeMode
    \brief whether the view should resize the window contents

    If this property is set to SizeViewToRootObject (the default), the view
    resizes to the size of the root item in the QML.

    If this property is set to SizeRootObjectToView, the view will
    automatically resize the root item to the size of the view.

    Regardless of this property, the sizeHint of the view
    is the initial size of the root item. Note though that
    since QML may load dynamically, that size may change.

    \sa initialSize()
*/

void QQuickWidget::setResizeMode(ResizeMode mode)
{
    Q_D(QQuickWidget);
    if (d->resizeMode == mode)
        return;

    if (d->root) {
        if (d->resizeMode == SizeViewToRootObject) {
            QQuickItemPrivate *p = QQuickItemPrivate::get(d->root);
            p->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
        }
    }

    d->resizeMode = mode;
    if (d->root) {
        d->initResize();
    }
}

void QQuickWidgetPrivate::initResize()
{
    if (root) {
        if (resizeMode == QQuickWidget::SizeViewToRootObject) {
            QQuickItemPrivate *p = QQuickItemPrivate::get(root);
            p->addItemChangeListener(this, QQuickItemPrivate::Geometry);
        }
    }
    updateSize();
}

void QQuickWidgetPrivate::updateSize()
{
    Q_Q(QQuickWidget);
    if (!root)
        return;

    if (resizeMode == QQuickWidget::SizeViewToRootObject) {
        QSize newSize = QSize(root->width(), root->height());
        if (newSize.isValid() && newSize != q->size()) {
            q->resize(newSize);
        }
    } else if (resizeMode == QQuickWidget::SizeRootObjectToView) {
        if (!qFuzzyCompare(q->width(), root->width()))
            root->setWidth(q->width());
        if (!qFuzzyCompare(q->height(), root->height()))
            root->setHeight(q->height());
    }
}

QSize QQuickWidgetPrivate::rootObjectSize() const
{
    QSize rootObjectSize(0,0);
    int widthCandidate = -1;
    int heightCandidate = -1;
    if (root) {
        widthCandidate = root->width();
        heightCandidate = root->height();
    }
    if (widthCandidate > 0) {
        rootObjectSize.setWidth(widthCandidate);
    }
    if (heightCandidate > 0) {
        rootObjectSize.setHeight(heightCandidate);
    }
    return rootObjectSize;
}

void QQuickWidgetPrivate::createContext()
{
    Q_Q(QQuickWidget);
    if (context)
        return;

    context = new QOpenGLContext();
    context->setFormat(q->window()->windowHandle()->requestedFormat());
    if (QSGContext::sharedOpenGLContext())
        context->setShareContext(QSGContext::sharedOpenGLContext()); // ??? is this correct
    if (!context->create()) {
        qWarning("QQuickWidget: failed to create OpenGL context");
        delete context;
        context = 0;
    }

    Q_ASSERT(q->window()->windowHandle()->handle());
    if (context->makeCurrent(q->window()->windowHandle()))
        renderControl->initialize(context);
    else
        qWarning("QQuickWidget: failed to make window surface current");
}

void QQuickWidget::createFramebufferObject()
{
    Q_D(QQuickWidget);

    if (d->fbo)
        delete d->fbo;
    QOpenGLContext *context = d->offscreenWindow->openglContext();

    if (!context) {
        qWarning("QQuickWidget: Attempted to create FBO with no context");
        return;
    }

    if (context->shareContext() != QWidgetPrivate::get(window())->shareContext()) {
        context->setShareContext(QWidgetPrivate::get(window())->shareContext());
        context->create();
    }

    Q_ASSERT(window()->windowHandle()->handle());
    context->makeCurrent(window()->windowHandle());
    d->fbo = new QOpenGLFramebufferObject(size());
    d->fbo->setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    d->offscreenWindow->setRenderTarget(d->fbo);

    // Sanity check: The window must not have an underlying platform window.
    // Having one would mean create() was called and platforms that only support
    // a single native window were in trouble.
    Q_ASSERT(!d->offscreenWindow->handle());
}

void QQuickWidget::destroyFramebufferObject()
{
    Q_D(QQuickWidget);
    if (d->fbo)
        delete d->fbo;
    d->fbo = 0;
}

QQuickWidget::ResizeMode QQuickWidget::resizeMode() const
{
    Q_D(const QQuickWidget);
    return d->resizeMode;
}

/*!
  \internal
 */
void QQuickWidget::continueExecute()
{
    Q_D(QQuickWidget);
    disconnect(d->component, SIGNAL(statusChanged(QQmlComponent::Status)), this, SLOT(continueExecute()));

    if (d->component->isError()) {
        QList<QQmlError> errorList = d->component->errors();
        foreach (const QQmlError &error, errorList) {
            QMessageLogger(error.url().toString().toLatin1().constData(), error.line(), 0).warning()
                    << error;
        }
        emit statusChanged(status());
        return;
    }

    QObject *obj = d->component->create();

    if (d->component->isError()) {
        QList<QQmlError> errorList = d->component->errors();
        foreach (const QQmlError &error, errorList) {
            QMessageLogger(error.url().toString().toLatin1().constData(), error.line(), 0).warning()
                    << error;
        }
        emit statusChanged(status());
        return;
    }

    d->setRootObject(obj);
    emit statusChanged(status());
}


/*!
  \internal
*/
void QQuickWidgetPrivate::setRootObject(QObject *obj)
{
    Q_Q(QQuickWidget);
    if (root == obj)
        return;
    if (QQuickItem *sgItem = qobject_cast<QQuickItem *>(obj)) {
        root = sgItem;
        sgItem->setParentItem(offscreenWindow->contentItem());
    } else {
        qWarning() << "QQuickWidget only supports loading of root objects that derive from QQuickItem." << endl
                   << endl
                   << "If your example is using QML 2, (such as qmlscene) and the .qml file you" << endl
                   << "loaded has 'import QtQuick 1.0' or 'import Qt 4.7', this error will occur." << endl
                   << endl
                   << "To load files with 'import QtQuick 1.0' or 'import Qt 4.7', use the" << endl
                   << "QDeclarativeView class in the Qt Quick 1 module." << endl;
        delete obj;
        root = 0;
    }
    if (root) {
        initialSize = rootObjectSize();
        if ((resizeMode == QQuickWidget::SizeViewToRootObject || q->width() <= 1 || q->height() <= 1) &&
            initialSize != q->size()) {
            q->resize(initialSize);
        }
        initResize();
    }
}

GLuint QQuickWidgetPrivate::textureId() const
{
    return fbo ? fbo->texture() : 0;
}

/*!
  \internal
  If the \l {QTimerEvent} {timer event} \a e is this
  view's resize timer, sceneResized() is emitted.
 */
void QQuickWidget::timerEvent(QTimerEvent* e)
{
    Q_D(QQuickWidget);
    if (!e || e->timerId() == d->resizetimer.timerId()) {
        d->updateSize();
        d->resizetimer.stop();
    }
}

/*!
    \internal
    Preferred size follows the root object geometry.
*/
QSize QQuickWidget::sizeHint() const
{
    Q_D(const QQuickWidget);
    QSize rootObjectSize = d->rootObjectSize();
    if (rootObjectSize.isEmpty()) {
        return size();
    } else {
        return rootObjectSize;
    }
}

/*!
  Returns the initial size of the root object.

  If \l resizeMode is QQuickItem::SizeRootObjectToView the root object will be
  resized to the size of the view.  initialSize contains the size of the
  root object before it was resized.
*/
QSize QQuickWidget::initialSize() const
{
    Q_D(const QQuickWidget);
    return d->initialSize;
}

/*!
  Returns the view's root \l {QQuickItem} {item}.
 */
QQuickItem *QQuickWidget::rootObject() const
{
    Q_D(const QQuickWidget);
    return d->root;
}

/*!
  \internal
  This function handles the \l {QResizeEvent} {resize event}
  \a e.
 */
void QQuickWidget::resizeEvent(QResizeEvent *e)
{
    Q_D(QQuickWidget);
    if (d->resizeMode == SizeRootObjectToView)
        d->updateSize();

    d->createContext();
    createFramebufferObject();
    d->offscreenWindow->resizeEvent(e);
    d->offscreenWindow->setGeometry(0, 0, e->size().width(), e->size().height());

    QOpenGLContext *context = d->offscreenWindow->openglContext();
    if (!context) {
        qWarning("QQuickWidget::resizeEvent() no OpenGL context");
        return;
    }

    Q_ASSERT(window()->windowHandle()->handle());
    context->makeCurrent(window()->windowHandle());
    d->renderControl->render();
    glFlush();
    context->doneCurrent();
}

/*! \reimp */
void QQuickWidget::keyPressEvent(QKeyEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_PROFILE(addEvent<QQuickProfiler::Key>());

    d->offscreenWindow->keyPressEvent(e);
}

/*! \reimp */
void QQuickWidget::keyReleaseEvent(QKeyEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_PROFILE(addEvent<QQuickProfiler::Key>());

    d->offscreenWindow->keyReleaseEvent(e);
}

/*! \reimp */
void QQuickWidget::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_PROFILE(addEvent<QQuickProfiler::Mouse>());

    QMouseEvent mappedEvent(e->type(), e->localPos(), e->screenPos(), e->button(), e->buttons(), e->modifiers());
    d->offscreenWindow->mouseMoveEvent(&mappedEvent);
}

void QQuickWidget::showEvent(QShowEvent *e)
{
    Q_D(QQuickWidget);
    QQuickWindowPrivate::get(d->offscreenWindow)->forceRendering = true;
    d->offscreenWindow->showEvent(e);
}

void QQuickWidget::hideEvent(QHideEvent *e)
{
    Q_D(QQuickWidget);
    QQuickWindowPrivate::get(d->offscreenWindow)->forceRendering = false;
    d->offscreenWindow->hideEvent(e);
}

/*! \reimp */
void QQuickWidget::mousePressEvent(QMouseEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_PROFILE(addEvent<QQuickProfiler::Mouse>());

    QMouseEvent mappedEvent(e->type(), e->localPos(), e->screenPos(), e->button(), e->buttons(), e->modifiers());
    d->offscreenWindow->mousePressEvent(&mappedEvent);
}

/*! \reimp */
void QQuickWidget::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_PROFILE(addEvent<QQuickProfiler::Mouse>());

    QMouseEvent mappedEvent(e->type(), e->localPos(), e->screenPos(), e->button(), e->buttons(), e->modifiers());
    d->offscreenWindow->mouseReleaseEvent(&mappedEvent);
}

/*! \reimp */
bool QQuickWidget::event(QEvent *e)
{
    Q_D(QQuickWidget);
    if (e->type() == QEvent::Timer) {
        d->eventPending = false;
        killTimer(d->updateTimer);
        d->updateTimer = 0;
        if (d->updatePending)
            d->renderSceneGraph();
        return true;
    }
    return QWidget::event(e);
}


// TODO: try to separate the two cases of
// 1. render() unconditionally without sync
// 2. sync() and then render if necessary
void QQuickWidget::triggerUpdate()
{
    Q_D(QQuickWidget);
    d->updatePending = true;
     if (!d->eventPending) {
        const int exhaustDelay = 5;
        d->updateTimer = startTimer(exhaustDelay, Qt::PreciseTimer);
        d->eventPending = true;
    }
}

QT_END_NAMESPACE
