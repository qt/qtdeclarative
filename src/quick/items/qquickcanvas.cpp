/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickcanvas.h"
#include "qquickcanvas_p.h"

#include "qquickitem.h"
#include "qquickitem_p.h"

#include <QtQuick/private/qsgrenderer_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <private/qsgflashnode_p.h>
#include <QtQuick/qsgengine.h>

#include <private/qguiapplication_p.h>
#include <QtGui/QInputPanel>

#include <private/qabstractanimation_p.h>

#include <QtGui/qpainter.h>
#include <QtGui/qevent.h>
#include <QtGui/qmatrix4x4.h>
#include <QtCore/qvarlengtharray.h>
#include "private/qabstractanimation_p.h"
#include <QtDeclarative/qdeclarativeincubator.h>

#include <private/qdeclarativedebugtrace_p.h>

QT_BEGIN_NAMESPACE

#define QQUICK_CANVAS_TIMING
#ifdef QQUICK_CANVAS_TIMING
static bool qquick_canvas_timing = !qgetenv("QML_CANVAS_TIMING").isEmpty();
static QTime threadTimer;
static int syncTime;
static int renderTime;
static int swapTime;
#endif

DEFINE_BOOL_CONFIG_OPTION(qmlFixedAnimationStep, QML_FIXED_ANIMATION_STEP)
DEFINE_BOOL_CONFIG_OPTION(qmlNoThreadedRenderer, QML_BAD_GUI_RENDER_LOOP)

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

void QQuickCanvasPrivate::updateFocusItemTransform()
{
    Q_Q(QQuickCanvas);
    QQuickItem *focus = q->activeFocusItem();
    if (focus && qApp->inputPanel()->inputItem() == focus)
        qApp->inputPanel()->setInputItemTransform(QQuickItemPrivate::get(focus)->itemToCanvasTransform());
}

class QQuickCanvasIncubationController : public QObject, public QDeclarativeIncubationController
{
public:
    QQuickCanvasIncubationController(QQuickCanvasPrivate *canvas)
    : m_canvas(canvas), m_eventSent(false) {}

protected:
    virtual bool event(QEvent *e)
    {
        if (e->type() == QEvent::User) {
            Q_ASSERT(m_eventSent);

            bool *amtp = m_canvas->thread->allowMainThreadProcessing();
            while (incubatingObjectCount()) {
                if (amtp)
                    incubateWhile(amtp);
                else
                    incubateFor(5);
                QCoreApplication::processEvents();
            }

            m_eventSent = false;
        }
        return QObject::event(e);
    }

    virtual void incubatingObjectCountChanged(int count)
    {
        if (count && !m_eventSent) {
            m_eventSent = true;
            QCoreApplication::postEvent(this, new QEvent(QEvent::User));
        }
    }

private:
    QQuickCanvasPrivate *m_canvas;
    bool m_eventSent;
};

class QQuickCanvasPlainRenderLoop : public QObject, public QQuickCanvasRenderLoop
{
public:
    QQuickCanvasPlainRenderLoop()
        : updatePending(false)
        , animationRunning(false)
    {
    }

    virtual void paint() {
        updatePending = false;
        if (animationRunning && animationDriver())
            animationDriver()->advance();
        polishItems();
        syncSceneGraph();
        makeCurrent();
        glViewport(0, 0, size.width(), size.height());
        renderSceneGraph(size);
        swapBuffers();

        if (animationRunning)
            maybeUpdate();
    }

    virtual QImage grab() {
        return qt_gl_read_framebuffer(size, false, false);
    }

    virtual void startRendering() {
        if (!glContext()) {
            createGLContext();
            makeCurrent();
            initializeSceneGraph();
        } else {
            makeCurrent();
        }
        maybeUpdate();
    }

    virtual void stopRendering() {
        cleanupNodesOnShutdown();
    }

    virtual void maybeUpdate() {
        if (!updatePending) {
            QCoreApplication::postEvent(this, new QEvent(QEvent::User));
            updatePending = true;
        }
    }

    virtual void animationStarted() {
        animationRunning = true;
        maybeUpdate();
    }

    virtual void animationStopped() {
        animationRunning = false;
    }

    virtual bool isRunning() const { return glContext(); } // Event loop is always running...
    virtual void resize(const QSize &s) { size = s; }
    virtual void setWindowSize(const QSize &s) { size = s; }

    bool event(QEvent *e) {
        if (e->type() == QEvent::User) {
            paint();
            return true;
        }
        return QObject::event(e);
    }

    QSize size;

    uint updatePending : 1;
    uint animationRunning : 1;
};



/*
Focus behavior
==============

Prior to being added to a valid canvas items can set and clear focus with no
effect.  Only once items are added to a canvas (by way of having a parent set that
already belongs to a canvas) do the focus rules apply.  Focus goes back to
having no effect if an item is removed from a canvas.

When an item is moved into a new focus scope (either being added to a canvas
for the first time, or having its parent changed), if the focus scope already has
a scope focused item that takes precedence over the item being added.  Otherwise,
the focus of the added tree is used.  In the case of of a tree of items being
added to a canvas for the first time, which may have a conflicted focus state (two
or more items in one scope having focus set), the same rule is applied item by item -
thus the first item that has focus will get it (assuming the scope doesn't already
have a scope focused item), and the other items will have their focus cleared.
*/

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

  RenderThread::isGuiBlocked: This is used to indicate that the GUI thread owns the
  lock. This variable is an integer to allow for recursive calls to lockInGui()
  without using a recursive mutex. See isGuiBlockPending.

  RenderThread::isPaintComplete: This variable is cleared when rendering starts and
  set once rendering is complete. It is monitored in the paintEvent(),
  resizeEvent() and grab() functions to force them to wait for rendering to
  complete.

  RenderThread::isGuiBlockPending: This variable is set in the render thread just
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

// #define FOCUS_DEBUG
// #define MOUSE_DEBUG
// #define TOUCH_DEBUG
// #define DIRTY_DEBUG
// #define THREAD_DEBUG

// #define FRAME_TIMING

#ifdef FRAME_TIMING
static QTime frameTimer;
int sceneGraphRenderTime;
int readbackTime;
#endif

QQuickItem::UpdatePaintNodeData::UpdatePaintNodeData()
: transformNode(0)
{
}

QQuickRootItem::QQuickRootItem()
{
}

void QQuickCanvas::exposeEvent(QExposeEvent *)
{
    Q_D(QQuickCanvas);
    d->thread->paint();
}

void QQuickCanvas::resizeEvent(QResizeEvent *)
{
    Q_D(QQuickCanvas);
    d->thread->resize(size());
}

void QQuickCanvas::animationStarted()
{
    d_func()->thread->animationStarted();
}

void QQuickCanvas::animationStopped()
{
    d_func()->thread->animationStopped();
}

void QQuickCanvas::showEvent(QShowEvent *)
{
    Q_D(QQuickCanvas);
    if (d->vsyncAnimations) {
        if (!d->animationDriver) {
            d->animationDriver = d->context->createAnimationDriver(this);
            connect(d->animationDriver, SIGNAL(started()), this, SLOT(animationStarted()), Qt::DirectConnection);
            connect(d->animationDriver, SIGNAL(stopped()), this, SLOT(animationStopped()), Qt::DirectConnection);
        }
        d->animationDriver->install();
    }

    if (!d->thread->isRunning()) {
        d->thread->setWindowSize(size());
        d->thread->startRendering();
    }
}

void QQuickCanvas::hideEvent(QHideEvent *)
{
    Q_D(QQuickCanvas);
    d->thread->stopRendering();
}

void QQuickCanvas::focusOutEvent(QFocusEvent *)
{
    Q_D(QQuickCanvas);
    d->rootItem->setFocus(false);
}

void QQuickCanvas::focusInEvent(QFocusEvent *)
{
    Q_D(QQuickCanvas);
    d->rootItem->setFocus(true);
}


/*!
    Sets weither this canvas should use vsync driven animations.

    This option can only be set on one single QQuickCanvas, and that it's
    vsync signal will then be used to drive all animations in the
    process.

    This feature is primarily useful for single QQuickCanvas, QML-only
    applications.

    \warning Enabling vsync on multiple QQuickCanvas instances has
    undefined behavior.
 */
void QQuickCanvas::setVSyncAnimations(bool enabled)
{
    Q_D(QQuickCanvas);
    if (visible()) {
        qWarning("QQuickCanvas::setVSyncAnimations: Cannot be changed when widget is shown");
        return;
    }
    d->vsyncAnimations = enabled;
}



/*!
    Returns true if this canvas should use vsync driven animations;
    otherwise returns false.
 */
bool QQuickCanvas::vsyncAnimations() const
{
    Q_D(const QQuickCanvas);
    return d->vsyncAnimations;
}

void QQuickCanvasPrivate::initializeSceneGraph()
{
    if (!context)
        context = QSGContext::createDefaultContext();

    if (context->isReady())
        return;

    QOpenGLContext *glctx = const_cast<QOpenGLContext *>(QOpenGLContext::currentContext());
    context->initialize(glctx);

    Q_Q(QQuickCanvas);

    if (!QQuickItemPrivate::get(rootItem)->itemNode()->parent()) {
        context->rootNode()->appendChildNode(QQuickItemPrivate::get(rootItem)->itemNode());
    }

    engine = new QSGEngine();
    engine->setCanvas(q);

    emit q_func()->sceneGraphInitialized();
}

