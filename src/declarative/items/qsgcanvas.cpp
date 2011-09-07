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

#include "qsgcanvas.h"
#include "qsgcanvas_p.h"

#include "qsgitem.h"
#include "qsgitem_p.h"

#include "qsgevent.h"

#include <private/qsgrenderer_p.h>
#include <private/qsgflashnode_p.h>

#include <private/qguiapplication_p.h>
#include <QtGui/QInputPanel>

#include <private/qabstractanimation_p.h>

#include <QtGui/qpainter.h>
#include <QtGui/qevent.h>
#include <QtGui/qmatrix4x4.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qabstractanimation.h>

#include <private/qdeclarativedebugtrace_p.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlFixedAnimationStep, QML_FIXED_ANIMATION_STEP)
DEFINE_BOOL_CONFIG_OPTION(qmlNoThreadedRenderer, QML_BAD_GUI_RENDER_LOOP)

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

void QSGCanvasPrivate::updateFocusItemTransform()
{
    Q_Q(QSGCanvas);
    QSGItem *focus = q->activeFocusItem();
    if (focus && qApp->inputPanel()->inputItem() == focus)
        qApp->inputPanel()->setInputItemTransform(QSGItemPrivate::get(focus)->itemToCanvasTransform());
}

class QSGCanvasPlainRenderLoop : public QObject, public QSGCanvasRenderLoop
{
public:
    QSGCanvasPlainRenderLoop()
        : updatePending(false)
        , animationRunning(false)
    {
        qWarning("QSGCanvas: using non-threaded render loop. Be very sure to not access scene graph "
                 "objects outside the QSGItem::updatePaintNode() call. Failing to do so will cause "
                 "your code to crash on other platforms!");
    }

