// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickwindow.h"
#include "qquickwindow_p.h"

#include "qquickitem.h"
#include "qquickitem_p.h"
#include "qquickevents_p_p.h"
#include "qquickgraphicsdevice_p.h"
#include "qquickwindowcontainer_p.h"
#include "qquicksafearea_p.h"

#include <QtQuick/private/qsgrenderer_p.h>
#include <QtQuick/private/qsgplaintexture_p.h>
#include <QtQuick/private/qquickpointerhandler_p.h>
#include <QtQuick/private/qquickpointerhandler_p_p.h>
#include <private/qsgrenderloop_p.h>
#include <private/qsgrhisupport_p.h>
#include <private/qquickrendercontrol_p.h>
#include <private/qquickanimatorcontroller_p.h>
#include <private/qquickprofiler_p.h>
#include <private/qquicktextinterface_p.h>

#include <private/qguiapplication_p.h>

#include <private/qabstractanimation_p.h>

#include <QtGui/qpainter.h>
#include <QtGui/qevent.h>
#include <QtGui/qmatrix4x4.h>
#include <QtGui/private/qevent_p.h>
#include <QtGui/private/qpointingdevice_p.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qabstractanimation.h>
#include <QtCore/QLibraryInfo>
#include <QtCore/QRunnable>
#include <QtQml/qqmlincubator.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/private/qqmlmetatype_p.h>

#include <QtQuick/private/qquickpixmap_p.h>

#include <private/qqmldebugserviceinterfaces_p.h>
#include <private/qqmldebugconnector_p.h>
#include <private/qsgdefaultrendercontext_p.h>
#include <private/qsgsoftwarerenderer_p.h>
#if QT_CONFIG(opengl)
#include <private/qopengl_p.h>
#include <QOpenGLContext>
#endif
#ifndef QT_NO_DEBUG_STREAM
#include <private/qdebug_p.h>
#endif
#include <QtCore/qpointer.h>

#include <rhi/qrhi.h>

#include <utility>
#include <mutex>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcDirty, "qt.quick.dirty")
Q_LOGGING_CATEGORY(lcQuickWindow, "qt.quick.window")

bool QQuickWindowPrivate::defaultAlphaBuffer = false;

#if defined(QT_QUICK_DEFAULT_TEXT_RENDER_TYPE)
QQuickWindow::TextRenderType QQuickWindowPrivate::textRenderType = QQuickWindow::QT_QUICK_DEFAULT_TEXT_RENDER_TYPE;
#else
QQuickWindow::TextRenderType QQuickWindowPrivate::textRenderType = QQuickWindow::QtTextRendering;
#endif

class QQuickWindowIncubationController : public QObject, public QQmlIncubationController
{
    Q_OBJECT

public:
    QQuickWindowIncubationController(QSGRenderLoop *loop)
        : m_renderLoop(loop), m_timer(0)
    {
        // Allow incubation for 1/3 of a frame.
        m_incubation_time = qMax(1, int(1000 / QGuiApplication::primaryScreen()->refreshRate()) / 3);

        QAnimationDriver *animationDriver = m_renderLoop->animationDriver();
        if (animationDriver) {
            connect(animationDriver, &QAnimationDriver::stopped, this, &QQuickWindowIncubationController::animationStopped);
            connect(m_renderLoop, &QSGRenderLoop::timeToIncubate, this, &QQuickWindowIncubationController::incubate);
        }
    }

protected:
    void timerEvent(QTimerEvent *) override
    {
        killTimer(m_timer);
        m_timer = 0;
        incubate();
    }

    void incubateAgain() {
        if (m_timer == 0) {
            // Wait for a while before processing the next batch. Using a
            // timer to avoid starvation of system events.
            m_timer = startTimer(m_incubation_time);
        }
    }

public slots:
    void incubate() {
        if (m_renderLoop && incubatingObjectCount()) {
            if (m_renderLoop->interleaveIncubation()) {
                incubateFor(m_incubation_time);
            } else {
                incubateFor(m_incubation_time * 2);
                if (incubatingObjectCount())
                    incubateAgain();
            }
        }
    }

    void animationStopped() { incubate(); }

protected:
    void incubatingObjectCountChanged(int count) override
    {
        if (count && m_renderLoop && !m_renderLoop->interleaveIncubation())
            incubateAgain();
    }

private:
    QPointer<QSGRenderLoop> m_renderLoop;
    int m_incubation_time;
    int m_timer;
};

#if QT_CONFIG(accessibility)
/*!
    Returns an accessibility interface for this window, or 0 if such an
    interface cannot be created.
*/
QAccessibleInterface *QQuickWindow::accessibleRoot() const
{
    return QAccessible::queryAccessibleInterface(const_cast<QQuickWindow*>(this));
}
#endif


/*
Focus behavior
==============

Prior to being added to a valid window items can set and clear focus with no
effect.  Only once items are added to a window (by way of having a parent set that
already belongs to a window) do the focus rules apply.  Focus goes back to
having no effect if an item is removed from a window.

When an item is moved into a new focus scope (either being added to a window
for the first time, or having its parent changed), if the focus scope already has
a scope focused item that takes precedence over the item being added.  Otherwise,
the focus of the added tree is used.  In the case of a tree of items being
added to a window for the first time, which may have a conflicted focus state (two
or more items in one scope having focus set), the same rule is applied item by item -
thus the first item that has focus will get it (assuming the scope doesn't already
have a scope focused item), and the other items will have their focus cleared.
*/

QQuickRootItem::QQuickRootItem()
{
    // child items with ItemObservesViewport can treat the window's content item
    // as the ultimate viewport: avoid populating SG nodes that fall outside
    setFlag(ItemIsViewport);
}

/*! \reimp */
void QQuickWindow::exposeEvent(QExposeEvent *)
{
    Q_D(QQuickWindow);
    if (d->windowManager)
        d->windowManager->exposureChanged(this);
}

/*! \reimp */
void QQuickWindow::resizeEvent(QResizeEvent *ev)
{
    Q_D(QQuickWindow);
    if (d->contentItem)
        d->contentItem->setSize(ev->size());
    if (d->windowManager)
        d->windowManager->resize(this);
}

/*! \reimp */
void QQuickWindow::showEvent(QShowEvent *)
{
    Q_D(QQuickWindow);
    if (d->windowManager)
        d->windowManager->show(this);
}

/*! \reimp */
void QQuickWindow::hideEvent(QHideEvent *)
{
    Q_D(QQuickWindow);
    if (auto da = d->deliveryAgentPrivate())
        da->handleWindowHidden(this);
    if (d->windowManager)
        d->windowManager->hide(this);
}

/*! \reimp */
void QQuickWindow::closeEvent(QCloseEvent *e)
{
    QQuickCloseEvent qev;
    qev.setAccepted(e->isAccepted());
    emit closing(&qev);
    e->setAccepted(qev.isAccepted());
}

/*! \reimp */
void QQuickWindow::focusOutEvent(QFocusEvent *ev)
{
    Q_D(QQuickWindow);
    if (d->contentItem)
        d->contentItem->setFocus(false, ev->reason());
}

/*! \reimp */
void QQuickWindow::focusInEvent(QFocusEvent *ev)
{
    Q_D(QQuickWindow);
    if (d->inDestructor)
        return;
    if (d->contentItem)
        d->contentItem->setFocus(true, ev->reason());
    if (auto da = d->deliveryAgentPrivate())
        da->updateFocusItemTransform();
}

#if QT_CONFIG(im)
static bool transformDirtyOnItemOrAncestor(const QQuickItem *item)
{
    while (item) {
        if (QQuickItemPrivate::get(item)->dirtyAttributes & (
            QQuickItemPrivate::TransformOrigin |
            QQuickItemPrivate::Transform |
            QQuickItemPrivate::BasicTransform |
            QQuickItemPrivate::Position |
            QQuickItemPrivate::Size |
            QQuickItemPrivate::ParentChanged |
            QQuickItemPrivate::Clip)) {
            return true;
        }
        item = item->parentItem();
    }
    return false;
}
#endif

/*!
 *  \internal

    A "polish loop" can occur inside QQuickWindowPrivate::polishItems(). It is when an item calls
    polish() on an(other?) item from updatePolish(). If this anomaly happens repeatedly and without
    interruption (of a well-behaved updatePolish() that doesn't call polish()), it is a strong
    indication that we are heading towards an infinite polish loop. A polish loop is not a bug in
    Qt Quick - it is a bug caused by ill-behaved items put in the scene.

    We can detect this sequence of polish loops easily, since the
    QQuickWindowPrivate::itemsToPolish is basically a stack: polish() will push to it, and
    polishItems() will pop from it.
    Therefore if updatePolish() calls polish(), the immediate next item polishItems() processes is
    the item that was polished by the previous call to updatePolish().
    We therefore just need to count the number of polish loops we detected in _sequence_.
*/
struct PolishLoopDetector
{
    PolishLoopDetector(const QVector<QQuickItem*> &itemsToPolish)
        : itemsToPolish(itemsToPolish)
    {
    }

    /*
     * returns true when it detected a likely infinite loop
     * (suggests it should abort the polish loop)
     **/
    bool check(QQuickItem *item, int itemsRemainingBeforeUpdatePolish)
    {
        if (itemsToPolish.size() > itemsRemainingBeforeUpdatePolish) {
            // Detected potential polish loop.
            ++numPolishLoopsInSequence;
            if (numPolishLoopsInSequence == 10000) {
                // We have looped 10,000 times without actually reducing the list of items to
                // polish, give up for now.
                // This is not a fix, just a remedy so that the application can be somewhat
                // responsive.
                numPolishLoopsInSequence = 0;
                return true;
            }
            if (numPolishLoopsInSequence >= 1000 && numPolishLoopsInSequence < 1005) {
                // Start to warn about polish loop after 1000 consecutive polish loops
                // Show the 5 next items involved in the polish loop.
                // (most likely they will be the same 5 items...)
                QQuickItem *guiltyItem = itemsToPolish.last();
                qmlWarning(item) << "possible QQuickItem::polish() loop";

                auto typeAndObjectName = [](QQuickItem *item) {
                    QString typeName = QQmlMetaType::prettyTypeName(item);
                    QString objName = item->objectName();
                    if (!objName.isNull())
                        return QLatin1String("%1(%2)").arg(typeName, objName);
                    return typeName;
                };

                qmlWarning(guiltyItem) << typeAndObjectName(guiltyItem)
                            << " called polish() inside updatePolish() of " << typeAndObjectName(item);
            }
        } else {
            numPolishLoopsInSequence = 0;
        }
        return false;
    }
    const QVector<QQuickItem*> &itemsToPolish;      // Just a ref to the one in polishItems()
    int numPolishLoopsInSequence = 0;
};

void QQuickWindowPrivate::polishItems()
{
    // An item can trigger polish on another item, or itself for that matter,
    // during its updatePolish() call. Because of this, we cannot simply
    // iterate through the set, we must continue pulling items out until it
    // is empty.
    // In the case where polish is called from updatePolish() either directly
    // or indirectly, we use a PolishLoopDetector to determine if a warning should
    // be printed to the user.

    PolishLoopDetector polishLoopDetector(itemsToPolish);
    while (!itemsToPolish.isEmpty()) {
        QQuickItem *item = itemsToPolish.takeLast();
        QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
        itemPrivate->polishScheduled = false;
        const int itemsRemaining = itemsToPolish.size();
        itemPrivate->updatePolish();
        item->updatePolish();
        if (polishLoopDetector.check(item, itemsRemaining) == true)
            break;
    }

#if QT_CONFIG(im)
    if (QQuickItem *focusItem = q_func()->activeFocusItem()) {
        // If the current focus item, or any of its anchestors, has changed location
        // inside the window, we need inform IM about it. This to ensure that overlays
        // such as selection handles will be updated.
        const bool isActiveFocusItem = (focusItem == QGuiApplication::focusObject());
        const bool hasImEnabled = focusItem->inputMethodQuery(Qt::ImEnabled).toBool();
        if (isActiveFocusItem && hasImEnabled && transformDirtyOnItemOrAncestor(focusItem))
            deliveryAgentPrivate()->updateFocusItemTransform();
    }
#endif

    if (needsChildWindowStackingOrderUpdate) {
        updateChildWindowStackingOrder();
        needsChildWindowStackingOrderUpdate = false;
    }
}

/*!
 * Schedules the window to render another frame.
 *
 * Calling QQuickWindow::update() differs from QQuickItem::update() in that
 * it always triggers a repaint, regardless of changes in the underlying
 * scene graph or not.
 */
void QQuickWindow::update()
{
    Q_D(QQuickWindow);
    if (d->windowManager)
        d->windowManager->update(this);
    else if (d->renderControl)
        QQuickRenderControlPrivate::get(d->renderControl)->update();
}

static void updatePixelRatioHelper(QQuickItem *item, float pixelRatio)
{
    if (item->flags() & QQuickItem::ItemHasContents) {
        QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
        itemPrivate->itemChange(QQuickItem::ItemDevicePixelRatioHasChanged, pixelRatio);
    }

    QList <QQuickItem *> items = item->childItems();
    for (int i = 0; i < items.size(); ++i)
        updatePixelRatioHelper(items.at(i), pixelRatio);
}

void QQuickWindow::physicalDpiChanged()
{
    Q_D(QQuickWindow);
    const qreal newPixelRatio = effectiveDevicePixelRatio();
    if (qFuzzyCompare(newPixelRatio, d->lastReportedItemDevicePixelRatio))
        return;
    d->lastReportedItemDevicePixelRatio = newPixelRatio;
    if (d->contentItem)
        updatePixelRatioHelper(d->contentItem, newPixelRatio);
    d->forcePolish();
}

void QQuickWindow::handleFontDatabaseChanged()
{
    Q_D(QQuickWindow);
    d->pendingFontUpdate = true;
}

void forcePolishHelper(QQuickItem *item)
{
    if (item->flags() & QQuickItem::ItemHasContents) {
        item->polish();
    }

    QList <QQuickItem *> items = item->childItems();
    for (int i=0; i<items.size(); ++i)
        forcePolishHelper(items.at(i));
}

void QQuickWindow::handleScreenChanged(QScreen *screen)
{
    Q_D(QQuickWindow);
    Q_UNUSED(screen);
    d->forcePolish();
}

/*!
    Schedules polish events on all items in the scene.
*/
void QQuickWindowPrivate::forcePolish()
{
    Q_Q(QQuickWindow);
    if (!q->screen())
        return;
    forcePolishHelper(contentItem);
}

void forceUpdate(QQuickItem *item)
{
    if (item->flags() & QQuickItem::ItemHasContents)
        item->update();
    QQuickItemPrivate::get(item)->dirty(QQuickItemPrivate::ChildrenUpdateMask);

    QList <QQuickItem *> items = item->childItems();
    for (int i=0; i<items.size(); ++i)
        forceUpdate(items.at(i));
}

void QQuickWindowRenderTarget::reset(QRhi *rhi, ResetFlags flags)
{
    if (rhi) {
        if (rt.owns)
            delete rt.renderTarget;

        delete res.texture;
        delete res.renderBuffer;
        delete res.rpDesc;
    }

    rt = {};
    res = {};

    if (!flags.testFlag(ResetFlag::KeepImplicitBuffers))
        implicitBuffers.reset(rhi);

    if (sw.owns)
        delete sw.paintDevice;

    sw = {};
}

void QQuickWindowRenderTarget::ImplicitBuffers::reset(QRhi *rhi)
{
    if (rhi) {
        delete depthStencil;
        delete depthStencilTexture;
        delete multisampleTexture;
    }
    *this = {};
}

void QQuickWindowPrivate::invalidateFontData(QQuickItem *item)
{
    QQuickTextInterface *textItem = qobject_cast<QQuickTextInterface *>(item);
    if (textItem != nullptr)
        textItem->invalidate();

    QList<QQuickItem *> children = item->childItems();
    for (QQuickItem *child : children)
        invalidateFontData(child);
}

void QQuickWindowPrivate::ensureCustomRenderTarget()
{
    // resolve() can be expensive when importing an existing native texture, so
    // it is important to only do it when the QQuickRenderTarget was really changed.
    if (!redirect.renderTargetDirty)
        return;

    redirect.renderTargetDirty = false;

    redirect.rt.reset(rhi, QQuickWindowRenderTarget::ResetFlag::KeepImplicitBuffers);

    if (!QQuickRenderTargetPrivate::get(&customRenderTarget)->resolve(rhi, &redirect.rt)) {
        qWarning("Failed to set up render target redirection for QQuickWindow");
        redirect.rt.reset(rhi);
    }
}

void QQuickWindowPrivate::setCustomCommandBuffer(QRhiCommandBuffer *cb)
{
    // ownership not transferred
    redirect.commandBuffer = cb;
}

void QQuickWindowPrivate::syncSceneGraph()
{
    Q_Q(QQuickWindow);

    const bool wasRtDirty = redirect.renderTargetDirty;
    ensureCustomRenderTarget();

    QRhiCommandBuffer *cb = nullptr;
    if (rhi) {
        if (redirect.commandBuffer)
            cb = redirect.commandBuffer;
        else
            cb = swapchain->currentFrameCommandBuffer();
    }
    context->prepareSync(q->effectiveDevicePixelRatio(), cb, graphicsConfig);

    animationController->beforeNodeSync();

    emit q->beforeSynchronizing();
    runAndClearJobs(&beforeSynchronizingJobs);

    if (pendingFontUpdate) {
        QFont::cleanup();
        invalidateFontData(contentItem);
        context->invalidateGlyphCaches();
    }

    if (Q_UNLIKELY(!renderer)) {
        forceUpdate(contentItem);

        QSGRootNode *rootNode = new QSGRootNode;
        rootNode->appendChildNode(QQuickItemPrivate::get(contentItem)->itemNode());
        const bool useDepth = graphicsConfig.isDepthBufferEnabledFor2D();
        const QSGRendererInterface::RenderMode renderMode = useDepth ? QSGRendererInterface::RenderMode2D
                                                                     : QSGRendererInterface::RenderMode2DNoDepthBuffer;
        renderer = context->createRenderer(renderMode);
        renderer->setRootNode(rootNode);
    } else if (Q_UNLIKELY(wasRtDirty)
               && q->rendererInterface()->graphicsApi() == QSGRendererInterface::Software) {
        auto softwareRenderer = static_cast<QSGSoftwareRenderer *>(renderer);
        softwareRenderer->markDirty();
    }

    updateDirtyNodes();

    animationController->afterNodeSync();

    renderer->setClearColor(clearColor);

    renderer->setVisualizationMode(visualizationMode);

    if (pendingFontUpdate) {
        context->flushGlyphCaches();
        pendingFontUpdate = false;
    }

    emit q->afterSynchronizing();
    runAndClearJobs(&afterSynchronizingJobs);
}

void QQuickWindowPrivate::emitBeforeRenderPassRecording(void *ud)
{
    QQuickWindow *w = reinterpret_cast<QQuickWindow *>(ud);
    emit w->beforeRenderPassRecording();
}

void QQuickWindowPrivate::emitAfterRenderPassRecording(void *ud)
{
    QQuickWindow *w = reinterpret_cast<QQuickWindow *>(ud);
    emit w->afterRenderPassRecording();
}

int QQuickWindowPrivate::multiViewCount()
{
    if (rhi) {
        ensureCustomRenderTarget();
        if (redirect.rt.rt.renderTarget)
            return redirect.rt.rt.multiViewCount;
    }

    // Note that on QRhi level 0 and 1 are often used interchangeably, as both mean
    // no-multiview. Here in Qt Quick let's always use 1 as the default
    // (no-multiview), so that higher layers (effects, materials) do not need to
    // handle both 0 and 1, only 1.
    return 1;
}