void QQuickCanvasPrivate::polishItems()
{
    while (!itemsToPolish.isEmpty()) {
        QSet<QQuickItem *>::Iterator iter = itemsToPolish.begin();
        QQuickItem *item = *iter;
        itemsToPolish.erase(iter);
        QQuickItemPrivate::get(item)->polishScheduled = false;
        item->updatePolish();
    }
    updateFocusItemTransform();
}


void QQuickCanvasPrivate::syncSceneGraph()
{
    updateDirtyNodes();

    // Copy the current state of clearing from canvas into renderer.
    context->renderer()->setClearColor(clearColor);
    QSGRenderer::ClearMode mode = QSGRenderer::ClearStencilBuffer | QSGRenderer::ClearDepthBuffer;
    if (clearBeforeRendering)
        mode |= QSGRenderer::ClearColorBuffer;
    context->renderer()->setClearMode(mode);
}


void QQuickCanvasPrivate::renderSceneGraph(const QSize &size)
{
    Q_Q(QQuickCanvas);
    context->renderer()->setDeviceRect(QRect(QPoint(0, 0), size));
    context->renderer()->setViewportRect(QRect(QPoint(0, 0), renderTarget ? renderTarget->size() : size));
    context->renderer()->setProjectionMatrixToDeviceRect();

    emit q->beforeRendering();
    context->renderNextFrame(renderTarget);
    emit q->afterRendering();

#ifdef FRAME_TIMING
    sceneGraphRenderTime = frameTimer.elapsed();
#endif

#ifdef FRAME_TIMING
//    int pixel;
//    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel);
    readbackTime = frameTimer.elapsed();
#endif
}


QQuickCanvasPrivate::QQuickCanvasPrivate()
    : rootItem(0)
    , activeFocusItem(0)
    , mouseGrabberItem(0)
    , dirtyItemList(0)
    , context(0)
    , clearColor(Qt::white)
    , vsyncAnimations(false)
    , clearBeforeRendering(true)
    , thread(0)
    , animationDriver(0)
    , renderTarget(0)
    , incubationController(0)
{
}

QQuickCanvasPrivate::~QQuickCanvasPrivate()
{
}

void QQuickCanvasPrivate::init(QQuickCanvas *c)
{
    QUnifiedTimer2* ut = QUnifiedTimer2::instance(true);
    if (qmlFixedAnimationStep())
        ut->setConsistentTiming(true);

    q_ptr = c;

    Q_Q(QQuickCanvas);

    rootItem = new QQuickRootItem;
    QQuickItemPrivate *rootItemPrivate = QQuickItemPrivate::get(rootItem);
    rootItemPrivate->canvas = q;
    rootItemPrivate->flags |= QQuickItem::ItemIsFocusScope;

    // In the absence of a focus in event on some platforms assume the window will
    // be activated immediately and set focus on the rootItem
    // ### Remove when QTBUG-22415 is resolved.
    //It is important that this call happens after the rootItem has a canvas..
    rootItem->setFocus(true);

    bool threaded = !qmlNoThreadedRenderer();

    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL)) {
        qWarning("QQuickCanvas: platform does not support threaded rendering!");
        threaded = false;
    }

    if (threaded)
        thread = new QQuickCanvasRenderThread();
    else
        thread = new QQuickCanvasPlainRenderLoop();

    thread->renderer = q;
    thread->d = this;

    context = QSGContext::createDefaultContext();
    thread->moveContextToThread(context);

    q->setSurfaceType(QWindow::OpenGLSurface);
    q->setFormat(context->defaultSurfaceFormat());
}

QDeclarativeListProperty<QObject> QQuickCanvasPrivate::data()
{
    initRootItem();
    return QQuickItemPrivate::get(rootItem)->data();
}

void QQuickCanvasPrivate::initRootItem()
{
    Q_Q(QQuickCanvas);
    q->connect(q, SIGNAL(widthChanged(int)),
            rootItem, SLOT(setWidth(int)));
    q->connect(q, SIGNAL(heightChanged(int)),
            rootItem, SLOT(setHeight(int)));
    rootItem->setWidth(q->width());
    rootItem->setHeight(q->height());
}

void QQuickCanvasPrivate::transformTouchPoints(QList<QTouchEvent::TouchPoint> &touchPoints, const QTransform &transform)
{
    for (int i=0; i<touchPoints.count(); i++) {
        QTouchEvent::TouchPoint &touchPoint = touchPoints[i];
        touchPoint.setRect(transform.mapRect(touchPoint.sceneRect()));
        touchPoint.setStartPos(transform.map(touchPoint.startScenePos()));
        touchPoint.setLastPos(transform.map(touchPoint.lastScenePos()));
    }
}


/*!
Translates the data in \a touchEvent to this canvas.  This method leaves the item local positions in
\a touchEvent untouched (these are filled in later).
*/
void QQuickCanvasPrivate::translateTouchEvent(QTouchEvent *touchEvent)
{
//    Q_Q(QQuickCanvas);

//    touchEvent->setWidget(q); // ### refactor...

    QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
    for (int i = 0; i < touchPoints.count(); ++i) {
        QTouchEvent::TouchPoint &touchPoint = touchPoints[i];

        touchPoint.setScreenRect(touchPoint.sceneRect());
        touchPoint.setStartScreenPos(touchPoint.startScenePos());
        touchPoint.setLastScreenPos(touchPoint.lastScenePos());

        touchPoint.setSceneRect(touchPoint.rect());
        touchPoint.setStartScenePos(touchPoint.startPos());
        touchPoint.setLastScenePos(touchPoint.lastPos());

        if (touchPoint.isPrimary())
            lastMousePosition = touchPoint.pos().toPoint();
    }
    touchEvent->setTouchPoints(touchPoints);
}

void QQuickCanvasPrivate::setFocusInScope(QQuickItem *scope, QQuickItem *item, FocusOptions options)
{
    Q_Q(QQuickCanvas);

    Q_ASSERT(item);
    Q_ASSERT(scope || item == rootItem);

#ifdef FOCUS_DEBUG
    qWarning() << "QQuickCanvasPrivate::setFocusInScope():";
    qWarning() << "    scope:" << (QObject *)scope;
    if (scope)
        qWarning() << "    scopeSubFocusItem:" << (QObject *)QQuickItemPrivate::get(scope)->subFocusItem;
    qWarning() << "    item:" << (QObject *)item;
    qWarning() << "    activeFocusItem:" << (QObject *)activeFocusItem;
#endif

    QQuickItemPrivate *scopePrivate = scope ? QQuickItemPrivate::get(scope) : 0;
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);

    QQuickItem *oldActiveFocusItem = 0;
    QQuickItem *newActiveFocusItem = 0;

    QVarLengthArray<QQuickItem *, 20> changed;

    // Does this change the active focus?
    if (item == rootItem || scopePrivate->activeFocus) {
        oldActiveFocusItem = activeFocusItem;
        newActiveFocusItem = item;
        while (newActiveFocusItem->isFocusScope() && newActiveFocusItem->scopedFocusItem())
            newActiveFocusItem = newActiveFocusItem->scopedFocusItem();

        if (oldActiveFocusItem) {
#ifndef QT_NO_IM
            qApp->inputPanel()->commit();
#endif

            activeFocusItem = 0;
            QFocusEvent event(QEvent::FocusOut, Qt::OtherFocusReason);
            q->sendEvent(oldActiveFocusItem, &event);

            QQuickItem *afi = oldActiveFocusItem;
            while (afi != scope) {
                if (QQuickItemPrivate::get(afi)->activeFocus) {
                    QQuickItemPrivate::get(afi)->activeFocus = false;
                    changed << afi;
                }
                afi = afi->parentItem();
            }
        }
    }

    if (item != rootItem) {
        QQuickItem *oldSubFocusItem = scopePrivate->subFocusItem;
        // Correct focus chain in scope
        if (oldSubFocusItem) {
            QQuickItem *sfi = scopePrivate->subFocusItem->parentItem();
            while (sfi != scope) {
                QQuickItemPrivate::get(sfi)->subFocusItem = 0;
                sfi = sfi->parentItem();
            }
        }
        {
            scopePrivate->subFocusItem = item;
            QQuickItem *sfi = scopePrivate->subFocusItem->parentItem();
            while (sfi != scope) {
                QQuickItemPrivate::get(sfi)->subFocusItem = item;
                sfi = sfi->parentItem();
            }
        }

        if (oldSubFocusItem) {
            QQuickItemPrivate::get(oldSubFocusItem)->focus = false;
            changed << oldSubFocusItem;
        }
    }

    if (!(options & DontChangeFocusProperty)) {
//        if (item != rootItem || QGuiApplication::focusWindow() == q) {    // QTBUG-22415
            itemPrivate->focus = true;
            changed << item;
//        }
    }

    if (newActiveFocusItem && rootItem->hasFocus()) {
        activeFocusItem = newActiveFocusItem;

        QQuickItemPrivate::get(newActiveFocusItem)->activeFocus = true;
        changed << newActiveFocusItem;

        QQuickItem *afi = newActiveFocusItem->parentItem();
        while (afi && afi != scope) {
            if (afi->isFocusScope()) {
                QQuickItemPrivate::get(afi)->activeFocus = true;
                changed << afi;
            }
            afi = afi->parentItem();
        }

        updateInputMethodData();

        QFocusEvent event(QEvent::FocusIn, Qt::OtherFocusReason);
        q->sendEvent(newActiveFocusItem, &event);
    } else {
        updateInputMethodData();
    }

    if (!changed.isEmpty())
        notifyFocusChangesRecur(changed.data(), changed.count() - 1);
}

