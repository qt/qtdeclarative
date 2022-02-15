/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qquickwidget.h"
#include "qquickwidget_p.h"
#include "qaccessiblequickwidgetfactory_p.h"

#include "private/qquickwindow_p.h"
#include "private/qquickitem_p.h"
#include "private/qquickitemchangelistener_p.h"
#include "private/qquickrendercontrol_p.h"
#include "private/qsgrhisupport_p.h"

#include "private/qsgsoftwarerenderer_p.h"

#include <private/qqmldebugconnector_p.h>
#include <private/qquickprofiler_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>

#include <QtQml/qqmlengine.h>
#include <private/qqmlengine_p.h>
#include <QtCore/qbasictimer.h>
#include <QtGui/QOffscreenSurface>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

#if QT_CONFIG(opengl)
#include <private/qopenglcontext_p.h>
#include <private/qopenglextensions_p.h>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QtOpenGL/qpa/qplatformbackingstoreopenglsupport.h>
#endif
#include <QtGui/QPainter>

#include <QtQuick/QSGRendererInterface>

#ifdef Q_OS_WIN
#if QT_CONFIG(messagebox)
#  include <QtWidgets/QMessageBox>
#endif
#  include <QtCore/QLibraryInfo>
#  include <QtCore/qt_windows.h>
#endif

#include <QtQuick/qquickgraphicsdevice.h>
#include <QtQuick/qquickrendertarget.h>

#include "private/qwidget_p.h"

#include <QtWidgets/qgraphicsscene.h>
#include <QtWidgets/qgraphicsview.h>

QT_BEGIN_NAMESPACE

QQuickWidgetOffscreenWindow::QQuickWidgetOffscreenWindow(QQuickWindowPrivate &dd, QQuickRenderControl *control)
:QQuickWindow(dd, control)
{
    setTitle(QString::fromLatin1("Offscreen"));
    setObjectName(QString::fromLatin1("QQuickWidgetOffscreenWindow"));
}

// override setVisble to prevent accidental offscreen window being created
// by base class.
class QQuickWidgetOffscreenWindowPrivate: public QQuickWindowPrivate {
public:
    void setVisible(bool visible) override {
        Q_Q(QWindow);
        // this stays always invisible
        visibility = visible ? QWindow::Windowed : QWindow::Hidden;
        q->visibilityChanged(visibility); // workaround for QTBUG-49054
    }
};

class QQuickWidgetRenderControlPrivate;

class QQuickWidgetRenderControl : public QQuickRenderControl
{
    Q_DECLARE_PRIVATE(QQuickWidgetRenderControl)
public:
    QQuickWidgetRenderControl(QQuickWidget *quickwidget);
    QWindow *renderWindow(QPoint *offset) override;

};

class QQuickWidgetRenderControlPrivate : public QQuickRenderControlPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickWidgetRenderControl)
    QQuickWidgetRenderControlPrivate(QQuickWidgetRenderControl *renderControl, QQuickWidget *qqw)
        : QQuickRenderControlPrivate(renderControl)
        , m_quickWidget(qqw)
    {
    }

    bool isRenderWindow(const QWindow *w) override {
#if QT_CONFIG(graphicsview)
        QWidgetPrivate *widgetd = QWidgetPrivate::get(m_quickWidget);
        auto *proxy = (widgetd && widgetd->extra) ? widgetd->extra->proxyWidget : nullptr;
        auto *scene = proxy ? proxy->scene() : nullptr;
        if (scene) {
            for (const auto &view : scene->views()) {
                if (view->window()->windowHandle() == w)
                    return true;
            }
        }

        return m_quickWidget->window()->windowHandle() == w;
#endif
    }
    QQuickWidget *m_quickWidget;
};

QQuickWidgetRenderControl::QQuickWidgetRenderControl(QQuickWidget *quickWidget)
    : QQuickRenderControl(*(new QQuickWidgetRenderControlPrivate(this, quickWidget)), nullptr)
{
}

QWindow *QQuickWidgetRenderControl::renderWindow(QPoint *offset)
{
    Q_D(QQuickWidgetRenderControl);
    if (offset)
        *offset = d->m_quickWidget->mapTo(d->m_quickWidget->window(), QPoint());

    QWindow *result = nullptr;
#if QT_CONFIG(graphicsview)
    QWidgetPrivate *widgetd = QWidgetPrivate::get(d->m_quickWidget);
    if (widgetd->extra) {
        if (auto proxy = widgetd->extra->proxyWidget) {
            auto scene = proxy->scene();
            if (scene) {
                const auto views = scene->views();
                if (!views.isEmpty()) {
                    // Get the first QGV containing the proxy. Not ideal, but the callers
                    // of this function aren't prepared to handle more than one render window.
                    auto candidateView = views.first();
                    result = candidateView->window()->windowHandle();
                }
            }
        }
    }
#endif
    if (!result)
        result = d->m_quickWidget->window()->windowHandle();

    return result;
}

void QQuickWidgetPrivate::initOffscreenWindow()
{
    Q_Q(QQuickWidget);
    offscreenWindow = new QQuickWidgetOffscreenWindow(*new QQuickWidgetOffscreenWindowPrivate(), renderControl);
    offscreenWindow->setScreen(q->screen());
    // Do not call create() on offscreenWindow.

    QWidget::connect(offscreenWindow, SIGNAL(sceneGraphInitialized()), q, SLOT(createFramebufferObject()));
    QWidget::connect(offscreenWindow, SIGNAL(sceneGraphInvalidated()), q, SLOT(destroyFramebufferObject()));
    QWidget::connect(offscreenWindow, &QQuickWindow::focusObjectChanged, q, &QQuickWidget::propagateFocusObjectChanged);

#if QT_CONFIG(accessibility)
    QAccessible::installFactory(&qAccessibleQuickWidgetFactory);
#endif
}

void QQuickWidgetPrivate::init(QQmlEngine* e)
{
    Q_Q(QQuickWidget);

    renderControl = new QQuickWidgetRenderControl(q);
    initOffscreenWindow();

    // Check if the Software Adaptation is being used
    auto sgRendererInterface = offscreenWindow->rendererInterface();
    if (sgRendererInterface && sgRendererInterface->graphicsApi() == QSGRendererInterface::Software)
        useSoftwareRenderer = true;

    if (!useSoftwareRenderer) {
#if QT_CONFIG(opengl)
        if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RasterGLSurface))
            setRenderToTexture();
        else