QRhiRenderTarget *QQuickWindowPrivate::activeCustomRhiRenderTarget()
{
    if (rhi) {
        ensureCustomRenderTarget();
        return redirect.rt.rt.renderTarget;
    }
    return nullptr;
}

void QQuickWindowPrivate::renderSceneGraph()
{
    Q_Q(QQuickWindow);
    if (!renderer)
        return;

    ensureCustomRenderTarget();

    QSGRenderTarget sgRenderTarget;
    if (rhi) {
        QRhiRenderTarget *rt;
        QRhiRenderPassDescriptor *rp;
        QRhiCommandBuffer *cb;
        if (redirect.rt.rt.renderTarget) {
            rt = redirect.rt.rt.renderTarget;
            rp = rt->renderPassDescriptor();
            if (!rp) {
                qWarning("Custom render target is set but no renderpass descriptor has been provided.");
                return;
            }
            cb = redirect.commandBuffer;
            if (!cb) {
                qWarning("Custom render target is set but no command buffer has been provided.");
                return;
            }
        } else {
            if (!swapchain) {
                qWarning("QQuickWindow: No render target (neither swapchain nor custom target was provided)");
                return;
            }
            rt = swapchain->currentFrameRenderTarget();
            rp = rpDescForSwapchain;
            cb = swapchain->currentFrameCommandBuffer();
        }
        sgRenderTarget = QSGRenderTarget(rt, rp, cb);
        sgRenderTarget.multiViewCount = multiViewCount();
    } else {
        sgRenderTarget = QSGRenderTarget(redirect.rt.sw.paintDevice);
    }

    context->beginNextFrame(renderer,
                            sgRenderTarget,
                            emitBeforeRenderPassRecording,
                            emitAfterRenderPassRecording,
                            q);

    animationController->advance();
    emit q->beforeRendering();
    runAndClearJobs(&beforeRenderingJobs);

    const qreal devicePixelRatio = q->effectiveDevicePixelRatio();
    QSize pixelSize;
    if (redirect.rt.rt.renderTarget)
        pixelSize = redirect.rt.rt.renderTarget->pixelSize();
    else if (redirect.rt.sw.paintDevice)
        pixelSize = QSize(redirect.rt.sw.paintDevice->width(), redirect.rt.sw.paintDevice->height());
    else if (rhi)
        pixelSize = swapchain->currentPixelSize();
    else // software or other backend
        pixelSize = q->size() * devicePixelRatio;

    renderer->setDevicePixelRatio(devicePixelRatio);
    renderer->setDeviceRect(QRect(QPoint(0, 0), pixelSize));
    renderer->setViewportRect(QRect(QPoint(0, 0), pixelSize));

    QSGAbstractRenderer::MatrixTransformFlags matrixFlags;
    bool flipY = rhi ? !rhi->isYUpInNDC() : false;
    if (!customRenderTarget.isNull() && customRenderTarget.mirrorVertically())
        flipY = !flipY;
    if (flipY)
        matrixFlags |= QSGAbstractRenderer::MatrixTransformFlipY;

    const QRectF rect(QPointF(0, 0), pixelSize / devicePixelRatio);
    renderer->setProjectionMatrixToRect(rect, matrixFlags, rhi && !rhi->isYUpInNDC());

    context->renderNextFrame(renderer);

    emit q->afterRendering();
    runAndClearJobs(&afterRenderingJobs);

    context->endNextFrame(renderer);

    if (renderer && renderer->hasVisualizationModeWithContinuousUpdate()) {
        // For the overdraw visualizer. This update is not urgent so avoid a
        // direct update() call, this is only here to keep the overdraw
        // visualization box rotating even when the scene is static.
        QCoreApplication::postEvent(q, new QEvent(QEvent::Type(FullUpdateRequest)));
    }
}

QQuickWindowPrivate::QQuickWindowPrivate()
    : contentItem(nullptr)
    , dirtyItemList(nullptr)
    , lastReportedItemDevicePixelRatio(0)
    , context(nullptr)
    , renderer(nullptr)
    , windowManager(nullptr)
    , renderControl(nullptr)
    , clearColor(Qt::white)
    , persistentGraphics(true)
    , persistentSceneGraph(true)
    , inDestructor(false)
    , incubationController(nullptr)
    , hasActiveSwapchain(false)
    , hasRenderableSwapchain(false)
    , swapchainJustBecameRenderable(false)
    , updatesEnabled(true)
{
}

QQuickWindowPrivate::~QQuickWindowPrivate()
{
    inDestructor = true;
    redirect.rt.reset(rhi);
    if (QQmlInspectorService *service = QQmlDebugConnector::service<QQmlInspectorService>())
        service->removeWindow(q_func());
    deliveryAgent = nullptr;
}

void QQuickWindowPrivate::setPalette(QQuickPalette* palette)
{
    if (windowPaletteRef == palette)
        return;

    if (windowPaletteRef)
        disconnect(windowPaletteRef, &QQuickPalette::changed, this, &QQuickWindowPrivate::updateWindowPalette);
    windowPaletteRef = palette;
    updateWindowPalette();
    if (windowPaletteRef)
        connect(windowPaletteRef, &QQuickPalette::changed, this, &QQuickWindowPrivate::updateWindowPalette);
}

void QQuickWindowPrivate::updateWindowPalette()
{
    QQuickPaletteProviderPrivateBase::setPalette(windowPaletteRef);
}

void QQuickWindowPrivate::updateChildrenPalettes(const QPalette &parentPalette)
{
    Q_Q(QQuickWindow);
    if (auto root = q->contentItem()) {
        for (auto &&child: root->childItems()) {
            QQuickItemPrivate::get(child)->inheritPalette(parentPalette);
        }
    }
}

void QQuickWindowPrivate::init(QQuickWindow *c, QQuickRenderControl *control)
{
    q_ptr = c;


    Q_Q(QQuickWindow);

    contentItem = new QQuickRootItem;
    contentItem->setObjectName(q->objectName());
    QQml_setParent_noEvent(contentItem, c);
    QQmlEngine::setObjectOwnership(contentItem, QQmlEngine::CppOwnership);
    QQuickItemPrivate *contentItemPrivate = QQuickItemPrivate::get(contentItem);
    contentItemPrivate->window = q;
    contentItemPrivate->windowRefCount = 1;
    contentItemPrivate->flags |= QQuickItem::ItemIsFocusScope;
    contentItem->setSize(q->size());
    deliveryAgent = new QQuickDeliveryAgent(contentItem);

    visualizationMode = qgetenv("QSG_VISUALIZE");
    renderControl = control;
    if (renderControl)
        QQuickRenderControlPrivate::get(renderControl)->window = q;

    if (!renderControl)
        windowManager = QSGRenderLoop::instance();

    Q_ASSERT(windowManager || renderControl);

    QObject::connect(static_cast<QGuiApplication *>(QGuiApplication::instance()),
                     &QGuiApplication::fontDatabaseChanged,
                     q,
                     &QQuickWindow::handleFontDatabaseChanged);

    if (q->screen()) {
        lastReportedItemDevicePixelRatio = q->effectiveDevicePixelRatio();
    }

    QSGContext *sg;
    if (renderControl) {
        QQuickRenderControlPrivate *renderControlPriv = QQuickRenderControlPrivate::get(renderControl);
        sg = renderControlPriv->sg;
        context = renderControlPriv->rc;
    } else {
        windowManager->addWindow(q);
        sg = windowManager->sceneGraphContext();
        context = windowManager->createRenderContext(sg);
    }

    q->setSurfaceType(windowManager ? windowManager->windowSurfaceType() : QSurface::OpenGLSurface);
    q->setFormat(sg->defaultSurfaceFormat());
    // When using Vulkan, associating a scenegraph-managed QVulkanInstance with
    // the window (but only when not using renderControl) is deferred to
    // QSGRhiSupport::createRhi(). This allows applications to set up their own
    // QVulkanInstance and set that on the window, if they wish to.

    animationController.reset(new QQuickAnimatorController(q));

    connections = {
        QObject::connect(context, &QSGRenderContext::initialized, q, &QQuickWindow::sceneGraphInitialized, Qt::DirectConnection),
        QObject::connect(context, &QSGRenderContext::invalidated, q, &QQuickWindow::sceneGraphInvalidated, Qt::DirectConnection),
        QObject::connect(context, &QSGRenderContext::invalidated, q, &QQuickWindow::cleanupSceneGraph, Qt::DirectConnection),

        QObject::connect(q, &QQuickWindow::focusObjectChanged, q, &QQuickWindow::activeFocusItemChanged),
        QObject::connect(q, &QQuickWindow::screenChanged, q, &QQuickWindow::handleScreenChanged),
        QObject::connect(qApp, &QGuiApplication::applicationStateChanged, q, &QQuickWindow::handleApplicationStateChanged),
        QObject::connect(q, &QQuickWindow::frameSwapped, q, &QQuickWindow::runJobsAfterSwap, Qt::DirectConnection),
    };

    if (QQmlInspectorService *service = QQmlDebugConnector::service<QQmlInspectorService>())
        service->addWindow(q);
}

void QQuickWindow::handleApplicationStateChanged(Qt::ApplicationState state)
{
    Q_D(QQuickWindow);
    if (state != Qt::ApplicationActive && d->contentItem) {
        auto da = d->deliveryAgentPrivate();
        Q_ASSERT(da);
        da->handleWindowDeactivate(this);
    }
}

/*!
    \property QQuickWindow::data
    \internal
*/

QQmlListProperty<QObject> QQuickWindowPrivate::data()
{
    QQmlListProperty<QObject> ret;

    ret.object = q_func();
    ret.append = QQuickWindowPrivate::data_append;
    ret.count = QQuickWindowPrivate::data_count;
    ret.at = QQuickWindowPrivate::data_at;
    ret.clear = QQuickWindowPrivate::data_clear;
    // replace is not supported by QQuickItem. Don't synthesize it.
    ret.removeLast = QQuickWindowPrivate::data_removeLast;

    return ret;
}

void QQuickWindowPrivate::dirtyItem(QQuickItem *item)
{
    Q_Q(QQuickWindow);

    QQuickItemPrivate *itemPriv = QQuickItemPrivate::get(item);
    if (itemPriv->dirtyAttributes & QQuickItemPrivate::ChildrenStackingChanged)
        needsChildWindowStackingOrderUpdate = true;

    q->maybeUpdate();
}

/*!
    \deprecated Use QPointerEvent::exclusiveGrabber().
    Returns the item which currently has the mouse grab.
*/
QQuickItem *QQuickWindow::mouseGrabberItem() const
{
    Q_D(const QQuickWindow);
    auto da = const_cast<QQuickWindowPrivate *>(d)->deliveryAgentPrivate();
    Q_ASSERT(da);
    // The normal use case is to call this function while an event is being delivered;
    // but if the caller knows about the event, it should call QPointerEvent::exclusiveGrabber() instead.
    if (auto epd = da->mousePointData())
        return qmlobject_cast<QQuickItem *>(epd->exclusiveGrabber);

    if (Q_LIKELY(d->deliveryAgentPrivate()->eventsInDelivery.isEmpty()))
        // mousePointData() checked that already: it's one reason epd can be null
        qCDebug(lcMouse, "mouse grabber ambiguous: no event is currently being delivered");
    // If no event is being delivered, we can return "the mouse" grabber,
    // but in general there could be more than one mouse, could be only a touchscreen etc.
    // That's why this function is obsolete.
    return qmlobject_cast<QQuickItem *>(QPointingDevicePrivate::get(QPointingDevice::primaryPointingDevice())->
                                        firstPointExclusiveGrabber());
}

void QQuickWindowPrivate::cleanup(QSGNode *n)
{
    Q_Q(QQuickWindow);

    Q_ASSERT(!cleanupNodeList.contains(n));
    cleanupNodeList.append(n);
    q->maybeUpdate();
}

/*!
    \qmltype Window
    \nativetype QQuickWindow
    \inqmlmodule QtQuick
    \ingroup qtquick-visual
    \brief Creates a new top-level window.

    The Window object creates a new top-level window for a Qt Quick scene. It automatically sets up the
    window for use with \c {QtQuick} graphical types.

    A Window can be declared inside an Item or inside another Window, in which
    case the inner Window will automatically become "transient for" the outer
    Window, with the outer Window as its \l transientParent. Most platforms will
    show the Window centered upon the outer window in this case, and there may be
    other platform-dependent behaviors, depending also on the \l flags. If the nested
    window is intended to be a dialog in your application, you should also set \l flags
    to \c Qt.Dialog, because some window managers will not provide the centering behavior
    without that flag.

    You can also declare multiple windows inside a top-level \l QtObject, in which
    case the windows will have no transient relationship.

    Alternatively you can set or bind \l x and \l y to position the Window
    explicitly on the screen.

    When the user attempts to close a window, the \l closing signal will be
    emitted. You can force the window to stay open (for example to prompt the
    user to save changes) by writing an \c onClosing handler that sets
    \c {close.accepted = false} unless it's safe to close the window (for example,
    because there are no more unsaved changes).

    \code
    onClosing: (close) => {
        if (document.changed) {
            close.accepted = false
            confirmExitPopup.open()
        }
    }

    // The confirmExitPopup allows user to save or discard the document,
    // or to cancel the closing.
    \endcode

    \section1 Styling

    As with all visual types in Qt Quick, Window supports
    \l {palette}{palettes}. However, as with types like \l Text, Window does
    not use palettes by default. For example, to change the background color
    of the window when the operating system's theme changes, the \l color must
    be set:

    \snippet qml/windowPalette.qml declaration-and-color
    \codeline
    \snippet qml/windowPalette.qml text-item
    \snippet qml/windowPalette.qml closing-brace

    Use \l {ApplicationWindow} (and \l {Label}) from \l {Qt Quick Controls}
    instead of Window to get automatic styling.
*/

/*!
    \class QQuickWindow
    \since 5.0

    \inmodule QtQuick

    \brief The QQuickWindow class provides the window for displaying a graphical QML scene.

    QQuickWindow provides the graphical scene management needed to interact with and display
    a scene of QQuickItems.

    A QQuickWindow always has a single invisible root item. To add items to this window,
    reparent the items to the root item or to an existing item in the scene.

    For easily displaying a scene from a QML file, see \l{QQuickView}.

    \section1 Rendering

    QQuickWindow uses a scene graph to represent what needs to be rendered.
    This scene graph is disconnected from the QML scene and potentially lives in
    another thread, depending on the platform implementation. Since the
    rendering scene graph lives independently from the QML scene, it can also be
    completely released without affecting the state of the QML scene.

    The sceneGraphInitialized() signal is emitted on the rendering thread before
    the QML scene is rendered to the screen for the first time. If the rendering
    scene graph has been released, the signal will be emitted again before the
    next frame is rendered. A visible, on-screen QQuickWindow is driven
    internally by a \c{render loop}, of which there are multiple implementations
    provided in the scene graph. For details on the scene graph rendering
    process, see \l{Qt Quick Scene Graph}.

    By default, a QQuickWindow renders using an accelerated 3D graphics API,
    such as OpenGL or Vulkan. See \l{Scene Graph Adaptations} for a detailed
    overview of scene graph backends and the supported graphics APIs.

    \warning It is crucial that graphics operations and interaction with the
    scene graph happens exclusively on the rendering thread, primarily during
    the updatePaintNode() phase.

    \warning As many of the signals related to rendering are emitted from the
    rendering thread, connections should be made using Qt::DirectConnection.

    \section2 Integration with Accelerated 3D Graphics APIs

    It is possible to integrate OpenGL, Vulkan, Metal, or Direct3D 11 calls
    directly into the QQuickWindow, as long as the QQuickWindow and the
    underlying scene graph is rendering using the same API. To access native
    graphics objects, such as device or context object handles, use
    QSGRendererInterface. An instance of QSGRendererInterface is queriable from
    QQuickWindow by calling rendererInterface(). The enablers for this
    integration are the beforeRendering(), beforeRenderPassRecording(),
    afterRenderPassRecording(), and related signals. These allow rendering
    underlays or overlays. Alternatively, QNativeInterface::QSGOpenGLTexture,
    QNativeInterface::QSGVulkanTexture, and other similar classes allow
    wrapping an existing native texture or image object in a QSGTexture that
    can then be used with the scene graph.

    \section2 Rendering without Acceleration

    A limited, pure software based rendering path is available as well. With the
    \c software backend, a number of Qt Quick features are not available, QML
    items relying on these will not be rendered at all. At the same time, this
    allows QQuickWindow to be functional even on systems where there is no 3D
    graphics API available at all. See \l{Qt Quick Software Adaptation} for more
    details.

    \section2 Redirected Rendering

    A QQuickWindow is not necessarily backed by a native window on screen. The
    rendering can be redirected to target a custom render target, such as a
    given native texture. This is achieved in combination with the
    QQuickRenderControl class, and functions such as setRenderTarget(),
    setGraphicsDevice(), and setGraphicsConfiguration().

    In this case, the QQuickWindow represents the scene, and provides the
    intrastructure for rendering a frame. It will not be backed by a render
    loop and a native window. Instead, in this case the application drives
    rendering, effectively substituting for the render loops. This allows
    generating image sequences, rendering into textures for use in external 3D
    engines, or rendering Qt Quick content within a VR environment.

    \section2 Resource Management

    QML will try to cache images and scene graph nodes to improve performance,
    but in some low-memory scenarios it might be required to aggressively
    release these resources. The releaseResources() function can be used to
    force the clean up of certain resources, especially resource that are cached
    and can be recreated later when needed again.

    Additionally, calling releaseResources() may result in releasing the entire
    scene graph and the associated graphics resources. The
    sceneGraphInvalidated() signal will be emitted when this happens. This
    behavior is controlled by the setPersistentGraphics() and
    setPersistentSceneGraph() functions.

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.

    \section2 Exposure and Visibility

    When a QQuickWindow instance is deliberately hidden with hide() or
    setVisible(false), it will stop rendering and its scene graph and graphics
    context might be released as well. This depends on the settings configured
    by setPersistentGraphics() and setPersistentSceneGraph(). The behavior in
    this respect is identical to explicitly calling the releaseResources()
    function. A window can become not exposed, in other words non-renderable, by
    other means as well. This depends on the platform and windowing system. For
    example, on Windows minimizing a window makes it stop rendering.  On \macos
    fully obscuring a window by other windows on top triggers the same. On
    Linux/X11, the behavior is dependent on the window manager.

    \section2 OpenGL Context and Surface Formats

    While it is possible to specify a QSurfaceFormat for every QQuickWindow by
    calling the member function setFormat(), windows may also be created from
    QML by using the Window and ApplicationWindow elements. In this case there
    is no C++ code involved in the creation of the window instance, yet
    applications may still wish to set certain surface format values, for
    example to request a given OpenGL version or profile. Such applications can
    call the static function QSurfaceFormat::setDefaultFormat() at startup. The
    specified format will be used for all Quick windows created afterwards.

    \section2 Vulkan Instance

    When using Vulkan, a QQuickWindow is automatically associated with a
    QVulkanInstance that is created and managed internally by the scene graph.
    This way most applications do not need to worry about having a \c
    VkInstance available since it all happens automatically. In advanced cases
    an application may wish to create its own QVulkanInstance, in order to
    configure it in a specific way. That is possible as well. Calling
    \l{QWindow::setVulkanInstance()}{setVulkanInstance()} on the QQuickWindow
    right after construction, before making it visible, leads to using the
    application-supplied QVulkanInstance (and the underlying \c VkInstance).
    When redirecting via QQuickRenderControl, there is no QVulkanInstance
    provided automatically, but rather the application is expected to provide
    its own and associate it with the QQuickWindow.

    \section2 Graphics Contexts and Devices

    When the scene graph is initialized, which typically happens when the
    window becomes exposed or, in case of redirected rendering, initialization
    is performed \l{QQuickRenderControl::initialize()}{via
    QQuickRenderControl}, the context or device objects necessary for rendering
    are created automatically. This includes OpenGL contexts, Direct3D devices
    and device contexts, Vulkan and Metal devices. These are also queriable by
    application code afterwards via
    \l{QSGRendererInterface::getResource()}{QSGRendererInterface}. When using
    the \c basic render loop, which performs all rendering on the GUI thread,
    the same context or device is used with all visible QQuickWindows. The \c
    threaded render loop uses a dedicated context or device object for each
    rendering thread, and so for each QQuickWindow. With some graphics APIs,
    there is a degree of customizability provided via
    setGraphicsConfiguration(). This makes it possible, for example, to specify
    the list of Vulkan extensions to enable on the \c VkDevice. Alternatively,
    it is also possible to provide a set of existing context or device objects
    for use by the QQuickWindow, instead of letting it construct its own. This
    is achieved through setGraphicsDevice().

    \sa QQuickView, QQuickRenderControl, QQuickRenderTarget,
    QQuickGraphicsDevice, QQuickGraphicsConfiguration, QSGRendererInterface
*/

