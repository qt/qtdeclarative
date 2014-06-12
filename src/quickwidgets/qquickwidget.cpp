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
#include <QtGui/QOffscreenSurface>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

#ifdef Q_OS_WIN
#  include <QtWidgets/QMessageBox>
#  include <QtCore/QLibraryInfo>
#  include <QtCore/qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

class QQuickWidgetRenderControl : public QQuickRenderControl
{
public:
    QQuickWidgetRenderControl(QQuickWidget *quickwidget) : m_quickWidget(quickwidget) {}
    QWindow *renderWindow(QPoint *offset) {
        if (offset)
            *offset = m_quickWidget->mapTo(m_quickWidget->window(), QPoint());
        return m_quickWidget->window()->windowHandle();
    }
private:
    QQuickWidget *m_quickWidget;
};

void QQuickWidgetPrivate::init(QQmlEngine* e)
{
    Q_Q(QQuickWidget);

    renderControl = new QQuickWidgetRenderControl(q);
    offscreenWindow = renderControl->createOffscreenWindow();
    offscreenWindow->setTitle(QString::fromLatin1("Offscreen"));
    // Do not call create() on offscreenWindow.

    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RasterGLSurface))
        setRenderToTexture();
    else
        qWarning("QQuickWidget is not supported on this platform.");

    engine = e;

    if (engine.isNull())
        engine = new QQmlEngine(q);

    if (!engine.data()->incubationController())
        engine.data()->setIncubationController(offscreenWindow->incubationController());

    if (QQmlDebugService::isDebuggingEnabled())
        QQmlInspectorService::instance()->addView(q);

#ifndef QT_NO_DRAGANDDROP
    q->setAcceptDrops(true);
#endif

    QWidget::connect(offscreenWindow, SIGNAL(sceneGraphInitialized()), q, SLOT(createFramebufferObject()));
    QWidget::connect(offscreenWindow, SIGNAL(sceneGraphInvalidated()), q, SLOT(destroyFramebufferObject()));
    QObject::connect(renderControl, SIGNAL(renderRequested()), q, SLOT(triggerUpdate()));
    QObject::connect(renderControl, SIGNAL(sceneChanged()), q, SLOT(triggerUpdate()));
}

void QQuickWidgetPrivate::stopRenderControl()
{
    if (!context) // this is not an error, could be called before creating the context, or multiple times
        return;

    bool success = context->makeCurrent(offscreenSurface);
    if (!success) {
        qWarning("QQuickWidget::stopRenderControl could not make context current");
        return;
    }

    renderControl->stop();
}

void QQuickWidgetPrivate::handleWindowChange()
{
    stopRenderControl();
    destroyContext();
}

QQuickWidgetPrivate::QQuickWidgetPrivate()
    : root(0)
    , component(0)
    , offscreenWindow(0)
    , offscreenSurface(0)
    , renderControl(0)
    , fbo(0)
    , resolvedFbo(0)
    , context(0)
    , resizeMode(QQuickWidget::SizeViewToRootObject)
    , initialSize(0,0)
    , eventPending(false)
    , updatePending(false)
    , fakeHidden(false)
{
}

QQuickWidgetPrivate::~QQuickWidgetPrivate()
{
    if (QQmlDebugService::isDebuggingEnabled())
        QQmlInspectorService::instance()->removeView(q_func());

    stopRenderControl();

    // context and offscreenSurface are current at this stage, if the context was created.
    Q_ASSERT(!context || (QOpenGLContext::currentContext() == context && context->surface() == offscreenSurface));
    delete offscreenWindow;
    delete renderControl;
    delete resolvedFbo;
    delete fbo;

    destroyContext();
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

    if (!q->isVisible() || fakeHidden)
        return;

    QOpenGLContext *context = offscreenWindow->openglContext();
    if (!context) {
        qWarning("QQuickWidget: render scenegraph with no context");
        return;
    }

    Q_ASSERT(offscreenSurface);
    context->makeCurrent(offscreenSurface);
    renderControl->polishItems();
    renderControl->sync();
    renderControl->render();
    glFlush();

    if (resolvedFbo) {
        QRect rect(QPoint(0, 0), fbo->size());
        QOpenGLFramebufferObject::blitFramebuffer(resolvedFbo, rect, fbo, rect);
    }

    context->doneCurrent();
    q->update();
}

/*!
    \module QtQuickWidgets
    \title Qt Quick Widgets C++ Classes
    \ingroup modules
    \brief The C++ API provided by the Qt Quick Widgets module
    \qtvariable quickwidgets

    To link against the module, add this line to your \l qmake
    \c .pro file:

    \code
    QT += quickwidgets
    \endcode

    For more information, see the QQuickWidget class documentation.
*/