#endif
            qWarning("QQuickWidget is not supported on this platform.");
    }

    if (QSGRhiSupport::instance()->rhiBackend() != QRhi::OpenGLES2)
        qWarning("QQuickWidget is only supported on OpenGL. Use QQuickWindow::setGraphicsApi() to override the default.");

    engine = e;

    if (!engine.isNull() && !engine.data()->incubationController())
        engine.data()->setIncubationController(offscreenWindow->incubationController());

#if QT_CONFIG(quick_draganddrop)
    q->setAcceptDrops(true);
#endif

    QObject::connect(renderControl, SIGNAL(renderRequested()), q, SLOT(triggerUpdate()));
    QObject::connect(renderControl, SIGNAL(sceneChanged()), q, SLOT(triggerUpdate()));
}

void QQuickWidgetPrivate::ensureEngine() const
{
    Q_Q(const QQuickWidget);
    if (!engine.isNull())
        return;

    engine = new QQmlEngine(const_cast<QQuickWidget*>(q));
    engine.data()->setIncubationController(offscreenWindow->incubationController());
}

void QQuickWidgetPrivate::invalidateRenderControl()
{
#if QT_CONFIG(opengl)
    if (!useSoftwareRenderer) {
        QOpenGLContext *current =QOpenGLContext::currentContext();
        if (!context || !current) // this is not an error, could be called before creating the context, or multiple times
            return;

        bool success = context->makeCurrent(offscreenSurface);
        if (!success) {
            qWarning("QQuickWidget::invalidateRenderControl could not make context current");
            return;
        }
    }
#endif

    renderControl->invalidate();

    // Many things can happen inside the above invalidate() call, including a
    // change of current context. Restore if needed since some code will rely
    // on the fact that this function makes and leaves the context current.
#if QT_CONFIG(opengl)
    if (!useSoftwareRenderer && context) {
        if (QOpenGLContext::currentContext() != context)
            context->makeCurrent(offscreenSurface);
    }
#endif
}

void QQuickWidgetPrivate::handleWindowChange()
{
    Q_Q(QQuickWidget);

    if (offscreenWindow->isPersistentSceneGraph() && qGuiApp->testAttribute(Qt::AA_ShareOpenGLContexts))
        return;

    // In case of !isPersistentSceneGraph or when we need a new context due to
    // the need to share resources with the new window's context, we must both
    // invalidate the scenegraph and destroy the context. QQuickRenderControl
    // must be recreated because its RHI will contain a dangling pointer to
    // the context.

    delete offscreenWindow;
    offscreenWindow = nullptr;
    delete renderControl;

    renderControl = new QQuickWidgetRenderControl(q);
    initOffscreenWindow();

    QObject::connect(renderControl, SIGNAL(renderRequested()), q, SLOT(triggerUpdate()));
    QObject::connect(renderControl, SIGNAL(sceneChanged()), q, SLOT(triggerUpdate()));

    execute();
    if (!useSoftwareRenderer)
        destroyContext();
}

QQuickWidgetPrivate::QQuickWidgetPrivate()
    : root(nullptr)
    , component(nullptr)
    , offscreenWindow(nullptr)
    , offscreenSurface(nullptr)
    , renderControl(nullptr)
#if QT_CONFIG(opengl)
    , fbo(nullptr)
    , resolvedFbo(nullptr)
    , context(nullptr)
#endif
    , resizeMode(QQuickWidget::SizeViewToRootObject)
    , initialSize(0,0)
    , eventPending(false)
    , updatePending(false)
    , fakeHidden(false)
    , requestedSamples(0)
    , useSoftwareRenderer(false)
    , forceFullUpdate(false)
{
}

QQuickWidgetPrivate::~QQuickWidgetPrivate()
{
    invalidateRenderControl();

    if (useSoftwareRenderer) {
        delete renderControl;
        delete offscreenWindow;
    } else {
#if QT_CONFIG(opengl)
        // context and offscreenSurface are current at this stage, if the context was created.
        Q_ASSERT(!context || (QOpenGLContext::currentContext() == context && context->surface() == offscreenSurface));
        delete resolvedFbo;
        delete fbo;
        delete offscreenWindow;
        delete renderControl;

        destroyContext();
#endif
    }
}

void QQuickWidgetPrivate::execute()
{
    Q_Q(QQuickWidget);
    ensureEngine();

    if (root) {
        delete root;
        root = nullptr;
    }
    if (component) {
        delete component;
        component = nullptr;
    }
    if (!source.isEmpty()) {
        component = new QQmlComponent(engine.data(), source, q);
        if (!component->isLoading()) {
            q->continueExecute();
        } else {
            QObject::connect(component, SIGNAL(statusChanged(QQmlComponent::Status)),
                             q, SLOT(continueExecute()));
        }
    }
}

void QQuickWidgetPrivate::itemGeometryChanged(QQuickItem *resizeItem, QQuickGeometryChange change,
                                              const QRectF &oldGeometry)
{
    Q_Q(QQuickWidget);
    if (resizeItem == root && resizeMode == QQuickWidget::SizeViewToRootObject) {
        // wait for both width and height to be changed
        resizetimer.start(0,q);
    }
    QQuickItemChangeListener::itemGeometryChanged(resizeItem, change, oldGeometry);
}