    virtual void paint() {
        if (animationRunning && animationDriver())
            animationDriver()->advance();
        polishItems();
        syncSceneGraph();
        makeCurrent();
        glViewport(0, 0, size.width(), size.height());
        renderSceneGraph(size);
        swapBuffers();
        updatePending = false;

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

    virtual void stopRendering() { }

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

  RenderThread::isExternalUpdatePending: This variable is set to false during
  the sync phase in the GUI thread and to true in maybeUpdate(). It is an
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

QSGItem::UpdatePaintNodeData::UpdatePaintNodeData()
: transformNode(0)
{
}

QSGRootItem::QSGRootItem()
{
}

void QSGCanvas::exposeEvent(QExposeEvent *)
{
    Q_D(QSGCanvas);
    d->thread->paint();
}

void QSGCanvas::resizeEvent(QResizeEvent *)
{
    Q_D(QSGCanvas);
    d->thread->resize(size());
}

void QSGCanvas::animationStarted()
{
    d_func()->thread->animationStarted();
}

void QSGCanvas::animationStopped()
{
    d_func()->thread->animationStopped();
}

void QSGCanvas::showEvent(QShowEvent *)
{
    Q_D(QSGCanvas);
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

void QSGCanvas::hideEvent(QHideEvent *)
{
    Q_D(QSGCanvas);
    d->thread->stopRendering();
}



/*!
    Sets weither this canvas should use vsync driven animations.

    This option can only be set on one single QSGCanvas, and that it's
    vsync signal will then be used to drive all animations in the
    process.

    This feature is primarily useful for single QSGCanvas, QML-only
    applications.

    \warning Enabling vsync on multiple QSGCanvas instances has
    undefined behavior.
 */
void QSGCanvas::setVSyncAnimations(bool enabled)
{
    Q_D(QSGCanvas);
    if (visible()) {
        qWarning("QSGCanvas::setVSyncAnimations: Cannot be changed when widget is shown");
        return;
    }
    d->vsyncAnimations = enabled;
}



/*!
    Returns true if this canvas should use vsync driven animations;
    otherwise returns false.
 */
bool QSGCanvas::vsyncAnimations() const
{
    Q_D(const QSGCanvas);
    return d->vsyncAnimations;
}

/*!
    This function is an attempt to localize all uses of QInputContext::update in
    one place up until the point where we have public API for the QInputContext API.
 */
void QSGCanvasPrivate::updateInputContext()
{
    // ### finer grained updates would be good
    qApp->inputPanel()->update(Qt::ImQueryAll);
}
/*!
    This function is an attempt to localize all uses of QInputContext::reset in
    one place up until the point where we have public API for the QInputContext API.
 */
void QSGCanvasPrivate::resetInputContext()
{
    qApp->inputPanel()->reset();
}


void QSGCanvasPrivate::initializeSceneGraph()
{
    if (!context)
        context = QSGContext::createDefaultContext();

    if (context->isReady())
        return;

    QOpenGLContext *glctx = const_cast<QOpenGLContext *>(QOpenGLContext::currentContext());
    context->initialize(glctx);

    Q_Q(QSGCanvas);
    QObject::connect(context->renderer(), SIGNAL(sceneGraphChanged()), q, SLOT(maybeUpdate()),
                     Qt::DirectConnection);

    if (!QSGItemPrivate::get(rootItem)->itemNode()->parent()) {
        context->rootNode()->appendChildNode(QSGItemPrivate::get(rootItem)->itemNode());
    }

    emit q_func()->sceneGraphInitialized();
}

void QSGCanvasPrivate::polishItems()
{
    while (!itemsToPolish.isEmpty()) {
        QSet<QSGItem *>::Iterator iter = itemsToPolish.begin();
        QSGItem *item = *iter;
        itemsToPolish.erase(iter);
        QSGItemPrivate::get(item)->polishScheduled = false;
        item->updatePolish();
    }
    updateFocusItemTransform();
}


void QSGCanvasPrivate::syncSceneGraph()
{
    updateDirtyNodes();
}


void QSGCanvasPrivate::renderSceneGraph(const QSize &size)
{
    context->renderer()->setDeviceRect(QRect(QPoint(0, 0), size));
    context->renderer()->setViewportRect(QRect(QPoint(0, 0), renderTarget ? renderTarget->size() : size));
    context->renderer()->setProjectionMatrixToDeviceRect();

    context->renderNextFrame(renderTarget);

#ifdef FRAME_TIMING
    sceneGraphRenderTime = frameTimer.elapsed();
#endif

#ifdef FRAME_TIMING
//    int pixel;
//    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel);
    readbackTime = frameTimer.elapsed();
#endif
}


// ### Do we need this?
void QSGCanvas::sceneGraphChanged()
{
//    Q_D(QSGCanvas);
//    d->needsRepaint = true;
}

QSGCanvasPrivate::QSGCanvasPrivate()
    : rootItem(0)
    , activeFocusItem(0)
    , mouseGrabberItem(0)
    , dirtyItemList(0)
    , context(0)
    , vsyncAnimations(false)
    , thread(0)
    , animationDriver(0)
    , renderTarget(0)
{
}

QSGCanvasPrivate::~QSGCanvasPrivate()
{
}

void QSGCanvasPrivate::init(QSGCanvas *c)
{
    QUnifiedTimer::instance(true)->setConsistentTiming(qmlFixedAnimationStep());

    q_ptr = c;

    Q_Q(QSGCanvas);

    rootItem = new QSGRootItem;
    QSGItemPrivate *rootItemPrivate = QSGItemPrivate::get(rootItem);
    rootItemPrivate->canvas = q;
    rootItemPrivate->flags |= QSGItem::ItemIsFocusScope;

    // QML always has focus. It is important that this call happens after the rootItem
    // has a canvas..
    rootItem->setFocus(true);

    bool threaded = !qmlNoThreadedRenderer();

    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedOpenGL)) {
        qWarning("QSGCanvas: platform does not support threaded rendering!");
        threaded = false;
    }

    if (threaded)
        thread = new QSGCanvasRenderThread();
    else
        thread = new QSGCanvasPlainRenderLoop();

    thread->renderer = q;
    thread->d = this;

    context = QSGContext::createDefaultContext();
    thread->moveContextToThread(context);
}

void QSGCanvasPrivate::transformTouchPoints(QList<QTouchEvent::TouchPoint> &touchPoints, const QTransform &transform)
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
void QSGCanvasPrivate::translateTouchEvent(QTouchEvent *touchEvent)
{
//    Q_Q(QSGCanvas);

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

void QSGCanvasPrivate::setFocusInScope(QSGItem *scope, QSGItem *item, FocusOptions options)
{
    Q_Q(QSGCanvas);

    Q_ASSERT(item);
    Q_ASSERT(scope || item == rootItem);

#ifdef FOCUS_DEBUG
    qWarning() << "QSGCanvasPrivate::setFocusInScope():";
    qWarning() << "    scope:" << (QObject *)scope;
    if (scope)
        qWarning() << "    scopeSubFocusItem:" << (QObject *)QSGItemPrivate::get(scope)->subFocusItem;
    qWarning() << "    item:" << (QObject *)item;
    qWarning() << "    activeFocusItem:" << (QObject *)activeFocusItem;
#endif

    QSGItemPrivate *scopePrivate = scope ? QSGItemPrivate::get(scope) : 0;
    QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item);

    QSGItem *oldActiveFocusItem = 0;
    QSGItem *newActiveFocusItem = 0;

    QVarLengthArray<QSGItem *, 20> changed;

    // Does this change the active focus?
    if (item == rootItem || scopePrivate->activeFocus) {
        oldActiveFocusItem = activeFocusItem;
        newActiveFocusItem = item;
        while (newActiveFocusItem->isFocusScope() && newActiveFocusItem->scopedFocusItem())
            newActiveFocusItem = newActiveFocusItem->scopedFocusItem();

        if (oldActiveFocusItem) {
#ifndef QT_NO_IM
            resetInputContext();
#endif

            activeFocusItem = 0;
            QFocusEvent event(QEvent::FocusOut, Qt::OtherFocusReason);
            q->sendEvent(oldActiveFocusItem, &event);

            QSGItem *afi = oldActiveFocusItem;
            while (afi != scope) {
                if (QSGItemPrivate::get(afi)->activeFocus) {
                    QSGItemPrivate::get(afi)->activeFocus = false;
                    changed << afi;
                }
                afi = afi->parentItem();
            }
        }
    }

    if (item != rootItem) {
        QSGItem *oldSubFocusItem = scopePrivate->subFocusItem;
        // Correct focus chain in scope
        if (oldSubFocusItem) {
            QSGItem *sfi = scopePrivate->subFocusItem->parentItem();
            while (sfi != scope) {
                QSGItemPrivate::get(sfi)->subFocusItem = 0;
                sfi = sfi->parentItem();
            }
        }
        {
            scopePrivate->subFocusItem = item;
            QSGItem *sfi = scopePrivate->subFocusItem->parentItem();
            while (sfi != scope) {
                QSGItemPrivate::get(sfi)->subFocusItem = item;
                sfi = sfi->parentItem();
            }
        }

        if (oldSubFocusItem) {
            QSGItemPrivate::get(oldSubFocusItem)->focus = false;
            changed << oldSubFocusItem;
        }
    }

    if (!(options & DontChangeFocusProperty)) {
        // if (item != rootItem || q->hasFocus()) { // ### refactor: focus handling...
            itemPrivate->focus = true;
            changed << item;
        // }
    }

    if (newActiveFocusItem) { // ### refactor:  && q->hasFocus()) {
        activeFocusItem = newActiveFocusItem;

        QSGItemPrivate::get(newActiveFocusItem)->activeFocus = true;
        changed << newActiveFocusItem;

        QSGItem *afi = newActiveFocusItem->parentItem();
        while (afi && afi != scope) {
            if (afi->isFocusScope()) {
                QSGItemPrivate::get(afi)->activeFocus = true;
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

void QSGCanvasPrivate::clearFocusInScope(QSGItem *scope, QSGItem *item, FocusOptions options)
{
    Q_Q(QSGCanvas);

    Q_UNUSED(item);
    Q_ASSERT(item);
    Q_ASSERT(scope || item == rootItem);

#ifdef FOCUS_DEBUG
    qWarning() << "QSGCanvasPrivate::clearFocusInScope():";
    qWarning() << "    scope:" << (QObject *)scope;
    qWarning() << "    item:" << (QObject *)item;
    qWarning() << "    activeFocusItem:" << (QObject *)activeFocusItem;
#endif

    QSGItemPrivate *scopePrivate = scope ? QSGItemPrivate::get(scope) : 0;

    QSGItem *oldActiveFocusItem = 0;
    QSGItem *newActiveFocusItem = 0;

    QVarLengthArray<QSGItem *, 20> changed;

    Q_ASSERT(item == rootItem || item == scopePrivate->subFocusItem);

    // Does this change the active focus?
    if (item == rootItem || scopePrivate->activeFocus) {
        oldActiveFocusItem = activeFocusItem;
        newActiveFocusItem = scope;

        Q_ASSERT(oldActiveFocusItem);

#ifndef QT_NO_IM
        resetInputContext();
#endif

        activeFocusItem = 0;
        QFocusEvent event(QEvent::FocusOut, Qt::OtherFocusReason);
        q->sendEvent(oldActiveFocusItem, &event);

        QSGItem *afi = oldActiveFocusItem;
        while (afi != scope) {
            if (QSGItemPrivate::get(afi)->activeFocus) {
                QSGItemPrivate::get(afi)->activeFocus = false;
                changed << afi;
            }
            afi = afi->parentItem();
        }
    }

    if (item != rootItem) {
        QSGItem *oldSubFocusItem = scopePrivate->subFocusItem;
        // Correct focus chain in scope
        if (oldSubFocusItem) {
            QSGItem *sfi = scopePrivate->subFocusItem->parentItem();
            while (sfi != scope) {
                QSGItemPrivate::get(sfi)->subFocusItem = 0;
                sfi = sfi->parentItem();
            }
        }
        scopePrivate->subFocusItem = 0;

        if (oldSubFocusItem && !(options & DontChangeFocusProperty)) {
            QSGItemPrivate::get(oldSubFocusItem)->focus = false;
            changed << oldSubFocusItem;
        }
    } else if (!(options & DontChangeFocusProperty)) {
        QSGItemPrivate::get(item)->focus = false;
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

void QSGCanvasPrivate::notifyFocusChangesRecur(QSGItem **items, int remaining)
{
    QDeclarativeGuard<QSGItem> item(*items);

    if (remaining)
        notifyFocusChangesRecur(items + 1, remaining - 1);

    if (item) {
        QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item);

        if (itemPrivate->notifiedFocus != itemPrivate->focus) {
            itemPrivate->notifiedFocus = itemPrivate->focus;
            emit item->focusChanged(itemPrivate->focus);
        }

        if (item && itemPrivate->notifiedActiveFocus != itemPrivate->activeFocus) {
            itemPrivate->notifiedActiveFocus = itemPrivate->activeFocus;
            itemPrivate->itemChange(QSGItem::ItemActiveFocusHasChanged, itemPrivate->activeFocus);
            emit item->activeFocusChanged(itemPrivate->activeFocus);
        }
    }
}

void QSGCanvasPrivate::updateInputMethodData()
{
    qApp->inputPanel()->setInputItem(activeFocusItem);
}

QVariant QSGCanvas::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QSGCanvas);
    if (!d->activeFocusItem || !(QSGItemPrivate::get(d->activeFocusItem)->flags & QSGItem::ItemAcceptsInputMethod))
        return QVariant();
    QVariant value = d->activeFocusItem->inputMethodQuery(query);

    //map geometry types
    QVariant::Type type = value.type();
    if (type == QVariant::RectF || type == QVariant::Rect) {
        const QTransform transform = QSGItemPrivate::get(d->activeFocusItem)->itemToCanvasTransform();
        value = transform.mapRect(value.toRectF());
    } else if (type == QVariant::PointF || type == QVariant::Point) {
        const QTransform transform = QSGItemPrivate::get(d->activeFocusItem)->itemToCanvasTransform();
        value = transform.map(value.toPointF());
    }
    return value;
}

void QSGCanvasPrivate::dirtyItem(QSGItem *)
{
    Q_Q(QSGCanvas);
    q->maybeUpdate();
}

void QSGCanvasPrivate::cleanup(QSGNode *n)
{
    Q_Q(QSGCanvas);

    Q_ASSERT(!cleanupNodeList.contains(n));
    cleanupNodeList.append(n);
    q->maybeUpdate();
}


QSGCanvas::QSGCanvas(QWindow *parent)
    : QWindow(*(new QSGCanvasPrivate), parent)
{
    Q_D(QSGCanvas);
    d->init(this);
}

QSGCanvas::QSGCanvas(QSGCanvasPrivate &dd, QWindow *parent)
    : QWindow(dd, parent)
{
    Q_D(QSGCanvas);
    d->init(this);
}

QSGCanvas::~QSGCanvas()
{
    Q_D(QSGCanvas);

    if (d->thread->isRunning()) {
        d->thread->stopRendering();
        delete d->thread;
        d->thread = 0;
    }

    // ### should we change ~QSGItem to handle this better?
    // manually cleanup for the root item (item destructor only handles these when an item is parented)
    QSGItemPrivate *rootItemPrivate = QSGItemPrivate::get(d->rootItem);
    rootItemPrivate->removeFromDirtyList();

    delete d->rootItem; d->rootItem = 0;
    d->cleanupNodes();
}

QSGItem *QSGCanvas::rootItem() const
{
    Q_D(const QSGCanvas);

    return d->rootItem;
}

QSGItem *QSGCanvas::activeFocusItem() const
{
    Q_D(const QSGCanvas);

    return d->activeFocusItem;
}

QSGItem *QSGCanvas::mouseGrabberItem() const
{
    Q_D(const QSGCanvas);

    return d->mouseGrabberItem;
}


bool QSGCanvasPrivate::clearHover()
{
    if (hoverItems.isEmpty())
        return false;

    QPointF pos = QCursor::pos(); // ### refactor: q->mapFromGlobal(QCursor::pos());

    bool accepted = false;
    foreach (QSGItem* item, hoverItems)
        accepted = sendHoverEvent(QEvent::HoverLeave, item, pos, pos, QGuiApplication::keyboardModifiers(), true) || accepted;
    hoverItems.clear();
    return accepted;
}


bool QSGCanvas::event(QEvent *e)
{
    Q_D(QSGCanvas);

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
    case QSGEvent::SGDragEnter:
    case QSGEvent::SGDragExit:
    case QSGEvent::SGDragMove:
    case QSGEvent::SGDragDrop:
        d->deliverDragEvent(static_cast<QSGDragEvent *>(e));
        break;
    case QEvent::WindowDeactivate:
        rootItem()->windowDeactivateEvent();
        break;
    default:
        break;
    }

    return QWindow::event(e);
}

void QSGCanvas::keyPressEvent(QKeyEvent *e)
{
    Q_D(QSGCanvas);

    if (d->activeFocusItem)
        sendEvent(d->activeFocusItem, e);
}

void QSGCanvas::keyReleaseEvent(QKeyEvent *e)
{
    Q_D(QSGCanvas);

    if (d->activeFocusItem)
        sendEvent(d->activeFocusItem, e);
}

void QSGCanvas::inputMethodEvent(QInputMethodEvent *e)
{
    Q_D(QSGCanvas);

    if (d->activeFocusItem)
        sendEvent(d->activeFocusItem, e);
}

bool QSGCanvasPrivate::deliverInitialMousePressEvent(QSGItem *item, QMouseEvent *event)
{
    Q_Q(QSGCanvas);

    QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item);
    if (itemPrivate->opacity == 0.0)
        return false;

    if (itemPrivate->flags & QSGItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(event->windowPos());
        if (!QRectF(0, 0, item->width(), item->height()).contains(p))
            return false;
    }

    QList<QSGItem *> children = itemPrivate->paintOrderChildItems();
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        QSGItem *child = children.at(ii);
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

bool QSGCanvasPrivate::deliverMouseEvent(QMouseEvent *event)
{
    Q_Q(QSGCanvas);

    lastMousePosition = event->windowPos();

    if (!mouseGrabberItem && 
         event->type() == QEvent::MouseButtonPress &&
         (event->button() & event->buttons()) == event->buttons()) {
        
        return deliverInitialMousePressEvent(rootItem, event);
    }

    if (mouseGrabberItem) {
        QSGItemPrivate *mgPrivate = QSGItemPrivate::get(mouseGrabberItem);
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

void QSGCanvas::mousePressEvent(QMouseEvent *event)
{
    Q_D(QSGCanvas);

#ifdef MOUSE_DEBUG
    qWarning() << "QSGCanvas::mousePressEvent()" << event->pos() << event->button() << event->buttons();
#endif

    d->deliverMouseEvent(event);
}

void QSGCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QSGCanvas);

#ifdef MOUSE_DEBUG
    qWarning() << "QSGCanvas::mouseReleaseEvent()" << event->pos() << event->button() << event->buttons();
#endif

    if (!d->mouseGrabberItem) {
        QWindow::mouseReleaseEvent(event);
        return;
    }

    d->deliverMouseEvent(event);
    d->mouseGrabberItem = 0;
}

void QSGCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QSGCanvas);

#ifdef MOUSE_DEBUG
    qWarning() << "QSGCanvas::mouseDoubleClickEvent()" << event->pos() << event->button() << event->buttons();
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

bool QSGCanvasPrivate::sendHoverEvent(QEvent::Type type, QSGItem *item,
                                      const QPointF &scenePos, const QPointF &lastScenePos,
                                      Qt::KeyboardModifiers modifiers, bool accepted)
{
    Q_Q(QSGCanvas);
    const QTransform transform = QSGItemPrivate::get(item)->canvasToItemTransform();

    //create copy of event
    QHoverEvent hoverEvent(type, transform.map(scenePos), transform.map(lastScenePos), modifiers);
    hoverEvent.setAccepted(accepted);

    q->sendEvent(item, &hoverEvent);

    return hoverEvent.isAccepted();
}

void QSGCanvas::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QSGCanvas);

#ifdef MOUSE_DEBUG
    qWarning() << "QSGCanvas::mouseMoveEvent()" << event->pos() << event->button() << event->buttons();
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

bool QSGCanvasPrivate::deliverHoverEvent(QSGItem *item, const QPointF &scenePos, const QPointF &lastScenePos,
                                         Qt::KeyboardModifiers modifiers, bool &accepted)
{
    QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item);
    if (itemPrivate->opacity == 0.0)
        return false;