/*!
    \class QQuickWidget
    \since 5.3
    \brief The QQuickWidget class provides a widget for displaying a Qt Quick user interface.

    \inmodule QtQuickWidgets

    This is a convenience wrapper for QQuickWindow which will automatically load and display a QML
    scene when given the URL of the main source file. Alternatively, you can instantiate your own
    objects using QQmlComponent and place them in a manually set up QQuickWidget.

    Typical usage:

    \code
    QQuickWidget *view = new QQuickWidget;
    view->setSource(QUrl::fromLocalFile("myqmlfile.qml"));
    view->show();
    \endcode

    To receive errors related to loading and executing QML with QQuickWidget,
    you can connect to the statusChanged() signal and monitor for QQuickWidget::Error.
    The errors are available via QQuickWidget::errors().

    QQuickWidget also manages sizing of the view and root object. By default, the \l resizeMode
    is SizeViewToRootObject, which will load the component and resize it to the
    size of the view. Alternatively the resizeMode may be set to SizeRootObjectToView which
    will resize the view to the size of the root object.

    \note QQuickWidget is an alternative to using QQuickView and QWidget::createWindowContainer().
    The restrictions on stacking order do not apply, making QQuickWidget the more flexible
    alternative, behaving more like an ordinary widget. This comes at the expense of
    performance. Unlike QQuickWindow and QQuickView, QQuickWidget involves rendering into OpenGL
    framebuffer objects. This will naturally carry a minor performance hit.

    \note Using QQuickWidget disables the threaded render loop on all platforms. This means that
    some of the benefits of threaded rendering, for example \l Animator classes and vsync driven
    animations, will not be available.

    \sa {Exposing Attributes of C++ Types to QML}, {Qt Quick Widgets Example}, QQuickView
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
    setFocusPolicy(Qt::StrongFocus);
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
    setFocusPolicy(Qt::StrongFocus);
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
    setFocusPolicy(Qt::StrongFocus);
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

  \note Setting a source URL will result in the QML component being
  instantiated, even if the URL is unchanged from the current value.
*/

/*!
    Sets the source to the \a url, loads the QML component and instantiates it.

    Ensure that the URL provided is full and correct, in particular, use
    \l QUrl::fromLocalFile() when loading a file from the local filesystem.

    Calling this method multiple times with the same URL will result
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

    Sets the source \a url, \a component and content \a item (root of the QML object hierarchy) directly.
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
    \value Error One or more errors occurred. Call errors() to retrieve a list
           of errors.
*/

/*! \enum QQuickWidget::ResizeMode

  This enum specifies how to resize the view.

  \value SizeViewToRootObject The view resizes with the root item in the QML.
  \value SizeRootObjectToView The view will automatically resize the root item to the size of the view.
*/