void QQuickWidgetPrivate::render(bool needsSync)
{
    if (!useSoftwareRenderer) {
#if QT_CONFIG(opengl)
        // createFramebufferObject() bails out when the size is empty. In this case
        // we cannot render either.
        if (!fbo)
            return;

        Q_ASSERT(context);

        bool current = context->makeCurrent(offscreenSurface);

        if (!current && !context->isValid()) {
            renderControl->invalidate();
            current = context->create() && context->makeCurrent(offscreenSurface);
            if (current) {
                offscreenWindow->setGraphicsDevice(QQuickGraphicsDevice::fromOpenGLContext(context));
                renderControl->initialize();
            }
        }

        if (!current) {
            qWarning("QQuickWidget: Cannot render due to failing makeCurrent()");
            return;
        }

        QOpenGLContextPrivate::get(context)->defaultFboRedirect = fbo->handle();

        renderControl->beginFrame();

        if (needsSync) {
            renderControl->polishItems();
            renderControl->sync();
        }

        renderControl->render();

        renderControl->endFrame();

        context->makeCurrent(offscreenSurface);
        if (resolvedFbo) {
            QRect rect(QPoint(0, 0), fbo->size());
            QOpenGLFramebufferObject::blitFramebuffer(resolvedFbo, rect, fbo, rect);
        }

        static_cast<QOpenGLExtensions *>(context->functions())->flushShared();

        QOpenGLContextPrivate::get(context)->defaultFboRedirect = 0;
#endif
    } else {
        //Software Renderer
        if (needsSync) {
            renderControl->polishItems();
            renderControl->sync();
        }
        if (!offscreenWindow)
            return;
        QQuickWindowPrivate *cd = QQuickWindowPrivate::get(offscreenWindow);
        auto softwareRenderer = static_cast<QSGSoftwareRenderer*>(cd->renderer);
        if (softwareRenderer && !softwareImage.isNull()) {
            softwareRenderer->setCurrentPaintDevice(&softwareImage);
            if (forceFullUpdate) {
                softwareRenderer->markDirty();
                forceFullUpdate = false;
            }
            renderControl->render();

            updateRegion += softwareRenderer->flushRegion();
        }
    }
}

void QQuickWidgetPrivate::renderSceneGraph()
{
    Q_Q(QQuickWidget);
    updatePending = false;

    if (!q->isVisible() || fakeHidden)
        return;

    if (!useSoftwareRenderer) {
#if QT_CONFIG(opengl)
        if (!context) {
            qWarning("QQuickWidget: Attempted to render scene with no context");
            return;
        }
#endif
        Q_ASSERT(offscreenSurface);
    }

    render(true);

#if QT_CONFIG(graphicsview)
    if (q->window()->graphicsProxyWidget())
        QWidgetPrivate::nearestGraphicsProxyWidget(q)->update();
    else
#endif
    {
        if (!useSoftwareRenderer)
            q->update(); // schedule composition
        else if (!updateRegion.isEmpty())
            q->update(updateRegion);
    }
}

QImage QQuickWidgetPrivate::grabFramebuffer()
{
    if (!useSoftwareRenderer) {
#if QT_CONFIG(opengl)
        if (!context)
            return QImage();

        context->makeCurrent(offscreenSurface);
#endif
    }


    // grabWindow() does not work for the OpenGL + render control case, so we
    // prefer the FBO's toImage() if available. When the software renderer
    // is in use, however, there will be no FBO and we fall back to grabWindow()
    // instead.
    return
#if QT_CONFIG(opengl)
            fbo != nullptr ? fbo->toImage() :
#endif
                           offscreenWindow->grabWindow();
}

// Intentionally not overriding the QQuickWindow's focusObject.
// Key events should go to our key event handlers, and then to the
// QQuickWindow, not any in-scene item.

/*!
    \module QtQuickWidgets
    \title Qt Quick Widgets C++ Classes
    \ingroup modules
    \brief The C++ API provided by the Qt Quick Widgets module.
    \qtcmakepackage QuickWidgets
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
    alternative, behaving more like an ordinary widget.

    \note However, the above mentioned advantages come at the expense of performance.
    Unlike QQuickWindow and QQuickView, QQuickWidget requires rendering into OpenGL
    framebuffer objects, which needs to be enforced by calling
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi) at startup.
    This will naturally carry a minor performance hit.

    \note Using QQuickWidget disables the threaded render loop on all platforms. This means that
    some of the benefits of threaded rendering, for example \l Animator classes and vsync driven
    animations, will not be available.

    \note Avoid calling winId() on a QQuickWidget. This function triggers the creation of
    a native window, resulting in reduced performance and possibly rendering glitches. The
    entire purpose of QQuickWidget is to render Quick scenes without a separate native
    window, hence making it a native widget should always be avoided.

    \section1 Scene Graph and Context Persistency

    QQuickWidget honors QQuickWindow::isPersistentSceneGraph(), meaning that
    applications can decide - by calling
    QQuickWindow::setPersistentSceneGraph() on the window returned from the
    quickWindow() function - to let scenegraph nodes and other Qt Quick scene
    related resources be released whenever the widget becomes hidden. By default
    persistency is enabled, just like with QQuickWindow.

    When running with the OpenGL backend of the scene graph, QQuickWindow
    offers the possibility to disable persistent OpenGL contexts as well. This
    setting is currently ignored by QQuickWidget and the context is always
    persistent. The OpenGL context is thus not destroyed when hiding the
    widget. The context is destroyed only when the widget is destroyed or when
    the widget gets reparented into another top-level widget's child hierarchy.
    However, some applications, in particular those that have their own
    graphics resources due to performing custom OpenGL rendering in the Qt
    Quick scene, may wish to disable the latter since they may not be prepared
    to handle the loss of the context when moving a QQuickWidget into another
    window. Such applications can set the
    QCoreApplication::AA_ShareOpenGLContexts attribute. For a discussion on the
    details of resource initialization and cleanup, refer to the QOpenGLWidget
    documentation.

    \note QQuickWidget offers less fine-grained control over its internal
    OpenGL context than QOpenGLWidget, and there are subtle differences, most
    notably that disabling the persistent scene graph will lead to destroying
    the context on a window change regardless of the presence of
    QCoreApplication::AA_ShareOpenGLContexts.

    \section1 Limitations

    Putting other widgets underneath and making the QQuickWidget transparent will not lead
    to the expected results: the widgets underneath will not be visible. This is because
    in practice the QQuickWidget is drawn before all other regular, non-OpenGL widgets,
    and so see-through types of solutions are not feasible. Other type of layouts, like
    having widgets on top of the QQuickWidget, will function as expected.

    When absolutely necessary, this limitation can be overcome by setting the
    Qt::WA_AlwaysStackOnTop attribute on the QQuickWidget. Be aware, however that this
    breaks stacking order. For example it will not be possible to have other widgets on
    top of the QQuickWidget, so it should only be used in situations where a
    semi-transparent QQuickWidget with other widgets visible underneath is required.

    This limitation only applies when there are other widgets underneath the QQuickWidget
    inside the same window. Making the window semi-transparent, with other applications
    and the desktop visible in the background, is done in the traditional way: Set
    Qt::WA_TranslucentBackground on the top-level window, request an alpha channel, and
    change the Qt Quick Scenegraph's clear color to Qt::transparent via setClearColor().

    \section1 Support when not using OpenGL

    In addition to OpenGL, the \c software backend of Qt Quick also supports
    QQuickWidget. Other backends, for example OpenVG, are not
    compatible however and attempting to construct a QQuickWidget will lead to
    problems.

    \section1 Tab Key Handling

    On press of the \c[TAB] key, the item inside the QQuickWidget gets focus. If
    this item can handle \c[TAB] key press, focus will change accordingly within
    the item, otherwise the next widget in the focus chain gets focus.

    \sa {Exposing Attributes of C++ Types to QML}, {Qt Quick Widgets Example}, QQuickView
*/