/*!
    Constructs a window for displaying a QML scene with parent window \a parent.
*/
QQuickWindow::QQuickWindow(QWindow *parent)
    : QQuickWindow(*new QQuickWindowPrivate, parent)
{
}



/*!
    \internal
*/
QQuickWindow::QQuickWindow(QQuickWindowPrivate &dd, QWindow *parent)
    : QWindow(dd, parent)
{
    Q_D(QQuickWindow);
    d->init(this);
}

/*!
    Constructs a window for displaying a QML scene, whose rendering will
    be controlled by the \a control object.
    Please refer to QQuickRenderControl's documentation for more information.

    \since 5.4
*/
QQuickWindow::QQuickWindow(QQuickRenderControl *control)
    : QWindow(*(new QQuickWindowPrivate), nullptr)
{
    Q_D(QQuickWindow);
    d->init(this, control);
}

/*!
    \internal
*/
QQuickWindow::QQuickWindow(QQuickWindowPrivate &dd, QQuickRenderControl *control)
    : QWindow(dd, nullptr)
{
    Q_D(QQuickWindow);
    d->init(this, control);
}

/*!
    Destroys the window.
*/
QQuickWindow::~QQuickWindow()
{
    Q_D(QQuickWindow);
    d->inDestructor = true;
    if (d->renderControl) {
        QQuickRenderControlPrivate::get(d->renderControl)->windowDestroyed();
    } else if (d->windowManager) {
        d->windowManager->removeWindow(this);
        d->windowManager->windowDestroyed(this);
    }

    disconnect(this, &QQuickWindow::focusObjectChanged, this, &QQuickWindow::activeFocusItemChanged);
    disconnect(this, &QQuickWindow::screenChanged, this, &QQuickWindow::handleScreenChanged);
    disconnect(qApp, &QGuiApplication::applicationStateChanged, this, &QQuickWindow::handleApplicationStateChanged);
    disconnect(this, &QQuickWindow::frameSwapped, this, &QQuickWindow::runJobsAfterSwap);

    delete d->incubationController; d->incubationController = nullptr;
    QQuickRootItem *root = d->contentItem;
    d->contentItem = nullptr;
    root->setParent(nullptr); // avoid QChildEvent delivery during deletion
    delete root;
    d->deliveryAgent = nullptr; // avoid forwarding events there during destruction


    {
        const std::lock_guard locker(d->renderJobMutex);
        qDeleteAll(std::exchange(d->beforeSynchronizingJobs, {}));
        qDeleteAll(std::exchange(d->afterSynchronizingJobs, {}));
        qDeleteAll(std::exchange(d->beforeRenderingJobs, {}));
        qDeleteAll(std::exchange(d->afterRenderingJobs, {}));;
        qDeleteAll(std::exchange(d->afterSwapJobs, {}));
    }

    // It is important that the pixmap cache is cleaned up during shutdown.
    // Besides playing nice, this also solves a practical problem that
    // QQuickTextureFactory implementations in other libraries need
    // have their destructors loaded while they the library is still
    // loaded into memory.
    QQuickPixmap::purgeCache();

    for (const QMetaObject::Connection &connection : d->connections)
        disconnect(connection);
}

#if QT_CONFIG(quick_shadereffect)
void qtquick_shadereffect_purge_gui_thread_shader_cache();
#endif

/*!
    This function tries to release redundant resources currently held by the QML scene.

    Calling this function requests the scene graph to release cached graphics
    resources, such as graphics pipeline objects, shader programs, or image
    data.

    Additionally, depending on the render loop in use, this function may also
    result in the scene graph and all window-related rendering resources to be
    released. If this happens, the sceneGraphInvalidated() signal will be
    emitted, allowing users to clean up their own graphics resources. The
    setPersistentGraphics() and setPersistentSceneGraph() functions can be used
    to prevent this from happening, if handling the cleanup is not feasible in
    the application, at the cost of higher memory usage.

    \note The releasing of cached graphics resources, such as graphics
    pipelines or shader programs is not dependent on the persistency hints. The
    releasing of those will happen regardless of the values of the persistent
    graphics and scenegraph hints.

    \note This function is not related to the QQuickItem::releaseResources()
    virtual function.

    \sa sceneGraphInvalidated(), setPersistentGraphics(), setPersistentSceneGraph()
 */

void QQuickWindow::releaseResources()
{
    Q_D(QQuickWindow);
    if (d->windowManager)
        d->windowManager->releaseResources(this);
    QQuickPixmap::purgeCache();
#if QT_CONFIG(quick_shadereffect)
    qtquick_shadereffect_purge_gui_thread_shader_cache();
#endif
}



/*!
    Sets whether the graphics resources (graphics device or context,
    swapchain, buffers, textures) should be preserved, and cannot be
    released until the last window is deleted, to \a persistent. The
    default value is true.

    When calling releaseResources(), or when the window gets hidden (more
    specifically, not renderable), some render loops have the possibility
    to release all, not just the cached, graphics resources. This can free
    up memory temporarily, but it also means the rendering engine will have
    to do a full, potentially costly reinitialization of the resources when
    the window needs to render again.

    \note The rules for when a window is not renderable are platform and
    window manager specific.

    \note All graphics resources are released when the last QQuickWindow is
    deleted, regardless of this setting.

    \note This is a hint, and is not guaranteed that it is taken into account.

    \note This hint does not apply to cached resources, that are relatively
    cheap to drop and then recreate later. Therefore, calling releaseResources()
    will typically lead to releasing those regardless of the value of this hint.

    \sa setPersistentSceneGraph(), sceneGraphInitialized(), sceneGraphInvalidated(), releaseResources()
 */

void QQuickWindow::setPersistentGraphics(bool persistent)
{
    Q_D(QQuickWindow);
    d->persistentGraphics = persistent;
}



/*!
    Returns whether essential graphics resources can be released during the
    lifetime of the QQuickWindow.

    \note This is a hint, and is not guaranteed that it is taken into account.

    \sa setPersistentGraphics()
 */

bool QQuickWindow::isPersistentGraphics() const
{
    Q_D(const QQuickWindow);
    return d->persistentGraphics;
}



/*!
    Sets whether the scene graph nodes and resources are \a persistent.
    Persistent means the nodes and resources cannot be released.
    The default value is \c true.

    When calling releaseResources(), when the window gets hidden (more
    specifically, not renderable), some render loops have the possibility
    to release the scene graph nodes and related graphics resources. This
    frees up memory temporarily, but will also mean the scene graph has to
    be rebuilt when the window renders next time.

    \note The rules for when a window is not renderable are platform and
    window manager specific.

    \note The scene graph nodes and resources are always released when the
    last QQuickWindow is deleted, regardless of this setting.

    \note This is a hint, and is not guaranteed that it is taken into account.

    \sa setPersistentGraphics(), sceneGraphInvalidated(), sceneGraphInitialized(), releaseResources()
 */

void QQuickWindow::setPersistentSceneGraph(bool persistent)
{
    Q_D(QQuickWindow);
    d->persistentSceneGraph = persistent;
}



/*!
    Returns whether the scene graph nodes and resources can be
    released during the lifetime of this QQuickWindow.

    \note This is a hint. When and how this happens is implementation
    specific.
 */

bool QQuickWindow::isPersistentSceneGraph() const
{
    Q_D(const QQuickWindow);
    return d->persistentSceneGraph;
}

/*!
    \qmlattachedproperty Item Window::contentItem
    \since 5.4

    This attached property holds the invisible root item of the scene or
    \c null if the item is not in a window. The Window attached property
    can be attached to any Item.
*/

/*!
    \property QQuickWindow::contentItem
    \brief The invisible root item of the scene.

  A QQuickWindow always has a single invisible root item containing all of its content.
  To add items to this window, reparent the items to the contentItem or to an existing
  item in the scene.
*/
QQuickItem *QQuickWindow::contentItem() const
{
    Q_D(const QQuickWindow);

    return d->contentItem;
}

/*!
    \property QQuickWindow::activeFocusItem

    \brief The item which currently has active focus or \c null if there is
    no item with active focus.

    \sa QQuickItem::forceActiveFocus(), {Keyboard Focus in Qt Quick}
*/
QQuickItem *QQuickWindow::activeFocusItem() const
{
    Q_D(const QQuickWindow);
    auto da = d->deliveryAgentPrivate();
    Q_ASSERT(da);
    return da->activeFocusItem;
}

/*!
  \internal
  \reimp
*/
QObject *QQuickWindow::focusObject() const
{
    Q_D(const QQuickWindow);
    auto da = d->deliveryAgentPrivate();
    Q_ASSERT(da);
    if (!d->inDestructor && da->activeFocusItem)
        return da->activeFocusItem;
    return const_cast<QQuickWindow*>(this);
}

/*! \reimp */
bool QQuickWindow::event(QEvent *event)
{
    Q_D(QQuickWindow);

    // bypass QWindow::event dispatching of input events: deliveryAgent takes care of it
    QQuickDeliveryAgent *da = d->deliveryAgent;
    if (event->isPointerEvent()) {
        /*
            We can't bypass the virtual functions like mousePressEvent() tabletEvent() etc.,
            for the sake of code that subclasses QQuickWindow and overrides them, even though
            we no longer need them as entry points for Qt Quick event delivery.
            So dispatch to them now, ahead of normal delivery, and stop them from calling
            back into this function if they were called from here (avoid recursion).
            It could also be that user code expects them to work as entry points, too;
            in that case, windowEventDispatch _won't_ be set, so the event comes here and
            we'll dispatch it further below.
        */
        if (d->windowEventDispatch)
            return false;
        {
            const bool wasAccepted = event->isAccepted();
            QScopedValueRollback windowEventDispatchGuard(d->windowEventDispatch, true);
            qCDebug(lcPtr) << "dispatching to window functions in case of override" << event;
            QWindow::event(event);
            if (event->isAccepted() && !wasAccepted)
                return true;
        }
        /*
            QQuickWindow does not override touchEvent(). If the application has a subclass
            of QQuickWindow which allows the event to remain accepted, it means they want
            to stop propagation here, so return early (below). But otherwise we will call
            QWindow::touchEvent(), which will ignore(); in that case, we need to continue
            with the usual delivery below, so we need to undo the ignore().
        */
        auto pe = static_cast<QPointerEvent *>(event);
        if (QQuickDeliveryAgentPrivate::isTouchEvent(pe))
            event->accept();
        // end of dispatch to user-overridden virtual window functions

        /*
            When delivering update and release events to existing grabbers,
            use the subscene delivery agent, if any.  A possible scenario:
            1) Two touchpoints pressed on the main window: QQuickWindowPrivate::deliveryAgent delivers to QQuick3DViewport,
            which does picking and finds two subscenes ("root" Items mapped onto two different 3D objects) to deliver it to.
            2) The QTouchEvent is split up so that each subscene sees points relevant to it.
            3) During delivery to either subscene, an item in the subscene grabs.
            4) The user moves finger(s) generating a move event: the correct grabber item needs to get the update
            via the same subscene delivery agent from which it got the press, so that the coord transform will be done properly.
            5) Likewise with the touchpoint releases.
            With single-point events (mouse, or only one finger) it's simplified: there can only be one subscene of interest;
            for (pt : pe->points()) would only iterate once, so we might as well skip that logic.
        */
        if (pe->pointCount()) {
            const bool synthMouse = QQuickDeliveryAgentPrivate::isSynthMouse(pe);
            if (QQuickDeliveryAgentPrivate::subsceneAgentsExist) {
                bool ret = false;
                // Split up the multi-point event according to the relevant QQuickDeliveryAgent that should deliver to each existing grabber
                // but send ungrabbed points to d->deliveryAgent()
                QFlatMap<QQuickDeliveryAgent*, QList<QEventPoint>> deliveryAgentsNeedingPoints;
                QEventPoint::States eventStates;

                auto insert = [&](QQuickDeliveryAgent *ptda, const QEventPoint &pt) {
                    if (pt.state() == QEventPoint::Pressed && !synthMouse)
                        pe->clearPassiveGrabbers(pt);
                    auto &ptList = deliveryAgentsNeedingPoints[ptda];
                    auto idEquals = [](auto id) { return [id] (const auto &e) { return e.id() == id; }; };
                    if (std::none_of(ptList.cbegin(), ptList.cend(), idEquals(pt.id())))
                        ptList.append(pt);
                };

                for (const auto &pt : pe->points()) {
                    eventStates |= pt.state();
                    auto epd = QPointingDevicePrivate::get(const_cast<QPointingDevice*>(pe->pointingDevice()))->queryPointById(pt.id());
                    Q_ASSERT(epd);
                    bool foundAgent = false;
                    if (!epd->exclusiveGrabber.isNull() && !epd->exclusiveGrabberContext.isNull()) {
                        if (auto ptda = qobject_cast<QQuickDeliveryAgent *>(epd->exclusiveGrabberContext.data())) {
                            insert(ptda, pt);
                            qCDebug(lcPtr) << pe->type() << "point" << pt.id() << pt.state()
                                           << "@" << pt.scenePosition() << "will be re-delivered via known grabbing agent" << ptda << "to" << epd->exclusiveGrabber.data();
                            foundAgent = true;
                        }
                    }
                    for (auto pgda : epd->passiveGrabbersContext) {
                        if (auto ptda = qobject_cast<QQuickDeliveryAgent *>(pgda.data())) {
                            insert(ptda, pt);
                            qCDebug(lcPtr) << pe->type() << "point" << pt.id() << pt.state()
                                           << "@" << pt.scenePosition() << "will be re-delivered via known passive-grabbing agent" << ptda;
                            foundAgent = true;
                        }
                    }
                    // fallback: if we didn't find remembered/known grabber agent(s), expect the root DA to handle it
                    if (!foundAgent)
                        insert(da, pt);
                }
                for (auto daAndPoints : deliveryAgentsNeedingPoints) {
                    if (pe->pointCount() > 1) {
                        Q_ASSERT(QQuickDeliveryAgentPrivate::isTouchEvent(pe));
                        // if all points have the same state, set the event type accordingly
                        QEvent::Type eventType = pe->type();
                        switch (eventStates) {
                        case QEventPoint::State::Pressed:
                            eventType = QEvent::TouchBegin;
                            break;
                        case QEventPoint::State::Released:
                            eventType = QEvent::TouchEnd;
                            break;
                        default:
                            eventType = QEvent::TouchUpdate;
                            break;
                        }
                        // Make a new touch event for the subscene, the same way QQuickItemPrivate::localizedTouchEvent() does it
                        QMutableTouchEvent te(eventType, pe->pointingDevice(), pe->modifiers(), daAndPoints.second);
                        te.setTimestamp(pe->timestamp());
                        te.accept();
                        qCDebug(lcTouch) << daAndPoints.first << "shall now receive" << &te;
                        ret = daAndPoints.first->event(&te) || ret;
                    } else {
                        qCDebug(lcPtr) << daAndPoints.first << "shall now receive" << pe;
                        ret = daAndPoints.first->event(pe) || ret;
                    }
                }

                if (ret) {
                    d->deliveryAgentPrivate()->clearGrabbers(pe);
                    return true;
                }
            } else if (!synthMouse) {
                // clear passive grabbers unless it's a system synth-mouse event
                // QTBUG-104890: Windows sends synth mouse events (which should be ignored) after touch events
                for (const auto &pt : pe->points()) {
                    if (pt.state() == QEventPoint::Pressed)
                        pe->clearPassiveGrabbers(pt);
                }
            }
        }

        // If it has no points, it's probably a TouchCancel, and DeliveryAgent needs to handle it.
        // If we didn't handle it in the block above, handle it now.
        // TODO should we deliver to all DAs at once then, since we don't know which one should get it?
        // or fix QTBUG-90851 so that the event always has points?
        bool ret = (da && da->event(event));

        d->deliveryAgentPrivate()->clearGrabbers(pe);

        if (pe->type() == QEvent::MouseButtonPress || pe->type() == QEvent::MouseButtonRelease) {
            // Ensure that we synthesize a context menu event as QWindow::event does, if necessary.
            // We only send the context menu event if the pointer event wasn't accepted (ret == false).
            d->maybeSynthesizeContextMenuEvent(static_cast<QMouseEvent *>(pe));
        }

        if (ret)
            return true;
    } else if (event->isInputEvent()) {
        if (da && da->event(event))
            return true;
    }

    switch (event->type()) {
    // a few more types that are not QInputEvents, but QQuickDeliveryAgent needs to handle them anyway
    case QEvent::FocusAboutToChange:
    case QEvent::Enter:
    case QEvent::Leave:
    case QEvent::InputMethod:
    case QEvent::InputMethodQuery:
#if QT_CONFIG(quick_draganddrop)
    case QEvent::DragEnter:
    case QEvent::DragLeave:
    case QEvent::DragMove:
    case QEvent::Drop:
#endif
        if (d->inDestructor)
            return false;
        if (da && da->event(event))
            return true;
        break;
    case QEvent::LanguageChange:
    case QEvent::LocaleChange:
        if (d->contentItem)
            QCoreApplication::sendEvent(d->contentItem, event);
        break;
    case QEvent::UpdateRequest:
        if (d->windowManager)
            d->windowManager->handleUpdateRequest(this);
        break;
    case QEvent::PlatformSurface:
        if ((static_cast<QPlatformSurfaceEvent *>(event))->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            // Ensure that the rendering thread is notified before
            // the QPlatformWindow is destroyed.
            if (d->windowManager)
                d->windowManager->hide(this);
        }
        break;
    case QEvent::WindowDeactivate:
        if (auto da = d->deliveryAgentPrivate())
            da->handleWindowDeactivate(this);
        Q_FALLTHROUGH();
    case QEvent::WindowActivate:
        if (d->contentItem)
            QCoreApplication::sendEvent(d->contentItem, event);
        break;
    case QEvent::ApplicationPaletteChange:
        d->inheritPalette(QGuiApplication::palette());
        if (d->contentItem)
            QCoreApplication::sendEvent(d->contentItem, event);
        break;
    case QEvent::DevicePixelRatioChange:
        physicalDpiChanged();
        break;
    case QEvent::SafeAreaMarginsChange:
        QQuickSafeArea::updateSafeAreasRecursively(d->contentItem);
        break;
    case QEvent::ChildWindowAdded: {
        auto *childEvent = static_cast<QChildWindowEvent*>(event);
        auto *childWindow = childEvent->child();
        qCDebug(lcQuickWindow) << "Child window" << childWindow << "added to" << this;
        if (childWindow->handle()) {
            // The reparenting has already resulted in the native window
            // being added to its parent, on top of all other windows. We need
            // to do a synchronous re-stacking of the windows here, to avoid
            // leaving the window in the wrong position while waiting for the
            // asynchronous callback to QQuickWindow::polishItems().
            d->updateChildWindowStackingOrder();
        } else {
            qCDebug(lcQuickWindow) << "No platform window yet."
                << "Deferring child window stacking until surface creation";
        }
        break;
    }
    default:
        break;
    }

    if (event->type() == QEvent::Type(QQuickWindowPrivate::FullUpdateRequest))
        update();
    else if (event->type() == QEvent::Type(QQuickWindowPrivate::TriggerContextCreationFailure))
        d->windowManager->handleContextCreationFailure(this);

    if (event->isPointerEvent())
        return true;
    else
        return QWindow::event(event);
}