/*!
    \fn void QQuickWidget::sceneGraphError(QQuickWindow::SceneGraphError error, const QString &message)

    This signal is emitted when an \a error occurred during scene graph initialization.

    Applications should connect to this signal if they wish to handle errors,
    like OpenGL context creation failures, in a custom way. When no slot is
    connected to the signal, the behavior will be different: Quick will print
    the \a message, or show a message box, and terminate the application.

    This signal will be emitted from the gui thread.

    \sa QQuickWindow::sceneGraphError()
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
    operation. When the status is not \l Error, an empty list is returned.

    \sa status
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
    \brief Determines whether the view should resize the window contents.

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

void QQuickWidgetPrivate::handleContextCreationFailure(const QSurfaceFormat &format, bool isEs)
{
    Q_Q(QQuickWidget);

    QString translatedMessage;
    QString untranslatedMessage;
    QQuickWindowPrivate::contextCreationFailureMessage(format, &translatedMessage, &untranslatedMessage, isEs);

    static const QMetaMethod errorSignal = QMetaMethod::fromSignal(&QQuickWidget::sceneGraphError);
    const bool signalConnected = q->isSignalConnected(errorSignal);
    if (signalConnected)
        emit q->sceneGraphError(QQuickWindow::ContextNotAvailable, translatedMessage);

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    if (!signalConnected && !QLibraryInfo::isDebugBuild() && !GetConsoleWindow())
        QMessageBox::critical(q, QCoreApplication::applicationName(), translatedMessage);
#endif // Q_OS_WIN && !Q_OS_WINCE && !Q_OS_WINRT
    if (!signalConnected)
        qFatal("%s", qPrintable(untranslatedMessage));
}

void QQuickWidgetPrivate::createContext()
{
    if (context)
        return;

    context = new QOpenGLContext;
    context->setFormat(offscreenWindow->requestedFormat());

    if (QOpenGLContextPrivate::globalShareContext())
        context->setShareContext(QOpenGLContextPrivate::globalShareContext());
    if (!context->create()) {
        const bool isEs = context->isOpenGLES();
        delete context;
        context = 0;
        handleContextCreationFailure(offscreenWindow->requestedFormat(), isEs);
        return;
    }

    offscreenSurface = new QOffscreenSurface;
    // Pass the context's format(), which, now that the underlying platform context is created,
    // contains a QSurfaceFormat representing the _actual_ format of the underlying
    // configuration. This is essential to get a surface that is compatible with the context.
    offscreenSurface->setFormat(context->format());
    offscreenSurface->create();

    if (context->makeCurrent(offscreenSurface))
        renderControl->initialize(context);
    else
        qWarning("QQuickWidget: failed to make window surface current");
}

void QQuickWidgetPrivate::destroyContext()
{
    delete offscreenSurface;
    offscreenSurface = 0;
    delete context;
    context = 0;
}

void QQuickWidget::createFramebufferObject()
{
    Q_D(QQuickWidget);

    QOpenGLContext *context = d->offscreenWindow->openglContext();

    if (!context) {
        qWarning("QQuickWidget: Attempted to create FBO with no context");
        return;
    }

    if (context->shareContext() != QWidgetPrivate::get(window())->shareContext()) {
        context->setShareContext(QWidgetPrivate::get(window())->shareContext());
        context->create();
    }

    context->makeCurrent(d->offscreenSurface);

    int samples = d->offscreenWindow->requestedFormat().samples();
    if (!QOpenGLExtensions(context).hasOpenGLExtension(QOpenGLExtensions::FramebufferMultisample))
        samples = 0;

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(samples);

    QSize fboSize = size() * window()->devicePixelRatio();

    delete d->fbo;
    d->fbo = new QOpenGLFramebufferObject(fboSize, format);
    d->offscreenWindow->setRenderTarget(d->fbo);

    if (samples > 0)
        d->resolvedFbo = new QOpenGLFramebufferObject(fboSize);

    // Sanity check: The window must not have an underlying platform window.
    // Having one would mean create() was called and platforms that only support
    // a single native window were in trouble.
    Q_ASSERT(!d->offscreenWindow->handle());
}

void QQuickWidget::destroyFramebufferObject()
{
    Q_D(QQuickWidget);
    delete d->fbo;
    d->fbo = 0;
    delete d->resolvedFbo;
    d->resolvedFbo = 0;
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
        bool resized = q->testAttribute(Qt::WA_Resized);
        if ((resizeMode == QQuickWidget::SizeViewToRootObject || !resized) &&
            initialSize != q->size()) {
            q->resize(initialSize);
        }
        initResize();
    }
}

GLuint QQuickWidgetPrivate::textureId() const
{
    Q_Q(const QQuickWidget);
    if (!q->isWindow() && q->internalWinId()) {
        qWarning() << "QQuickWidget cannot be used as a native child widget."
                   << "Consider setting Qt::AA_DontCreateNativeWidgetSiblings";
        return 0;
    }
    return resolvedFbo ? resolvedFbo->texture()
        : (fbo ? fbo->texture() : 0);
}

/*!
  \internal
  Handle item resize and scene updates.
 */