/*!
    \fn void QQuickWidget::statusChanged(QQuickWidget::Status status)
    This signal is emitted when the component's current \a status changes.
*/

/*!
  Constructs a QQuickWidget with the given \a parent.
  The default value of \a parent is 0.

*/
QQuickWidget::QQuickWidget(QWidget *parent)
    : QWidget(*(new QQuickWidgetPrivate), parent, {})
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
    : QQuickWidget(parent)
{
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
    : QWidget(*(new QQuickWidgetPrivate), parent, {})
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
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
    d->root = nullptr;
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
        const QList<QQmlError> errorList = d->component->errors();
        for (const QQmlError &error : errorList) {
            QMessageLogger(error.url().toString().toLatin1().constData(), error.line(), nullptr).warning()
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
    d->ensureEngine();
    return const_cast<QQmlEngine *>(d->engine.data());
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
    d->ensureEngine();
    return d->engine.data()->rootContext();
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

    This signal will be emitted from the GUI thread.

    \sa QQuickWindow::sceneGraphError()
 */

/*!
    \property QQuickWidget::status
    The component's current \l{QQuickWidget::Status} {status}.
*/

QQuickWidget::Status QQuickWidget::status() const
{
    Q_D(const QQuickWidget);
    if (!d->engine && !d->source.isEmpty())
        return QQuickWidget::Error;

    if (!d->component)
        return QQuickWidget::Null;

    if (d->component->status() == QQmlComponent::Ready && !d->root)
        return QQuickWidget::Error;

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

    if (!d->engine && !d->source.isEmpty()) {
        QQmlError error;
        error.setDescription(QLatin1String("QQuickWidget: invalid qml engine."));
        errs << error;
    }
    if (d->component && d->component->status() == QQmlComponent::Ready && !d->root) {
        QQmlError error;
        error.setDescription(QLatin1String("QQuickWidget: invalid root object."));
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
        if (newSize.isValid()) {
            if (newSize != q->size()) {
                q->resize(newSize);
                q->updateGeometry();
            } else if (offscreenWindow->size().isEmpty()) {
                // QQuickDeliveryAgentPrivate::deliverHoverEvent() ignores events that
                // occur outside of QQuickRootItem's geometry, so we need it to match root's size.
                offscreenWindow->contentItem()->setSize(newSize);
            }
        }
    } else if (resizeMode == QQuickWidget::SizeRootObjectToView) {
        const bool needToUpdateWidth = !qFuzzyCompare(q->width(), root->width());
        const bool needToUpdateHeight = !qFuzzyCompare(q->height(), root->height());

        if (needToUpdateWidth && needToUpdateHeight) {
            // Make sure that we have realistic sizing behavior by following
            // what on-screen windows would do and resize everything, not just
            // the root item. We do this because other types may be relying on
            // us to behave correctly.
            const QSizeF newSize(q->width(), q->height());
            offscreenWindow->resize(newSize.toSize());
            offscreenWindow->contentItem()->setSize(newSize);
            root->setSize(newSize);
        } else if (needToUpdateWidth) {
            const int newWidth = q->width();
            offscreenWindow->setWidth(newWidth);
            offscreenWindow->contentItem()->setWidth(newWidth);
            root->setWidth(newWidth);
        } else if (needToUpdateHeight) {
            const int newHeight = q->height();
            offscreenWindow->setHeight(newHeight);
            offscreenWindow->contentItem()->setHeight(newHeight);
            root->setHeight(newHeight);
        }
    }
}

/*!
  \internal

  Update the position of the offscreen window, so it matches the position of the QQuickWidget.
 */
void QQuickWidgetPrivate::updatePosition()
{
    Q_Q(QQuickWidget);
    if (offscreenWindow == nullptr)
        return;

    const QPoint &pos = q->mapToGlobal(QPoint(0, 0));
    if (offscreenWindow->position() != pos)
        offscreenWindow->setPosition(pos);
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

void QQuickWidgetPrivate::handleContextCreationFailure(const QSurfaceFormat &)
{
    Q_Q(QQuickWidget);

    QString translatedMessage;
    QString untranslatedMessage;
    QQuickWindowPrivate::rhiCreationFailureMessage(QLatin1String("OpenGL"), &translatedMessage, &untranslatedMessage);

    static const QMetaMethod errorSignal = QMetaMethod::fromSignal(&QQuickWidget::sceneGraphError);
    const bool signalConnected = q->isSignalConnected(errorSignal);
    if (signalConnected)
        emit q->sceneGraphError(QQuickWindow::ContextNotAvailable, translatedMessage);

#if defined(Q_OS_WIN) && QT_CONFIG(messagebox)
    if (!signalConnected && !QLibraryInfo::isDebugBuild() && !GetConsoleWindow())
        QMessageBox::critical(q, QCoreApplication::applicationName(), translatedMessage);
#endif // Q_OS_WIN
    if (!signalConnected)
        qFatal("%s", qPrintable(untranslatedMessage));
}

// Never called by Software Rendering backend
void QQuickWidgetPrivate::createContext()
{
#if QT_CONFIG(opengl)
    Q_Q(QQuickWidget);

    // On hide-show we may invalidate() (when !isPersistentSceneGraph) but our
    // context is kept. We may need to initialize() again, though.
    const bool reinit = context && !offscreenWindow->isSceneGraphInitialized();

    if (!reinit) {
        if (context)
            return;

        context = new QOpenGLContext;
        context->setFormat(offscreenWindow->requestedFormat());
        context->setScreen(q->screen());
        QOpenGLContext *shareContext = qt_gl_global_share_context();
        if (!shareContext)
            shareContext = QWidgetPrivate::get(q->window())->shareContext();
        if (shareContext) {
            context->setShareContext(shareContext);
            context->setScreen(shareContext->screen());
        }
        if (!context->create()) {
            delete context;
            context = nullptr;
            handleContextCreationFailure(offscreenWindow->requestedFormat());
            return;
        }

        offscreenSurface = new QOffscreenSurface;
        // Pass the context's format(), which, now that the underlying platform context is created,
        // contains a QSurfaceFormat representing the _actual_ format of the underlying
        // configuration. This is essential to get a surface that is compatible with the context.
        offscreenSurface->setFormat(context->format());
        offscreenSurface->setScreen(context->screen());
        offscreenSurface->create();
    }

    if (context->makeCurrent(offscreenSurface)) {
        if (!offscreenWindow->isSceneGraphInitialized()) {
            offscreenWindow->setGraphicsDevice(QQuickGraphicsDevice::fromOpenGLContext(context));
            renderControl->initialize();
        }
    } else
#endif
        qWarning("QQuickWidget: Failed to make context current");
}

// Never called by Software Rendering backend
void QQuickWidgetPrivate::destroyContext()
{
#if QT_CONFIG(opengl)
    delete context;
    context = nullptr;
#endif
    delete offscreenSurface;
    offscreenSurface = nullptr;
}

void QQuickWidget::createFramebufferObject()
{
    Q_D(QQuickWidget);

    // Could come from Show -> createContext -> sceneGraphInitialized in which case the size may
    // still be invalid on some platforms. Bail out. A resize will come later on.
    if (size().isEmpty())
        return;

    // Even though this is just an offscreen window we should set the position on it, as it might be
    // useful for an item to know the actual position of the scene.
    // Note: The position will be update when we get a move event (see: updatePosition()).
    const QPoint &globalPos = mapToGlobal(QPoint(0, 0));
    d->offscreenWindow->setGeometry(globalPos.x(), globalPos.y(), width(), height());

    if (d->useSoftwareRenderer) {
        const QSize imageSize = size() * devicePixelRatio();
        d->softwareImage = QImage(imageSize, QImage::Format_ARGB32_Premultiplied);
        d->softwareImage.setDevicePixelRatio(devicePixelRatio());
        d->forceFullUpdate = true;
        return;
    }

#if QT_CONFIG(opengl)
    if (!d->context) {
        qWarning("QQuickWidget: Attempted to create FBO with no context");
        return;
    }

    QOpenGLContext *shareWindowContext = QWidgetPrivate::get(window())->shareContext();
    if (shareWindowContext && d->context->shareContext() != shareWindowContext && !qGuiApp->testAttribute(Qt::AA_ShareOpenGLContexts)) {
        d->context->setShareContext(shareWindowContext);
        d->context->setScreen(shareWindowContext->screen());
        if (!d->context->create())
            qWarning("QQuickWidget: Failed to recreate context");
        // The screen may be different so we must recreate the offscreen surface too.
        // Unlike QOpenGLContext, QOffscreenSurface's create() does not recreate so have to destroy() first.
        d->offscreenSurface->destroy();
        d->offscreenSurface->setScreen(d->context->screen());
        d->offscreenSurface->create();
    }

    bool current = d->context->makeCurrent(d->offscreenSurface);
    if (!current) {
        qWarning("QQuickWidget: Failed to make context current when creating FBO");
        return;
    }

    int samples = d->requestedSamples;
    if (!QOpenGLExtensions(d->context).hasOpenGLExtension(QOpenGLExtensions::FramebufferMultisample))
        samples = 0;

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(samples);

    // The default framebuffer for normal windows have sRGB support on OS X which leads to the Qt Quick text item
    // utilizing sRGB blending. To get identical results with QQuickWidget we have to have our framebuffer backed
    // by an sRGB texture.
#ifdef Q_OS_MACOS
    if (d->context->hasExtension("GL_ARB_framebuffer_sRGB")
            && d->context->hasExtension("GL_EXT_texture_sRGB")
            && d->context->hasExtension("GL_EXT_texture_sRGB_decode")) {
        format.setInternalTextureFormat(GL_SRGB8_ALPHA8_EXT);
    }
#endif

    const QSize fboSize = size() * devicePixelRatio();

    // Could be a simple hide - show, in which case the previous fbo is just fine.
    if (!d->fbo || d->fbo->size() != fboSize) {
        delete d->fbo;
        d->fbo = new QOpenGLFramebufferObject(fboSize, format);
    }

    // When compositing in the backingstore, sampling the sRGB texture would perform an
    // sRGB-linear conversion which is not what we want when the final framebuffer (the window's)
    // is sRGB too. Disable the conversion.
#ifdef Q_OS_MACOS
    if (format.internalTextureFormat() == GL_SRGB8_ALPHA8_EXT) {
        QOpenGLFunctions *funcs = d->context->functions();
        funcs->glBindTexture(GL_TEXTURE_2D, d->fbo->texture());
        funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT);
    }
#endif

    GLuint textureId = d->fbo->texture();
    d->offscreenWindow->setRenderTarget( QQuickRenderTarget::fromOpenGLTexture(textureId, fboSize, samples));

    d->renderControl->setSamples(samples);

    if (samples > 0)
        d->resolvedFbo = new QOpenGLFramebufferObject(fboSize);

    // Sanity check: The window must not have an underlying platform window.
    // Having one would mean create() was called and platforms that only support
    // a single native window were in trouble.
    Q_ASSERT(!d->offscreenWindow->handle());
#endif
}

void QQuickWidget::destroyFramebufferObject()
{
    Q_D(QQuickWidget);

    if (d->useSoftwareRenderer) {
        d->softwareImage = QImage();
        return;
    }

#if QT_CONFIG(opengl)
    delete d->fbo;
    d->fbo = nullptr;
    delete d->resolvedFbo;
    d->resolvedFbo = nullptr;
#endif
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
        const QList<QQmlError> errorList = d->component->errors();
        for (const QQmlError &error : errorList) {
            QMessageLogger(error.url().toString().toLatin1().constData(), error.line(), nullptr).warning()
                    << error;
        }
        emit statusChanged(status());
        return;
    }

    QObject *obj = d->component->create();

    if (d->component->isError()) {
        const QList<QQmlError> errorList = d->component->errors();
        for (const QQmlError &error : errorList) {
            QMessageLogger(error.url().toString().toLatin1().constData(), error.line(), nullptr).warning()
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
    } else if (qobject_cast<QWindow *>(obj)) {
        qWarning() << "QQuickWidget does not support using windows as a root item." << Qt::endl
                   << Qt::endl
                   << "If you wish to create your root window from QML, consider using QQmlApplicationEngine instead." << Qt::endl;
    } else {
        qWarning() << "QQuickWidget only supports loading of root objects that derive from QQuickItem." << Qt::endl
                   << Qt::endl
                   << "Ensure your QML code is written for QtQuick 2, and uses a root that is or" << Qt::endl
                   << "inherits from QtQuick's Item (not a Timer, QtObject, etc)." << Qt::endl;
        delete obj;
        root = nullptr;
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

#if QT_CONFIG(opengl)
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

QPlatformTextureList::Flags QQuickWidgetPrivate::textureListFlags()
{
    QPlatformTextureList::Flags flags = QWidgetPrivate::textureListFlags();
    flags |= QPlatformTextureList::NeedsPremultipliedAlphaBlending;
    return flags;
}
#endif

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
  Returns the view's root \l {QQuickItem} {item}. Can be null
  when setSource() has not been called, if it was called with
  broken QtQuick code or while the QtQuick contents are being created.
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

    bool needsSync = false;
    if (d->fakeHidden) {
        //restart rendering
        d->fakeHidden = false;
        needsSync = true;
    }

    // Software Renderer
    if (d->useSoftwareRenderer) {
        needsSync = true;
        if (d->softwareImage.size() != size() * devicePixelRatio()) {
            createFramebufferObject();
        }
    } else {
#if QT_CONFIG(opengl)
        if (d->context) {
            // Bail out when receiving a resize after scenegraph invalidation. This can happen
            // during hide - resize - show sequences and also during application exit.
            if (!d->fbo && !d->offscreenWindow->isSceneGraphInitialized())
                return;
            if (!d->fbo || d->fbo->size() != size() * devicePixelRatio()) {
                needsSync = true;
                createFramebufferObject();
            }
        } else {
            // This will result in a scenegraphInitialized() signal which
            // is connected to createFramebufferObject().
            needsSync = true;
            d->createContext();
        }

        if (!d->context) {
            qWarning("QQuickWidget::resizeEvent() no OpenGL context");
            return;
        }
#endif
    }

    d->render(needsSync);
}

/*! \reimp */
bool QQuickWidget::focusNextPrevChild(bool next)
{
    Q_D(QQuickWidget);
    QKeyEvent event(QEvent::KeyPress, next ? Qt::Key_Tab : Qt::Key_Backtab, Qt::NoModifier);
    Q_QUICK_INPUT_PROFILE(QQuickProfiler::Key, QQuickProfiler::InputKeyPress, event.key(),
                          Qt::NoModifier);
    QCoreApplication::sendEvent(d->offscreenWindow, &event);

    QKeyEvent releaseEvent(QEvent::KeyRelease, next ? Qt::Key_Tab : Qt::Key_Backtab, Qt::NoModifier);
    Q_QUICK_INPUT_PROFILE(QQuickProfiler::Key, QQuickProfiler::InputKeyRelease, releaseEvent.key(),
                          Qt::NoModifier);
    QCoreApplication::sendEvent(d->offscreenWindow, &releaseEvent);
    return event.isAccepted();
}

/*! \reimp */
void QQuickWidget::keyPressEvent(QKeyEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_INPUT_PROFILE(QQuickProfiler::Key, QQuickProfiler::InputKeyPress, e->key(),
                          e->modifiers());

    QCoreApplication::sendEvent(d->offscreenWindow, e);
}

/*! \reimp */
void QQuickWidget::keyReleaseEvent(QKeyEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_INPUT_PROFILE(QQuickProfiler::Key, QQuickProfiler::InputKeyRelease, e->key(),
                          e->modifiers());

    QCoreApplication::sendEvent(d->offscreenWindow, e);
}

/*! \reimp */
void QQuickWidget::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_INPUT_PROFILE(QQuickProfiler::Mouse, QQuickProfiler::InputMouseMove, e->position().x(),
                          e->position().y());

    // Put position into the event's position and scenePosition, and globalPosition into the
    // event's globalPosition. This way the scenePosition in e is ignored and is replaced by
    // position. This is necessary because QQuickWindow thinks of itself as a
    // top-level window always.
    QMouseEvent mappedEvent(e->type(), e->position(), e->position(), e->globalPosition(),
                            e->button(), e->buttons(), e->modifiers(), e->source());
    QCoreApplication::sendEvent(d->offscreenWindow, &mappedEvent);
    e->setAccepted(mappedEvent.isAccepted());
}

/*! \reimp */
void QQuickWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_INPUT_PROFILE(QQuickProfiler::Mouse, QQuickProfiler::InputMouseDoubleClick,
                          e->button(), e->buttons());

    // As the second mouse press is suppressed in widget windows we emulate it here for QML.
    // See QTBUG-25831
    QMouseEvent pressEvent(QEvent::MouseButtonPress, e->position(), e->position(), e->globalPosition(),
                           e->button(), e->buttons(), e->modifiers(), e->source());
    QCoreApplication::sendEvent(d->offscreenWindow, &pressEvent);
    e->setAccepted(pressEvent.isAccepted());
    QMouseEvent mappedEvent(e->type(), e->position(), e->position(), e->globalPosition(),
                            e->button(), e->buttons(), e->modifiers(), e->source());
    QCoreApplication::sendEvent(d->offscreenWindow, &mappedEvent);
}

/*! \reimp */
void QQuickWidget::showEvent(QShowEvent *)
{
    Q_D(QQuickWidget);
    bool shouldTriggerUpdate = true;

    if (!d->useSoftwareRenderer) {
        d->createContext();

        if (d->offscreenWindow->isSceneGraphInitialized()) {
            shouldTriggerUpdate = false;
            d->render(true);
            // render() may have led to a QQuickWindow::update() call (for
            // example, having a scene with a QQuickFramebufferObject::Renderer
            // calling update() in its render()) which in turn results in
            // renderRequested in the rendercontrol, ending up in
            // triggerUpdate. In this case just calling update() is not
            // acceptable, we need the full renderSceneGraph issued from
            // timerEvent().
            if (!d->eventPending && d->updatePending) {
                d->updatePending = false;
                update();
            }
        }
    }

    if (shouldTriggerUpdate)
        triggerUpdate();

    // note offscreenWindow  is "QQuickWidgetOffscreenWindow" instance
    d->offscreenWindow->setVisible(true);
    if (QQmlInspectorService *service = QQmlDebugConnector::service<QQmlInspectorService>())
        service->setParentWindow(d->offscreenWindow, window()->windowHandle());
}

/*! \reimp */
void QQuickWidget::hideEvent(QHideEvent *)
{
    Q_D(QQuickWidget);
    if (!d->offscreenWindow->isPersistentSceneGraph())
        d->invalidateRenderControl();
    // note offscreenWindow  is "QQuickWidgetOffscreenWindow" instance
    d->offscreenWindow->setVisible(false);
    if (QQmlInspectorService *service = QQmlDebugConnector::service<QQmlInspectorService>())
        service->setParentWindow(d->offscreenWindow, d->offscreenWindow);
}

/*! \reimp */
void QQuickWidget::mousePressEvent(QMouseEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_INPUT_PROFILE(QQuickProfiler::Mouse, QQuickProfiler::InputMousePress, e->button(),
                          e->buttons());

    QMouseEvent mappedEvent(e->type(), e->position(), e->position(), e->globalPosition(),
                            e->button(), e->buttons(), e->modifiers(), e->source());
    QCoreApplication::sendEvent(d->offscreenWindow, &mappedEvent);
    e->setAccepted(mappedEvent.isAccepted());
}

/*! \reimp */
void QQuickWidget::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_INPUT_PROFILE(QQuickProfiler::Mouse, QQuickProfiler::InputMouseRelease, e->button(),
                          e->buttons());

    QMouseEvent mappedEvent(e->type(), e->position(), e->position(), e->globalPosition(),
                            e->button(), e->buttons(), e->modifiers(), e->source());
    QCoreApplication::sendEvent(d->offscreenWindow, &mappedEvent);
    e->setAccepted(mappedEvent.isAccepted());
}