void QQuickWindowPrivate::maybeSynthesizeContextMenuEvent(QMouseEvent *event)
{
    // See comment in QQuickWindow::event; we need to follow that pattern here,
    // otherwise the context menu event will be sent before the press (since
    // QQuickWindow::mousePressEvent returns early if windowEventDispatch is true).
    // If we don't do this, the incorrect order will cause the menu to
    // immediately close when the press is delivered.
    // Also, don't send QContextMenuEvent if a menu has already been opened while
    // handling a QMouseEvent in which the right button was pressed or released.
    if (windowEventDispatch || !rmbContextMenuEventEnabled)
        return;

    QWindowPrivate::maybeSynthesizeContextMenuEvent(event);
}

void QQuickWindowPrivate::updateChildWindowStackingOrder(QQuickItem *item)
{
    Q_Q(QQuickWindow);

    if (!item) {
        qCDebug(lcQuickWindow) << "Updating child window stacking order for" << q;
        item = contentItem;
    }
    auto *itemPrivate = QQuickItemPrivate::get(item);
    const auto paintOrderChildItems = itemPrivate->paintOrderChildItems();
    for (auto *child : paintOrderChildItems) {
        if (auto *windowContainer = qobject_cast<QQuickWindowContainer*>(child)) {
            auto *window = windowContainer->containedWindow();
            if (!window) {
                qCDebug(lcQuickWindow) << windowContainer << "has no contained window yet";
                continue;
            }
            if (window->parent() != q) {
                qCDebug(lcQuickWindow) << window << "is not yet child of this window";
                continue;
            }
            qCDebug(lcQuickWindow) << "Raising" << window << "owned by" << windowContainer;
            window->raise();
        }

        updateChildWindowStackingOrder(child);
    }
}

/*! \reimp */
void QQuickWindow::keyPressEvent(QKeyEvent *e)
{
    Q_D(QQuickWindow);
    if (d->windowEventDispatch)
        return;
    auto da = d->deliveryAgentPrivate();
    Q_ASSERT(da);
    da->deliverKeyEvent(e);
}

/*! \reimp */
void QQuickWindow::keyReleaseEvent(QKeyEvent *e)
{
    Q_D(QQuickWindow);
    if (d->windowEventDispatch)
        return;
    auto da = d->deliveryAgentPrivate();
    Q_ASSERT(da);
    da->deliverKeyEvent(e);
}

#if QT_CONFIG(wheelevent)
/*! \reimp */
void QQuickWindow::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickWindow);
    if (d->windowEventDispatch)
        return;
    auto da = d->deliveryAgentPrivate();
    Q_ASSERT(da);
    da->deliverSinglePointEventUntilAccepted(event);
}
#endif // wheelevent

#if QT_CONFIG(tabletevent)
/*! \reimp */
void QQuickWindow::tabletEvent(QTabletEvent *event)
{
    Q_D(QQuickWindow);
    if (d->windowEventDispatch)
        return;
    auto da = d->deliveryAgentPrivate();
    Q_ASSERT(da);
    da->deliverPointerEvent(event);
}
#endif // tabletevent

/*! \reimp */
void QQuickWindow::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickWindow);
    if (d->windowEventDispatch)
        return;
    auto da = d->deliveryAgentPrivate();
    Q_ASSERT(da);
    da->handleMouseEvent(event);
}
/*! \reimp */
void QQuickWindow::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickWindow);
    if (d->windowEventDispatch)
        return;
    auto da = d->deliveryAgentPrivate();
    Q_ASSERT(da);
    da->handleMouseEvent(event);
}
/*! \reimp */
void QQuickWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QQuickWindow);
    if (d->windowEventDispatch)
        return;
    auto da = d->deliveryAgentPrivate();
    Q_ASSERT(da);
    da->handleMouseEvent(event);
}
/*! \reimp */
void QQuickWindow::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickWindow);
    if (d->windowEventDispatch)
        return;
    auto da = d->deliveryAgentPrivate();
    Q_ASSERT(da);
    da->handleMouseEvent(event);
}

#if QT_CONFIG(cursor)
void QQuickWindowPrivate::updateCursor(const QPointF &scenePos, QQuickItem *rootItem)
{
    Q_Q(QQuickWindow);
    if (!rootItem)
        rootItem = contentItem;
    auto cursorItemAndHandler = findCursorItemAndHandler(rootItem, scenePos);
    if (cursorItem != cursorItemAndHandler.first || cursorHandler != cursorItemAndHandler.second ||
        (cursorItemAndHandler.second && QQuickPointerHandlerPrivate::get(cursorItemAndHandler.second)->cursorDirty)) {
        QWindow *renderWindow = QQuickRenderControl::renderWindowFor(q);
        QWindow *window = renderWindow ? renderWindow : q;
        cursorItem = cursorItemAndHandler.first;
        cursorHandler = cursorItemAndHandler.second;
        if (cursorHandler)
            QQuickPointerHandlerPrivate::get(cursorItemAndHandler.second)->cursorDirty = false;
        if (cursorItem) {
            const auto cursor = QQuickItemPrivate::get(cursorItem)->effectiveCursor(cursorHandler);
            qCDebug(lcHoverTrace) << "setting cursor" << cursor << "from" << cursorHandler << "or" << cursorItem;
            window->setCursor(cursor);
        } else {
            qCDebug(lcHoverTrace) << "unsetting cursor";
            window->unsetCursor();
        }
    }
}

std::pair<QQuickItem*, QQuickPointerHandler*> QQuickWindowPrivate::findCursorItemAndHandler(QQuickItem *item, const QPointF &scenePos) const
{
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(scenePos);
        if (!item->contains(p))
            return {nullptr, nullptr};
    }

    if (itemPrivate->subtreeCursorEnabled) {
        QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();
        for (int ii = children.size() - 1; ii >= 0; --ii) {
            QQuickItem *child = children.at(ii);
            if (!child->isVisible() || !child->isEnabled() || QQuickItemPrivate::get(child)->culled)
                continue;
            auto ret = findCursorItemAndHandler(child, scenePos);
            if (ret.first)
                return ret;
        }
        if (itemPrivate->hasCursorHandler) {
            if (auto handler = itemPrivate->effectiveCursorHandler()) {
                if (handler->parentContains(scenePos))
                    return {item, handler};
            }
        }
        if (itemPrivate->hasCursor) {
            QPointF p = item->mapFromScene(scenePos);
            if (item->contains(p))
                return {item, nullptr};
        }
    }

    return {nullptr, nullptr};
}
#endif

void QQuickWindowPrivate::clearFocusObject()
{
    if (auto da = deliveryAgentPrivate())
        da->clearFocusObject();
}

void QQuickWindowPrivate::setFocusToTarget(FocusTarget target, Qt::FocusReason reason)
{
    if (!contentItem)
        return;

    QQuickItem *newFocusItem = nullptr;
    switch (target) {
    case FocusTarget::First:
    case FocusTarget::Last: {
        const bool forward = (target == FocusTarget::First);
        newFocusItem = QQuickItemPrivate::nextPrevItemInTabFocusChain(contentItem, forward);
        if (newFocusItem) {
            const auto *itemPriv = QQuickItemPrivate::get(newFocusItem);
            if (itemPriv->subFocusItem && itemPriv->flags & QQuickItem::ItemIsFocusScope)
                deliveryAgentPrivate()->clearFocusInScope(newFocusItem, itemPriv->subFocusItem, reason);
        }
        break;
    }
    case FocusTarget::Next:
    case FocusTarget::Prev: {
        const auto da = deliveryAgentPrivate();
        Q_ASSERT(da);
        QQuickItem *focusItem = da->focusTargetItem() ? da->focusTargetItem() : contentItem;
        bool forward = (target == FocusTarget::Next);
        newFocusItem = QQuickItemPrivate::nextPrevItemInTabFocusChain(focusItem, forward);
        break;
    }
    default:
        break;
    }

    if (newFocusItem)
        newFocusItem->forceActiveFocus(reason);
}

/*!
    \qmlproperty list<QtObject> Window::data
    \qmldefault

    The data property allows you to freely mix visual children, resources
    and other Windows in a Window.

    If you assign another Window to the data list, the nested window will
    become "transient for" the outer Window.

    If you assign an \l Item to the data list, it becomes a child of the
    Window's \l contentItem, so that it appears inside the window. The item's
    parent will be the window's contentItem, which is the root of the Item
    ownership tree within that Window.

    If you assign any other object type, it is added as a resource.

    It should not generally be necessary to refer to the \c data property,
    as it is the default property for Window and thus all child items are
    automatically assigned to this property.

    \sa QWindow::transientParent()
 */

void QQuickWindowPrivate::data_append(QQmlListProperty<QObject> *property, QObject *o)
{
    if (!o)
        return;
    QQuickWindow *that = static_cast<QQuickWindow *>(property->object);
    QQmlListProperty<QObject> itemProperty = QQuickItemPrivate::get(that->contentItem())->data();
    itemProperty.append(&itemProperty, o);
}

qsizetype QQuickWindowPrivate::data_count(QQmlListProperty<QObject> *property)
{
    QQuickWindow *win = static_cast<QQuickWindow*>(property->object);
    if (!win || !win->contentItem() || !QQuickItemPrivate::get(win->contentItem())->data().count)
        return 0;
    QQmlListProperty<QObject> itemProperty = QQuickItemPrivate::get(win->contentItem())->data();
    return itemProperty.count(&itemProperty);
}

QObject *QQuickWindowPrivate::data_at(QQmlListProperty<QObject> *property, qsizetype i)
{
    QQuickWindow *win = static_cast<QQuickWindow*>(property->object);
    QQmlListProperty<QObject> itemProperty = QQuickItemPrivate::get(win->contentItem())->data();
    return itemProperty.at(&itemProperty, i);
}

void QQuickWindowPrivate::data_clear(QQmlListProperty<QObject> *property)
{
    QQuickWindow *win = static_cast<QQuickWindow*>(property->object);
    QQmlListProperty<QObject> itemProperty = QQuickItemPrivate::get(win->contentItem())->data();
    itemProperty.clear(&itemProperty);
}

void QQuickWindowPrivate::data_removeLast(QQmlListProperty<QObject> *property)
{
    QQuickWindow *win = static_cast<QQuickWindow*>(property->object);
    QQmlListProperty<QObject> itemProperty = QQuickItemPrivate::get(win->contentItem())->data();
    itemProperty.removeLast(&itemProperty);
}

bool QQuickWindowPrivate::isRenderable() const
{
    Q_Q(const QQuickWindow);
    return ((q->isExposed() && q->isVisible())) && q->geometry().isValid();
}

void QQuickWindowPrivate::rhiCreationFailureMessage(const QString &backendName,
                                                    QString *translatedMessage,
                                                    QString *untranslatedMessage)
{
    const char msg[] = QT_TRANSLATE_NOOP("QQuickWindow",
        "Failed to initialize graphics backend for %1.");
    *translatedMessage = QQuickWindow::tr(msg).arg(backendName);
    *untranslatedMessage = QString::fromLatin1(msg).arg(backendName);
}

void QQuickWindowPrivate::cleanupNodes()
{
    qDeleteAll(cleanupNodeList);
    cleanupNodeList.clear();
}

void QQuickWindowPrivate::cleanupNodesOnShutdown(QQuickItem *item)
{
    QQuickItemPrivate *p = QQuickItemPrivate::get(item);
    if (p->itemNodeInstance) {
        delete p->itemNodeInstance;
        p->itemNodeInstance = nullptr;

        if (p->extra.isAllocated()) {
            p->extra->opacityNode = nullptr;
            p->extra->clipNode = nullptr;
            p->extra->rootNode = nullptr;
        }

        p->paintNode = nullptr;

        p->dirty(QQuickItemPrivate::Window);
    }

    // Qt 7: Make invalidateSceneGraph a virtual member of QQuickItem
    if (p->flags & QQuickItem::ItemHasContents) {
        const QMetaObject *mo = item->metaObject();
        int index = mo->indexOfSlot("invalidateSceneGraph()");
        if (index >= 0) {
            const QMetaMethod &method = mo->method(index);
            // Skip functions named invalidateSceneGraph() in QML items.
            if (strstr(method.enclosingMetaObject()->className(), "_QML_") == nullptr)
                method.invoke(item, Qt::DirectConnection);
        }
    }

    for (int ii = 0; ii < p->childItems.size(); ++ii)
        cleanupNodesOnShutdown(p->childItems.at(ii));
}

// This must be called from the render thread, with the main thread frozen
void QQuickWindowPrivate::cleanupNodesOnShutdown()
{
    Q_Q(QQuickWindow);
    cleanupNodes();
    cleanupNodesOnShutdown(contentItem);
    for (QSet<QQuickItem *>::const_iterator it = parentlessItems.begin(), cend = parentlessItems.end(); it != cend; ++it)
        cleanupNodesOnShutdown(*it);
    animationController->windowNodesDestroyed();
    q->cleanupSceneGraph();
}

void QQuickWindowPrivate::updateDirtyNodes()
{
    qCDebug(lcDirty) << "QQuickWindowPrivate::updateDirtyNodes():";

    cleanupNodes();

    QQuickItem *updateList = dirtyItemList;
    dirtyItemList = nullptr;
    if (updateList) QQuickItemPrivate::get(updateList)->prevDirtyItem = &updateList;

    while (updateList) {
        QQuickItem *item = updateList;
        QQuickItemPrivate *itemPriv = QQuickItemPrivate::get(item);
        itemPriv->removeFromDirtyList();

        qCDebug(lcDirty) << "   QSGNode:" << item << qPrintable(itemPriv->dirtyToString());
        updateDirtyNode(item);
    }
}

static inline QSGNode *qquickitem_before_paintNode(QQuickItemPrivate *d)
{
    const QList<QQuickItem *> childItems = d->paintOrderChildItems();
    QQuickItem *before = nullptr;
    for (int i=0; i<childItems.size(); ++i) {
        QQuickItemPrivate *dd = QQuickItemPrivate::get(childItems.at(i));
        // Perform the same check as the in fetchNextNode below.
        if (dd->z() < 0 && (dd->explicitVisible || (dd->extra.isAllocated() && dd->extra->effectRefCount)))
            before = childItems.at(i);
        else
            break;
    }
    return Q_UNLIKELY(before) ? QQuickItemPrivate::get(before)->itemNode() : nullptr;
}

static QSGNode *fetchNextNode(QQuickItemPrivate *itemPriv, int &ii, bool &returnedPaintNode)
{
    QList<QQuickItem *> orderedChildren = itemPriv->paintOrderChildItems();

    for (; ii < orderedChildren.size() && orderedChildren.at(ii)->z() < 0; ++ii) {
        QQuickItemPrivate *childPrivate = QQuickItemPrivate::get(orderedChildren.at(ii));
        if (!childPrivate->explicitVisible &&
            (!childPrivate->extra.isAllocated() || !childPrivate->extra->effectRefCount))
            continue;

        ii++;
        return childPrivate->itemNode();
    }

    if (itemPriv->paintNode && !returnedPaintNode) {
        returnedPaintNode = true;
        return itemPriv->paintNode;
    }

    for (; ii < orderedChildren.size(); ++ii) {
        QQuickItemPrivate *childPrivate = QQuickItemPrivate::get(orderedChildren.at(ii));
        if (!childPrivate->explicitVisible &&
            (!childPrivate->extra.isAllocated() || !childPrivate->extra->effectRefCount))
            continue;

        ii++;
        return childPrivate->itemNode();
    }

    return nullptr;
}