void QQuickCanvasPrivate::clearFocusInScope(QQuickItem *scope, QQuickItem *item, FocusOptions options)
{
    Q_Q(QQuickCanvas);

    Q_UNUSED(item);
    Q_ASSERT(item);
    Q_ASSERT(scope || item == rootItem);

#ifdef FOCUS_DEBUG
    qWarning() << "QQuickCanvasPrivate::clearFocusInScope():";
    qWarning() << "    scope:" << (QObject *)scope;
    qWarning() << "    item:" << (QObject *)item;
    qWarning() << "    activeFocusItem:" << (QObject *)activeFocusItem;
#endif

    QQuickItemPrivate *scopePrivate = scope ? QQuickItemPrivate::get(scope) : 0;

    QQuickItem *oldActiveFocusItem = 0;
    QQuickItem *newActiveFocusItem = 0;

    QVarLengthArray<QQuickItem *, 20> changed;

    Q_ASSERT(item == rootItem || item == scopePrivate->subFocusItem);

    // Does this change the active focus?
    if (item == rootItem || scopePrivate->activeFocus) {
        oldActiveFocusItem = activeFocusItem;
        newActiveFocusItem = scope;

        Q_ASSERT(oldActiveFocusItem);

#ifndef QT_NO_IM
        qApp->inputPanel()->commit();
#endif

        activeFocusItem = 0;
        QFocusEvent event(QEvent::FocusOut, Qt::OtherFocusReason);
        q->sendEvent(oldActiveFocusItem, &event);

        QQuickItem *afi = oldActiveFocusItem;
        while (afi != scope) {
            if (QQuickItemPrivate::get(afi)->activeFocus) {
                QQuickItemPrivate::get(afi)->activeFocus = false;
                changed << afi;
            }
            afi = afi->parentItem();
        }
    }

    if (item != rootItem) {
        QQuickItem *oldSubFocusItem = scopePrivate->subFocusItem;
        // Correct focus chain in scope
        if (oldSubFocusItem) {
            QQuickItem *sfi = scopePrivate->subFocusItem->parentItem();
            while (sfi != scope) {
                QQuickItemPrivate::get(sfi)->subFocusItem = 0;
                sfi = sfi->parentItem();
            }
        }
        scopePrivate->subFocusItem = 0;

        if (oldSubFocusItem && !(options & DontChangeFocusProperty)) {
            QQuickItemPrivate::get(oldSubFocusItem)->focus = false;
            changed << oldSubFocusItem;
        }
    } else if (!(options & DontChangeFocusProperty)) {
        QQuickItemPrivate::get(item)->focus = false;
        changed << item;
    }

    if (newActiveFocusItem) {
        Q_ASSERT(newActiveFocusItem == scope);
        activeFocusItem = scope;

        updateInputMethodData();

        QFocusEvent event(QEvent::FocusIn, Qt::OtherFocusReason);
        q->sendEvent(newActiveFocusItem, &event);
    } else {
        updateInputMethodData();
    }

    if (!changed.isEmpty())
        notifyFocusChangesRecur(changed.data(), changed.count() - 1);
}

void QQuickCanvasPrivate::notifyFocusChangesRecur(QQuickItem **items, int remaining)
{
    QDeclarativeGuard<QQuickItem> item(*items);

    if (remaining)
        notifyFocusChangesRecur(items + 1, remaining - 1);

    if (item) {
        QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);

        if (itemPrivate->notifiedFocus != itemPrivate->focus) {
            itemPrivate->notifiedFocus = itemPrivate->focus;
            emit item->focusChanged(itemPrivate->focus);
        }

        if (item && itemPrivate->notifiedActiveFocus != itemPrivate->activeFocus) {
            itemPrivate->notifiedActiveFocus = itemPrivate->activeFocus;
            itemPrivate->itemChange(QQuickItem::ItemActiveFocusHasChanged, itemPrivate->activeFocus);
            emit item->activeFocusChanged(itemPrivate->activeFocus);
        }
    }
}

void QQuickCanvasPrivate::updateInputMethodData()
{
    QQuickItem *inputItem = 0;
    if (activeFocusItem && activeFocusItem->flags() & QQuickItem::ItemAcceptsInputMethod)
        inputItem = activeFocusItem;
    qApp->inputPanel()->setInputItem(inputItem);
}

void QQuickCanvasPrivate::dirtyItem(QQuickItem *)
{
    Q_Q(QQuickCanvas);
    q->maybeUpdate();
}

void QQuickCanvasPrivate::cleanup(QSGNode *n)
{
    Q_Q(QQuickCanvas);

    Q_ASSERT(!cleanupNodeList.contains(n));
    cleanupNodeList.append(n);
    q->maybeUpdate();
}


/*!
    \qmlclass Window QQuickCanvas
    \inqmlmodule QtQuick.Window 2
    \brief The Window object creates a new top-level window.

    The Window object creates a new top-level window for a QtQuick scene. It automatically sets up the
    window for use with QtQuick 2.0 graphical elements.
*/
/*!
    \class QQuickCanvas
    \since QtQuick 2.0
    \brief The QQuickCanvas class provides the canvas for displaying a graphical QML scene

    \inmodule QtQuick

    QQuickCanvas provides the graphical scene management needed to interact with and display
    a scene of QQuickItems.

    A QQuickCanvas always has a single invisible root item. To add items to this canvas,
    reparent the items to the root item or to an existing item in the scene.

    For easily displaying a scene from a QML file, see \l{QQuickView}.
*/
QQuickCanvas::QQuickCanvas(QWindow *parent)
    : QWindow(*(new QQuickCanvasPrivate), parent)
{
    Q_D(QQuickCanvas);
    d->init(this);
}

QQuickCanvas::QQuickCanvas(QQuickCanvasPrivate &dd, QWindow *parent)
    : QWindow(dd, parent)
{
    Q_D(QQuickCanvas);
    d->init(this);
}

QQuickCanvas::~QQuickCanvas()
{
    Q_D(QQuickCanvas);

    if (d->thread->isRunning())
        d->thread->stopRendering();

    // ### should we change ~QQuickItem to handle this better?
    // manually cleanup for the root item (item destructor only handles these when an item is parented)
    QQuickItemPrivate *rootItemPrivate = QQuickItemPrivate::get(d->rootItem);
    rootItemPrivate->removeFromDirtyList();

    delete d->incubationController; d->incubationController = 0;

    delete d->rootItem; d->rootItem = 0;

    delete d->thread; d->thread = 0;
}

/*!
  Returns the invisible root item of the scene.

  A QQuickCanvas always has a single invisible root item. To add items to this canvas,
  reparent the items to the root item or to an existing item in the scene.
*/
QQuickItem *QQuickCanvas::rootItem() const
{
    Q_D(const QQuickCanvas);

    return d->rootItem;
}

/*!
  Returns the item which currently has active focus.
*/
QQuickItem *QQuickCanvas::activeFocusItem() const
{
    Q_D(const QQuickCanvas);

    return d->activeFocusItem;
}

/*!
  Returns the item which currently has the mouse grab.
*/
QQuickItem *QQuickCanvas::mouseGrabberItem() const
{
    Q_D(const QQuickCanvas);

    return d->mouseGrabberItem;
}


/*!
    \qmlproperty color QtQuick2.Window::Window::color

    The background color for the window.

    Setting this property is more efficient than using a separate Rectangle.
*/

bool QQuickCanvasPrivate::clearHover()
{
    if (hoverItems.isEmpty())
        return false;

    QPointF pos = QCursor::pos(); // ### refactor: q->mapFromGlobal(QCursor::pos());

    bool accepted = false;
    foreach (QQuickItem* item, hoverItems)
        accepted = sendHoverEvent(QEvent::HoverLeave, item, pos, pos, QGuiApplication::keyboardModifiers(), true) || accepted;
    hoverItems.clear();
    return accepted;
}


bool QQuickCanvas::event(QEvent *e)
{
    Q_D(QQuickCanvas);

    switch (e->type()) {

    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
        QTouchEvent *touch = static_cast<QTouchEvent *>(e);
        d->translateTouchEvent(touch);
        d->deliverTouchEvent(touch);
        if (!touch->isAccepted())
            return false;
        break;
    }
    case QEvent::Leave:
        d->clearHover();
        d->lastMousePosition = QPoint();
        break;
    case QEvent::DragEnter:
    case QEvent::DragLeave:
    case QEvent::DragMove:
    case QEvent::Drop:
        d->deliverDragEvent(&d->dragGrabber, e);
        break;
    case QEvent::WindowDeactivate:
        rootItem()->windowDeactivateEvent();
        break;
    default:
        break;
    }

    return QWindow::event(e);
}

void QQuickCanvas::keyPressEvent(QKeyEvent *e)
{
    Q_D(QQuickCanvas);

    if (d->activeFocusItem)
        sendEvent(d->activeFocusItem, e);
}

void QQuickCanvas::keyReleaseEvent(QKeyEvent *e)
{
    Q_D(QQuickCanvas);

    if (d->activeFocusItem)
        sendEvent(d->activeFocusItem, e);
}

