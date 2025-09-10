// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickwidget.h"
#include "qquickwidget_p.h"
#include "qaccessiblequickwidgetfactory_p.h"
#include <QtWidgets/private/qwidgetrepaintmanager_p.h>

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

#if QT_CONFIG(graphicsview)
#include <QtWidgets/qgraphicsscene.h>
#include <QtWidgets/qgraphicsview.h>
#endif

#if QT_CONFIG(vulkan)
#include <QtGui/private/qvulkandefaultinstance_p.h>
#endif

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

    ensureBackingScene();
    offscreenWindow->setScreen(q->screen());
    // Do not call create() on offscreenWindow.

    QWidget::connect(offscreenWindow, SIGNAL(sceneGraphInitialized()), q, SLOT(createFramebufferObject()));
    QWidget::connect(offscreenWindow, SIGNAL(sceneGraphInvalidated()), q, SLOT(destroyFramebufferObject()));
    QWidget::connect(offscreenWindow, &QQuickWindow::focusObjectChanged, q, &QQuickWidget::propagateFocusObjectChanged);

#if QT_CONFIG(accessibility)
    QAccessible::installFactory(&qAccessibleQuickWidgetFactory);
#endif
}

void QQuickWidgetPrivate::ensureBackingScene()
{
    // This should initialize, if not already done, the absolute minimum set of
    // mandatory backing resources, meaning the QQuickWindow and its
    // QQuickRenderControl. This function may be called very early on upon
    // construction, including before init() even.

    Q_Q(QQuickWidget);
    if (!renderControl)
        renderControl = new QQuickWidgetRenderControl(q);
    if (!offscreenWindow) {
        offscreenWindow = new QQuickWidgetOffscreenWindow(*new QQuickWidgetOffscreenWindowPrivate(), renderControl);
        offscreenWindow->setProperty("_q_parentWidget", QVariant::fromValue(q));
    }

    // Check if the Software Adaptation is being used
    auto sgRendererInterface = offscreenWindow->rendererInterface();
    if (sgRendererInterface && sgRendererInterface->graphicsApi() == QSGRendererInterface::Software)
        useSoftwareRenderer = true;
}

void QQuickWidgetPrivate::init(QQmlEngine* e)
{
    Q_Q(QQuickWidget);

    initOffscreenWindow();

    if (!useSoftwareRenderer) {
        if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RhiBasedRendering))
            setRenderToTexture();
        else
            qWarning("QQuickWidget is not supported on this platform.");
    }

    engine = e;

    if (!engine.isNull() && !engine.data()->incubationController())
        engine.data()->setIncubationController(offscreenWindow->incubationController());

    q->setMouseTracking(true);
    q->setFocusPolicy(Qt::StrongFocus);
#ifndef Q_OS_MACOS
    /*
        Usually, a QTouchEvent comes from a touchscreen, and we want those
        touch events in Qt Quick. But on macOS, there are no touchscreens, and
        WA_AcceptTouchEvents has a different meaning: QApplication::notify()
        calls the native-integration function registertouchwindow() to change
        NSView::allowedTouchTypes to include NSTouchTypeMaskIndirect when the
        trackpad cursor enters the window, and removes that mask when the
        cursor exits. In other words, WA_AcceptTouchEvents enables getting
        discrete touchpoints from the trackpad. We rather prefer to get mouse,
        wheel and native gesture events from the trackpad (because those
        provide more of a "native feel"). The only exception is for
        MultiPointTouchArea, and it takes care of that for itself. So don't
        automatically set WA_AcceptTouchEvents on macOS. The user can still do
        it, but we don't recommend it.
    */
    q->setAttribute(Qt::WA_AcceptTouchEvents);
#endif

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
    if (!useSoftwareRenderer && rhi) {
        // For the user's own OpenGL code connected to some QQuickWindow signals.
        rhi->makeThreadLocalNativeContextCurrent();
    }

    renderControl->invalidate();
}