void QQuickWindowPrivate::updateDirtyNode(QQuickItem *item)
{
    QQuickItemPrivate *itemPriv = QQuickItemPrivate::get(item);
    quint32 dirty = itemPriv->dirtyAttributes;
    itemPriv->dirtyAttributes = 0;

    if ((dirty & QQuickItemPrivate::TransformUpdateMask) ||
        (dirty & QQuickItemPrivate::Size && itemPriv->origin() != QQuickItem::TopLeft &&
         (itemPriv->scale() != 1. || itemPriv->rotation() != 0.))) {

        QMatrix4x4 matrix;

        if (itemPriv->x != 0. || itemPriv->y != 0.)
            matrix.translate(itemPriv->x, itemPriv->y);

        for (int ii = itemPriv->transforms.size() - 1; ii >= 0; --ii)
            itemPriv->transforms.at(ii)->applyTo(&matrix);

        if (itemPriv->scale() != 1. || itemPriv->rotation() != 0.) {
            QPointF origin = item->transformOriginPoint();
            matrix.translate(origin.x(), origin.y());
            if (itemPriv->scale() != 1.)
                matrix.scale(itemPriv->scale(), itemPriv->scale());
            if (itemPriv->rotation() != 0.)
                matrix.rotate(itemPriv->rotation(), 0, 0, 1);
            matrix.translate(-origin.x(), -origin.y());
        }

        itemPriv->itemNode()->setMatrix(matrix);
    }

    const bool clipEffectivelyChanged = dirty & (QQuickItemPrivate::Clip | QQuickItemPrivate::Window);
    if (clipEffectivelyChanged) {
        QSGNode *parent = itemPriv->opacityNode() ? (QSGNode *)itemPriv->opacityNode()
                                                  : (QSGNode *)itemPriv->itemNode();
        QSGNode *child = itemPriv->rootNode();

        if (bool initializeClipNode = item->clip() && itemPriv->clipNode() == nullptr;
            initializeClipNode) {
            QQuickDefaultClipNode *clip = new QQuickDefaultClipNode(item->clipRect());
            itemPriv->extra.value().clipNode = clip;
            clip->update();

            if (!child) {
                parent->reparentChildNodesTo(clip);
                parent->appendChildNode(clip);
            } else {
                parent->removeChildNode(child);
                clip->appendChildNode(child);
                parent->appendChildNode(clip);
            }

        } else if (bool updateClipNode = item->clip() && itemPriv->clipNode() != nullptr;
                   updateClipNode) {
            QQuickDefaultClipNode *clip = itemPriv->clipNode();
            clip->setClipRect(item->clipRect());
            clip->update();
        } else if (bool removeClipNode = !item->clip() && itemPriv->clipNode() != nullptr;
                   removeClipNode) {
            QQuickDefaultClipNode *clip = itemPriv->clipNode();
            parent->removeChildNode(clip);
            if (child) {
                clip->removeChildNode(child);
                parent->appendChildNode(child);
            } else {
                clip->reparentChildNodesTo(parent);
            }

            delete itemPriv->clipNode();
            itemPriv->extra->clipNode = nullptr;
        }
    }

    const int effectRefCount = itemPriv->extra.isAllocated() ? itemPriv->extra->effectRefCount : 0;
    const bool effectRefEffectivelyChanged =
            (dirty & (QQuickItemPrivate::EffectReference | QQuickItemPrivate::Window))
            && ((effectRefCount == 0) != (itemPriv->rootNode() == nullptr));
    if (effectRefEffectivelyChanged) {
        if (dirty & QQuickItemPrivate::ChildrenUpdateMask)
            itemPriv->childContainerNode()->removeAllChildNodes();

        QSGNode *parent = itemPriv->clipNode();
        if (!parent)
            parent = itemPriv->opacityNode();
        if (!parent)
            parent = itemPriv->itemNode();

        if (itemPriv->extra.isAllocated() && itemPriv->extra->effectRefCount) {
            Q_ASSERT(itemPriv->rootNode() == nullptr);
            QSGRootNode *root = new QSGRootNode();
            itemPriv->extra->rootNode = root;
            parent->reparentChildNodesTo(root);
            parent->appendChildNode(root);
        } else {
            Q_ASSERT(itemPriv->rootNode() != nullptr);
            QSGRootNode *root = itemPriv->rootNode();
            parent->removeChildNode(root);
            root->reparentChildNodesTo(parent);
            delete itemPriv->rootNode();
            itemPriv->extra->rootNode = nullptr;
        }
    }

    if (dirty & QQuickItemPrivate::ChildrenUpdateMask) {
        int ii = 0;
        bool fetchedPaintNode = false;
        QList<QQuickItem *> orderedChildren = itemPriv->paintOrderChildItems();
        int desiredNodesSize = orderedChildren.size() + (itemPriv->paintNode ? 1 : 0);

        // now start making current state match the promised land of
        // desiredNodes. in the case of our current state matching desiredNodes
        // (though why would we get ChildrenUpdateMask with no changes?) then we
        // should make no changes at all.

        // how many nodes did we process, when examining changes
        int desiredNodesProcessed = 0;

        // currentNode is how far, in our present tree, we have processed. we
        // make use of this later on to trim the current child list if the
        // desired list is shorter.
        QSGNode *groupNode = itemPriv->childContainerNode();
        QSGNode *currentNode = groupNode->firstChild();
        QSGNode *desiredNode = nullptr;

        while (currentNode && (desiredNode = fetchNextNode(itemPriv, ii, fetchedPaintNode))) {
            if (currentNode != desiredNode) {
                // uh oh... reality and our utopic paradise are diverging!
                // we need to reconcile this...
                if (currentNode->nextSibling() == desiredNode) {
                    // nice and simple: a node was removed, and the next in line is correct.
                    groupNode->removeChildNode(currentNode);
                } else {
                    // a node needs to be added..
                    // remove it from any pre-existing parent, and push it before currentNode,
                    // so it's in the correct place...
                    if (desiredNode->parent()) {
                        desiredNode->parent()->removeChildNode(desiredNode);
                    }
                    groupNode->insertChildNodeBefore(desiredNode, currentNode);
                }

                // continue iteration at the correct point, now desiredNode is in place...
                currentNode = desiredNode;
            }

            currentNode = currentNode->nextSibling();
            desiredNodesProcessed++;
        }

        // if we didn't process as many nodes as in the new list, then we have
        // more nodes at the end of desiredNodes to append to our list.
        // this will be the case when adding new nodes, for instance.
        if (desiredNodesProcessed < desiredNodesSize) {
            while ((desiredNode = fetchNextNode(itemPriv, ii, fetchedPaintNode))) {
                if (desiredNode->parent())
                    desiredNode->parent()->removeChildNode(desiredNode);
                groupNode->appendChildNode(desiredNode);
            }
        } else if (currentNode) {
            // on the other hand, if we processed less than our current node
            // tree, then nodes have been _removed_ from the scene, and we need
            // to take care of that here.
            while (currentNode) {
                QSGNode *node = currentNode->nextSibling();
                groupNode->removeChildNode(currentNode);
                currentNode = node;
            }
        }
    }

    if ((dirty & QQuickItemPrivate::Size) && itemPriv->clipNode()) {
        itemPriv->clipNode()->setRect(item->clipRect());
        itemPriv->clipNode()->update();
    }

    if (dirty & (QQuickItemPrivate::OpacityValue | QQuickItemPrivate::Visible
                 | QQuickItemPrivate::HideReference | QQuickItemPrivate::Window))
    {
        qreal opacity = itemPriv->explicitVisible && (!itemPriv->extra.isAllocated() || itemPriv->extra->hideRefCount == 0)
                      ? itemPriv->opacity() : qreal(0);

        if (opacity != 1 && !itemPriv->opacityNode()) {
            QSGOpacityNode *node = new QSGOpacityNode;
            itemPriv->extra.value().opacityNode = node;

            QSGNode *parent = itemPriv->itemNode();
            QSGNode *child = itemPriv->clipNode();
            if (!child)
                child = itemPriv->rootNode();

            if (child) {
                parent->removeChildNode(child);
                node->appendChildNode(child);
                parent->appendChildNode(node);
            } else {
                parent->reparentChildNodesTo(node);
                parent->appendChildNode(node);
            }
        }
        if (itemPriv->opacityNode())
            itemPriv->opacityNode()->setOpacity(opacity);
    }

    if (dirty & QQuickItemPrivate::ContentUpdateMask) {

        if (itemPriv->flags & QQuickItem::ItemHasContents) {
            updatePaintNodeData.transformNode = itemPriv->itemNode();
            itemPriv->paintNode = item->updatePaintNode(itemPriv->paintNode, &updatePaintNodeData);

            Q_ASSERT(itemPriv->paintNode == nullptr ||
                     itemPriv->paintNode->parent() == nullptr ||
                     itemPriv->paintNode->parent() == itemPriv->childContainerNode());

            if (itemPriv->paintNode && itemPriv->paintNode->parent() == nullptr) {
                QSGNode *before = qquickitem_before_paintNode(itemPriv);
                if (before && before->parent()) {
                    Q_ASSERT(before->parent() == itemPriv->childContainerNode());
                    itemPriv->childContainerNode()->insertChildNodeAfter(itemPriv->paintNode, before);
                } else {
                    itemPriv->childContainerNode()->prependChildNode(itemPriv->paintNode);
                }
            }
        } else if (itemPriv->paintNode) {
            delete itemPriv->paintNode;
            itemPriv->paintNode = nullptr;
        }
    }

#ifndef QT_NO_DEBUG
    // Check consistency.

    QList<QSGNode *> nodes;
    nodes << itemPriv->itemNodeInstance
          << itemPriv->opacityNode()
          << itemPriv->clipNode()
          << itemPriv->rootNode()
          << itemPriv->paintNode;
    nodes.removeAll(nullptr);

    Q_ASSERT(nodes.constFirst() == itemPriv->itemNodeInstance);
    for (int i=1; i<nodes.size(); ++i) {
        QSGNode *n = nodes.at(i);
        // Failing this means we messed up reparenting
        Q_ASSERT(n->parent() == nodes.at(i-1));
        // Only the paintNode and the one who is childContainer may have more than one child.
        Q_ASSERT(n == itemPriv->paintNode || n == itemPriv->childContainerNode() || n->childCount() == 1);
    }
#endif

}

bool QQuickWindowPrivate::emitError(QQuickWindow::SceneGraphError error, const QString &msg)
{
    Q_Q(QQuickWindow);
    static const QMetaMethod errorSignal = QMetaMethod::fromSignal(&QQuickWindow::sceneGraphError);
    if (q->isSignalConnected(errorSignal)) {
        emit q->sceneGraphError(error, msg);
        return true;
    }
    return false;
}

void QQuickWindow::maybeUpdate()
{
    Q_D(QQuickWindow);
    if (d->renderControl)
        QQuickRenderControlPrivate::get(d->renderControl)->maybeUpdate();
    else if (d->windowManager)
        d->windowManager->maybeUpdate(this);
}

void QQuickWindow::cleanupSceneGraph()
{
    Q_D(QQuickWindow);
    if (!d->renderer)
        return;

    delete d->renderer->rootNode();
    delete d->renderer;
    d->renderer = nullptr;

    d->runAndClearJobs(&d->beforeSynchronizingJobs);
    d->runAndClearJobs(&d->afterSynchronizingJobs);
    d->runAndClearJobs(&d->beforeRenderingJobs);
    d->runAndClearJobs(&d->afterRenderingJobs);
    d->runAndClearJobs(&d->afterSwapJobs);
}

QOpenGLContext *QQuickWindowPrivate::openglContext()
{
#if QT_CONFIG(opengl)
    if (context && context->isValid()) {
        QSGRendererInterface *rif = context->sceneGraphContext()->rendererInterface(context);
        if (rif) {
            Q_Q(QQuickWindow);
            return reinterpret_cast<QOpenGLContext *>(rif->getResource(q, QSGRendererInterface::OpenGLContextResource));
        }
    }
#endif
    return nullptr;
}

/*!
    Returns true if the scene graph has been initialized; otherwise returns false.
 */
bool QQuickWindow::isSceneGraphInitialized() const
{
    Q_D(const QQuickWindow);
    return d->context != nullptr && d->context->isValid();
}

/*!
    \fn void QQuickWindow::frameSwapped()

    This signal is emitted when a frame has been queued for presenting. With
    vertical synchronization enabled the signal is emitted at most once per
    vsync interval in a continuously animating scene.

    This signal will be emitted from the scene graph rendering thread.
*/

/*!
    \qmlsignal QtQuick::Window::frameSwapped()

    This signal is emitted when a frame has been queued for presenting. With
    vertical synchronization enabled the signal is emitted at most once per
    vsync interval in a continuously animating scene.
 */

/*!
    \fn void QQuickWindow::sceneGraphInitialized()

    This signal is emitted when the scene graph has been initialized.

    This signal will be emitted from the scene graph rendering thread.
 */

/*!
    \qmlsignal QtQuick::Window::sceneGraphInitialized()
    \internal
 */

/*!
    \fn void QQuickWindow::sceneGraphInvalidated()

    This signal is emitted when the scene graph has been invalidated.

    This signal implies that the graphics rendering context used
    has been invalidated and all user resources tied to that context
    should be released.

    When rendering with OpenGL, the QOpenGLContext of this window will
    be bound when this function is called. The only exception is if
    the native OpenGL has been destroyed outside Qt's control, for
    instance through EGL_CONTEXT_LOST.

    This signal will be emitted from the scene graph rendering thread.
 */

/*!
    \qmlsignal QtQuick::Window::sceneGraphInvalidated()
    \internal
 */

/*!
    \fn void QQuickWindow::sceneGraphError(SceneGraphError error, const QString &message)

    This signal is emitted when an \a error occurred during scene graph initialization.

    Applications should connect to this signal if they wish to handle errors,
    like graphics context creation failures, in a custom way. When no slot is
    connected to the signal, the behavior will be different: Quick will print
    the \a message, or show a message box, and terminate the application.

    This signal will be emitted from the GUI thread.

    \since 5.3
 */

/*!
    \qmlsignal QtQuick::Window::sceneGraphError(SceneGraphError error, QString message)

    This signal is emitted when an \a error occurred during scene graph initialization.

    You can implement onSceneGraphError(error, message) to handle errors,
    such as graphics context creation failures, in a custom way.
    If no handler is connected to this signal, Quick will print the \a message,
    or show a message box, and terminate the application.

    \since 5.3
 */

/*!
    \class QQuickCloseEvent
    \internal
    \since 5.1

    \inmodule QtQuick

    \brief Notification that a \l QQuickWindow is about to be closed
*/
/*!
    \qmltype CloseEvent
    \nativetype QQuickCloseEvent
    \inqmlmodule QtQuick
    \ingroup qtquick-visual
    \brief Notification that a \l Window is about to be closed.
    \since 5.1

    Notification that a window is about to be closed by the windowing system
    (e.g. the user clicked the title bar close button). The CloseEvent contains
    an accepted property which can be set to false to abort closing the window.
*/

/*!
    \qmlproperty bool CloseEvent::accepted

    This property indicates whether the application will allow the user to
    close the window.  It is true by default.
*/

/*!
    \internal
    \fn void QQuickWindow::closing(QQuickCloseEvent *close)
    \since 5.1

    This signal is emitted when the window receives the event \a close from
    the windowing system.

    On \macOs, Qt will create a menu item \c Quit if there is no menu item
    whose text is "quit" or "exit". This menu item calls the \c QCoreApplication::quit
    signal, not the \c QQuickWindow::closing() signal.

    \sa {QMenuBar as a Global Menu Bar}
*/

/*!
    \qmlsignal QtQuick::Window::closing(CloseEvent close)
    \since 5.1

    This signal is emitted when the user tries to close the window.

    This signal includes a \a close parameter. The \c {close.accepted}
    property is true by default so that the window is allowed to close; but you
    can implement an \c onClosing handler and set \c {close.accepted = false} if
    you need to do something else before the window can be closed.
 */

/*!
    Sets the render target for this window to be \a target.

    A QQuickRenderTarget serves as an opaque handle for a renderable native
    object, most commonly a 2D texture, and associated metadata, such as the
    size in pixels.

    A default constructed QQuickRenderTarget means no redirection. A valid
    \a target, created via one of the static QQuickRenderTarget factory functions,
    on the other hand, enables redirection of the rendering of the Qt Quick
    scene: it will no longer target the color buffers for the surface
    associated with the window, but rather the textures or other graphics
    objects specified in \a target.

    For example, assuming the scenegraph is using Vulkan to render, one can
    redirect its output into a \c VkImage. For graphics APIs like Vulkan, the
    image layout must be provided as well. QQuickRenderTarget instances are
    implicitly shared and are copyable and can be passed by value. They do not
    own the associated native objects (such as, the VkImage in the example),
    however.

    \badcode
        QQuickRenderTarget rt = QQuickRenderTarget::fromVulkanImage(vulkanImage, VK_IMAGE_LAYOUT_PREINITIALIZED, pixelSize);
        quickWindow->setRenderTarget(rt);
    \endcode

    This function is very often used in combination with QQuickRenderControl
    and an invisible QQuickWindow, in order to render Qt Quick content into a
    texture, without creating an on-screen native window for this QQuickWindow.

    When the desired target, or associated data, such as the size, changes,
    call this function with a new QQuickRenderTarget. Constructing
    QQuickRenderTarget instances and calling this function is cheap, but be
    aware that setting a new \a target with a different native object or other
    data may lead to potentially expensive initialization steps when the
    scenegraph is about to render the next frame. Therefore change the target
    only when necessary.

    \note The window does not take ownership of any native objects referenced
    in \a target.

    \note It is the caller's responsibility to ensure the native objects
    referred to in \a target are valid for the scenegraph renderer too. For
    instance, with Vulkan, Metal, and Direct3D this implies that the texture or
    image is created on the same graphics device that is used by the scenegraph
    internally. Therefore, when texture objects created on an already existing
    device or context are involved, this function is often used in combination
    with setGraphicsDevice().

    \note With graphics APIs where relevant, the application must pay attention
    to image layout transitions performed by the scenegraph. For example, once
    a VkImage is associated with the scenegraph by calling this function, its
    layout will transition to \c VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL when
    rendering a frame.

    \warning This function can only be called from the thread doing the
    rendering.

    \since 6.0

    \sa QQuickRenderControl, setGraphicsDevice(), setGraphicsApi()
 */
void QQuickWindow::setRenderTarget(const QQuickRenderTarget &target)
{
    Q_D(QQuickWindow);
    if (target != d->customRenderTarget) {
        d->customRenderTarget = target;
        d->redirect.renderTargetDirty = true;
    }
}

/*!
    \return the QQuickRenderTarget passed to setRenderTarget(), or a default
    constructed one otherwise

    \since 6.0

    \sa setRenderTarget()
 */
QQuickRenderTarget QQuickWindow::renderTarget() const
{
    Q_D(const QQuickWindow);
    return d->customRenderTarget;
}

#ifdef Q_OS_WEBOS
class GrabWindowForProtectedContent : public QRunnable
{
public:
    GrabWindowForProtectedContent(QQuickWindow *window, QImage *image, QWaitCondition *condition)
        : m_window(window)
        , m_image(image)
        , m_condition(condition)
    {
    }

    bool checkGrabbable()
    {
        if (!m_window)
            return false;
        if (!m_image)
            return false;
        if (!QQuickWindowPrivate::get(m_window))
            return false;

        return true;
    }

    void run() override
    {
        if (!checkGrabbable())
            return;

        *m_image = QSGRhiSupport::instance()->grabOffscreenForProtectedContent(m_window);
        if (m_condition)
            m_condition->wakeOne();
        return;
    }

private:
    QQuickWindow *m_window;
    QImage *m_image;
    QWaitCondition *m_condition;

};
#endif

/*!
    Grabs the contents of the window and returns it as an image.

    It is possible to call the grabWindow() function when the window is not
    visible. This requires that the window is \l{QWindow::create()} {created}
    and has a valid size and that no other QQuickWindow instances are rendering
    in the same process.

    \note When using this window in combination with QQuickRenderControl, the
    result of this function is an empty image, unless the \c software backend
    is in use. This is because when redirecting the output to an
    application-managed graphics resource (such as, a texture) by using
    QQuickRenderControl and setRenderTarget(), the application is better suited
    for managing and executing an eventual read back operation, since it is in
    full control of the resource to begin with.

    \warning Calling this function will cause performance problems.

    \warning This function can only be called from the GUI thread.
 */
QImage QQuickWindow::grabWindow()
{
    Q_D(QQuickWindow);

    if (!d->isRenderable() && !d->renderControl) {
        // backends like software can grab regardless of the window state
        if (d->windowManager && (d->windowManager->flags() & QSGRenderLoop::SupportsGrabWithoutExpose))
            return d->windowManager->grab(this);

        if (!isSceneGraphInitialized()) {
            // We do not have rendering up and running. Forget the render loop,
            // do a frame completely offscreen and synchronously into a
            // texture. This can be *very* slow due to all the device/context
            // and resource initialization but the documentation warns for it,
            // and is still important for some use cases.
            Q_ASSERT(!d->rhi);
            return QSGRhiSupport::instance()->grabOffscreen(this);
        }
    }

#ifdef Q_OS_WEBOS
    if (requestedFormat().testOption(QSurfaceFormat::ProtectedContent)) {
        QImage image;
        QMutex mutex;
        QWaitCondition condition;
        mutex.lock();
        GrabWindowForProtectedContent *job = new GrabWindowForProtectedContent(this, &image, &condition);
        if (!job) {
            qWarning("QQuickWindow::grabWindow: Failed to create a job for capturing protected content");
            mutex.unlock();
            return QImage();
        }
        scheduleRenderJob(job, QQuickWindow::NoStage);
        condition.wait(&mutex);
        mutex.unlock();
        return image;
    }
#endif
    // The common case: we have an exposed window with an initialized
    // scenegraph, meaning we can request grabbing via the render loop, or we
    // are not targeting the window, in which case the request is to be
    // forwarded to the rendercontrol.
    if (d->renderControl)
        return QQuickRenderControlPrivate::get(d->renderControl)->grab();
    else if (d->windowManager)
        return d->windowManager->grab(this);

    return QImage();
}

/*!
    Returns an incubation controller that splices incubation between frames
    for this window. QQuickView automatically installs this controller for you,
    otherwise you will need to install it yourself using \l{QQmlEngine::setIncubationController()}.

    The controller is owned by the window and will be destroyed when the window
    is deleted.
*/
QQmlIncubationController *QQuickWindow::incubationController() const
{
    Q_D(const QQuickWindow);

    if (!d->windowManager)
        return nullptr; // TODO: make sure that this is safe

    if (!d->incubationController)
        d->incubationController = new QQuickWindowIncubationController(d->windowManager);
    return d->incubationController;
}