#if QT_CONFIG(wheelevent)
/*! \reimp */
void QQuickWidget::wheelEvent(QWheelEvent *e)
{
    Q_D(QQuickWidget);
    Q_QUICK_INPUT_PROFILE(QQuickProfiler::Mouse, QQuickProfiler::InputMouseWheel,
                          e->angleDelta().x(), e->angleDelta().y());

    // Wheel events only have local and global positions, no need to map.
    QCoreApplication::sendEvent(d->offscreenWindow, e);
}
#endif

/*!
   \reimp
*/
void QQuickWidget::focusInEvent(QFocusEvent * event)
{
    Q_D(QQuickWidget);
    d->offscreenWindow->focusInEvent(event);
}

/*!
   \reimp
*/
void QQuickWidget::focusOutEvent(QFocusEvent * event)
{
    Q_D(QQuickWidget);
    d->offscreenWindow->focusOutEvent(event);
}

static Qt::WindowState resolveWindowState(Qt::WindowStates states)
{
    // No more than one of these 3 can be set
    if (states & Qt::WindowMinimized)
        return Qt::WindowMinimized;
    if (states & Qt::WindowMaximized)
        return Qt::WindowMaximized;
    if (states & Qt::WindowFullScreen)
        return Qt::WindowFullScreen;

    // No state means "windowed" - we ignore Qt::WindowActive
    return Qt::WindowNoState;
}