    if (itemPrivate->flags & QSGItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(scenePos);
        if (!QRectF(0, 0, item->width(), item->height()).contains(p))
            return false;
    }

    QList<QSGItem *> children = itemPrivate->paintOrderChildItems();
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        QSGItem *child = children.at(ii);
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
                QList<QSGItem *> itemsToHover;
                QSGItem* parent = item;
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
                        QSGItem *itemToHover = itemsToHover[i];
                        if (QSGItemPrivate::get(itemToHover)->hoverEnabled) {
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

bool QSGCanvasPrivate::deliverWheelEvent(QSGItem *item, QWheelEvent *event)
{
    Q_Q(QSGCanvas);
    QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item);
    if (itemPrivate->opacity == 0.0)
        return false;

    if (itemPrivate->flags & QSGItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(event->posF());
        if (!QRectF(0, 0, item->width(), item->height()).contains(p))
            return false;
    }

    QList<QSGItem *> children = itemPrivate->paintOrderChildItems();
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        QSGItem *child = children.at(ii);
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
void QSGCanvas::wheelEvent(QWheelEvent *event)
{
    Q_D(QSGCanvas);
#ifdef MOUSE_DEBUG
    qWarning() << "QSGCanvas::wheelEvent()" << event->pos() << event->delta() << event->orientation();
#endif
    event->ignore();
    d->deliverWheelEvent(d->rootItem, event);
}
#endif // QT_NO_WHEELEVENT

bool QSGCanvasPrivate::deliverTouchEvent(QTouchEvent *event)
{
#ifdef TOUCH_DEBUG
    if (event->type() == QEvent::TouchBegin)
        qWarning("touchBeginEvent");
    else if (event->type() == QEvent::TouchUpdate)
        qWarning("touchUpdateEvent");
    else if (event->type() == QEvent::TouchEnd)
        qWarning("touchEndEvent");
#endif

    QHash<QSGItem *, QList<QTouchEvent::TouchPoint> > updatedPoints;

    if (event->type() == QTouchEvent::TouchBegin) {     // all points are new touch points
        QSet<int> acceptedNewPoints;
        deliverTouchPoints(rootItem, event, event->touchPoints(), &acceptedNewPoints, &updatedPoints);
        if (acceptedNewPoints.count() > 0)
            event->accept();
        return event->isAccepted();
    }

    const QList<QTouchEvent::TouchPoint> &touchPoints = event->touchPoints();
    QList<QTouchEvent::TouchPoint> newPoints;
    QSGItem *item = 0;
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

bool QSGCanvasPrivate::deliverTouchPoints(QSGItem *item, QTouchEvent *event, const QList<QTouchEvent::TouchPoint> &newPoints, QSet<int> *acceptedNewPoints, QHash<QSGItem *, QList<QTouchEvent::TouchPoint> > *updatedPoints)
{
    Q_Q(QSGCanvas);
    QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item);

    if (itemPrivate->opacity == 0.0)
        return false;

    if (itemPrivate->flags & QSGItem::ItemClipsChildrenToShape) {
        QRectF bounds(0, 0, item->width(), item->height());
        for (int i=0; i<newPoints.count(); i++) {
            QPointF p = item->mapFromScene(newPoints[i].scenePos());
            if (!bounds.contains(p))
                return false;
        }
    }

    QList<QSGItem *> children = itemPrivate->paintOrderChildItems();
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        QSGItem *child = children.at(ii);
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

void QSGCanvasPrivate::deliverDragEvent(QSGDragEvent *event)
{
    Q_Q(QSGCanvas);
    if (event->type() == QSGEvent::SGDragExit || event->type() == QSGEvent::SGDragDrop) {
        if (QSGItem *grabItem = event->grabItem()) {
            event->setPosition(grabItem->mapFromScene(event->scenePosition()));
            q->sendEvent(grabItem, event);
        }
    } else if (!deliverDragEvent(rootItem, event)) {
        if (QSGItem *grabItem = event->grabItem()) {
            QSGDragEvent exitEvent(QSGEvent::SGDragExit, *event);
            exitEvent.setPosition(grabItem->mapFromScene(event->scenePosition()));
            q->sendEvent(grabItem, &exitEvent);
            event->setDropItem(0);
            event->setGrabItem(0);
        }
        event->setAccepted(false);
    }
}

bool QSGCanvasPrivate::deliverDragEvent(QSGItem *item, QSGDragEvent *event)
{
    Q_Q(QSGCanvas);
    QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item);
    if (itemPrivate->opacity == 0.0)
        return false;

    if (itemPrivate->flags & QSGItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(event->scenePosition());
        if (!QRectF(0, 0, item->width(), item->height()).contains(p))
            return false;
    }

    QList<QSGItem *> children = itemPrivate->paintOrderChildItems();
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        QSGItem *child = children.at(ii);
        if (!child->isVisible() || !child->isEnabled())
            continue;
        if (deliverDragEvent(child, event))
            return true;
    }

    QPointF p = item->mapFromScene(event->scenePosition());
    if (QRectF(0, 0, item->width(), item->height()).contains(p)) {
        event->setPosition(p);

        if (event->type() == QSGEvent::SGDragMove && item != event->grabItem()) {
            QSGDragEvent enterEvent(QSGEvent::SGDragEnter, *event);
            q->sendEvent(item, &enterEvent);
            if (enterEvent.isAccepted()) {
                if (QSGItem *grabItem = event->grabItem()) {
                    QSGDragEvent exitEvent(QSGEvent::SGDragExit, *event);
                    q->sendEvent(grabItem, &exitEvent);
                }
                event->setDropItem(enterEvent.dropItem());
                event->setGrabItem(item);
            } else {
                return false;
            }
        }

        q->sendEvent(item, event);
        if (event->isAccepted()) {
            event->setGrabItem(item);
            return true;
        }
        event->setAccepted(true);
    }

    return false;
}

bool QSGCanvasPrivate::sendFilteredMouseEvent(QSGItem *target, QSGItem *item, QMouseEvent *event)
{
    if (!target)
        return false;

    if (sendFilteredMouseEvent(target->parentItem(), item, event))
        return true;

    QSGItemPrivate *targetPrivate = QSGItemPrivate::get(target);
    if (targetPrivate->filtersChildMouseEvents)
        if (target->childMouseEventFilter(item, event))
            return true;

    return false;
}

bool QSGCanvas::sendEvent(QSGItem *item, QEvent *e)
{
    Q_D(QSGCanvas);

    if (!item) {
        qWarning("QSGCanvas::sendEvent: Cannot send event to a null item");
        return false;
    }

    Q_ASSERT(e);

    switch (e->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        e->accept();
        QSGItemPrivate::get(item)->deliverKeyEvent(static_cast<QKeyEvent *>(e));
        while (!e->isAccepted() && (item = item->parentItem())) {
            e->accept();
            QSGItemPrivate::get(item)->deliverKeyEvent(static_cast<QKeyEvent *>(e));
        }
        break;
    case QEvent::InputMethod:
        e->accept();
        QSGItemPrivate::get(item)->deliverInputMethodEvent(static_cast<QInputMethodEvent *>(e));
        while (!e->isAccepted() && (item = item->parentItem())) {
            e->accept();
            QSGItemPrivate::get(item)->deliverInputMethodEvent(static_cast<QInputMethodEvent *>(e));
        }
        break;
    case QEvent::FocusIn:
    case QEvent::FocusOut:
        QSGItemPrivate::get(item)->deliverFocusEvent(static_cast<QFocusEvent *>(e));
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
        // XXX todo - should sendEvent be doing this?  how does it relate to forwarded events? 
        {
            QMouseEvent *se = static_cast<QMouseEvent *>(e);
            if (!d->sendFilteredMouseEvent(item->parentItem(), item, se)) {
                se->accept();
                QSGItemPrivate::get(item)->deliverMouseEvent(se);
            }
        }
        break;
    case QEvent::Wheel:
        QSGItemPrivate::get(item)->deliverWheelEvent(static_cast<QWheelEvent *>(e));
        break;
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
        QSGItemPrivate::get(item)->deliverHoverEvent(static_cast<QHoverEvent *>(e));
        break;
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        QSGItemPrivate::get(item)->deliverTouchEvent(static_cast<QTouchEvent *>(e));
        break;
    case QSGEvent::SGDragEnter:
    case QSGEvent::SGDragExit:
    case QSGEvent::SGDragMove:
    case QSGEvent::SGDragDrop:
        QSGItemPrivate::get(item)->deliverDragEvent(static_cast<QSGDragEvent *>(e));
        break;
    default:
        break;
    }

    return false;
}

void QSGCanvasPrivate::cleanupNodes()
{
    for (int ii = 0; ii < cleanupNodeList.count(); ++ii)
        delete cleanupNodeList.at(ii);
    cleanupNodeList.clear();
}

void QSGCanvasPrivate::updateDirtyNodes()
{
#ifdef DIRTY_DEBUG
    qWarning() << "QSGCanvasPrivate::updateDirtyNodes():";
#endif

    cleanupNodes();

    QSGItem *updateList = dirtyItemList;
    dirtyItemList = 0;
    if (updateList) QSGItemPrivate::get(updateList)->prevDirtyItem = &updateList;

    while (updateList) {
        QSGItem *item = updateList;
        QSGItemPrivate *itemPriv = QSGItemPrivate::get(item);
        itemPriv->removeFromDirtyList();

#ifdef DIRTY_DEBUG
        qWarning() << "   QSGNode:" << item << qPrintable(itemPriv->dirtyToString());
#endif
        updateDirtyNode(item);
    }
}

void QSGCanvasPrivate::updateDirtyNode(QSGItem *item)
{
#ifdef QML_RUNTIME_TESTING
    bool didFlash = false;
#endif

    QSGItemPrivate *itemPriv = QSGItemPrivate::get(item);
    quint32 dirty = itemPriv->dirtyAttributes;
    itemPriv->dirtyAttributes = 0;

    if ((dirty & QSGItemPrivate::TransformUpdateMask) ||
        (dirty & QSGItemPrivate::Size && itemPriv->origin != QSGItem::TopLeft &&
         (itemPriv->scale != 1. || itemPriv->rotation != 0.))) {

        QMatrix4x4 matrix;

        if (itemPriv->x != 0. || itemPriv->y != 0.)
            matrix.translate(itemPriv->x, itemPriv->y);

        for (int ii = itemPriv->transforms.count() - 1; ii >= 0; --ii)
            itemPriv->transforms.at(ii)->applyTo(&matrix);

        if (itemPriv->scale != 1. || itemPriv->rotation != 0.) {
            QPointF origin = itemPriv->computeTransformOrigin();
            matrix.translate(origin.x(), origin.y());
            if (itemPriv->scale != 1.)
                matrix.scale(itemPriv->scale, itemPriv->scale);
            if (itemPriv->rotation != 0.)
                matrix.rotate(itemPriv->rotation, 0, 0, 1);
            matrix.translate(-origin.x(), -origin.y());
        }

        itemPriv->itemNode()->setMatrix(matrix);
    }

    bool clipEffectivelyChanged = dirty & QSGItemPrivate::Clip &&
                                  ((item->clip() == false) != (itemPriv->clipNode == 0));
    bool effectRefEffectivelyChanged = dirty & QSGItemPrivate::EffectReference &&
                                  ((itemPriv->effectRefCount == 0) != (itemPriv->rootNode == 0));

    if (clipEffectivelyChanged) {
        QSGNode *parent = itemPriv->opacityNode ? (QSGNode *) itemPriv->opacityNode : (QSGNode *)itemPriv->itemNode();
        QSGNode *child = itemPriv->rootNode ? (QSGNode *)itemPriv->rootNode : (QSGNode *)itemPriv->groupNode;

        if (item->clip()) {
            Q_ASSERT(itemPriv->clipNode == 0);
            itemPriv->clipNode = new QSGDefaultClipNode(item->boundingRect());
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

    if (dirty & QSGItemPrivate::ChildrenUpdateMask)
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

    if (dirty & QSGItemPrivate::ChildrenUpdateMask) {
        QSGNode *groupNode = itemPriv->groupNode;
        if (groupNode)
            groupNode->removeAllChildNodes();

        QList<QSGItem *> orderedChildren = itemPriv->paintOrderChildItems();
        int ii = 0;

        for (; ii < orderedChildren.count() && orderedChildren.at(ii)->z() < 0; ++ii) {
            QSGItemPrivate *childPrivate = QSGItemPrivate::get(orderedChildren.at(ii));
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
            QSGItemPrivate *childPrivate = QSGItemPrivate::get(orderedChildren.at(ii));
            if (!childPrivate->explicitVisible && !childPrivate->effectRefCount)
                continue;
            if (childPrivate->itemNode()->parent())
                childPrivate->itemNode()->parent()->removeChildNode(childPrivate->itemNode());

            itemPriv->childContainerNode()->appendChildNode(childPrivate->itemNode());
        }
    }

    if ((dirty & QSGItemPrivate::Size) && itemPriv->clipNode) {
        itemPriv->clipNode->setRect(item->boundingRect());
        itemPriv->clipNode->update();
    }

    if (dirty & (QSGItemPrivate::OpacityValue | QSGItemPrivate::Visible | QSGItemPrivate::HideReference)) {
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

    if (dirty & QSGItemPrivate::ContentUpdateMask) {

        if (itemPriv->flags & QSGItem::ItemHasContents) {
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
    Q_Q(QSGCanvas);
    if (didFlash) {
        q->maybeUpdate();
    }
#endif

}

void QSGCanvas::maybeUpdate()
{
    Q_D(QSGCanvas);

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
 */

QSGEngine *QSGCanvas::sceneGraphEngine() const
{
    Q_D(const QSGCanvas);
    if (d->context && d->context->isReady())
        return d->context->engine();
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

void QSGCanvas::setRenderTarget(QOpenGLFramebufferObject *fbo)
{
    Q_D(QSGCanvas);
    if (d->context && d->context && QThread::currentThread() != d->context->thread()) {
        qWarning("QSGCanvas::setRenderThread: Cannot set render target from outside the rendering thread");
        return;
    }

    d->renderTarget = fbo;
}



/*!
    Returns the render target for this canvas.

    The default is to render to the surface of the canvas, in which
    case the render target is 0.
 */
QOpenGLFramebufferObject *QSGCanvas::renderTarget() const
{
    Q_D(const QSGCanvas);
    return d->renderTarget;
}


/*!
    Grabs the contents of the framebuffer and returns it as an image.

    This function might not work if the view is not visible.

    \warning Calling this function will cause performance problems.

    \warning This function can only be called from the GUI thread.
 */
QImage QSGCanvas::grabFrameBuffer()
{
    Q_D(QSGCanvas);
    return d->thread ? d->thread->grab() : QImage();
}



void QSGCanvasRenderLoop::createGLContext()
{
    gl = new QOpenGLContext();
    gl->create();
}


void QSGCanvasRenderThread::run()
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
        inSync = true;
        syncSceneGraph();
        inSync = false;

        // Wake GUI after sync to let it continue animating and event processing.
        wake();
        unlock();
#ifdef THREAD_DEBUG
        printf("                RenderThread: sync done\n");
#endif



#ifdef THREAD_DEBUG
        printf("                RenderThread: rendering... %d x %d\n", windowSize.width(), windowSize.height());
#endif

        renderSceneGraph(windowSize);

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

        // Process any "deleteLater" objects...
        QCoreApplication::processEvents();
    }

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



bool QSGCanvasRenderThread::event(QEvent *e)
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



void QSGCanvasRenderThread::exhaustSyncEvent()
{
    if (isGuiBlockPending) {
        sync(true);
        syncAlreadyHappened = true;
    }
}



void QSGCanvasRenderThread::sync(bool guiAlreadyLocked)
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
    variable to keep track of how many recursion levels the gui is locket with.
    We only actually acquire the mutex for the first level to avoid deadlocking
    ourselves.
 */

void QSGCanvasRenderThread::lockInGui()
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



void QSGCanvasRenderThread::unlockInGui()
{
#ifdef THREAD_DEBUG
    printf("GUI: releasing lock... %d\n", isGuiBlocked);
#endif
    --isGuiBlocked;
    if (!isGuiBlocked)
        unlock();
}




void QSGCanvasRenderThread::animationStarted()
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



void QSGCanvasRenderThread::animationStopped()
{
#ifdef THREAD_DEBUG
    printf("GUI: animationStopped()...\n");
#endif

    lockInGui();
    animationRunning = false;
    unlockInGui();
}


void QSGCanvasRenderThread::paint()
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



void QSGCanvasRenderThread::resize(const QSize &size)
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



void QSGCanvasRenderThread::startRendering()
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



void QSGCanvasRenderThread::stopRendering()
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



QImage QSGCanvasRenderThread::grab()
{
    if (!isRunning())
        return QImage();

    if (QThread::currentThread() != qApp->thread()) {
        qWarning("QSGCanvas::grabFrameBuffer: can only be called from the GUI thread");
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



void QSGCanvasRenderThread::maybeUpdate()
{
    Q_ASSERT_X(QThread::currentThread() == QCoreApplication::instance()->thread() || inSync,
               "QSGCanvas::update",
               "Function can only be called from GUI thread or during QSGItem::updatePaintNode()");

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


#include "moc_qsgcanvas.cpp"

QT_END_NAMESPACE