void QQuickWidgetPrivate::handleWindowChange()
{
    Q_Q(QQuickWidget);

    if (offscreenWindow->isPersistentSceneGraph()
            && qGuiApp->testAttribute(Qt::AA_ShareOpenGLContexts)
            && rhiConfig().api() == QPlatformBackingStoreRhiConfig::OpenGL)
    {
        return;
    }

    // In case of !isPersistentSceneGraph or when we need a new context due to
    // the need to share resources with the new window's context, we must both
    // invalidate the scenegraph and destroy the context. QQuickRenderControl
    // must be recreated because its RHI will contain a dangling pointer to
    // the context.

    QScopedPointer<QQuickWindow> oldOffScreenWindow(offscreenWindow); // Do not delete before reparenting sgItem
    offscreenWindow = nullptr;
    delete renderControl;

    renderControl = new QQuickWidgetRenderControl(q);
    initOffscreenWindow();

    QObject::connect(renderControl, SIGNAL(renderRequested()), q, SLOT(triggerUpdate()));
    QObject::connect(renderControl, SIGNAL(sceneChanged()), q, SLOT(triggerUpdate()));

    if (QQuickItem *sgItem = qobject_cast<QQuickItem *>(root))
        sgItem->setParentItem(offscreenWindow->contentItem());
}

QQuickWidgetPrivate::QQuickWidgetPrivate()
    : root(nullptr)
    , component(nullptr)
    , offscreenWindow(nullptr)
    , renderControl(nullptr)
    , rhi(nullptr)
    , outputTexture(nullptr)
    , depthStencil(nullptr)
    , msaaBuffer(nullptr)
    , rt(nullptr)
    , rtRp(nullptr)
    , resizeMode(QQuickWidget::SizeViewToRootObject)
    , initialSize(0,0)
    , eventPending(false)
    , updatePending(false)
    , fakeHidden(false)
    , requestedSamples(0)
    , useSoftwareRenderer(false)
    , forceFullUpdate(false)
    , deviceLost(false)
{
}