bool QQuickCanvasPrivate::deliverInitialMousePressEvent(QQuickItem *item, QMouseEvent *event)
{
    Q_Q(QQuickCanvas);

    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    if (itemPrivate->opacity == 0.0)
        return false;

    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(event->windowPos());
        if (!QRectF(0, 0, item->width(), item->height()).contains(p))
            return false;
    }

    QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        QQuickItem *child = children.at(ii);
        if (!child->isVisible() || !child->isEnabled())
            continue;
        if (deliverInitialMousePressEvent(child, event))
            return true;
    }

    if (itemPrivate->acceptedMouseButtons & event->button()) {
        QPointF p = item->mapFromScene(event->windowPos());
        if (QRectF(0, 0, item->width(), item->height()).contains(p)) {
            QMouseEvent me(event->type(), p, event->windowPos(), event->screenPos(),
                           event->button(), event->buttons(), event->modifiers());
            me.accept();
            mouseGrabberItem = item;
            q->sendEvent(item, &me);
            event->setAccepted(me.isAccepted());
            if (me.isAccepted())
                return true;
            mouseGrabberItem->ungrabMouse();
            mouseGrabberItem = 0;
        }
    }

    return false;
}

bool QQuickCanvasPrivate::deliverMouseEvent(QMouseEvent *event)
{
    Q_Q(QQuickCanvas);

    lastMousePosition = event->windowPos();

    if (!mouseGrabberItem &&
         event->type() == QEvent::MouseButtonPress &&
         (event->button() & event->buttons()) == event->buttons()) {
        return deliverInitialMousePressEvent(rootItem, event);
    }

    if (mouseGrabberItem) {
        QQuickItemPrivate *mgPrivate = QQuickItemPrivate::get(mouseGrabberItem);
        const QTransform &transform = mgPrivate->canvasToItemTransform();
        QMouseEvent me(event->type(), transform.map(event->windowPos()), event->windowPos(), event->screenPos(),
                       event->button(), event->buttons(), event->modifiers());
        me.accept();
        q->sendEvent(mouseGrabberItem, &me);
        event->setAccepted(me.isAccepted());
        if (me.isAccepted())
            return true;
    }

    return false;
}

void QQuickCanvas::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickCanvas);

#ifdef MOUSE_DEBUG
    qWarning() << "QQuickCanvas::mousePressEvent()" << event->pos() << event->button() << event->buttons();
#endif

    d->deliverMouseEvent(event);
}

void QQuickCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickCanvas);

#ifdef MOUSE_DEBUG
    qWarning() << "QQuickCanvas::mouseReleaseEvent()" << event->pos() << event->button() << event->buttons();
#endif

    if (!d->mouseGrabberItem) {
        QWindow::mouseReleaseEvent(event);
        return;
    }

    d->deliverMouseEvent(event);
    d->mouseGrabberItem = 0;
}

void QQuickCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QQuickCanvas);

#ifdef MOUSE_DEBUG
    qWarning() << "QQuickCanvas::mouseDoubleClickEvent()" << event->pos() << event->button() << event->buttons();
#endif

    if (!d->mouseGrabberItem && (event->button() & event->buttons()) == event->buttons()) {
        if (d->deliverInitialMousePressEvent(d->rootItem, event))
            event->accept();
        else
            event->ignore();
        return;
    }

    d->deliverMouseEvent(event);
}

bool QQuickCanvasPrivate::sendHoverEvent(QEvent::Type type, QQuickItem *item,
                                      const QPointF &scenePos, const QPointF &lastScenePos,
                                      Qt::KeyboardModifiers modifiers, bool accepted)
{
    Q_Q(QQuickCanvas);
    const QTransform transform = QQuickItemPrivate::get(item)->canvasToItemTransform();

    //create copy of event
    QHoverEvent hoverEvent(type, transform.map(scenePos), transform.map(lastScenePos), modifiers);
    hoverEvent.setAccepted(accepted);

    q->sendEvent(item, &hoverEvent);

    return hoverEvent.isAccepted();
}

void QQuickCanvas::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickCanvas);

#ifdef MOUSE_DEBUG
    qWarning() << "QQuickCanvas::mouseMoveEvent()" << event->pos() << event->button() << event->buttons();
#endif

    if (!d->mouseGrabberItem) {
        if (d->lastMousePosition.isNull())
            d->lastMousePosition = event->windowPos();
        QPointF last = d->lastMousePosition;
        d->lastMousePosition = event->windowPos();

        bool accepted = event->isAccepted();
        bool delivered = d->deliverHoverEvent(d->rootItem, event->windowPos(), last, event->modifiers(), accepted);
        if (!delivered) {
            //take care of any exits
            accepted = d->clearHover();
        }
        event->setAccepted(accepted);
        return;
    }

    d->deliverMouseEvent(event);
}

bool QQuickCanvasPrivate::deliverHoverEvent(QQuickItem *item, const QPointF &scenePos, const QPointF &lastScenePos,
                                         Qt::KeyboardModifiers modifiers, bool &accepted)
{
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    if (itemPrivate->opacity == 0.0)
        return false;

    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(scenePos);
        if (!QRectF(0, 0, item->width(), item->height()).contains(p))
            return false;
    }

    QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        QQuickItem *child = children.at(ii);
        if (!child->isVisible() || !child->isEnabled())
            continue;
        if (deliverHoverEvent(child, scenePos, lastScenePos, modifiers, accepted))
            return true;
    }

    if (itemPrivate->hoverEnabled) {
        QPointF p = item->mapFromScene(scenePos);
        if (QRectF(0, 0, item->width(), item->height()).contains(p)) {
            if (!hoverItems.isEmpty() && hoverItems[0] == item) {
                //move
                accepted = sendHoverEvent(QEvent::HoverMove, item, scenePos, lastScenePos, modifiers, accepted);
            } else {
                QList<QQuickItem *> itemsToHover;
                QQuickItem* parent = item;
                itemsToHover << item;
                while ((parent = parent->parentItem()))
                    itemsToHover << parent;

                // Leaving from previous hovered items until we reach the item or one of its ancestors.
                while (!hoverItems.isEmpty() && !itemsToHover.contains(hoverItems[0])) {
                    sendHoverEvent(QEvent::HoverLeave, hoverItems[0], scenePos, lastScenePos, modifiers, accepted);
                    hoverItems.removeFirst();
                }

                if (!hoverItems.isEmpty() && hoverItems[0] == item){//Not entering a new Item
                    // ### Shouldn't we send moves for the parent items as well?
                    accepted = sendHoverEvent(QEvent::HoverMove, item, scenePos, lastScenePos, modifiers, accepted);
                } else {
                    // Enter items that are not entered yet.
                    int startIdx = -1;
                    if (!hoverItems.isEmpty())
                        startIdx = itemsToHover.indexOf(hoverItems[0]) - 1;
                    if (startIdx == -1)
                        startIdx = itemsToHover.count() - 1;

                    for (int i = startIdx; i >= 0; i--) {
                        QQuickItem *itemToHover = itemsToHover[i];
                        if (QQuickItemPrivate::get(itemToHover)->hoverEnabled) {
                            hoverItems.prepend(itemToHover);
                            sendHoverEvent(QEvent::HoverEnter, itemToHover, scenePos, lastScenePos, modifiers, accepted);
                        }
                    }
                }
            }
            return true;
        }
    }

    return false;
}

bool QQuickCanvasPrivate::deliverWheelEvent(QQuickItem *item, QWheelEvent *event)
{
    Q_Q(QQuickCanvas);
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    if (itemPrivate->opacity == 0.0)
        return false;

    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(event->posF());
        if (!QRectF(0, 0, item->width(), item->height()).contains(p))
            return false;
    }

    QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        QQuickItem *child = children.at(ii);
        if (!child->isVisible() || !child->isEnabled())
            continue;
        if (deliverWheelEvent(child, event))
            return true;
    }

    QPointF p = item->mapFromScene(event->posF());
    if (QRectF(0, 0, item->width(), item->height()).contains(p)) {
        QWheelEvent wheel(p, event->delta(), event->buttons(), event->modifiers(), event->orientation());
        wheel.accept();
        q->sendEvent(item, &wheel);
        if (wheel.isAccepted()) {
            event->accept();
            return true;
        }
    }

    return false;
}

#ifndef QT_NO_WHEELEVENT
void QQuickCanvas::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickCanvas);
#ifdef MOUSE_DEBUG
    qWarning() << "QQuickCanvas::wheelEvent()" << event->pos() << event->delta() << event->orientation();
#endif
    event->ignore();
    d->deliverWheelEvent(d->rootItem, event);
}
#endif // QT_NO_WHEELEVENT

bool QQuickCanvasPrivate::deliverTouchEvent(QTouchEvent *event)
{
#ifdef TOUCH_DEBUG
    if (event->type() == QEvent::TouchBegin)
        qWarning("touchBeginEvent");
    else if (event->type() == QEvent::TouchUpdate)
        qWarning("touchUpdateEvent");
    else if (event->type() == QEvent::TouchEnd)
        qWarning("touchEndEvent");
#endif

    QHash<QQuickItem *, QList<QTouchEvent::TouchPoint> > updatedPoints;

    if (event->type() == QTouchEvent::TouchBegin) {     // all points are new touch points
        QSet<int> acceptedNewPoints;
        deliverTouchPoints(rootItem, event, event->touchPoints(), &acceptedNewPoints, &updatedPoints);
        if (acceptedNewPoints.count() > 0)
            event->accept();
        return event->isAccepted();
    }

    const QList<QTouchEvent::TouchPoint> &touchPoints = event->touchPoints();
    QList<QTouchEvent::TouchPoint> newPoints;
    QQuickItem *item = 0;
    for (int i=0; i<touchPoints.count(); i++) {
        const QTouchEvent::TouchPoint &touchPoint = touchPoints[i];
        switch (touchPoint.state()) {
            case Qt::TouchPointPressed:
                newPoints << touchPoint;
                break;
            case Qt::TouchPointMoved:
            case Qt::TouchPointStationary:
            case Qt::TouchPointReleased:
                if (itemForTouchPointId.contains(touchPoint.id())) {
                    item = itemForTouchPointId[touchPoint.id()];
                    if (item)
                        updatedPoints[item].append(touchPoint);
                }
                break;
            default:
                break;
        }
    }

    if (newPoints.count() > 0 || updatedPoints.count() > 0) {
        QSet<int> acceptedNewPoints;
        int prevCount = updatedPoints.count();
        deliverTouchPoints(rootItem, event, newPoints, &acceptedNewPoints, &updatedPoints);
        if (acceptedNewPoints.count() > 0 || updatedPoints.count() != prevCount)
            event->accept();
    }

    if (event->touchPointStates() & Qt::TouchPointReleased) {
        for (int i=0; i<touchPoints.count(); i++) {
            if (touchPoints[i].state() == Qt::TouchPointReleased)
                itemForTouchPointId.remove(touchPoints[i].id());
        }
    }

    return event->isAccepted();
}