/*!
    \enum QQuickWindow::CreateTextureOption

    The CreateTextureOption enums are used to customize a texture is wrapped.

    \value TextureHasAlphaChannel The texture has an alpha channel and should
    be drawn using blending.

    \value TextureHasMipmaps The texture has mipmaps and can be drawn with
    mipmapping enabled.

    \value TextureOwnsGLTexture As of Qt 6.0, this flag is not used in practice
    and is ignored. Native graphics resource ownership is not transferable to
    the wrapping QSGTexture, because Qt Quick may not have the necessary details
    on how such an object and the associated memory should be freed.

    \value TextureCanUseAtlas The image can be uploaded into a texture atlas.

    \value TextureIsOpaque The texture will return false for
    QSGTexture::hasAlphaChannel() and will not be blended. This flag was added
    in Qt 5.6.

 */

/*!
    \enum QQuickWindow::SceneGraphError

    This enum describes the error in a sceneGraphError() signal.

    \value ContextNotAvailable graphics context creation failed. This typically means that
    no suitable OpenGL implementation was found, for example because no graphics drivers
    are installed and so no OpenGL 2 support is present. On mobile and embedded boards
    that use OpenGL ES such an error is likely to indicate issues in the windowing system
    integration and possibly an incorrect configuration of Qt.

    \since 5.3
 */

/*!
    \enum QQuickWindow::TextRenderType
    \since 5.10

    This enum describes the default render type of text-like elements in Qt
    Quick (\l Text, \l TextInput, etc.).

    Select NativeTextRendering if you prefer text to look native on the target
    platform and do not require advanced features such as transformation of the
    text. Using such features in combination with the NativeTextRendering
    render type will lend poor and sometimes pixelated results.

    Both \c QtTextRendering and \c CurveTextRendering are hardware-accelerated techniques.
    \c QtTextRendering is the faster of the two, but uses more memory and will exhibit rendering
    artifacts at large sizes. \c CurveTextRendering should be considered as an alternative in cases
    where \c QtTextRendering does not give good visual results or where reducing graphics memory
    consumption is a priority.

    \value QtTextRendering Use Qt's own rasterization algorithm.
    \value NativeTextRendering Use the operating system's native rasterizer for text.
    \value CurveTextRendering  Text is rendered using a curve rasterizer running directly on
                               the graphics hardware. (Introduced in Qt 6.7.0.)
*/

/*!
    \fn void QQuickWindow::beforeSynchronizing()

    This signal is emitted before the scene graph is synchronized with the QML state.

    Even though the signal is emitted from the scene graph rendering thread,
    the GUI thread is guaranteed to be blocked, like it is in
    QQuickItem::updatePaintNode(). Therefore, it is safe to access GUI thread
    thread data in a slot or lambda that is connected with
    Qt::DirectConnection.

    This signal can be used to do any preparation required before calls to
    QQuickItem::updatePaintNode().

    When using OpenGL, the QOpenGLContext used for rendering by the scene graph
    will be bound at this point.

    \warning This signal is emitted from the scene graph rendering thread. If your
    slot function needs to finish before execution continues, you must make sure that
    the connection is direct (see Qt::ConnectionType).

    \warning When using OpenGL, be aware that setting OpenGL 3.x or 4.x specific
    states and leaving these enabled or set to non-default values when returning
    from the connected slot can interfere with the scene graph's rendering.
*/

/*!
    \qmlsignal QtQuick::Window::beforeSynchronizing()
    \internal
*/

/*!
    \fn void QQuickWindow::afterSynchronizing()

    This signal is emitted after the scene graph is synchronized with the QML state.

    This signal can be used to do preparation required after calls to
    QQuickItem::updatePaintNode(), while the GUI thread is still locked.

    When using OpenGL, the QOpenGLContext used for rendering by the scene graph
    will be bound at this point.

    \warning This signal is emitted from the scene graph rendering thread. If your
    slot function needs to finish before execution continues, you must make sure that
    the connection is direct (see Qt::ConnectionType).

    \warning When using OpenGL, be aware that setting OpenGL 3.x or 4.x specific
    states and leaving these enabled or set to non-default values when returning
    from the connected slot can interfere with the scene graph's rendering.

    \since 5.3
 */

/*!
    \qmlsignal QtQuick::Window::afterSynchronizing()
    \internal
    \since 5.3
 */

/*!
    \fn void QQuickWindow::beforeRendering()

    This signal is emitted after the preparations for the frame have been done,
    meaning there is a command buffer in recording mode, where applicable. If
    desired, the slot function connected to this signal can query native
    resources like the command before via QSGRendererInterface. Note however
    that the recording of the main render pass is not yet started at this point
    and it is not possible to add commands within that pass. Starting a pass
    means clearing the color, depth, and stencil buffers so it is not possible
    to achieve an underlay type of rendering by just connecting to this
    signal. Rather, connect to beforeRenderPassRecording(). However, connecting
    to this signal is still important if the recording of copy type of commands
    is desired since those cannot be enqueued within a render pass.

    \warning This signal is emitted from the scene graph rendering thread. If your
    slot function needs to finish before execution continues, you must make sure that
    the connection is direct (see Qt::ConnectionType).

    \note When using OpenGL, be aware that setting OpenGL 3.x or 4.x specific
    states and leaving these enabled or set to non-default values when
    returning from the connected slot can interfere with the scene graph's
    rendering. The QOpenGLContext used for rendering by the scene graph will be
    bound when the signal is emitted.

    \sa rendererInterface(), {Scene Graph - RHI Under QML}, {Scene Graph -
    OpenGL Under QML}, {Scene Graph - Metal Under QML}, {Scene Graph - Vulkan
    Under QML}, {Scene Graph - Direct3D 11 Under QML}
*/

/*!
    \qmlsignal QtQuick::Window::beforeRendering()
    \internal
*/

/*!
    \fn void QQuickWindow::afterRendering()

    The signal is emitted after scene graph has added its commands to the
    command buffer, which is not yet submitted to the graphics queue. If
    desired, the slot function connected to this signal can query native
    resources, like the command buffer, before via QSGRendererInterface.  Note
    however that the render pass (or passes) are already recorded at this point
    and it is not possible to add more commands within the scenegraph's
    pass. Instead, use afterRenderPassRecording() for that. This signal has
    therefore limited use in Qt 6, unlike in Qt 5. Rather, it is the combination
    of beforeRendering() and beforeRenderPassRecording(), or beforeRendering()
    and afterRenderPassRecording(), that is typically used to achieve under- or
    overlaying of the custom rendering.

    \warning This signal is emitted from the scene graph rendering thread. If your
    slot function needs to finish before execution continues, you must make sure that
    the connection is direct (see Qt::ConnectionType).

    \note When using OpenGL, be aware that setting OpenGL 3.x or 4.x specific
    states and leaving these enabled or set to non-default values when
    returning from the connected slot can interfere with the scene graph's
    rendering. The QOpenGLContext used for rendering by the scene graph will be
    bound when the signal is emitted.

    \sa rendererInterface(), {Scene Graph - RHI Under QML}, {Scene Graph -
    OpenGL Under QML}, {Scene Graph - Metal Under QML}, {Scene Graph - Vulkan
    Under QML}, {Scene Graph - Direct3D 11 Under QML}
 */

/*!
    \qmlsignal QtQuick::Window::afterRendering()
    \internal
 */

/*!
    \fn void QQuickWindow::beforeRenderPassRecording()

    This signal is emitted before the scenegraph starts recording commands for
    the main render pass. (Layers have their own passes and are fully recorded
    by the time this signal is emitted.) The render pass is already active on
    the command buffer when the signal is emitted.

    This signal is emitted later than beforeRendering() and it guarantees that
    not just the frame, but also the recording of the scenegraph's main render
    pass is active. This allows inserting commands without having to generate an
    entire, separate render pass (which would typically clear the attached
    images). The native graphics objects can be queried via
    QSGRendererInterface.

    \note Resource updates (uploads, copies) typically cannot be enqueued from
    within a render pass. Therefore, more complex user rendering will need to
    connect to both beforeRendering() and this signal.

    \warning This signal is emitted from the scene graph rendering thread. If your
    slot function needs to finish before execution continues, you must make sure that
    the connection is direct (see Qt::ConnectionType).

    \sa rendererInterface()

    \since 5.14

    \sa {Scene Graph - RHI Under QML}
*/

/*!
    \qmlsignal QtQuick::Window::beforeRenderPassRecording()
    \internal
    \since 5.14
*/

/*!
    \fn void QQuickWindow::afterRenderPassRecording()

    This signal is emitted after the scenegraph has recorded the commands for
    its main render pass, but the pass is not yet finalized on the command
    buffer.

    This signal is emitted earlier than afterRendering(), and it guarantees that
    not just the frame but also the recording of the scenegraph's main render
    pass is still active. This allows inserting commands without having to
    generate an entire, separate render pass (which would typically clear the
    attached images). The native graphics objects can be queried via
    QSGRendererInterface.

    \note Resource updates (uploads, copies) typically cannot be enqueued from
    within a render pass. Therefore, more complex user rendering will need to
    connect to both beforeRendering() and this signal.

    \warning This signal is emitted from the scene graph rendering thread. If your
    slot function needs to finish before execution continues, you must make sure that
    the connection is direct (see Qt::ConnectionType).

    \sa rendererInterface()

    \since 5.14

    \sa {Scene Graph - RHI Under QML}
*/

/*!
    \fn void QQuickWindow::beforeFrameBegin()

    This signal is emitted before the scene graph starts preparing the frame.
    This precedes signals like beforeSynchronizing() or beforeRendering(). It is
    the earliest signal that is emitted by the scene graph rendering thread
    when starting to prepare a new frame.

    This signal is relevant for lower level graphics frameworks that need to
    execute certain operations, such as resource cleanup, at a stage where Qt
    Quick has not initiated the recording of a new frame via the underlying
    rendering hardware interface APIs.

    \warning This signal is emitted from the scene graph rendering thread. If your
    slot function needs to finish before execution continues, you must make sure that
    the connection is direct (see Qt::ConnectionType).

    \since 6.0

    \sa afterFrameEnd(), rendererInterface()
*/

/*!
    \qmlsignal QtQuick::Window::beforeFrameBegin()
    \internal
*/

/*!
    \fn void QQuickWindow::afterFrameEnd()

    This signal is emitted when the scene graph has submitted a frame. This is
    emitted after all other related signals, such as afterRendering(). It is
    the last signal that is emitted by the scene graph rendering thread when
    rendering a frame.

    \note Unlike frameSwapped(), this signal is guaranteed to be emitted also
    when the Qt Quick output is redirected via QQuickRenderControl.

    \warning This signal is emitted from the scene graph rendering thread. If your
    slot function needs to finish before execution continues, you must make sure that
    the connection is direct (see Qt::ConnectionType).

    \since 6.0

    \sa beforeFrameBegin(), rendererInterface()
*/

/*!
    \qmlsignal QtQuick::Window::afterFrameEnd()
    \internal
*/

/*!
    \qmlsignal QtQuick::Window::afterRenderPassRecording()
    \internal
    \since 5.14
*/

/*!
    \fn void QQuickWindow::afterAnimating()

    This signal is emitted on the GUI thread before requesting the render thread to
    perform the synchronization of the scene graph.

    Unlike the other similar signals, this one is emitted on the GUI thread
    instead of the render thread. It can be used to synchronize external
    animation systems with the QML content. At the same time this means that
    this signal is not suitable for triggering graphics operations.

    \since 5.3
 */

/*!
    \qmlsignal QtQuick::Window::afterAnimating()

    This signal is emitted on the GUI thread before requesting the render thread to
    perform the synchronization of the scene graph.

    You can implement onAfterAnimating to do additional processing after each animation step.

    \since 5.3
 */

/*!
    \fn void QQuickWindow::sceneGraphAboutToStop()

    This signal is emitted on the render thread when the scene graph is
    about to stop rendering. This happens usually because the window
    has been hidden.

    Applications may use this signal to release resources, but should be
    prepared to reinstantiated them again fast. The scene graph and the
    graphics context are not released at this time.

    \warning This signal is emitted from the scene graph rendering thread. If your
    slot function needs to finish before execution continues, you must make sure that
    the connection is direct (see Qt::ConnectionType).

    \warning Make very sure that a signal handler for sceneGraphAboutToStop() leaves the
    graphics context in the same state as it was when the signal handler was entered.
    Failing to do so can result in the scene not rendering properly.

    \sa sceneGraphInvalidated()
    \since 5.3
 */

/*!
    \qmlsignal QtQuick::Window::sceneGraphAboutToStop()
    \internal
    \since 5.3
 */

/*!
    \overload
 */

QSGTexture *QQuickWindow::createTextureFromImage(const QImage &image) const
{
    return createTextureFromImage(image, {});
}


/*!
    Creates a new QSGTexture from the supplied \a image. If the image has an
    alpha channel, the corresponding texture will have an alpha channel.

    The caller of the function is responsible for deleting the returned texture.
    The underlying native texture object is then destroyed together with the
    QSGTexture.

    When \a options contains TextureCanUseAtlas, the engine may put the image
    into a texture atlas. Textures in an atlas need to rely on
    QSGTexture::normalizedTextureSubRect() for their geometry and will not
    support QSGTexture::Repeat. Other values from CreateTextureOption are
    ignored.

    When \a options contains TextureIsOpaque, the engine will create an RGB
    texture which returns false for QSGTexture::hasAlphaChannel(). Opaque
    textures will in most cases be faster to render. When this flag is not set,
    the texture will have an alpha channel based on the image's format.

    When \a options contains TextureHasMipmaps, the engine will create a texture
    which can use mipmap filtering. Mipmapped textures can not be in an atlas.

    Setting TextureHasAlphaChannel in \a options serves no purpose for this
    function since assuming an alpha channel and blending is the default. To opt
    out, set TextureIsOpaque.

    When the scene graph uses OpenGL, the returned texture will be using \c
    GL_TEXTURE_2D as texture target and \c GL_RGBA as internal format. With
    other graphics APIs, the texture format is typically \c RGBA8. Reimplement
    QSGTexture to create textures with different parameters.

    \warning This function will return 0 if the scene graph has not yet been
    initialized.

    \warning The returned texture is not memory managed by the scene graph and
    must be explicitly deleted by the caller on the rendering thread. This is
    achieved by deleting the texture from a QSGNode destructor or by using
    deleteLater() in the case where the texture already has affinity to the
    rendering thread.

    This function can be called from both the main and the render thread.

    \sa sceneGraphInitialized(), QSGTexture
 */

QSGTexture *QQuickWindow::createTextureFromImage(const QImage &image, CreateTextureOptions options) const
{
    Q_D(const QQuickWindow);
    if (!isSceneGraphInitialized()) // check both for d->context and d->context->isValid()
         return nullptr;
    uint flags = 0;
    if (options & TextureCanUseAtlas)     flags |= QSGRenderContext::CreateTexture_Atlas;
    if (options & TextureHasMipmaps)      flags |= QSGRenderContext::CreateTexture_Mipmap;
    if (!(options & TextureIsOpaque))     flags |= QSGRenderContext::CreateTexture_Alpha;
    return d->context->createTexture(image, flags);
}

/*!
    Creates a new QSGTexture from the supplied \a texture.

    Use \a options to customize the texture attributes. Only the
    TextureHasAlphaChannel flag is taken into account by this function. When
    set, the resulting QSGTexture is always treated by the scene graph renderer
    as needing blending. For textures that are fully opaque, not setting the
    flag can save the cost of performing alpha blending during rendering. The
    flag has no direct correspondence to the \l{QRhiTexture::format()}{format}
    of the QRhiTexture, i.e. not setting the flag while having a texture format
    such as the commonly used \l QRhiTexture::RGBA8 is perfectly normal.

    Mipmapping is not controlled by \a options since \a texture is already
    created and has the presence or lack of mipmaps baked in.

    The returned QSGTexture owns the QRhiTexture, meaning \a texture is
    destroyed together with the returned QSGTexture.

    If \a texture owns its underlying native graphics resources (OpenGL texture
    object, Vulkan image, etc.), that depends on how the QRhiTexture was created
    (\l{QRhiTexture::create()} or \l{QRhiTexture::createFrom()}), and that is
    not controlled or changed by this function.

    \note This is only functional when the scene graph has already initialized
    and is using the default, \l{QRhi}-based \l{Scene Graph
    Adaptations}{adaptation}. The return value is \nullptr otherwise.

    \note This function can only be called on the scene graph render thread.

    \since 6.6

    \sa createTextureFromImage(), sceneGraphInitialized(), QSGTexture
 */
QSGTexture *QQuickWindow::createTextureFromRhiTexture(QRhiTexture *texture, CreateTextureOptions options) const
{
    Q_D(const QQuickWindow);
    if (!d->rhi)
        return nullptr;

    QSGPlainTexture *t = new QSGPlainTexture;
    t->setOwnsTexture(true);
    t->setTexture(texture);
    t->setHasAlphaChannel(options & QQuickWindow::TextureHasAlphaChannel);
    t->setTextureSize(texture->pixelSize());
    return t;
}

// Legacy, private alternative to createTextureFromRhiTexture() that internally
// creates a QRhiTexture wrapping the existing native graphics resource.
// New code should prefer using the public API.
QSGTexture *QQuickWindowPrivate::createTextureFromNativeTexture(quint64 nativeObjectHandle,
                                                                int nativeLayoutOrState,
                                                                uint nativeFormat,
                                                                const QSize &size,
                                                                QQuickWindow::CreateTextureOptions options,
                                                                TextureFromNativeTextureFlags flags) const
{
    if (!rhi)
        return nullptr;

    QSGPlainTexture *texture = new QSGPlainTexture;
    texture->setTextureFromNativeTexture(rhi, nativeObjectHandle, nativeLayoutOrState, nativeFormat,
                                         size, options, flags);
    texture->setHasAlphaChannel(options & QQuickWindow::TextureHasAlphaChannel);
    // note that the QRhiTexture does not (and cannot) own the native object
    texture->setOwnsTexture(true); // texture meaning the QRhiTexture here, not the native object
    texture->setTextureSize(size);
    return texture;
}

/*!
    \qmlproperty color Window::color

    The background color for the window.

    Setting this property is more efficient than using a separate Rectangle.

    \note If you set the color to \c "transparent" or to a color with alpha translucency,
    you should also set suitable \l flags such as \c {flags: Qt.FramelessWindowHint}.
    Otherwise, window translucency may not be enabled consistently on all platforms.
*/

/*!
    \property QQuickWindow::color
    \brief The color used to clear the color buffer at the beginning of each frame.

    By default, the clear color is white.

    \sa setDefaultAlphaBuffer()
 */

void QQuickWindow::setColor(const QColor &color)
{
    Q_D(QQuickWindow);
    if (color == d->clearColor)
        return;

    if (color.alpha() != d->clearColor.alpha()) {
        QSurfaceFormat fmt = requestedFormat();
        if (color.alpha() < 255)
            fmt.setAlphaBufferSize(8);
        else
            fmt.setAlphaBufferSize(-1);
        setFormat(fmt);
    }
    d->clearColor = color;
    emit colorChanged(color);
    update();
}

QColor QQuickWindow::color() const
{
    return d_func()->clearColor;
}

/*!
    \brief Returns whether to use alpha transparency on newly created windows.

    \since 5.1
    \sa setDefaultAlphaBuffer()
 */