void QQuickWidget::timerEvent(QTimerEvent* e)
{
    Q_D(QQuickWidget);
    if (!e || e->timerId() == d->resizetimer.timerId()) {
        d->updateSize();
        d->resizetimer.stop();
    } else if (e->timerId() == d->updateTimer.timerId()) {
        d->eventPending = false;
        d->updateTimer.stop();
        if (d->updatePending)
            d->renderSceneGraph();
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

  If \l resizeMode is SizeRootObjectToView, the root object will be
  resized to the size of the view. This function returns the size of the
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

    if (e->size().isEmpty()) {
        //stop rendering
        d->fakeHidden = true;
        return;
    }
    if (d->fakeHidden) {
        //restart rendering
        d->fakeHidden = false;
        d->renderControl->sync();
    }

    d->createContext();
    createFramebufferObject();
    d->offscreenWindow->resizeEvent(e);
    d->offscreenWindow->setGeometry(0, 0, e->size().width(), e->size().height());

    QOpenGLContext *context = d->offscreenWindow->openglContext();
    if (!context) {
        qWarning("QQuickWidget::resizeEvent() no OpenGL context");
        return;
    }

    context->makeCurrent(d->offscreenSurface);
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

    // Use the constructor taking localPos and screenPos. That puts localPos into the
    // event's localPos and windowPos, and screenPos into the event's screenPos. This way
    // the windowPos in e is ignored and is replaced by localPos. This is necessary
    // because QQuickWindow thinks of itself as a top-level window always.
    QMouseEvent mappedEvent(e->type(), e->localPos(), e->screenPos(), e->button(), e->buttons(), e->modifiers());
    d->offscreenWindow->mouseMoveEvent(&mappedEvent);
}

/*! \reimp */
void QQuickWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_PROFILE(addEvent<QQuickProfiler::Mouse>());

    // As the second mouse press is suppressed in widget windows we emulate it here for QML.
    // See QTBUG-25831
    QMouseEvent pressEvent(QEvent::MouseButtonPress, e->localPos(), e->screenPos(), e->button(),
                           e->buttons(), e->modifiers());
    d->offscreenWindow->mousePressEvent(&pressEvent);
    QMouseEvent mappedEvent(e->type(), e->localPos(), e->screenPos(), e->button(), e->buttons(),
                            e->modifiers());
    d->offscreenWindow->mouseDoubleClickEvent(&mappedEvent);
}

/*! \reimp */
void QQuickWidget::showEvent(QShowEvent *)
{
    Q_D(QQuickWidget);
    d->updatePending = false;
    d->createContext();
    triggerUpdate();
}

/*! \reimp */
void QQuickWidget::hideEvent(QHideEvent *)
{
    Q_D(QQuickWidget);
    d->stopRenderControl();
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

#ifndef QT_NO_WHEELEVENT
/*! \reimp */
void QQuickWidget::wheelEvent(QWheelEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_PROFILE(addEvent<QQuickProfiler::Mouse>());

    // Wheel events only have local and global positions, no need to map.
    d->offscreenWindow->wheelEvent(e);
}
#endif


void QQuickWidget::focusInEvent(QFocusEvent * event)
{
    Q_D(QQuickWidget);
    d->offscreenWindow->focusInEvent(event);
}

void QQuickWidget::focusOutEvent(QFocusEvent * event)
{
    Q_D(QQuickWidget);
    d->offscreenWindow->focusOutEvent(event);
}

/*! \reimp */
bool QQuickWidget::event(QEvent *e)
{
    Q_D(QQuickWidget);

    switch (e->type()) {
#ifndef QT_NO_DRAGANDDROP
    case QEvent::Drop:
    case QEvent::DragMove:
    case QEvent::DragLeave:
        // Drag/drop events only have local pos, so no need to map,
        // but QQuickWindow::event() does not return true
        d->offscreenWindow->event(e);
        return e->isAccepted();
    case QEvent::DragEnter:
        // Don't reject drag events for the entire widget when one
        // item rejects the drag enter
        d->offscreenWindow->event(e);
        e->accept();
        return true;
#endif
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:
    case QEvent::TouchCancel:
        // Touch events only have local and global positions, no need to map.
        return d->offscreenWindow->event(e);
    case QEvent::WindowChangeInternal:
        d->handleWindowChange();
        break;
    default:
        break;
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
        d->updateTimer.start(exhaustDelay, Qt::PreciseTimer, this);
        d->eventPending = true;
    }
}

/*!
    Sets the surface \a format for the context and offscreen surface used
    by this widget.

    Call this function when there is a need to request a context for a
    given OpenGL version or profile. The sizes for depth, stencil and
    alpha buffers are taken care of automatically and there is no need
    to request those explicitly.

    \sa QWindow::setFormat(), QWindow::format(), format()
*/
void QQuickWidget::setFormat(const QSurfaceFormat &format)
{
    Q_D(QQuickWidget);
    QSurfaceFormat currentFormat = d->offscreenWindow->format();
    QSurfaceFormat newFormat = format;
    newFormat.setDepthBufferSize(qMax(newFormat.depthBufferSize(), currentFormat.depthBufferSize()));
    newFormat.setStencilBufferSize(qMax(newFormat.stencilBufferSize(), currentFormat.stencilBufferSize()));
    newFormat.setAlphaBufferSize(qMax(newFormat.alphaBufferSize(), currentFormat.alphaBufferSize()));
    d->offscreenWindow->setFormat(newFormat);
}

/*!
    Returns the actual surface format.

    If the widget has not yet been shown, the requested format is returned.

    \sa setFormat()
*/
QSurfaceFormat QQuickWidget::format() const
{
    Q_D(const QQuickWidget);
    return d->offscreenWindow->format();
}

QT_END_NAMESPACE