static void remapInputMethodQueryEvent(QObject *object, QInputMethodQueryEvent *e)
{
    auto item = qobject_cast<QQuickItem *>(object);
    if (!item)
        return;

    // Remap all QRectF values.
    for (auto query : {Qt::ImCursorRectangle, Qt::ImAnchorRectangle, Qt::ImInputItemClipRectangle}) {
        if (e->queries() & query) {
            auto value = e->value(query);
            if (value.canConvert<QRectF>())
                e->setValue(query, item->mapRectToScene(value.toRectF()));
        }
    }
    // Remap all QPointF values.
    if (e->queries() & Qt::ImCursorPosition) {
        auto value = e->value(Qt::ImCursorPosition);
        if (value.canConvert<QPointF>())
            e->setValue(Qt::ImCursorPosition, item->mapToScene(value.toPointF()));
    }
}

/*! \reimp */
bool QQuickWidget::event(QEvent *e)
{
    Q_D(QQuickWidget);

    switch (e->type()) {

    case QEvent::Leave:
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:
    case QEvent::TouchCancel:
        // Touch events only have local and global positions, no need to map.
        return QCoreApplication::sendEvent(d->offscreenWindow, e);

    case QEvent::FocusAboutToChange:
        return QCoreApplication::sendEvent(d->offscreenWindow, e);

    case QEvent::InputMethod:
        return QCoreApplication::sendEvent(d->offscreenWindow->focusObject(), e);
    case QEvent::InputMethodQuery:
        {
            bool eventResult = QCoreApplication::sendEvent(d->offscreenWindow->focusObject(), e);
            // The result in focusObject are based on offscreenWindow. But
            // the inputMethodTransform won't get updated because the focus
            // is on QQuickWidget. We need to remap the value based on the
            // widget.
            remapInputMethodQueryEvent(d->offscreenWindow->focusObject(), static_cast<QInputMethodQueryEvent *>(e));
            return eventResult;
        }

    case QEvent::WindowChangeInternal:
        d->handleWindowChange();
        break;

    case QEvent::ScreenChangeInternal: {
        QScreen *newScreen = screen();
        if (d->offscreenWindow)
            d->offscreenWindow->setScreen(newScreen);
        if (d->offscreenSurface)
            d->offscreenSurface->setScreen(newScreen);
#if QT_CONFIG(opengl)
        if (d->context)
            d->context->setScreen(newScreen);
#endif

        if (d->useSoftwareRenderer
#if QT_CONFIG(opengl)
            || d->fbo
#endif
           ) {
            // This will check the size taking the devicePixelRatio into account
            // and recreate if needed.
            createFramebufferObject();
            d->render(true);
        }
        break;
    }
    case QEvent::Show:
    case QEvent::Move:
        d->updatePosition();
        break;

    case QEvent::WindowStateChange:
        d->offscreenWindow->setWindowState(resolveWindowState(windowState()));
        break;

    case QEvent::ShortcutOverride:
        return QCoreApplication::sendEvent(d->offscreenWindow, e);

    case QEvent::Enter: {
        QEnterEvent *enterEvent = static_cast<QEnterEvent *>(e);
        QEnterEvent mappedEvent(enterEvent->position(), enterEvent->scenePosition(),
                                enterEvent->globalPosition());
        const bool ret = QCoreApplication::sendEvent(d->offscreenWindow, &mappedEvent);
        e->setAccepted(mappedEvent.isAccepted());
        return ret;
    }
    default:
        break;
    }

    return QWidget::event(e);
}