bool QQuickWindow::hasDefaultAlphaBuffer()
{
    return QQuickWindowPrivate::defaultAlphaBuffer;
}

/*!
    \brief \a useAlpha specifies whether to use alpha transparency on newly created windows.
    \since 5.1

    In any application which expects to create translucent windows, it's necessary to set
    this to true before creating the first QQuickWindow. The default value is false.

    \sa hasDefaultAlphaBuffer()
 */
void QQuickWindow::setDefaultAlphaBuffer(bool useAlpha)
{
    QQuickWindowPrivate::defaultAlphaBuffer = useAlpha;
}

/*!
    \struct QQuickWindow::GraphicsStateInfo
    \inmodule QtQuick
    \since 5.14

    \brief Describes some of the RHI's graphics state at the point of a
    \l{QQuickWindow::beginExternalCommands()}{beginExternalCommands()} call.
 */

/*!
    \variable QQuickWindow::GraphicsStateInfo::currentFrameSlot
    \since 5.14
    \brief the current frame slot index while recording a frame.

    When the scenegraph renders with lower level 3D APIs such as Vulkan or
    Metal, it is the Qt's responsibility to ensure blocking whenever starting a
    new frame and finding the CPU is already a certain number of frames ahead
    of the GPU (because the command buffer submitted in frame no. \c{current} -
    \c{FramesInFlight} has not yet completed). With other graphics APIs, such
    as OpenGL or Direct 3D 11 this level of control is not exposed to the API
    client but rather handled by the implementation of the graphics API.

    By extension, this also means that the appropriate double (or triple)
    buffering of resources, such as buffers, is up to the graphics API client
    to manage. Most commonly, a uniform buffer where the data changes between
    frames cannot simply change its contents when submitting a frame, given
    that the frame may still be active ("in flight") when starting to record
    the next frame. To avoid stalling the pipeline, one way is to have multiple
    buffers (and memory allocations) under the hood, thus realizing at least a
    double buffered scheme for such resources.

    Applications that integrate rendering done directly with a graphics API
    such as Vulkan may want to perform a similar double or triple buffering of
    their own graphics resources, in a way that is compatible with the Qt
    rendering engine's frame submission process. That then involves knowing the
    values for the maximum number of in-flight frames (which is typically 2 or
    3) and the current frame slot index, which is a number running 0, 1, ..,
    FramesInFlight-1, and then wrapping around. The former is exposed in the
    \l{QQuickWindow::GraphicsStateInfo::framesInFlight}{framesInFlight}
    variable. The latter, current index, is this value.

    For an example of using these values in practice, refer to the {Scene Graph
    - Vulkan Under QML} and {Scene Graph - Vulkan Texture Import} examples.
 */

/*!
    \variable QQuickWindow::GraphicsStateInfo::framesInFlight
    \since 5.14
    \brief the maximum number of frames kept in flight.

    See \l{QQuickWindow::GraphicsStateInfo::currentFrameSlot}{currentFrameSlot}
    for a detailed description.
 */

/*!
    \return a reference to a GraphicsStateInfo struct describing some of the
    RHI's internal state, in particular, the double or tripple buffering status
    of the backend (such as, the Vulkan or Metal integrations). This is
    relevant when the underlying graphics APIs is Vulkan or Metal, and the
    external rendering code wishes to perform double or tripple buffering of
    its own often-changing resources, such as, uniform buffers, in order to
    avoid stalling the pipeline.
 */
const QQuickWindow::GraphicsStateInfo &QQuickWindow::graphicsStateInfo()
{
    Q_D(QQuickWindow);
    if (d->rhi) {
        d->rhiStateInfo.currentFrameSlot = d->rhi->currentFrameSlot();
        d->rhiStateInfo.framesInFlight = d->rhi->resourceLimit(QRhi::FramesInFlight);
    }
    return d->rhiStateInfo;
}

/*!
    When mixing raw graphics (OpenGL, Vulkan, Metal, etc.) commands with scene
    graph rendering, it is necessary to call this function before recording
    commands to the command buffer used by the scene graph to render its main
    render pass. This is to avoid clobbering state.

    In practice this function is often called from a slot connected to the
    beforeRenderPassRecording() or afterRenderPassRecording() signals.

    The function does not need to be called when recording commands to the
    application's own command buffer (such as, a VkCommandBuffer or
    MTLCommandBuffer + MTLRenderCommandEncoder created and managed by the
    application, not retrieved from the scene graph). With graphics APIs where
    no native command buffer concept is exposed (OpenGL, Direct 3D 11),
    beginExternalCommands() and endExternalCommands() together provide a
    replacement for the Qt 5 resetOpenGLState() function.

    Calling this function and endExternalCommands() is not necessary within the
    \l{QSGRenderNode::render()}{render()} implementation of a QSGRenderNode
    because the scene graph performs the necessary steps implicitly for render
    nodes.

    Native graphics objects (such as, graphics device, command buffer or
    encoder) are accessible via QSGRendererInterface::getResource().

    \warning Watch out for the fact that
    QSGRendererInterface::CommandListResource may return a different object
    between beginExternalCommands() - endExternalCommands(). This can happen
    when the underlying implementation provides a dedicated secondary command
    buffer for recording external graphics commands within a render pass.
    Therefore, always query CommandListResource after calling this function. Do
    not attempt to reuse an object from an earlier query.

    \note When the scenegraph is using OpenGL, pay attention to the fact that
    the OpenGL state in the context can have arbitrary settings, and this
    function does not perform any resetting of the state back to defaults.

    \sa endExternalCommands(), QQuickOpenGLUtils::resetOpenGLState()

    \since 5.14
 */
void QQuickWindow::beginExternalCommands()
{
    Q_D(QQuickWindow);
    if (d->rhi && d->context && d->context->isValid()) {
        QSGDefaultRenderContext *rc = static_cast<QSGDefaultRenderContext *>(d->context);
        QRhiCommandBuffer *cb = rc->currentFrameCommandBuffer();
        if (cb)
            cb->beginExternal();
    }
}

/*!
    When mixing raw graphics (OpenGL, Vulkan, Metal, etc.) commands with scene
    graph rendering, it is necessary to call this function after recording
    commands to the command buffer used by the scene graph to render its main
    render pass. This is to avoid clobbering state.

    In practice this function is often called from a slot connected to the
    beforeRenderPassRecording() or afterRenderPassRecording() signals.

    The function does not need to be called when recording commands to the
    application's own command buffer (such as, a VkCommandBuffer or
    MTLCommandBuffer + MTLRenderCommandEncoder created and managed by the
    application, not retrieved from the scene graph). With graphics APIs where
    no native command buffer concept is exposed (OpenGL, Direct 3D 11),
    beginExternalCommands() and endExternalCommands() together provide a
    replacement for the Qt 5 resetOpenGLState() function.

    Calling this function and beginExternalCommands() is not necessary within the
    \l{QSGRenderNode::render()}{render()} implementation of a QSGRenderNode
    because the scene graph performs the necessary steps implicitly for render
    nodes.

    \sa beginExternalCommands(), QQuickOpenGLUtils::resetOpenGLState()

    \since 5.14
 */
void QQuickWindow::endExternalCommands()
{
    Q_D(QQuickWindow);
    if (d->rhi && d->context && d->context->isValid()) {
        QSGDefaultRenderContext *rc = static_cast<QSGDefaultRenderContext *>(d->context);
        QRhiCommandBuffer *cb = rc->currentFrameCommandBuffer();
        if (cb)
            cb->endExternal();
    }
}

/*!
    \qmlproperty string Window::title

    The window's title in the windowing system.

    The window title might appear in the title area of the window decorations,
    depending on the windowing system and the window flags. It might also
    be used by the windowing system to identify the window in other contexts,
    such as in the task switcher.
 */

/*!
    \qmlproperty Qt::WindowModality Window::modality

    The modality of the window.

    A modal window prevents other windows from receiving input events.
    Possible values are Qt.NonModal (the default), Qt.WindowModal,
    and Qt.ApplicationModal.
 */

/*!
    \qmlproperty Qt::WindowFlags Window::flags

    The window flags of the window.

    The window flags control the window's appearance in the windowing system,
    whether it's a dialog, popup, or a regular window, and whether it should
    have a title bar, etc.

    The flags that you read from this property might differ from the ones
    that you set if the requested flags could not be fulfilled.

    \snippet qml/splashWindow.qml entire

    \sa Qt::WindowFlags, {Qt Quick Examples - Window and Screen}
 */

/*!
    \qmlattachedproperty Window Window::window
    \since 5.7

    This attached property holds the item's window.
    The Window attached property can be attached to any Item.
*/

/*!
    \qmlattachedproperty int Window::width
    \qmlattachedproperty int Window::height
    \since 5.5

    These attached properties hold the size of the item's window.
    The Window attached property can be attached to any Item.
*/

/*!
    \qmlproperty int Window::x
    \qmlproperty int Window::y
    \qmlproperty int Window::width
    \qmlproperty int Window::height

    Defines the window's position and size.

    The (x,y) position is relative to the \l Screen if there is only one,
    or to the virtual desktop (arrangement of multiple screens).

    \note Not all windowing systems support setting or querying top level
    window positions. On such a system, programmatically moving windows
    may not have any effect, and artificial values may be returned for
    the current positions, such as \c QPoint(0, 0).

    \qml
    Window { x: 100; y: 100; width: 100; height: 100 }
    \endqml

    \image screen-and-window-dimensions.jpg
 */

/*!
    \qmlproperty int Window::minimumWidth
    \qmlproperty int Window::minimumHeight
    \since 5.1

    Defines the window's minimum size.

    This is a hint to the window manager to prevent resizing below the specified
    width and height.
 */

/*!
    \qmlproperty int Window::maximumWidth
    \qmlproperty int Window::maximumHeight
    \since 5.1

    Defines the window's maximum size.

    This is a hint to the window manager to prevent resizing above the specified
    width and height.
 */

/*!
    \qmlproperty bool Window::visible

    Whether the window is visible on the screen.

    Setting visible to false is the same as setting \l visibility to \l {QWindow::}{Hidden}.

    The default value is \c false, unless overridden by setting \l visibility.

    \sa visibility
 */

/*!
    \keyword qml-window-visibility-prop
    \qmlproperty QWindow::Visibility Window::visibility

    The screen-occupation state of the window.

    Visibility is whether the window should appear in the windowing system as
    normal, minimized, maximized, fullscreen or hidden.

    To set the visibility to \l {QWindow::}{AutomaticVisibility} means to give the
    window a default visible state, which might be \l {QWindow::}{FullScreen} or
    \l {QWindow::}{Windowed} depending on the platform. However when reading the
    visibility property you will always get the actual state, never
    \c AutomaticVisibility.

    When a window is not \l visible, its visibility is \c Hidden.
    Setting visibility to \l {QWindow::}{Hidden} is the same as setting \l visible to \c false.

    The default value is \l {QWindow::}{Hidden}

    \snippet qml/windowVisibility.qml entire

    \sa visible, {Qt Quick Examples - Window and Screen}
    \since 5.1
 */

/*!
    \qmlattachedproperty QWindow::Visibility Window::visibility
    \readonly
    \since 5.4

    This attached property holds whether the window is currently shown
    in the windowing system as normal, minimized, maximized, fullscreen or
    hidden. The \c Window attached property can be attached to any Item. If the
    item is not shown in any window, the value will be \l {QWindow::}{Hidden}.

    \sa visible, {qml-window-visibility-prop}{visibility}
*/

/*!
    \qmlproperty Item Window::contentItem
    \readonly
    \brief The invisible root item of the scene.
*/

/*!
    \qmlproperty Qt::ScreenOrientation Window::contentOrientation

    This is a hint to the window manager in case it needs to display
    additional content like popups, dialogs, status bars, or similar
    in relation to the window.

    The recommended orientation is \l {Screen::orientation}{Screen.orientation}, but
    an application doesn't have to support all possible orientations,
    and thus can opt to ignore the current screen orientation.

    The difference between the window and the content orientation
    determines how much to rotate the content by.

    The default value is Qt::PrimaryOrientation.

    \sa Screen

    \since 5.1
 */

/*!
    \qmlproperty real Window::opacity

    The opacity of the window.

    If the windowing system supports window opacity, this can be used to fade the
    window in and out, or to make it semitransparent.

    A value of 1.0 or above is treated as fully opaque, whereas a value of 0.0 or below
    is treated as fully transparent. Values inbetween represent varying levels of
    translucency between the two extremes.

    The default value is 1.0.

    \since 5.1
 */

/*!
    \qmlproperty variant Window::screen

    The screen with which the window is associated.

    If specified before showing a window, will result in the window being shown
    on that screen, unless an explicit window position has been set. The value
    must be an element from the Qt.application.screens array.

    \note To ensure that the window is associated with the desired screen when
    the underlying native window is created, make sure this property is set as
    early as possible and that the setting of its value is not deferred. This
    can be particularly important on embedded platforms without a windowing system,
    where only one window per screen is allowed at a time. Setting the screen after
    a window has been created does not move the window if the new screen is part of
    the same virtual desktop as the old screen.

    \since 5.9

    \sa QWindow::setScreen(), QWindow::screen(), QScreen, {QtQml::Qt::application}{Qt.application}
 */

/*!
    \qmlproperty QWindow Window::transientParent
    \since 5.13

    The window for which this window is a transient pop-up.

    This is a hint to the window manager that this window is a dialog or pop-up
    on behalf of the transient parent. It usually means that the transient
    window will be centered over its transient parent when it is initially
    shown, that minimizing the parent window will also minimize the transient
    window, and so on; however results vary somewhat from platform to platform.

    Declaring a Window inside an Item or another Window, either via the
    \l{Window::data}{default property} or a dedicated property, will automatically
    set up a transient parent relationship to the containing window,
    unless the \l transientParent property is explicitly set. This applies
    when creating Window items via \l [QML] {QtQml::Qt::createComponent()}
    {Qt.createComponent} or \l [QML] {QtQml::Qt::createQmlObject()}
    {Qt.createQmlObject} as well, as long as an Item or Window is passed
    as the \c parent argument.

    A Window with a transient parent will not be shown until its transient
    parent is shown, even if the \l visible property is \c true. This also
    applies for the automatic transient parent relationship described above.
    In particular, if the Window's containing element is an Item, the window
    will not be shown until the containing item is added to a scene, via its
    \l{Concepts - Visual Parent in Qt Quick}{visual parent hierarchy}. Setting
    the \l transientParent to \c null will override this behavior:

    \snippet qml/nestedWindowTransientParent.qml 0
    \snippet qml/nestedWindowTransientParent.qml 1

    In order to cause the window to be centered above its transient parent by
    default, depending on the window manager, it may also be necessary to set
    the \l Window::flags property with a suitable \l Qt::WindowType (such as
    \c Qt::Dialog).

    \sa {QQuickWindow::}{parent()}
*/

/*!
    \property QQuickWindow::transientParent
    \brief The window for which this window is a transient pop-up.
    \since 5.13

    This is a hint to the window manager that this window is a dialog or pop-up
    on behalf of the transient parent, which may be any kind of \l QWindow.

    In order to cause the window to be centered above its transient parent by
    default, depending on the window manager, it may also be necessary to set
    the \l flags property with a suitable \l Qt::WindowType (such as \c Qt::Dialog).

    \sa parent()
 */

/*!
    \qmlproperty Item Window::activeFocusItem
    \since 5.1

    The item which currently has active focus or \c null if there is
    no item with active focus.
 */

/*!
    \qmlattachedproperty Item Window::activeFocusItem
    \since 5.4

    This attached property holds the item which currently has active focus or
    \c null if there is no item with active focus. The Window attached property
    can be attached to any Item.
*/

/*!
    \qmlproperty bool Window::active
    \since 5.1

    The active status of the window.

    \snippet qml/windowPalette.qml declaration-and-color
    \snippet qml/windowPalette.qml closing-brace

    \sa requestActivate()
 */

/*!
    \qmlattachedproperty bool Window::active
    \since 5.4

    This attached property tells whether the window is active. The Window
    attached property can be attached to any Item.

    Here is an example which changes a label to show the active state of the
    window in which it is shown:

    \snippet qml/windowActiveAttached.qml entire
*/

/*!
    \qmlmethod QtQuick::Window::requestActivate()
    \since 5.1

    Requests the window to be activated, i.e. receive keyboard focus.
 */

/*!
    \qmlmethod QtQuick::Window::alert(int msec)
    \since 5.1

    Causes an alert to be shown for \a msec milliseconds. If \a msec is \c 0
    (the default), then the alert is shown indefinitely until the window
    becomes active again.

    In alert state, the window indicates that it demands attention, for example
    by flashing or bouncing the taskbar entry.
*/

/*!
    \qmlmethod QtQuick::Window::close()

    Closes the window.

    When this method is called, or when the user tries to close the window by
    its title bar button, the \l closing signal will be emitted. If there is no
    handler, or the handler does not revoke permission to close, the window
    will subsequently close. If the QGuiApplication::quitOnLastWindowClosed
    property is \c true, and there are no other windows open, the application
    will quit.
*/

/*!
    \qmlmethod QtQuick::Window::raise()

    Raises the window in the windowing system.

    Requests that the window be raised to appear above other windows.
*/

/*!
    \qmlmethod QtQuick::Window::lower()

    Lowers the window in the windowing system.

    Requests that the window be lowered to appear below other windows.
*/

/*!
    \qmlmethod QtQuick::Window::show()

    Shows the window.

    This is equivalent to calling showFullScreen(), showMaximized(), or showNormal(),
    depending on the platform's default behavior for the window type and flags.

    \sa showFullScreen(), showMaximized(), showNormal(), hide(), QQuickItem::flags()
*/

/*!
    \qmlmethod QtQuick::Window::hide()

    Hides the window.

    Equivalent to setting \l visible to \c false or \l visibility to \l {QWindow::}{Hidden}.

    \sa show()
*/

/*!
    \qmlmethod QtQuick::Window::showMinimized()

    Shows the window as minimized.

    Equivalent to setting \l visibility to \l {QWindow::}{Minimized}.
*/

/*!
    \qmlmethod QtQuick::Window::showMaximized()

    Shows the window as maximized.

    Equivalent to setting \l visibility to \l {QWindow::}{Maximized}.
*/

/*!
    \qmlmethod QtQuick::Window::showFullScreen()

    Shows the window as fullscreen.

    Equivalent to setting \l visibility to \l {QWindow::}{FullScreen}.
*/

/*!
    \qmlmethod QtQuick::Window::showNormal()

    Shows the window as normal, i.e. neither maximized, minimized, nor fullscreen.

    Equivalent to setting \l visibility to \l {QWindow::}{Windowed}.
*/

/*!
    \enum QQuickWindow::RenderStage
    \since 5.4

    \value BeforeSynchronizingStage Before synchronization.
    \value AfterSynchronizingStage After synchronization.
    \value BeforeRenderingStage Before rendering.
    \value AfterRenderingStage After rendering.
    \value AfterSwapStage After the frame is swapped.
    \value NoStage As soon as possible. This value was added in Qt 5.6.

    \sa {Scene Graph and Rendering}
 */