void QQuickWidgetPrivate::destroy()
{
    Q_Q(QQuickWidget);
    invalidateRenderControl();
    q->destroyFramebufferObject();
    delete offscreenWindow;
    delete renderControl;
    offscreenRenderer.reset();
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
    Q_Q(QQuickWidget);
    if (!useSoftwareRenderer) {
        if (deviceLost) {
            deviceLost = false;
            initializeWithRhi();
            q->createFramebufferObject();
        }

        if (!rhi) {
            qWarning("QQuickWidget: Attempted to render scene with no rhi");
            return;
        }

        // createFramebufferObject() bails out when the size is empty. In this case
        // we cannot render either.
        if (!outputTexture)
            return;

        renderControl->beginFrame();
        QQuickRenderControlPrivate::FrameStatus frameStatus = QQuickRenderControlPrivate::get(renderControl)->frameStatus;
        if (frameStatus == QQuickRenderControlPrivate::DeviceLostInBeginFrame) {
            // graphics resources controlled by us must be released
            invalidateRenderControl();
            // skip this round and hope that the tlw's repaint manager will manage to reinitialize
            deviceLost = true;
            return;
        }
        if (frameStatus != QQuickRenderControlPrivate::RecordingFrame) {
            qWarning("QQuickWidget: Failed to begin recording a frame");
            return;
        }

        if (needsSync) {
            renderControl->polishItems();
            renderControl->sync();
        }

        renderControl->render();

        renderControl->endFrame();
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
    if (!useSoftwareRenderer && !rhi)
        return QImage();

    // grabWindow() does not work for the rhi case, we are in control of the
    // render target, and so it is up to us to read it back. When the software
    // renderer is in use, just call grabWindow().

    if (outputTexture) {
        render(true);
        QRhiCommandBuffer *cb = nullptr;
        rhi->beginOffscreenFrame(&cb);
        QRhiResourceUpdateBatch *resUpd = rhi->nextResourceUpdateBatch();
        QRhiReadbackResult readResult;
        resUpd->readBackTexture(QRhiReadbackDescription(outputTexture), &readResult);
        cb->resourceUpdate(resUpd);
        rhi->endOffscreenFrame();
        if (!readResult.data.isEmpty()) {
            QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                                readResult.pixelSize.width(), readResult.pixelSize.height(),
                                QImage::Format_RGBA8888_Premultiplied);
            if (rhi->isYUpInFramebuffer())
                return wrapperImage.mirrored();
            else
                return wrapperImage.copy();
        }
        return QImage();
    }

    return offscreenWindow->grabWindow();
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

    \section1 Performance Considerations

    QQuickWidget is an alternative to using QQuickView and QWidget::createWindowContainer().
    The restrictions on stacking order do not apply, making QQuickWidget the more flexible
    alternative, behaving more like an ordinary widget.

    However, the above mentioned advantages come at the expense of performance:
    \list

    \li Unlike QQuickWindow and QQuickView, QQuickWidget involves at least one
    additional render pass targeting an offscreen color buffer, typically a 2D
    texture, followed by drawing a texture quad. This means increased load
    especially for the fragment processing of the GPU.

    \li Using QQuickWidget disables the \l{threaded_render_loop}{threaded render loop} on all
    platforms. This means that some of the benefits of threaded rendering, for example
    \l Animator classes and vsync driven animations, will not be available.
    \endlist

    \note Avoid calling winId() on a QQuickWidget. This function triggers the creation of
    a native window, resulting in reduced performance and possibly rendering glitches. The
    entire purpose of QQuickWidget is to render Quick scenes without a separate native
    window, hence making it a native widget should always be avoided.

    \section1 Graphics API Support

    QQuickWidget is functional with all the 3D graphics APIs supported by Qt
    Quick, as well as the \c software backend. Other backends, for example
    OpenVG, are not compatible however and attempting to construct a
    QQuickWidget will lead to problems.

    Overriding the platform's default graphics API is done the same way as with
    QQuickWindow and QQuickView: either by calling
    QQuickWindow::setGraphicsApi() early on before constructing the first
    QQuickWidget, or by setting the \c{QSG_RHI_BACKEND} environment variable.

    \note One top-level window can only use one single graphics API for
    rendering. For example, attempting to place a QQuickWidget using Vulkan and
    a QOpenGLWidget in the widget hierarchy of the same top-level window,
    problems will occur and one of the widgets will not be rendering as
    expected.

    \section1 Scene Graph and Context Persistency

    QQuickWidget honors QQuickWindow::isPersistentSceneGraph(), meaning that
    applications can decide - by calling
    QQuickWindow::setPersistentSceneGraph() on the window returned from the
    quickWindow() function - to let scenegraph nodes and other Qt Quick scene
    related resources be released whenever the widget becomes hidden. By default
    persistency is enabled, just like with QQuickWindow.

    When running with the OpenGL, QQuickWindow offers the possibility to
    disable persistent OpenGL contexts as well. This setting is currently
    ignored by QQuickWidget and the context is always persistent. The OpenGL
    context is thus not destroyed when hiding the widget. The context is
    destroyed only when the widget is destroyed or when the widget gets
    reparented into another top-level widget's child hierarchy. However, some
    applications, in particular those that have their own graphics resources
    due to performing custom OpenGL rendering in the Qt Quick scene, may wish
    to disable the latter since they may not be prepared to handle the loss of
    the context when moving a QQuickWidget into another window. Such
    applications can set the QCoreApplication::AA_ShareOpenGLContexts
    attribute. For a discussion on the details of resource initialization and
    cleanup, refer to the QOpenGLWidget documentation.

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
  Constructs a QQuickWidget with a default QML engine as a child of \a parent.

  The default value of \a parent is \c nullptr.
*/
QQuickWidget::QQuickWidget(QWidget *parent)
    : QWidget(*(new QQuickWidgetPrivate), parent, {})
{
    d_func()->init();
}

/*!
  Constructs a QQuickWidget with a default QML engine and the given QML \a source
  as a child of \a parent.

  The default value of \a parent is \c nullptr.
 */
QQuickWidget::QQuickWidget(const QUrl &source, QWidget *parent)
    : QQuickWidget(parent)
{
    setSource(source);
}

/*!
  Constructs a QQuickWidget with the given QML \a engine as a child of \a parent.

  \note The QQuickWidget does not take ownership of the given \a engine object;
  it is the caller's responsibility to destroy the engine. If the \a engine is deleted
  before the view, \l status() will return \l QQuickWidget::Error.
*/
QQuickWidget::QQuickWidget(QQmlEngine* engine, QWidget *parent)
    : QWidget(*(new QQuickWidgetPrivate), parent, {})
{
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

    if (d->rhi)
        d->rhi->removeCleanupCallback(this);

    // NB! resetting graphics resources must be done from this destructor,
    // *not* from the private class' destructor. This is due to how destruction
    // works and due to the QWidget dtor (for toplevels) destroying the repaint
    // manager and rhi before the (QObject) private gets destroyed. Hence must
    // do it here early on.
    d->destroy();
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
                offscreenWindow->resize(newSize);
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
    QQuickWindowPrivate::rhiCreationFailureMessage(QLatin1String("QRhi"), &translatedMessage, &untranslatedMessage);

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

static inline QPlatformBackingStoreRhiConfig::Api graphicsApiToBackingStoreRhiApi(QSGRendererInterface::GraphicsApi api)
{
    switch (api) {
    case QSGRendererInterface::OpenGL:
        return QPlatformBackingStoreRhiConfig::OpenGL;
    case QSGRendererInterface::Vulkan:
        return QPlatformBackingStoreRhiConfig::Vulkan;
    case QSGRendererInterface::Direct3D11:
        return QPlatformBackingStoreRhiConfig::D3D11;
    case QSGRendererInterface::Direct3D12:
        return QPlatformBackingStoreRhiConfig::D3D12;
    case QSGRendererInterface::Metal:
        return QPlatformBackingStoreRhiConfig::Metal;
    default:
        return QPlatformBackingStoreRhiConfig::Null;
    }
}

// Never called by Software Rendering backend
void QQuickWidgetPrivate::initializeWithRhi()
{
    Q_Q(QQuickWidget);

    // when reparenting, the rhi may suddenly be different
    if (rhi) {
        QRhi *backingStoreRhi = QWidgetPrivate::rhi();
        if (backingStoreRhi && rhi != backingStoreRhi)
            rhi = nullptr;
    }

    // On hide-show we may invalidate() (when !isPersistentSceneGraph) but our
    // context is kept. We may need to initialize() again, though.
    const bool onlyNeedsSgInit = rhi && !offscreenWindow->isSceneGraphInitialized();

    if (!onlyNeedsSgInit) {
        if (rhi)
            return;

        if (QRhi *backingStoreRhi = QWidgetPrivate::rhi()) {
            rhi = backingStoreRhi;
            // We don't own the RHI, so make sure we clean up if it goes away
            rhi->addCleanupCallback(q, [this](QRhi *rhi) {
                if (this->rhi == rhi) {
                    invalidateRenderControl();
                    deviceLost = true;
                    this->rhi = nullptr;
                }
            });
        }

        if (!rhi) {
            // The widget (and its parent chain, if any) may not be shown at
            // all, yet one may still want to use it for grabs. This is
            // ridiculous of course because the rendering infrastructure is
            // tied to the top-level widget that initializes upon expose, but
            // it has to be supported.
            offscreenRenderer.setConfig(rhiConfig());
            offscreenRenderer.setFormat(q->format());
            // no window passed in, so no swapchain, but we get a functional QRhi which we own
            if (offscreenRenderer.create())
                rhi = offscreenRenderer.rhi();
        }

        // Could be that something else already initialized the window with some
        // other graphics API for the QRhi, that's not good.
        if (rhi && rhi->backend() != QBackingStoreRhiSupport::apiToRhiBackend(graphicsApiToBackingStoreRhiApi(QQuickWindow::graphicsApi()))) {
            qWarning("The top-level window is not using the expected graphics API for composition, "
                     "'%s' is not compatible with this QQuickWidget",
                     rhi->backendName());
            rhi = nullptr;
        }
    }

    if (rhi) {
        if (!offscreenWindow->isSceneGraphInitialized()) {
            offscreenWindow->setGraphicsDevice(QQuickGraphicsDevice::fromRhi(rhi));
#if QT_CONFIG(vulkan)
            if (QWindow *w = q->window()->windowHandle())
                offscreenWindow->setVulkanInstance(w->vulkanInstance());
            else if (rhi == offscreenRenderer.rhi())
                offscreenWindow->setVulkanInstance(QVulkanDefaultInstance::instance());
#endif
            renderControl->initialize();
        }
    } else {
        qWarning("QQuickWidget: Failed to get a QRhi from the top-level widget's window");
    }
}

void QQuickWidget::createFramebufferObject()
{
    Q_D(QQuickWidget);

    // Could come from Show -> initializeWithRhi -> sceneGraphInitialized in which case the size may
    // still be invalid on some platforms. Bail out. A resize will come later on.
    if (size().isEmpty())
        return;

    // Even though this is just an offscreen window we should set the position on it, as it might be
    // useful for an item to know the actual position of the scene.
    // Note: The position will be update when we get a move event (see: updatePosition()).
    const QPoint &globalPos = mapToGlobal(QPoint(0, 0));
    d->offscreenWindow->setGeometry(globalPos.x(), globalPos.y(), width(), height());
    d->offscreenWindow->contentItem()->setSize(QSizeF(width(), height()));

    if (d->useSoftwareRenderer) {
        const QSize imageSize = size() * devicePixelRatio();
        d->softwareImage = QImage(imageSize, QImage::Format_ARGB32_Premultiplied);
        d->softwareImage.setDevicePixelRatio(devicePixelRatio());
        d->forceFullUpdate = true;
        return;
    }

    if (!d->rhi) {
        qWarning("QQuickWidget: Attempted to create output texture with no QRhi");
        return;
    }

    int samples = d->requestedSamples;
    if (d->rhi->isFeatureSupported(QRhi::MultisampleRenderBuffer))
        samples = QSGRhiSupport::chooseSampleCount(samples, d->rhi);
    else
        samples = 0;

    const int minTexSize = d->rhi->resourceLimit(QRhi::TextureSizeMin);
    const int maxTexSize = d->rhi->resourceLimit(QRhi::TextureSizeMax);

    QSize fboSize = size() * devicePixelRatio();
    if (fboSize.width() > maxTexSize || fboSize.height() > maxTexSize) {
        qWarning("QQuickWidget: Requested backing texture size is %dx%d, but the maximum texture size for the 3D API implementation is %dx%d",
                 fboSize.width(), fboSize.height(),
                 maxTexSize, maxTexSize);
    }
    fboSize.setWidth(qMin(maxTexSize, qMax(minTexSize, fboSize.width())));
    fboSize.setHeight(qMin(maxTexSize, qMax(minTexSize, fboSize.height())));

    // Could be a simple hide - show, in which case the previous texture is just fine.
    if (!d->outputTexture) {
        d->outputTexture = d->rhi->newTexture(QRhiTexture::RGBA8, fboSize, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
        if (!d->outputTexture->create()) {
            qWarning("QQuickWidget: failed to create output texture of size %dx%d",
                     fboSize.width(), fboSize.height());
        }
    }
    if (!d->depthStencil) {
        d->depthStencil = d->rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, fboSize, samples);
        if (!d->depthStencil->create()) {
            qWarning("QQuickWidget: failed to create depth/stencil buffer of size %dx%d and sample count %d",
                     fboSize.width(), fboSize.height(), samples);
        }
    }
    if (samples > 1 && !d->msaaBuffer) {
        d->msaaBuffer = d->rhi->newRenderBuffer(QRhiRenderBuffer::Color, fboSize, samples);
        if (!d->msaaBuffer->create()) {
            qWarning("QQuickWidget: failed to create multisample renderbuffer of size %dx%d and sample count %d",
                     fboSize.width(), fboSize.height(), samples);
        }
    }
    if (!d->rt) {
        QRhiTextureRenderTargetDescription rtDesc;
        QRhiColorAttachment colorAtt;
        if (samples <= 1) {
            colorAtt.setTexture(d->outputTexture);
        } else {
            colorAtt.setRenderBuffer(d->msaaBuffer);
            colorAtt.setResolveTexture(d->outputTexture);
        }
        rtDesc.setColorAttachments({ colorAtt });
        rtDesc.setDepthStencilBuffer(d->depthStencil);
        d->rt = d->rhi->newTextureRenderTarget(rtDesc);
        d->rtRp = d->rt->newCompatibleRenderPassDescriptor();
        d->rt->setRenderPassDescriptor(d->rtRp);
        d->rt->create();
    }
    if (d->outputTexture->pixelSize() != fboSize) {
        d->outputTexture->setPixelSize(fboSize);
        if (!d->outputTexture->create()) {
            qWarning("QQuickWidget: failed to create resized output texture of size %dx%d",
                     fboSize.width(), fboSize.height());
        }
        d->depthStencil->setPixelSize(fboSize);
        if (!d->depthStencil->create()) {
            qWarning("QQuickWidget: failed to create resized depth/stencil buffer of size %dx%d",
                     fboSize.width(), fboSize.height());
        }
        if (d->msaaBuffer) {
            d->msaaBuffer->setPixelSize(fboSize);
            if (!d->msaaBuffer->create()) {
                qWarning("QQuickWidget: failed to create resized multisample renderbuffer of size %dx%d",
                         fboSize.width(), fboSize.height());
            }
        }
    }

    d->offscreenWindow->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(d->rt));

    d->renderControl->setSamples(samples);

    // Sanity check: The window must not have an underlying platform window.
    // Having one would mean create() was called and platforms that only support
    // a single native window were in trouble.
    Q_ASSERT(!d->offscreenWindow->handle());
}

void QQuickWidget::destroyFramebufferObject()
{
    Q_D(QQuickWidget);

    if (d->useSoftwareRenderer) {
        d->softwareImage = QImage();
        return;
    }

    delete d->rt;
    d->rt = nullptr;
    delete d->rtRp;
    d->rtRp = nullptr;
    delete d->depthStencil;
    d->depthStencil = nullptr;
    delete d->msaaBuffer;
    d->msaaBuffer = nullptr;
    delete d->outputTexture;
    d->outputTexture = nullptr;
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

QPlatformBackingStoreRhiConfig QQuickWidgetPrivate::rhiConfig() const
{
    const_cast<QQuickWidgetPrivate *>(this)->ensureBackingScene();
    if (useSoftwareRenderer)
        return {};

    QPlatformBackingStoreRhiConfig config(graphicsApiToBackingStoreRhiApi(QQuickWindow::graphicsApi()));

    QQuickWindowPrivate *wd = QQuickWindowPrivate::get(offscreenWindow);
    // This is only here to support some of the env.vars. (such as
    // QSG_RHI_DEBUG_LAYER). There is currently no way to set a
    // QQuickGraphicsConfiguration for a QQuickWidget, which means things like
    // the pipeline cache are just not available. That is something to support
    // on the widget/backingstore level since that's where the QRhi is
    // controlled in this case.
    const bool debugLayerRequested = wd->graphicsConfig.isDebugLayerEnabled();
    config.setDebugLayer(debugLayerRequested);
    return config;
}

QWidgetPrivate::TextureData QQuickWidgetPrivate::texture() const
{
    return { outputTexture, nullptr };
}

QPlatformTextureList::Flags QQuickWidgetPrivate::textureListFlags()
{
    QPlatformTextureList::Flags flags = QWidgetPrivate::textureListFlags();
    flags |= QPlatformTextureList::NeedsPremultipliedAlphaBlending;
    return flags;
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
        if (d->rhi) {
            // Bail out when receiving a resize after scenegraph invalidation. This can happen
            // during hide - resize - show sequences and also during application exit.
            if (!d->outputTexture && !d->offscreenWindow->isSceneGraphInitialized())
                return;
            if (!d->outputTexture || d->outputTexture->pixelSize() != size() * devicePixelRatio()) {
                needsSync = true;
                createFramebufferObject();
            }
        } else {
            // This will result in a scenegraphInitialized() signal which
            // is connected to createFramebufferObject().
            needsSync = true;
            d->initializeWithRhi();
        }

        if (!d->rhi) {
            qWarning("QQuickWidget::resizeEvent() no QRhi");
            return;
        }
    }

    d->render(needsSync);
}

/*! \reimp */
bool QQuickWidget::focusNextPrevChild(bool next)
{
    Q_D(QQuickWidget);

    const auto *da = QQuickWindowPrivate::get(d->offscreenWindow)->deliveryAgentPrivate();
    Q_ASSERT(da);

    auto *currentTarget = da->focusTargetItem();
    Q_ASSERT(currentTarget);

    auto *nextTarget = QQuickItemPrivate::nextPrevItemInTabFocusChain(currentTarget, next, false);
    // If no child to focus, behaves like its base class (QWidget)
    if (!nextTarget)
        return QWidget::focusNextPrevChild(next);

    // Otherwise, simulates focus event for the offscreen window (QQuickWindow)
    const Qt::Key k = next ? Qt::Key_Tab : Qt::Key_Backtab;
    QKeyEvent event(QEvent::KeyPress, k, Qt::NoModifier);
    Q_QUICK_INPUT_PROFILE(QQuickProfiler::Key, QQuickProfiler::InputKeyPress, k, Qt::NoModifier);
    QCoreApplication::sendEvent(d->offscreenWindow, &event);

    QKeyEvent releaseEvent(QEvent::KeyRelease, k, Qt::NoModifier);
    Q_QUICK_INPUT_PROFILE(QQuickProfiler::Key, QQuickProfiler::InputKeyRelease, k, Qt::NoModifier);
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
    // It's not just the timestamp but also the globalPressPosition, velocity etc.
    mappedEvent.setTimestamp(e->timestamp());
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
    pressEvent.setTimestamp(e->timestamp());
    QCoreApplication::sendEvent(d->offscreenWindow, &pressEvent);
    e->setAccepted(pressEvent.isAccepted());
    QMouseEvent mappedEvent(e->type(), e->position(), e->position(), e->globalPosition(),
                            e->button(), e->buttons(), e->modifiers(), e->source());
    mappedEvent.setTimestamp(e->timestamp());
    QCoreApplication::sendEvent(d->offscreenWindow, &mappedEvent);
}

/*! \reimp */
void QQuickWidget::showEvent(QShowEvent *)
{
    Q_D(QQuickWidget);
    bool shouldTriggerUpdate = true;

    if (!d->useSoftwareRenderer) {
        d->initializeWithRhi();

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
    mappedEvent.setTimestamp(e->timestamp());
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
    mappedEvent.setTimestamp(e->timestamp());
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

    using FocusTarget = QWindowPrivate::FocusTarget;
    const Qt::FocusReason reason = event->reason();

    switch (reason) {
    // if there has been an item focused:
    // set the first item focused, when the reason is TabFocusReason
    // set the last item focused, when the reason is BacktabFocusReason
    case Qt::TabFocusReason:
    case Qt::BacktabFocusReason: {
        const bool forward = reason == Qt::FocusReason::TabFocusReason;
        const FocusTarget target = forward ? FocusTarget::First : FocusTarget::Last;
        QQuickWindowPrivate::get(d->offscreenWindow)->setFocusToTarget(target, reason);
    } break;
    default:
        break;
    }

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
    case QEvent::TouchCancel: {
        // Touch events only have local and global positions, no need to map.
        bool res = QCoreApplication::sendEvent(d->offscreenWindow, e);
        if (e->isAccepted() && e->type() == QEvent::TouchBegin) {
            // If the TouchBegin got accepted, then make sure all points that have
            // an exclusive grabber are also accepted so that the widget code for
            // delivering touch events make this widget an implicit grabber of those
            // points.
            QPointerEvent *pointerEvent = static_cast<QPointerEvent *>(e);
            auto deliveredPoints = pointerEvent->points();
            for (auto &point : deliveredPoints) {
                if (pointerEvent->exclusiveGrabber(point) || !pointerEvent->passiveGrabbers(point).isEmpty())
                    point.setAccepted(true);
            }
        }
        return res;
    }

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

    case QEvent::WindowAboutToChangeInternal:
        if (d->rhi)
            d->rhi->removeCleanupCallback(this);
        d->invalidateRenderControl();
        d->deviceLost = true;
        d->rhi = nullptr;
        break;

    case QEvent::WindowChangeInternal:
        d->handleWindowChange();
        break;

    case QEvent::ScreenChangeInternal:
    {
        QScreen *newScreen = screen();
        if (d->offscreenWindow)
            d->offscreenWindow->setScreen(newScreen);
        break;
    }
    case QEvent::DevicePixelRatioChange:
        if (d->useSoftwareRenderer || d->outputTexture) {
            // This will check the size taking the devicePixelRatio into account
            // and recreate if needed.
            createFramebufferObject();
            d->render(true);
        }
        if (d->offscreenWindow) {
            QEvent dprChangeEvent(QEvent::DevicePixelRatioChange);
            QGuiApplication::sendEvent(d->offscreenWindow, &dprChangeEvent);
        }
        break;
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

QT_END_NAMESPACE

#include "moc_qquickwidget_p.cpp"

#include "moc_qquickwidget.cpp"