bool QQuickCanvasPrivate::deliverTouchPoints(QQuickItem *item, QTouchEvent *event, const QList<QTouchEvent::TouchPoint> &newPoints, QSet<int> *acceptedNewPoints, QHash<QQuickItem *, QList<QTouchEvent::TouchPoint> > *updatedPoints)
{
    Q_Q(QQuickCanvas);
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);

    if (itemPrivate->opacity == 0.0)
        return false;

    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        QRectF bounds(0, 0, item->width(), item->height());
        for (int i=0; i<newPoints.count(); i++) {
            QPointF p = item->mapFromScene(newPoints[i].scenePos());
            if (!bounds.contains(p))
                return false;
        }
    }

    QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        QQuickItem *child = children.at(ii);
        if (!child->isEnabled())
            continue;
        if (deliverTouchPoints(child, event, newPoints, acceptedNewPoints, updatedPoints))
            return true;
    }

    QList<QTouchEvent::TouchPoint> matchingPoints;
    if (newPoints.count() > 0 && acceptedNewPoints->count() < newPoints.count()) {
        QRectF bounds(0, 0, item->width(), item->height());
        for (int i=0; i<newPoints.count(); i++) {
            if (acceptedNewPoints->contains(newPoints[i].id()))
                continue;
            QPointF p = item->mapFromScene(newPoints[i].scenePos());
            if (bounds.contains(p))
                matchingPoints << newPoints[i];
        }
    }

    if (matchingPoints.count() > 0 || (*updatedPoints)[item].count() > 0) {
        QList<QTouchEvent::TouchPoint> &eventPoints = (*updatedPoints)[item];
        eventPoints.append(matchingPoints);
        transformTouchPoints(eventPoints, itemPrivate->canvasToItemTransform());

        Qt::TouchPointStates eventStates;
        for (int i=0; i<eventPoints.count(); i++)
            eventStates |= eventPoints[i].state();
        // if all points have the same state, set the event type accordingly
        QEvent::Type eventType;
        switch (eventStates) {
            case Qt::TouchPointPressed:
                eventType = QEvent::TouchBegin;
                break;
            case Qt::TouchPointReleased:
                eventType = QEvent::TouchEnd;
                break;
            default:
                eventType = QEvent::TouchUpdate;
                break;
        }

        if (eventStates != Qt::TouchPointStationary) {
            QTouchEvent touchEvent(eventType);
            // touchEvent.setWidget(q); // ### refactor: what is the consequence of not setting the widget?
            touchEvent.setDeviceType(event->deviceType());
            touchEvent.setModifiers(event->modifiers());
            touchEvent.setTouchPointStates(eventStates);
            touchEvent.setTouchPoints(eventPoints);
            touchEvent.setTimestamp(event->timestamp());

            touchEvent.accept();
            q->sendEvent(item, &touchEvent);

            if (touchEvent.isAccepted()) {
                for (int i=0; i<matchingPoints.count(); i++) {
                    itemForTouchPointId[matchingPoints[i].id()] = item;
                    acceptedNewPoints->insert(matchingPoints[i].id());
                }
            }
        }
    }

    updatedPoints->remove(item);
    if (acceptedNewPoints->count() == newPoints.count() && updatedPoints->isEmpty())
        return true;

    return false;
}

void QQuickCanvasPrivate::deliverDragEvent(QQuickDragGrabber *grabber, QEvent *event)
{
    Q_Q(QQuickCanvas);
    grabber->resetTarget();
    QQuickDragGrabber::iterator grabItem = grabber->begin();
    if (grabItem != grabber->end()) {
        Q_ASSERT(event->type() != QEvent::DragEnter);
        if (event->type() == QEvent::Drop) {
            QDropEvent *e = static_cast<QDropEvent *>(event);
            for (e->setAccepted(false); !e->isAccepted() && grabItem != grabber->end(); grabItem = grabber->release(grabItem)) {
                QPointF p = (**grabItem)->mapFromScene(e->pos());
                QDropEvent translatedEvent(
                        p.toPoint(),
                        e->possibleActions(),
                        e->mimeData(),
                        e->mouseButtons(),
                        e->keyboardModifiers());
                QQuickDropEventEx::copyActions(&translatedEvent, *e);
                q->sendEvent(**grabItem, &translatedEvent);
                e->setAccepted(translatedEvent.isAccepted());
                e->setDropAction(translatedEvent.dropAction());
                grabber->setTarget(**grabItem);
            }
        }
        if (event->type() != QEvent::DragMove) {    // Either an accepted drop or a leave.
            QDragLeaveEvent leaveEvent;
            for (; grabItem != grabber->end(); grabItem = grabber->release(grabItem))
                q->sendEvent(**grabItem, &leaveEvent);
            return;
        } else for (; grabItem != grabber->end(); grabItem = grabber->release(grabItem)) {
            QDragMoveEvent *moveEvent = static_cast<QDragMoveEvent *>(event);
            if (deliverDragEvent(grabber, **grabItem, moveEvent)) {
                moveEvent->setAccepted(true);
                for (++grabItem; grabItem != grabber->end();) {
                    QPointF p = (**grabItem)->mapFromScene(moveEvent->pos());
                    if (QRectF(0, 0, (**grabItem)->width(), (**grabItem)->height()).contains(p)) {
                        QDragMoveEvent translatedEvent(
                                p.toPoint(),
                                moveEvent->possibleActions(),
                                moveEvent->mimeData(),
                                moveEvent->mouseButtons(),
                                moveEvent->keyboardModifiers());
                        QQuickDropEventEx::copyActions(&translatedEvent, *moveEvent);
                        q->sendEvent(**grabItem, &translatedEvent);
                        ++grabItem;
                    } else {
                        QDragLeaveEvent leaveEvent;
                        q->sendEvent(**grabItem, &leaveEvent);
                        grabItem = grabber->release(grabItem);
                    }
                }
                return;
            } else {
                QDragLeaveEvent leaveEvent;
                q->sendEvent(**grabItem, &leaveEvent);
            }
        }
    }
    if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
        QDragMoveEvent *e = static_cast<QDragMoveEvent *>(event);
        QDragEnterEvent enterEvent(
                e->pos(),
                e->possibleActions(),
                e->mimeData(),
                e->mouseButtons(),
                e->keyboardModifiers());
        QQuickDropEventEx::copyActions(&enterEvent, *e);
        event->setAccepted(deliverDragEvent(grabber, rootItem, &enterEvent));
    }
}

bool QQuickCanvasPrivate::deliverDragEvent(QQuickDragGrabber *grabber, QQuickItem *item, QDragMoveEvent *event)
{
    Q_Q(QQuickCanvas);
    bool accepted = false;
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    if (itemPrivate->opacity == 0.0 || !item->isVisible() || !item->isEnabled())
        return false;

    QPointF p = item->mapFromScene(event->pos());
    if (QRectF(0, 0, item->width(), item->height()).contains(p)) {
        if (event->type() == QEvent::DragMove || itemPrivate->flags & QQuickItem::ItemAcceptsDrops) {
            QDragMoveEvent translatedEvent(
                    p.toPoint(),
                    event->possibleActions(),
                    event->mimeData(),
                    event->mouseButtons(),
                    event->keyboardModifiers(),
                    event->type());
            QQuickDropEventEx::copyActions(&translatedEvent, *event);
            q->sendEvent(item, &translatedEvent);
            if (event->type() == QEvent::DragEnter) {
                if (translatedEvent.isAccepted()) {
                    grabber->grab(item);
                    accepted = true;
                }
            } else {
                accepted = true;
            }
        }
    } else if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        return false;
    }

    QDragEnterEvent enterEvent(
            event->pos(),
            event->possibleActions(),
            event->mimeData(),
            event->mouseButtons(),
            event->keyboardModifiers());
    QQuickDropEventEx::copyActions(&enterEvent, *event);
    QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        if (deliverDragEvent(grabber, children.at(ii), &enterEvent))
            return true;
    }

    return accepted;
}

bool QQuickCanvasPrivate::sendFilteredMouseEvent(QQuickItem *target, QQuickItem *item, QEvent *event)
{
    if (!target)
        return false;

    QQuickItemPrivate *targetPrivate = QQuickItemPrivate::get(target);
    if (targetPrivate->filtersChildMouseEvents)
        if (target->childMouseEventFilter(item, event))
            return true;

    if (sendFilteredMouseEvent(target->parentItem(), item, event))
        return true;

    return false;
}