#if QT_CONFIG(quick_draganddrop)

/*! \reimp */
void QQuickWidget::dragEnterEvent(QDragEnterEvent *e)
{
    Q_D(QQuickWidget);
    // Don't reject drag events for the entire widget when one
    // item rejects the drag enter
    d->offscreenWindow->event(e);
    e->accept();
}

/*! \reimp */
void QQuickWidget::dragMoveEvent(QDragMoveEvent *e)
{
    Q_D(QQuickWidget);
    // Drag/drop events only have local pos, so no need to map,
    // but QQuickWindow::event() does not return true
    d->offscreenWindow->event(e);
}

/*! \reimp */
void QQuickWidget::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_D(QQuickWidget);
    d->offscreenWindow->event(e);
}

/*! \reimp */
void QQuickWidget::dropEvent(QDropEvent *e)
{
    Q_D(QQuickWidget);
    d->offscreenWindow->event(e);
}

#endif // quick_draganddrop

// TODO: try to separate the two cases of
// 1. render() unconditionally without sync
// 2. sync() and then render if necessary
void QQuickWidget::triggerUpdate()
{
    Q_D(QQuickWidget);
    d->updatePending = true;
     if (!d->eventPending) {
        // There's no sense in immediately kicking a render off now, as
        // there may be a number of triggerUpdate calls to come from a multitude
        // of different sources (network, touch/mouse/keyboard, timers,
        // animations, ...), and we want to batch them all into single frames as
        // much as possible for the sake of interactivity and responsiveness.
        //
        // To achieve this, we set a timer and only perform the rendering when
        // this is complete.
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

    // Do not include the sample count. Requesting a multisampled context is not necessary
    // since we render into an FBO, never to an actual surface. What's more, attempting to
    // create a pbuffer with a multisampled config crashes certain implementations. Just
    // avoid the entire hassle, the result is the same.
    d->requestedSamples = newFormat.samples();
    newFormat.setSamples(0);

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

/*!
  Renders a frame and reads it back into an image.

  \note This is a potentially expensive operation.
 */
QImage QQuickWidget::grabFramebuffer() const
{
    return const_cast<QQuickWidgetPrivate *>(d_func())->grabFramebuffer();
}

/*!
  Sets the clear \a color. By default this is an opaque color.

  To get a semi-transparent QQuickWidget, call this function with
  \a color set to Qt::transparent, set the Qt::WA_TranslucentBackground
  widget attribute on the top-level window, and request an alpha
  channel via setFormat().

  \sa QQuickWindow::setColor()
 */
void QQuickWidget::setClearColor(const QColor &color)
{
    Q_D(QQuickWidget);
    d->offscreenWindow->setColor(color);
}

/*!
    \since 5.5

    Returns the offscreen QQuickWindow which is used by this widget to drive
    the Qt Quick rendering. This is useful if you want to use QQuickWindow
    APIs that are not currently exposed by QQuickWidget, for instance
    connecting to the QQuickWindow::beforeRendering() signal in order
    to draw native OpenGL content below Qt Quick's own rendering.

    \warning Use the return value of this function with caution. In
    particular, do not ever attempt to show the QQuickWindow, and be
    very careful when using other QWindow-only APIs.

    \warning The offscreen window may be deleted (and recreated) during
    the life time of the QQuickWidget, particularly when the widget is
    moved to another QQuickWindow. If you need to know when the window
    has been replaced, connect to its destroyed() signal.
*/
QQuickWindow *QQuickWidget::quickWindow() const
{
    Q_D(const QQuickWidget);
    return d->offscreenWindow;
}

/*!
  \reimp
 */
void QQuickWidget::paintEvent(QPaintEvent *event)
{
    Q_D(QQuickWidget);
    if (d->useSoftwareRenderer) {
        QPainter painter(this);
        d->updateRegion = d->updateRegion.united(event->region());
        if (d->updateRegion.isNull()) {
            //Paint everything
            painter.drawImage(rect(), d->softwareImage);
        } else {
            QTransform transform;
            transform.scale(devicePixelRatio(), devicePixelRatio());
            //Paint only the updated areas
            QRegion targetRegion;
            d->updateRegion.swap(targetRegion);
            for (auto targetRect : targetRegion) {
                auto sourceRect = transform.mapRect(QRectF(targetRect));
                painter.drawImage(targetRect, d->softwareImage, sourceRect);
            }
        }
    }
}

void QQuickWidget::propagateFocusObjectChanged(QObject *focusObject)
{
    Q_D(QQuickWidget);
    if (QApplication::focusObject() != this)
        return;
    if (QWindow *window = d->windowHandle(QWidgetPrivate::WindowHandleMode::TopLevel))
        emit window->focusObjectChanged(focusObject);
}

#if QT_CONFIG(opengl)
Q_CONSTRUCTOR_FUNCTION(qt_registerDefaultPlatformBackingStoreOpenGLSupport);
#endif

QT_END_NAMESPACE

#include "moc_qquickwidget.cpp"