/*!
    \since 5.4

    Schedules \a job to run when the rendering of this window reaches
    the given \a stage.

    This is a convenience to the equivalent signals in QQuickWindow for
    "one shot" tasks.

    The window takes ownership over \a job and will delete it when the
    job is completed.

    If rendering is shut down before \a job has a chance to run, the
    job will be run and then deleted as part of the scene graph cleanup.
    If the window is never shown and no rendering happens before the QQuickWindow
    is destroyed, all pending jobs will be destroyed without their run()
    method being called.

    If the rendering is happening on a different thread, then the job
    will happen on the rendering thread.

    If \a stage is \l NoStage, \a job will be run at the earliest opportunity
    whenever the render thread is not busy rendering a frame. If the window is
    not exposed, and is not renderable, at the time the job is either posted or
    handled, the job is deleted without executing the run() method.  If a
    non-threaded renderer is in use, the run() method of the job is executed
    synchronously. When rendering with OpenGL, the OpenGL context is changed to
    the renderer's context before executing any job, including \l NoStage jobs.

    \note This function does not trigger rendering; the jobs targeting any other
    stage than NoStage will be stored run until rendering is triggered elsewhere.
    To force the job to run earlier, call QQuickWindow::update();

    \sa beforeRendering(), afterRendering(), beforeSynchronizing(),
    afterSynchronizing(), frameSwapped(), sceneGraphInvalidated()
 */

void QQuickWindow::scheduleRenderJob(QRunnable *job, RenderStage stage)
{
    Q_D(QQuickWindow);

    d->renderJobMutex.lock();
    if (stage == BeforeSynchronizingStage) {
        d->beforeSynchronizingJobs << job;
    } else if (stage == AfterSynchronizingStage) {
        d->afterSynchronizingJobs << job;
    } else if (stage == BeforeRenderingStage) {
        d->beforeRenderingJobs << job;
    } else if (stage == AfterRenderingStage) {
        d->afterRenderingJobs << job;
    } else if (stage == AfterSwapStage) {
        d->afterSwapJobs << job;
    } else if (stage == NoStage) {
        if (d->renderControl && d->rhi && d->rhi->thread() == QThread::currentThread()) {
            job->run();
            delete job;
        } else if (isExposed()) {
            d->windowManager->postJob(this, job);
        } else {
            delete job;
        }
    }
    d->renderJobMutex.unlock();
}

void QQuickWindowPrivate::runAndClearJobs(QList<QRunnable *> *jobs)
{
    renderJobMutex.lock();
    QList<QRunnable *> jobList = *jobs;
    jobs->clear();
    renderJobMutex.unlock();

    for (QRunnable *r : std::as_const(jobList)) {
        r->run();
        delete r;
    }
}

void QQuickWindow::runJobsAfterSwap()
{
    Q_D(QQuickWindow);
    d->runAndClearJobs(&d->afterSwapJobs);
}

/*!
    Returns the device pixel ratio for this window.

    This is different from QWindow::devicePixelRatio() in that it supports
    redirected rendering via QQuickRenderControl and QQuickRenderTarget. When
    using a QQuickRenderControl, the QQuickWindow is often not fully created,
    meaning it is never shown and there is no underlying native window created
    in the windowing system. As a result, querying properties like the device
    pixel ratio cannot give correct results. This function takes into account
    both QQuickRenderControl::renderWindowFor() and
    QQuickRenderTarget::devicePixelRatio(). When no redirection is in effect,
    the result is same as QWindow::devicePixelRatio().

    \sa QQuickRenderControl, QQuickRenderTarget, setRenderTarget(), QWindow::devicePixelRatio()
 */
qreal QQuickWindow::effectiveDevicePixelRatio() const
{
    Q_D(const QQuickWindow);
    QWindow *w = QQuickRenderControl::renderWindowFor(const_cast<QQuickWindow *>(this));
    if (w)
        return w->devicePixelRatio();

    if (!d->customRenderTarget.isNull())
        return d->customRenderTarget.devicePixelRatio();

    return devicePixelRatio();
}

/*!
    \return the current renderer interface. The value is always valid and is never null.

    \note This function can be called at any time after constructing the
    QQuickWindow, even while isSceneGraphInitialized() is still false. However,
    some renderer interface functions, in particular
    QSGRendererInterface::getResource() will not be functional until the
    scenegraph is up and running. Backend queries, like
    QSGRendererInterface::graphicsApi() or QSGRendererInterface::shaderType(),
    will always be functional on the other hand.

    \note The ownership of the returned pointer stays with Qt. The returned
    instance may or may not be shared between different QQuickWindow instances,
    depending on the scenegraph backend in use. Therefore applications are
    expected to query the interface object for each QQuickWindow instead of
    reusing the already queried pointer.

    \sa QSGRenderNode, QSGRendererInterface

    \since 5.8
 */
QSGRendererInterface *QQuickWindow::rendererInterface() const
{
    Q_D(const QQuickWindow);

    // no context validity check - it is essential to be able to return a
    // renderer interface instance before scenegraphInitialized() is emitted
    // (depending on the backend, that can happen way too late for some of the
    // rif use cases, like examining the graphics api or shading language in
    // use)

    return d->context->sceneGraphContext()->rendererInterface(d->context);
}

/*!
    \return the QRhi object used by this window for rendering.

    Available only when the window is using Qt's 3D API and shading language
    abstractions, meaning the result is always null when using the \c software
    adaptation.

    The result is valid only when rendering has been initialized, which is
    indicated by the emission of the sceneGraphInitialized() signal. Before
    that point, the returned value is null. With a regular, on-screen
    QQuickWindow scenegraph initialization typically happens when the native
    window gets exposed (shown) the first time. When using QQuickRenderControl,
    initialization is done in the explicit
    \l{QQuickRenderControl::initialize()}{initialize()} call.

    In practice this function is a shortcut to querying the QRhi via the
    QSGRendererInterface.

    \since 6.6
 */
QRhi *QQuickWindow::rhi() const
{
    Q_D(const QQuickWindow);
    return d->rhi;
}

/*!
    \return the QRhiSwapChain used by this window, if there is one.

    \note Only on-screen windows backed by one of the standard render loops
    (such as, \c basic or \c threaded) will have a swapchain. Otherwise the
    returned value is null. For example, the result is always null when the
    window is used with QQuickRenderControl.

    \since 6.6
 */
QRhiSwapChain *QQuickWindow::swapChain() const
{
    Q_D(const QQuickWindow);
    return d->swapchain;
}

/*!
    Requests the specified graphics \a api.

    When the built-in, default graphics adaptation is used, \a api specifies
    which graphics API (OpenGL, Vulkan, Metal, or Direct3D) the scene graph
    should use to render. In addition, the \c software backend is built-in as
    well, and can be requested by setting \a api to
    QSGRendererInterface::Software.

    Unlike setSceneGraphBackend(), which can only be used to request a given
    backend (shipped either built-in or installed as dynamically loaded
    plugins), this function works with the higher level concept of graphics
    APIs. It covers the backends that ship with Qt Quick, and thus have
    corresponding values in the QSGRendererInterface::GraphicsApi enum.

    When this function is not called at all, and the equivalent environment
    variable \c{QSG_RHI_BACKEND} is not set either, the scene graph will choose
    the graphics API to use based on the platform.

    This function becomes important in applications that are only prepared for
    rendering with a given API. For example, if there is native OpenGL or
    Vulkan rendering done by the application, it will want to ensure Qt Quick
    is rendering using OpenGL or Vulkan too. Such applications are expected to
    call this function early in their main() function.

    \note The call to the function must happen before constructing the first
    QQuickWindow in the application. The graphics API cannot be changed
    afterwards.

    \note When used in combination with QQuickRenderControl, this rule is
    relaxed: it is possible to change the graphics API, but only when all
    existing QQuickRenderControl and QQuickWindow instances have been
    destroyed.

    To query what graphics API the scene graph is using to render,
    QSGRendererInterface::graphicsApi() after the scene graph
    \l{QQuickWindow::isSceneGraphInitialized()}{has initialized}, which
    typically happens either when the window becomes visible for the first time, or
    when QQuickRenderControl::initialize() is called.

    To switch back to the default behavior, where the scene graph chooses a
    graphics API based on the platform and other conditions, set \a api to
    QSGRendererInterface::Unknown.

    \since 6.0
 */
void QQuickWindow::setGraphicsApi(QSGRendererInterface::GraphicsApi api)
{
    // Special cases: these are different scenegraph backends.
    switch (api) {
    case QSGRendererInterface::Software:
        setSceneGraphBackend(QStringLiteral("software"));
        break;
    case QSGRendererInterface::OpenVG:
        setSceneGraphBackend(QStringLiteral("openvg"));
        break;
    default:
        break;
    }

    // Standard case: tell the QRhi-based default adaptation what graphics api
    // (QRhi backend) to use.
    if (QSGRendererInterface::isApiRhiBased(api) || api == QSGRendererInterface::Unknown)
        QSGRhiSupport::instance_internal()->configure(api);
}

/*!
    \return the graphics API that would be used by the scene graph if it was
    initialized at this point in time.

    The standard way to query the API used by the scene graph is to use
    QSGRendererInterface::graphicsApi() once the scene graph has initialized,
    for example when or after the sceneGraphInitialized() signal is emitted. In
    that case one gets the true, real result, because then it is known that
    everything was initialized correctly using that graphics API.

    This is not always convenient. If the application needs to set up external
    frameworks, or needs to work with setGraphicsDevice() in a manner that
    depends on the scene graph's built in API selection logic, it is not always
    feasiable to defer such operations until after the QQuickWindow has been
    made visible or QQuickRenderControl::initialize() has been called.

    Therefore, this static function is provided as a counterpart to
    setGraphicsApi(): it can be called at any time, and the result reflects
    what API the scene graph would choose if it was initialized at the point of
    the call.

    \note This static function is intended to be called on the main (GUI)
    thread only. For querying the API when rendering, use QSGRendererInterface
    since that object lives on the render thread.

    \note This function does not take scene graph backends into account.

    \since 6.0
 */
QSGRendererInterface::GraphicsApi QQuickWindow::graphicsApi()
{
    // Note that this applies the settings e.g. from the env vars
    // (QSG_RHI_BACKEND) if it was not done at least once already. Whereas if
    // setGraphicsApi() was called before, or the scene graph is already
    // initialized, then this is just a simple query.
    return QSGRhiSupport::instance()->graphicsApi();
}

/*!
    Requests a Qt Quick scenegraph \a backend. Backends can either be built-in
    or be installed in form of dynamically loaded plugins.

    \overload

    \note The call to the function must happen before constructing the first
    QQuickWindow in the application. It cannot be changed afterwards.

    See \l{Switch Between Adaptations in Your Application} for more information
    about the list of backends. If \a backend is invalid or an error occurs, the
    request is ignored.

    \note Calling this function is equivalent to setting the
    \c QT_QUICK_BACKEND or \c QMLSCENE_DEVICE environment variables. However, this
    API is safer to use in applications that spawn other processes as there is
    no need to worry about environment inheritance.

    \since 5.8
 */
void QQuickWindow::setSceneGraphBackend(const QString &backend)
{
    QSGContext::setBackend(backend);
}

/*!
    Returns the requested Qt Quick scenegraph backend.

    \note The return value of this function may still be outdated by
    subsequent calls to setSceneGraphBackend() until the first QQuickWindow in the
    application has been constructed.

    \note The value only reflects the request in the \c{QT_QUICK_BACKEND}
    environment variable after a QQuickWindow has been constructed.

    \since 5.9
 */
QString QQuickWindow::sceneGraphBackend()
{
    return QSGContext::backend();
}

/*!
    Sets the graphics device objects for this window. The scenegraph will use
    existing device, physical device, and other objects specified by \a device
    instead of creating new ones.

    This function is very often used in combination with QQuickRenderControl
    and setRenderTarget(), in order to redirect Qt Quick rendering into a
    texture.

    A default constructed QQuickGraphicsDevice does not change the default
    behavior in any way. Once a \a device created via one of the
    QQuickGraphicsDevice factory functions, such as,
    QQuickGraphicsDevice::fromDeviceObjects(), is passed in, and the scenegraph
    uses a matching graphics API (with the example of fromDeviceObjects(), that
    would be Vulkan), the scenegraph will use the existing device objects (such
    as, the \c VkPhysicalDevice, \c VkDevice, and graphics queue family index,
    in case of Vulkan) encapsulated by the QQuickGraphicsDevice. This allows
    using the same device, and so sharing resources, such as buffers and
    textures, between Qt Quick and native rendering engines.

    \warning This function can only be called before initializing the
    scenegraph and will have no effect if called afterwards. In practice this
    typically means calling it right before QQuickRenderControl::initialize().

    As an example, this time with Direct3D, the typical usage is expected to be
    the following:

    \badcode
        // native graphics resources set up by a custom D3D rendering engine
        ID3D11Device *device;
        ID3D11DeviceContext *context;
        ID3D11Texture2D *texture;
        ...
        // now to redirect Qt Quick content into 'texture' we could do the following:
        QQuickRenderControl *renderControl = new QQuickRenderControl;
        QQuickWindow *window = new QQuickWindow(renderControl); // this window will never be shown on-screen
        ...
        window->setGraphicsDevice(QQuickGraphicsDevice::fromDeviceAndContext(device, context));
        renderControl->initialize();
        window->setRenderTarget(QQuickRenderTarget::fromD3D11Texture(texture, textureSize);
        ...
    \endcode

    The key aspect of using this function is to ensure that resources or
    handles to resources, such as \c texture in the above example, are visible
    to and usable by both the external rendering engine and the scenegraph
    renderer. This requires using the same graphics device (or with OpenGL,
    OpenGL context).

    QQuickGraphicsDevice instances are implicitly shared, copyable, and
    can be passed by value. They do not own the associated native objects (such
    as, the ID3D11Device in the example).

    \note Using QQuickRenderControl does not always imply having to call this
    function. When adopting an existing device or context is not needed, this
    function should not be called, and the scene graph will then initialize its
    own devices and contexts normally, just as it would with an on-screen
    QQuickWindow.

    \since 6.0

    \sa QQuickRenderControl, setRenderTarget(), setGraphicsApi()
 */
void QQuickWindow::setGraphicsDevice(const QQuickGraphicsDevice &device)
{
    Q_D(QQuickWindow);
    d->customDeviceObjects = device;
}

/*!
    \return the QQuickGraphicsDevice passed to setGraphicsDevice(), or a
    default constructed one otherwise

    \since 6.0

    \sa setGraphicsDevice()
 */
QQuickGraphicsDevice QQuickWindow::graphicsDevice() const
{
    Q_D(const QQuickWindow);
    return d->customDeviceObjects;
}

/*!
    Sets the graphics configuration for this window. \a config contains various
    settings that may be taken into account by the scene graph when
    initializing the underlying graphics devices and contexts.

    Such additional configuration, specifying for example what device
    extensions to enable for Vulkan, becomes relevant and essential when
    integrating native graphics rendering code that relies on certain
    extensions. The same is true when integrating with an external 3D or VR
    engines, such as OpenXR.

    \note The configuration is ignored when adopting existing graphics devices
    via setGraphicsDevice() since the scene graph is then not in control of the
    actual construction of those objects.

    QQuickGraphicsConfiguration instances are implicitly shared, copyable, and
    can be passed by value.

    \warning Setting a QQuickGraphicsConfiguration on a QQuickWindow must
    happen early enough, before the scene graph is initialized for the first
    time for that window. With on-screen windows this means the call must be
    done before invoking show() on the QQuickWindow or QQuickView. With
    QQuickRenderControl the configuration must be finalized before calling
    \l{QQuickRenderControl::initialize()}{initialize()}.

    \since 6.0
  */
void QQuickWindow::setGraphicsConfiguration(const QQuickGraphicsConfiguration &config)
{
    Q_D(QQuickWindow);
    d->graphicsConfig = config;
}

/*!
    \return the QQuickGraphicsConfiguration passed to
    setGraphicsConfiguration(), or a default constructed one otherwise.

    \since 6.0

    \sa setGraphicsConfiguration()
 */
QQuickGraphicsConfiguration QQuickWindow::graphicsConfiguration() const
{
    Q_D(const QQuickWindow);
    return d->graphicsConfig;
}

/*!
    Creates a text node. When the scenegraph is not initialized, the return value is null.

    \since 6.7
    \sa QSGTextNode
 */
QSGTextNode *QQuickWindow::createTextNode() const
{
    Q_D(const QQuickWindow);
    return isSceneGraphInitialized() ? d->context->sceneGraphContext()->createTextNode(d->context) : nullptr;
}

/*!
    Creates a simple rectangle node. When the scenegraph is not initialized, the return value is null.

    This is cross-backend alternative to constructing a QSGSimpleRectNode directly.

    \since 5.8
    \sa QSGRectangleNode
 */
QSGRectangleNode *QQuickWindow::createRectangleNode() const
{
    Q_D(const QQuickWindow);
    return isSceneGraphInitialized() ? d->context->sceneGraphContext()->createRectangleNode() : nullptr;
}

/*!
    Creates a simple image node. When the scenegraph is not initialized, the return value is null.

    This is cross-backend alternative to constructing a QSGSimpleTextureNode directly.

    \since 5.8
    \sa QSGImageNode
 */
QSGImageNode *QQuickWindow::createImageNode() const
{
    Q_D(const QQuickWindow);
    return isSceneGraphInitialized() ? d->context->sceneGraphContext()->createImageNode() : nullptr;
}

/*!
    Creates a nine patch node. When the scenegraph is not initialized, the return value is null.

    \since 5.8
 */
QSGNinePatchNode *QQuickWindow::createNinePatchNode() const
{
    Q_D(const QQuickWindow);
    return isSceneGraphInitialized() ? d->context->sceneGraphContext()->createNinePatchNode() : nullptr;
}

/*!
    \since 5.10

    Returns the render type of text-like elements in Qt Quick.
    The default is QQuickWindow::QtTextRendering.

    \sa setTextRenderType()
*/
QQuickWindow::TextRenderType QQuickWindow::textRenderType()
{
    return QQuickWindowPrivate::textRenderType;
}

/*!
    \since 5.10

    Sets the default render type of text-like elements in Qt Quick to \a renderType.

    \note setting the render type will only affect elements created afterwards;
    the render type of existing elements will not be modified.

    \sa textRenderType()
*/
void QQuickWindow::setTextRenderType(QQuickWindow::TextRenderType renderType)
{
    QQuickWindowPrivate::textRenderType = renderType;
}


/*!
    \since 6.0
    \qmlproperty Palette Window::palette

    This property holds the palette currently set for the window.

    The default palette depends on the system environment. QGuiApplication maintains a system/theme
    palette which serves as a default for all application windows. You can also set the default palette
    for windows by passing a custom palette to QGuiApplication::setPalette(), before loading any QML.

    Window propagates explicit palette properties to child items and controls,
    overriding any system defaults for that property.

    \snippet qml/windowPalette.qml entire

    \sa Item::palette, Popup::palette, ColorGroup, SystemPalette
    //! internal \sa QQuickAbstractPaletteProvider, QQuickPalette
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QQuickWindow *win)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    if (!win) {
        debug << "QQuickWindow(nullptr)";
        return debug;
    }

    debug << win->metaObject()->className() << '(' << static_cast<const void *>(win);
    if (win->isActive())
        debug << " active";
    if (win->isExposed())
        debug << " exposed";
    debug << ", visibility=" << win->visibility() << ", flags=" << win->flags();
    if (!win->title().isEmpty())
        debug << ", title=" << win->title();
    if (!win->objectName().isEmpty())
        debug << ", name=" << win->objectName();
    if (win->parent())
        debug << ", parent=" << static_cast<const void *>(win->parent());
    if (win->transientParent())
        debug << ", transientParent=" << static_cast<const void *>(win->transientParent());
    debug << ", geometry=";
    QtDebugUtils::formatQRect(debug, win->geometry());
    debug << ')';
    return debug;
}
#endif

QT_END_NAMESPACE

#include "qquickwindow.moc"
#include "moc_qquickwindow_p.cpp"
#include "moc_qquickwindow.cpp"