/*!
    Propagates an event to a QQuickItem on the canvas
*/
bool QQuickCanvas::sendEvent(QQuickItem *item, QEvent *e)
{
    Q_D(QQuickCanvas);

    if (!item) {
        qWarning("QQuickCanvas::sendEvent: Cannot send event to a null item");
        return false;
    }

    Q_ASSERT(e);

    switch (e->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        e->accept();
        QQuickItemPrivate::get(item)->deliverKeyEvent(static_cast<QKeyEvent *>(e));
        while (!e->isAccepted() && (item = item->parentItem())) {
            e->accept();
            QQuickItemPrivate::get(item)->deliverKeyEvent(static_cast<QKeyEvent *>(e));
        }
        break;
    case QEvent::FocusIn:
    case QEvent::FocusOut:
        QQuickItemPrivate::get(item)->deliverFocusEvent(static_cast<QFocusEvent *>(e));
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
        // XXX todo - should sendEvent be doing this?  how does it relate to forwarded events?
        if (!d->sendFilteredMouseEvent(item->parentItem(), item, e)) {
            e->accept();
            QQuickItemPrivate::get(item)->deliverMouseEvent(static_cast<QMouseEvent *>(e));
        }
        break;
    case QEvent::Wheel:
        QQuickItemPrivate::get(item)->deliverWheelEvent(static_cast<QWheelEvent *>(e));
        break;
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
        QQuickItemPrivate::get(item)->deliverHoverEvent(static_cast<QHoverEvent *>(e));
        break;
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        // XXX todo - should sendEvent be doing this?  how does it relate to forwarded events?
        if (!d->sendFilteredMouseEvent(item->parentItem(), item, e)) {
            e->accept();
            QQuickItemPrivate::get(item)->deliverTouchEvent(static_cast<QTouchEvent *>(e));
        }
        break;
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::DragLeave:
    case QEvent::Drop:
        QQuickItemPrivate::get(item)->deliverDragEvent(e);
        break;
    default:
        break;
    }

    return false;
}

void QQuickCanvasPrivate::cleanupNodes()
{
    for (int ii = 0; ii < cleanupNodeList.count(); ++ii)
        delete cleanupNodeList.at(ii);
    cleanupNodeList.clear();
}

void QQuickCanvasPrivate::cleanupNodesOnShutdown(QQuickItem *item)
{
    QQuickItemPrivate *p = QQuickItemPrivate::get(item);
    if (p->itemNodeInstance) {
        delete p->itemNodeInstance;
        p->itemNodeInstance = 0;
        p->opacityNode = 0;
        p->clipNode = 0;
        p->groupNode = 0;
        p->paintNode = 0;
    }

    for (int ii = 0; ii < p->childItems.count(); ++ii)
        cleanupNodesOnShutdown(p->childItems.at(ii));
}

// This must be called from the render thread, with the main thread frozen
void QQuickCanvasPrivate::cleanupNodesOnShutdown()
{
    cleanupNodes();

    cleanupNodesOnShutdown(rootItem);
}

void QQuickCanvasPrivate::updateDirtyNodes()
{
#ifdef DIRTY_DEBUG
    qWarning() << "QQuickCanvasPrivate::updateDirtyNodes():";
#endif

    cleanupNodes();

    QQuickItem *updateList = dirtyItemList;
    dirtyItemList = 0;
    if (updateList) QQuickItemPrivate::get(updateList)->prevDirtyItem = &updateList;

    while (updateList) {
        QQuickItem *item = updateList;
        QQuickItemPrivate *itemPriv = QQuickItemPrivate::get(item);
        itemPriv->removeFromDirtyList();

#ifdef DIRTY_DEBUG
        qWarning() << "   QSGNode:" << item << qPrintable(itemPriv->dirtyToString());
#endif
        updateDirtyNode(item);
    }
}

void QQuickCanvasPrivate::updateDirtyNode(QQuickItem *item)
{
#ifdef QML_RUNTIME_TESTING
    bool didFlash = false;
#endif

    QQuickItemPrivate *itemPriv = QQuickItemPrivate::get(item);
    quint32 dirty = itemPriv->dirtyAttributes;
    itemPriv->dirtyAttributes = 0;

    if ((dirty & QQuickItemPrivate::TransformUpdateMask) ||
        (dirty & QQuickItemPrivate::Size && itemPriv->origin != QQuickItem::TopLeft &&
         (itemPriv->scale != 1. || itemPriv->rotation != 0.))) {

        QMatrix4x4 matrix;

        if (itemPriv->x != 0. || itemPriv->y != 0.)
            matrix.translate(itemPriv->x, itemPriv->y);

        for (int ii = itemPriv->transforms.count() - 1; ii >= 0; --ii)
            itemPriv->transforms.at(ii)->applyTo(&matrix);

        if (itemPriv->scale != 1. || itemPriv->rotation != 0.) {
            QPointF origin = item->transformOriginPoint();
            matrix.translate(origin.x(), origin.y());
            if (itemPriv->scale != 1.)
                matrix.scale(itemPriv->scale, itemPriv->scale);
            if (itemPriv->rotation != 0.)
                matrix.rotate(itemPriv->rotation, 0, 0, 1);
            matrix.translate(-origin.x(), -origin.y());
        }

        itemPriv->itemNode()->setMatrix(matrix);
    }

    bool clipEffectivelyChanged = dirty & QQuickItemPrivate::Clip &&
                                  ((item->clip() == false) != (itemPriv->clipNode == 0));
    bool effectRefEffectivelyChanged = dirty & QQuickItemPrivate::EffectReference &&
                                  ((itemPriv->effectRefCount == 0) != (itemPriv->rootNode == 0));

    if (clipEffectivelyChanged) {
        QSGNode *parent = itemPriv->opacityNode ? (QSGNode *) itemPriv->opacityNode : (QSGNode *)itemPriv->itemNode();
        QSGNode *child = itemPriv->rootNode ? (QSGNode *)itemPriv->rootNode : (QSGNode *)itemPriv->groupNode;

        if (item->clip()) {
            Q_ASSERT(itemPriv->clipNode == 0);
            itemPriv->clipNode = new QQuickDefaultClipNode(item->boundingRect());
            itemPriv->clipNode->update();

            if (child)
                parent->removeChildNode(child);
            parent->appendChildNode(itemPriv->clipNode);
            if (child)
                itemPriv->clipNode->appendChildNode(child);

        } else {
            Q_ASSERT(itemPriv->clipNode != 0);
            parent->removeChildNode(itemPriv->clipNode);
            if (child)
                itemPriv->clipNode->removeChildNode(child);
            delete itemPriv->clipNode;
            itemPriv->clipNode = 0;
            if (child)
                parent->appendChildNode(child);
        }
    }

    if (dirty & QQuickItemPrivate::ChildrenUpdateMask)
        itemPriv->childContainerNode()->removeAllChildNodes();

    if (effectRefEffectivelyChanged) {
        QSGNode *parent = itemPriv->clipNode;
        if (!parent)
            parent = itemPriv->opacityNode;
        if (!parent)
            parent = itemPriv->itemNode();
        QSGNode *child = itemPriv->groupNode;

        if (itemPriv->effectRefCount) {
            Q_ASSERT(itemPriv->rootNode == 0);
            itemPriv->rootNode = new QSGRootNode;

            if (child)
                parent->removeChildNode(child);
            parent->appendChildNode(itemPriv->rootNode);
            if (child)
                itemPriv->rootNode->appendChildNode(child);
        } else {
            Q_ASSERT(itemPriv->rootNode != 0);
            parent->removeChildNode(itemPriv->rootNode);
            if (child)
                itemPriv->rootNode->removeChildNode(child);
            delete itemPriv->rootNode;
            itemPriv->rootNode = 0;
            if (child)
                parent->appendChildNode(child);
        }
    }

    if (dirty & QQuickItemPrivate::ChildrenUpdateMask) {
        QSGNode *groupNode = itemPriv->groupNode;
        if (groupNode)
            groupNode->removeAllChildNodes();

        QList<QQuickItem *> orderedChildren = itemPriv->paintOrderChildItems();
        int ii = 0;

        for (; ii < orderedChildren.count() && orderedChildren.at(ii)->z() < 0; ++ii) {
            QQuickItemPrivate *childPrivate = QQuickItemPrivate::get(orderedChildren.at(ii));
            if (!childPrivate->explicitVisible && !childPrivate->effectRefCount)
                continue;
            if (childPrivate->itemNode()->parent())
                childPrivate->itemNode()->parent()->removeChildNode(childPrivate->itemNode());

            itemPriv->childContainerNode()->appendChildNode(childPrivate->itemNode());
        }
        itemPriv->beforePaintNode = itemPriv->groupNode ? itemPriv->groupNode->lastChild() : 0;

        if (itemPriv->paintNode)
            itemPriv->childContainerNode()->appendChildNode(itemPriv->paintNode);

        for (; ii < orderedChildren.count(); ++ii) {
            QQuickItemPrivate *childPrivate = QQuickItemPrivate::get(orderedChildren.at(ii));
            if (!childPrivate->explicitVisible && !childPrivate->effectRefCount)
                continue;
            if (childPrivate->itemNode()->parent())
                childPrivate->itemNode()->parent()->removeChildNode(childPrivate->itemNode());

            itemPriv->childContainerNode()->appendChildNode(childPrivate->itemNode());
        }
    }

    if ((dirty & QQuickItemPrivate::Size) && itemPriv->clipNode) {
        itemPriv->clipNode->setRect(item->boundingRect());
        itemPriv->clipNode->update();
    }

    if (dirty & (QQuickItemPrivate::OpacityValue | QQuickItemPrivate::Visible | QQuickItemPrivate::HideReference)) {
        qreal opacity = itemPriv->explicitVisible && itemPriv->hideRefCount == 0
                      ? itemPriv->opacity : qreal(0);

        if (opacity != 1 && !itemPriv->opacityNode) {
            itemPriv->opacityNode = new QSGOpacityNode;

            QSGNode *parent = itemPriv->itemNode();
            QSGNode *child = itemPriv->clipNode;
            if (!child)
                child = itemPriv->rootNode;
            if (!child)
                child = itemPriv->groupNode;

            if (child)
                parent->removeChildNode(child);
            parent->appendChildNode(itemPriv->opacityNode);
            if (child)
                itemPriv->opacityNode->appendChildNode(child);
        }
        if (itemPriv->opacityNode)
            itemPriv->opacityNode->setOpacity(opacity);
    }

    if (dirty & QQuickItemPrivate::ContentUpdateMask) {

        if (itemPriv->flags & QQuickItem::ItemHasContents) {
            updatePaintNodeData.transformNode = itemPriv->itemNode();
            itemPriv->paintNode = item->updatePaintNode(itemPriv->paintNode, &updatePaintNodeData);

            Q_ASSERT(itemPriv->paintNode == 0 ||
                     itemPriv->paintNode->parent() == 0 ||
                     itemPriv->paintNode->parent() == itemPriv->childContainerNode());

            if (itemPriv->paintNode && itemPriv->paintNode->parent() == 0) {
                if (itemPriv->beforePaintNode)
                    itemPriv->childContainerNode()->insertChildNodeAfter(itemPriv->paintNode, itemPriv->beforePaintNode);
                else
                    itemPriv->childContainerNode()->prependChildNode(itemPriv->paintNode);
            }
        } else if (itemPriv->paintNode) {
            delete itemPriv->paintNode;
            itemPriv->paintNode = 0;
        }
    }

#ifndef QT_NO_DEBUG
    // Check consistency.
    const QSGNode *nodeChain[] = {
        itemPriv->itemNodeInstance,
        itemPriv->opacityNode,
        itemPriv->clipNode,
        itemPriv->rootNode,
        itemPriv->groupNode,
        itemPriv->paintNode,
    };

    int ip = 0;
    for (;;) {
        while (ip < 5 && nodeChain[ip] == 0)
            ++ip;
        if (ip == 5)
            break;
        int ic = ip + 1;
        while (ic < 5 && nodeChain[ic] == 0)
            ++ic;
        const QSGNode *parent = nodeChain[ip];
        const QSGNode *child = nodeChain[ic];
        if (child == 0) {
            Q_ASSERT(parent == itemPriv->groupNode || parent->childCount() == 0);
        } else {
            Q_ASSERT(parent == itemPriv->groupNode || parent->childCount() == 1);
            Q_ASSERT(child->parent() == parent);
            bool containsChild = false;
            for (QSGNode *n = parent->firstChild(); n; n = n->nextSibling())
                containsChild |= (n == child);
            Q_ASSERT(containsChild);
        }
        ip = ic;
    }
#endif

#ifdef QML_RUNTIME_TESTING
    if (itemPriv->sceneGraphContext()->isFlashModeEnabled()) {
        QSGFlashNode *flash = new QSGFlashNode();
        flash->setRect(item->boundingRect());
        itemPriv->childContainerNode()->appendChildNode(flash);
        didFlash = true;
    }
    Q_Q(QQuickCanvas);
    if (didFlash) {
        q->maybeUpdate();
    }
#endif

}

void QQuickCanvas::maybeUpdate()
{
    Q_D(QQuickCanvas);

    if (d->thread && d->thread->isRunning())
        d->thread->maybeUpdate();
}

/*!
    \fn void QSGEngine::sceneGraphInitialized();

    This signal is emitted when the scene graph has been initialized.

    This signal will be emitted from the scene graph rendering thread.
 */

/*!
    Returns the QSGEngine used for this scene.

    The engine will only be available once the scene graph has been
    initialized. Register for the sceneGraphEngine() signal to get
    notification about this.

    \deprecated
 */

QSGEngine *QQuickCanvas::sceneGraphEngine() const
{
    Q_D(const QQuickCanvas);
    qWarning("QQuickCanvas::sceneGraphEngine() is deprecated, use members of QQuickCanvas instead");
    if (d->context && d->context->isReady())
        return d->engine;
    return 0;
}



/*!
    Sets the render target for this canvas to be \a fbo.

    The specified fbo must be created in the context of the canvas
    or one that shares with it.

    \warning
    This function can only be called from the thread doing
    the rendering.
 */

void QQuickCanvas::setRenderTarget(QOpenGLFramebufferObject *fbo)
{
    Q_D(QQuickCanvas);
    if (d->context && d->context && QThread::currentThread() != d->context->thread()) {
        qWarning("QQuickCanvas::setRenderThread: Cannot set render target from outside the rendering thread");
        return;
    }

    d->renderTarget = fbo;
}



/*!
    Returns the render target for this canvas.

    The default is to render to the surface of the canvas, in which
    case the render target is 0.
 */
QOpenGLFramebufferObject *QQuickCanvas::renderTarget() const
{
    Q_D(const QQuickCanvas);
    return d->renderTarget;
}


/*!
    Grabs the contents of the framebuffer and returns it as an image.

    This function might not work if the view is not visible.

    \warning Calling this function will cause performance problems.

    \warning This function can only be called from the GUI thread.
 */
QImage QQuickCanvas::grabFrameBuffer()
{
    Q_D(QQuickCanvas);
    return d->thread ? d->thread->grab() : QImage();
}

/*!
    Returns an incubation controller that splices incubation between frames
    for this canvas. QQuickView automatically installs this controller for you,
    otherwise you will need to install it yourself using \l{QDeclarativeEngine::setIncubationController}

    The controller is owned by the canvas and will be destroyed when the canvas
    is deleted.
*/
QDeclarativeIncubationController *QQuickCanvas::incubationController() const
{
    Q_D(const QQuickCanvas);

    if (!d->incubationController)
        d->incubationController = new QQuickCanvasIncubationController(const_cast<QQuickCanvasPrivate *>(d));
    return d->incubationController;
}



/*!
    \enum QQuickCanvas::CreateTextureOption

    The CreateTextureOption enums are used to customize a texture is wrapped.

    \value TextureHasAlphaChannel The texture has an alpha channel and should
    be drawn using blending.

    \value TextureHasMipmaps The texture has mipmaps and can be drawn with
    mipmapping enabled.

    \value TextureOwnsGLTexture The texture object owns the texture id and
    will delete the GL texture when the texture object is deleted.
 */

/*!
    \fn void QQuickCanvas::beforeRendering()

    This signal is emitted before the scene starts rendering.

    Combined with the modes for clearing the background, this option
    can be used to paint using raw GL under QML content.

    The GL context used for rendering the scene graph will be bound
    at this point.

    Since this signal is emitted from the scene graph rendering thread, the receiver should
    be on the scene graph thread or the connection should be Qt::DirectConnection.

*/

/*!
    \fn void QQuickCanvas::afterRendering()

    This signal is emitted after the scene has completed rendering, before swapbuffers is called.

    This signal can be used to paint using raw GL on top of QML content,
    or to do screen scraping of the current frame buffer.

    The GL context used for rendering the scene graph will be bound at this point.

    Since this signal is emitted from the scene graph rendering thread, the receiver should
    be on the scene graph thread or the connection should be Qt::DirectConnection.
 */



/*!
    Sets weither the scene graph rendering of QML should clear the color buffer
    before it starts rendering to \a enbled.

    By disabling clearing of the color buffer, it is possible to do GL painting
    under the scene graph.

    The color buffer is cleared by default.

    \sa beforeRendering()
 */

void QQuickCanvas::setClearBeforeRendering(bool enabled)
{
    Q_D(QQuickCanvas);
    d->clearBeforeRendering = enabled;
}



/*!
    Returns weither clearing of the color buffer is done before rendering or not.
 */

bool QQuickCanvas::clearBeforeRendering() const
{
    Q_D(const QQuickCanvas);
    return d->clearBeforeRendering;
}



/*!
    Creates a new QSGTexture from the supplied \a image. If the image has an
    alpha channel, the corresponding texture will have an alpha channel.

    The caller of the function is responsible for deleting the returned texture.
    The actual GL texture will be deleted when the texture object is deleted.

    \warning This function will return 0 if the scene graph has not yet been
    initialized.

    This function can be called both from the GUI thread and the rendering thread.

    \sa sceneGraphInitialized()
 */

QSGTexture *QQuickCanvas::createTextureFromImage(const QImage &image) const
{
    Q_D(const QQuickCanvas);
    if (d->context)
        return d->context->createTexture(image);
    else
        return 0;
}



/*!
    Creates a new QSGTexture object from an existing GL texture \a id.

    The caller of the function is responsible for deleting the returned texture.

    Use \a options to customize the texture attributes.

    \warning This function will return 0 if the scenegraph has not yet been
    initialized.

    \sa sceneGraphInitialized()
 */
QSGTexture *QQuickCanvas::createTextureFromId(uint id, const QSize &size, CreateTextureOptions options) const
{
    Q_D(const QQuickCanvas);
    if (d->context) {
        QSGPlainTexture *texture = new QSGPlainTexture();
        texture->setTextureId(id);
        texture->setHasAlphaChannel(options & TextureHasAlphaChannel);
        texture->setHasMipmaps(options & TextureHasMipmaps);
        texture->setOwnsTexture(options & TextureOwnsGLTexture);
        texture->setTextureSize(size);
        return texture;
    }
    return 0;
}


/*!
    Sets the color used to clear the opengl context to \a color.

    Setting the clear color has no effect when clearing is disabled.

    \sa setClearBeforeRendering()
 */

void QQuickCanvas::setClearColor(const QColor &color)
{
    if (color == d_func()->clearColor)
        return;
    d_func()->clearColor = color;
    emit clearColorChanged(color);
}



/*!
    Returns the color used to clear the opengl context.
 */

QColor QQuickCanvas::clearColor() const
{
    return d_func()->clearColor;
}



void QQuickCanvasRenderLoop::createGLContext()
{
    gl = new QOpenGLContext();
    gl->setFormat(renderer->requestedFormat());
    gl->create();
}

void QQuickCanvasRenderThread::run()
{
#ifdef THREAD_DEBUG
    qDebug("QML Rendering Thread Started");
#endif

    if (!glContext()) {
        createGLContext();
        makeCurrent();
        initializeSceneGraph();
    } else {
        makeCurrent();
    }

    while (!shouldExit) {
        lock();

        bool sizeChanged = false;
        isExternalUpdatePending = false;

        if (renderedSize != windowSize) {
#ifdef THREAD_DEBUG
            printf("                RenderThread: window has changed size...\n");
#endif
            glViewport(0, 0, windowSize.width(), windowSize.height());
            sizeChanged = true;
        }

#ifdef THREAD_DEBUG
        printf("                RenderThread: preparing to sync...\n");
#endif

        if (!isGuiBlocked) {
            isGuiBlockPending = true;

#ifdef THREAD_DEBUG
            printf("                RenderThread: aquired sync lock...\n");
#endif
            allowMainThreadProcessingFlag = false;
            QCoreApplication::postEvent(this, new QEvent(QEvent::User));
#ifdef THREAD_DEBUG
            printf("                RenderThread: going to sleep...\n");
#endif
            wait();

            isGuiBlockPending = false;
        }

#ifdef THREAD_DEBUG
        printf("                RenderThread: Doing locked sync\n");
#endif
#ifdef QQUICK_CANVAS_TIMING
        if (qquick_canvas_timing)
            threadTimer.start();
#endif
        inSync = true;
        syncSceneGraph();
        inSync = false;

        // Wake GUI after sync to let it continue animating and event processing.
        allowMainThreadProcessingFlag = true;
        wake();
        unlock();
#ifdef THREAD_DEBUG
        printf("                RenderThread: sync done\n");
#endif
#ifdef QQUICK_CANVAS_TIMING
        if (qquick_canvas_timing)
            syncTime = threadTimer.elapsed();
#endif

#ifdef THREAD_DEBUG
        printf("                RenderThread: rendering... %d x %d\n", windowSize.width(), windowSize.height());
#endif

        renderSceneGraph(windowSize);
#ifdef QQUICK_CANVAS_TIMING
        if (qquick_canvas_timing)
            renderTime = threadTimer.elapsed() - syncTime;
#endif

        // The content of the target buffer is undefined after swap() so grab needs
        // to happen before swap();
        if (doGrab) {
#ifdef THREAD_DEBUG
            printf("                RenderThread: doing a grab...\n");
#endif
            grabContent = qt_gl_read_framebuffer(windowSize, false, false);
            doGrab = false;
        }

#ifdef THREAD_DEBUG
        printf("                RenderThread: wait for swap...\n");
#endif

        swapBuffers();
#ifdef THREAD_DEBUG
        printf("                RenderThread: swap complete...\n");
#endif
#ifdef QQUICK_CANVAS_TIMING
        if (qquick_canvas_timing) {
            swapTime = threadTimer.elapsed() - renderTime;
            qDebug() << "- Breakdown of frame time: sync:" << syncTime
                     << "ms render:" << renderTime << "ms swap:" << swapTime
                     << "ms total:" << swapTime + renderTime << "ms";
        }
#endif

        lock();
        isPaintCompleted = true;
        if (sizeChanged)
            renderedSize = windowSize;

        // Wake the GUI thread now that rendering is complete, to signal that painting
        // is done, resizing is done or grabbing is completed. For grabbing, we're
        // signalling this much later than needed (we could have done it before swap)
        // but we don't want to lock an extra time.
        wake();

        if (!animationRunning && !isExternalUpdatePending && !shouldExit && !doGrab) {
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
    cleanupNodesOnShutdown();

#ifdef THREAD_DEBUG
    printf("                RenderThread: render loop exited... Good Night!\n");
#endif

    doneCurrent();

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



bool QQuickCanvasRenderThread::event(QEvent *e)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());

    if (e->type() == QEvent::User) {
        if (!syncAlreadyHappened)
            sync(false);

        syncAlreadyHappened = false;

        if (animationRunning && animationDriver()) {
#ifdef THREAD_DEBUG
            qDebug("GUI: Advancing animations...\n");
#endif

            animationDriver()->advance();

#ifdef THREAD_DEBUG
            qDebug("GUI: Animations advanced...\n");
#endif
        }

        return true;
    }

    return QThread::event(e);
}



void QQuickCanvasRenderThread::exhaustSyncEvent()
{
    if (isGuiBlockPending) {
        sync(true);
        syncAlreadyHappened = true;
    }
}



void QQuickCanvasRenderThread::sync(bool guiAlreadyLocked)
{
#ifdef THREAD_DEBUG
    printf("GUI: sync - %s\n", guiAlreadyLocked ? "outside event" : "inside event");
#endif
    if (!guiAlreadyLocked)
        lockInGui();

    renderThreadAwakened = false;

    polishItems();

    wake();
    wait();

    if (!guiAlreadyLocked)
        unlockInGui();
}




/*!
    Acquires the mutex for the GUI thread. The function uses the isGuiBlocked
    variable to keep track of how many recursion levels the gui is locked with.
    We only actually acquire the mutex for the first level to avoid deadlocking
    ourselves.
 */

void QQuickCanvasRenderThread::lockInGui()
{
    // We must avoid recursive locking in the GUI thread, hence we
    // only lock when we are the first one to try to block.
    if (!isGuiBlocked)
        lock();

    isGuiBlocked++;

#ifdef THREAD_DEBUG
    printf("GUI: aquired lock... %d\n", isGuiBlocked);
#endif
}



void QQuickCanvasRenderThread::unlockInGui()
{
#ifdef THREAD_DEBUG
    printf("GUI: releasing lock... %d\n", isGuiBlocked);
#endif
    --isGuiBlocked;
    if (!isGuiBlocked)
        unlock();
}




void QQuickCanvasRenderThread::animationStarted()
{
#ifdef THREAD_DEBUG
    printf("GUI: animationStarted()\n");
#endif

    lockInGui();

    animationRunning = true;

    if (isRenderBlocked)
        wake();

    unlockInGui();
}



void QQuickCanvasRenderThread::animationStopped()
{
#ifdef THREAD_DEBUG
    printf("GUI: animationStopped()...\n");
#endif

    lockInGui();
    animationRunning = false;
    unlockInGui();
}


void QQuickCanvasRenderThread::paint()
{
#ifdef THREAD_DEBUG
    printf("GUI: paint called..\n");
#endif

    lockInGui();
    exhaustSyncEvent();

    isPaintCompleted = false;
    while (isRunning() && !isPaintCompleted) {
        if (isRenderBlocked)
            wake();
        wait();
    }
    unlockInGui();
}



void QQuickCanvasRenderThread::resize(const QSize &size)
{
#ifdef THREAD_DEBUG
    printf("GUI: Resize Event: %dx%d\n", size.width(), size.height());
#endif

    if (!isRunning()) {
        windowSize = size;
        return;
    }

    lockInGui();
    exhaustSyncEvent();

    windowSize = size;

    while (isRunning() && renderedSize != windowSize) {
        if (isRenderBlocked)
            wake();
        wait();
    }
    unlockInGui();
}



void QQuickCanvasRenderThread::startRendering()
{
#ifdef THREAD_DEBUG
    printf("GUI: Starting Render Thread\n");
#endif
    hasExited = false;
    shouldExit = false;
    isGuiBlocked = 0;
    isGuiBlockPending = false;
    start();
}



void QQuickCanvasRenderThread::stopRendering()
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

}



QImage QQuickCanvasRenderThread::grab()
{
    if (!isRunning())
        return QImage();

    if (QThread::currentThread() != qApp->thread()) {
        qWarning("QQuickCanvas::grabFrameBuffer: can only be called from the GUI thread");
        return QImage();
    }

#ifdef THREAD_DEBUG
    printf("GUI: doing a pixelwise grab..\n");
#endif

    lockInGui();
    exhaustSyncEvent();

    doGrab = true;
    isPaintCompleted = false;
    while (isRunning() && !isPaintCompleted) {
        if (isRenderBlocked)
            wake();
        wait();
    }

    QImage grabbed = grabContent;
    grabContent = QImage();

    unlockInGui();

    return grabbed;
}



void QQuickCanvasRenderThread::maybeUpdate()
{
    Q_ASSERT_X(QThread::currentThread() == QCoreApplication::instance()->thread() || inSync,
               "QQuickCanvas::update",
               "Function can only be called from GUI thread or during QQuickItem::updatePaintNode()");

    if (inSync) {
        isExternalUpdatePending = true;

    } else if (!renderThreadAwakened) {
#ifdef THREAD_DEBUG
        printf("GUI: doing update...\n");
#endif
        renderThreadAwakened = true;
        lockInGui();
        isExternalUpdatePending = true;
        if (isRenderBlocked)
            wake();
        unlockInGui();
    }
}


#include "moc_qquickcanvas.cpp"

QT_END_NAMESPACE
