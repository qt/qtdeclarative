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

#include <private/qabstractanimation_p.h>

#include <QtGui/qpainter.h>
#include <QtGui/qgraphicssceneevent.h>
#include <QtGui/qmatrix4x4.h>
#include <QtGui/qinputcontext.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qabstractanimation.h>

#include <private/qdeclarativedebugtrace_p.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlNoThreadedRenderer, QML_NO_THREADED_RENDERER)
DEFINE_BOOL_CONFIG_OPTION(qmlFixedAnimationStep, QML_FIXED_ANIMATION_STEP)

extern Q_OPENGL_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

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

void QSGCanvas::paintEvent(QPaintEvent *)
{
    Q_D(QSGCanvas);

    if (!d->threadedRendering) {
#ifdef FRAME_TIMING
        int lastFrame = frameTimer.restart();
#endif

        if (d->animationDriver && d->animationDriver->isRunning())
            d->animationDriver->advance();

#ifdef FRAME_TIMING
        int animationTime = frameTimer.elapsed();
#endif

        Q_ASSERT(d->context);

        d->polishItems();

        QDeclarativeDebugTrace::addEvent(QDeclarativeDebugTrace::FramePaint);
        QDeclarativeDebugTrace::startRange(QDeclarativeDebugTrace::Painting);

#ifdef FRAME_TIMING
        int polishTime = frameTimer.elapsed();
#endif

        makeCurrent();

#ifdef FRAME_TIMING
        int makecurrentTime = frameTimer.elapsed();
#endif

        d->syncSceneGraph();

#ifdef FRAME_TIMING
        int syncTime = frameTimer.elapsed();
#endif

        d->renderSceneGraph(d->widgetSize);

        swapBuffers();

#ifdef FRAME_TIMING
        printf("FrameTimes, last=%d, animations=%d, polish=%d, makeCurrent=%d, sync=%d, sgrender=%d, readback=%d, total=%d\n",
               lastFrame,
               animationTime,
               polishTime - animationTime,
               makecurrentTime - polishTime,
               syncTime - makecurrentTime,
               sceneGraphRenderTime - syncTime,
               readbackTime - sceneGraphRenderTime,
               frameTimer.elapsed());
#endif

        QDeclarativeDebugTrace::endRange(QDeclarativeDebugTrace::Painting);

        if (d->animationDriver && d->animationDriver->isRunning())
            update();
    } else {
        if (updatesEnabled()) {
            d->thread->paint();
            setUpdatesEnabled(false);
        }
    }
}

void QSGCanvas::resizeEvent(QResizeEvent *e)
{
    Q_D(QSGCanvas);
    if (d->threadedRendering) {
        d->thread->resize(e->size());
    } else {
        d->widgetSize = e->size();
        d->viewportSize = d->widgetSize;
        QGLWidget::resizeEvent(e);
    }
}

void QSGCanvas::showEvent(QShowEvent *e)
{
    Q_D(QSGCanvas);

    QGLWidget::showEvent(e);

    if (!d->contextFailed) {
        if (d->threadedRendering) {
            if (d->vsyncAnimations) {
                if (!d->animationDriver) {
                    d->animationDriver = d->context->createAnimationDriver(this);
                    connect(d->animationDriver, SIGNAL(started()), d->thread, SLOT(animationStarted()), Qt::DirectConnection);
                    connect(d->animationDriver, SIGNAL(stopped()), d->thread, SLOT(animationStopped()), Qt::DirectConnection);
                }
                d->animationDriver->install();
            }
            d->thread->startRenderThread();
            setUpdatesEnabled(true);
        } else {
            makeCurrent();

            if (!d->context || !d->context->isReady()) {
                d->initializeSceneGraph();
                if (d->vsyncAnimations) {
                    d->animationDriver = d->context->createAnimationDriver(this);
                    connect(d->animationDriver, SIGNAL(started()), this, SLOT(update()));
                }
            }

            if (d->animationDriver)
                d->animationDriver->install();
        }
    }
}

void QSGCanvas::hideEvent(QHideEvent *e)
{
    Q_D(QSGCanvas);

    if (!d->contextFailed) {
        if (d->threadedRendering) {
            d->thread->stopRenderThread();
        }

        if (d->animationDriver)
            d->animationDriver->uninstall();
    }

    QGLWidget::hideEvent(e);
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
    if (isVisible()) {
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



void QSGCanvas::focusOutEvent(QFocusEvent *event)
{
    Q_D(QSGCanvas);
    d->rootItem->setFocus(false);
    QGLWidget::focusOutEvent(event);
}

void QSGCanvas::focusInEvent(QFocusEvent *event)
{
    Q_D(QSGCanvas);
    d->rootItem->setFocus(true);
    QGLWidget::focusInEvent(event);
}

void QSGCanvasPrivate::initializeSceneGraph()
{
    if (!context)
        context = QSGContext::createDefaultContext();

    if (context->isReady())
        return;

    QGLContext *glctx = const_cast<QGLContext *>(QGLContext::currentContext());
    context->initialize(glctx);

    if (!threadedRendering) {
        Q_Q(QSGCanvas);
        QObject::connect(context->renderer(), SIGNAL(sceneGraphChanged()), q, SLOT(maybeUpdate()),
                         Qt::DirectConnection);
    }

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
    , contextFailed(false)
    , threadedRendering(false)
    , animationRunning(false)
    , renderThreadAwakened(false)
    , vsyncAnimations(false)
    , thread(0)
    , animationDriver(0)
    , renderTarget(0)
{
    threadedRendering = !qmlNoThreadedRenderer();
}

QSGCanvasPrivate::~QSGCanvasPrivate()
{
}

void QSGCanvasPrivate::init(QSGCanvas *c)
{
    QUnifiedTimer::instance(true)->setConsistentTiming(qmlFixedAnimationStep());

    if (!c->context() || !c->context()->isValid()) {
        contextFailed = true;
        qWarning("QSGCanvas: Couldn't acquire a GL context.");
    }

    q_ptr = c;

    Q_Q(QSGCanvas);

    q->setAttribute(Qt::WA_AcceptTouchEvents);
    q->setFocusPolicy(Qt::StrongFocus);

    rootItem = new QSGRootItem;
    QSGItemPrivate *rootItemPrivate = QSGItemPrivate::get(rootItem);
    rootItemPrivate->canvas = q;
    rootItemPrivate->flags |= QSGItem::ItemIsFocusScope;

    context = QSGContext::createDefaultContext();

    if (threadedRendering) {
        thread = new QSGCanvasRenderThread;
        thread->renderer = q;
        thread->d = this;
    }

}

void QSGCanvasPrivate::sceneMouseEventForTransform(QGraphicsSceneMouseEvent &sceneEvent,
                                                   const QTransform &transform)
{
    sceneEvent.setPos(transform.map(sceneEvent.scenePos()));
    sceneEvent.setLastPos(transform.map(sceneEvent.lastScenePos()));
    for (int ii = 0; ii < 5; ++ii) {
        if (sceneEvent.buttons() & (1 << ii)) {
            sceneEvent.setButtonDownPos((Qt::MouseButton)(1 << ii), 
                                        transform.map(sceneEvent.buttonDownScenePos((Qt::MouseButton)(1 << ii))));
        }
    }
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

QEvent::Type QSGCanvasPrivate::sceneMouseEventTypeFromMouseEvent(QMouseEvent *event)
{
    switch(event->type()) {
    default:
        Q_ASSERT(!"Unknown event type");
    case QEvent::MouseButtonPress:
        return QEvent::GraphicsSceneMousePress;
    case QEvent::MouseButtonRelease:
        return QEvent::GraphicsSceneMouseRelease;
    case QEvent::MouseButtonDblClick:
        return QEvent::GraphicsSceneMouseDoubleClick;
    case QEvent::MouseMove:
        return QEvent::GraphicsSceneMouseMove;
    }
}

/*!
Fill in the data in \a sceneEvent based on \a event.  This method leaves the item local positions in
\a sceneEvent untouched.  Use sceneMouseEventForTransform() to fill in those details.
*/
void QSGCanvasPrivate::sceneMouseEventFromMouseEvent(QGraphicsSceneMouseEvent &sceneEvent, QMouseEvent *event)
{
    Q_Q(QSGCanvas);

    Q_ASSERT(event);

    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        if ((event->button() & event->buttons()) == event->buttons()) {
            lastMousePosition = event->pos();
        }

        switch (event->button()) {
        default:
            Q_ASSERT(!"Unknown button");
        case Qt::LeftButton:
            buttonDownPositions[0] = event->pos();
            break;
        case Qt::RightButton:
            buttonDownPositions[1] = event->pos();
            break;
        case Qt::MiddleButton:
            buttonDownPositions[2] = event->pos();
            break;
        case Qt::XButton1:
            buttonDownPositions[3] = event->pos();
            break;
        case Qt::XButton2:
            buttonDownPositions[4] = event->pos();
            break;
        }
    }

    sceneEvent.setScenePos(event->pos());
    sceneEvent.setScreenPos(event->globalPos());
    sceneEvent.setLastScenePos(lastMousePosition);
    sceneEvent.setLastScreenPos(q->mapToGlobal(lastMousePosition));
    sceneEvent.setButtons(event->buttons());
    sceneEvent.setButton(event->button());
    sceneEvent.setModifiers(event->modifiers());
    sceneEvent.setWidget(q);

    for (int ii = 0; ii < 5; ++ii) {
        if (sceneEvent.buttons() & (1 << ii)) {
            sceneEvent.setButtonDownScenePos((Qt::MouseButton)(1 << ii), buttonDownPositions[ii]);
            sceneEvent.setButtonDownScreenPos((Qt::MouseButton)(1 << ii), q->mapToGlobal(buttonDownPositions[ii]));
        }
    }

    lastMousePosition = event->pos();
}

/*!
Translates the data in \a touchEvent to this canvas.  This method leaves the item local positions in
\a touchEvent untouched (these are filled in later).
*/
void QSGCanvasPrivate::translateTouchEvent(QTouchEvent *touchEvent)
{
    Q_Q(QSGCanvas);

    touchEvent->setWidget(q);

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
            if (QInputContext *ic = inputContext())
                ic->reset();
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
        if (item != rootItem || q->hasFocus()) {
            itemPrivate->focus = true;
            changed << item;
        }
    }

    if (newActiveFocusItem && q->hasFocus()) {
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
        if (QInputContext *ic = inputContext())
            ic->reset();
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
    Q_Q(QSGCanvas);
    bool enabled = activeFocusItem
                   && (QSGItemPrivate::get(activeFocusItem)->flags & QSGItem::ItemAcceptsInputMethod);
    q->setAttribute(Qt::WA_InputMethodEnabled, enabled);
    q->setInputMethodHints(enabled ? activeFocusItem->inputMethodHints() : Qt::ImhNone);
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

static QGLFormat tweakFormat(const QGLFormat &format = QGLFormat::defaultFormat())
{
    QGLFormat f = format;
    f.setSwapInterval(1);
    return f;
}

QSGCanvas::QSGCanvas(QWidget *parent, Qt::WindowFlags f)
    : QGLWidget(*(new QSGCanvasPrivate), tweakFormat(), parent, (QGLWidget *) 0, f)
{
    Q_D(QSGCanvas);

    d->init(this);
}

QSGCanvas::QSGCanvas(const QGLFormat &format, QWidget *parent, Qt::WindowFlags f)
    : QGLWidget(*(new QSGCanvasPrivate), tweakFormat(format), parent, (QGLWidget *) 0, f)
{
    Q_D(QSGCanvas);

    d->init(this);
}

QSGCanvas::QSGCanvas(QSGCanvasPrivate &dd, QWidget *parent, Qt::WindowFlags f)
: QGLWidget(dd, tweakFormat(), parent, 0, f)
{
    Q_D(QSGCanvas);

    d->init(this);
}

QSGCanvas::QSGCanvas(QSGCanvasPrivate &dd, const QGLFormat &format, QWidget *parent, Qt::WindowFlags f)
: QGLWidget(dd, tweakFormat(format), parent, 0, f)
{
    Q_D(QSGCanvas);

    d->init(this);
}

QSGCanvas::~QSGCanvas()
{
    Q_D(QSGCanvas);

    if (d->threadedRendering && d->thread->isRunning()) {
        d->thread->stopRenderThread();
        delete d->thread;
        d->thread = 0;
    }

    // ### should we change ~QSGItem to handle this better?
    // manually cleanup for the root item (item destructor only handles these when an item is parented)
    QSGItemPrivate *rootItemPrivate = QSGItemPrivate::get(d->rootItem);
    rootItemPrivate->removeFromDirtyList();

    delete d->rootItem; d->rootItem = 0;
    d->cleanupNodes();

    if (!d->contextFailed) {
        // We need to remove all references to textures pointing to "our" QSGContext
        // from the QDeclarativePixmapCache. Call into the cache to remove the GL / Scene Graph
        // part of those cache entries.
        // To "play nice" with other GL apps that are potentially running in the GUI thread,
        // We get the current context and only temporarily make our own current
        QGLContext *currentContext = const_cast<QGLContext *>(QGLContext::currentContext());
        makeCurrent();
        extern void qt_declarative_pixmapstore_clean(QSGContext *context);
        qt_declarative_pixmapstore_clean(d->context);
        delete d->context;
        if (currentContext)
            currentContext->makeCurrent();
    }
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
    Q_Q(QSGCanvas);
    if (hoverItems.isEmpty())
        return false;

    QPointF pos = q->mapFromGlobal(QCursor::pos());

    bool accepted = false;
    foreach (QSGItem* item, hoverItems)
        accepted = sendHoverEvent(QEvent::HoverLeave, item, pos, pos, QApplication::keyboardModifiers(), true) || accepted;
    hoverItems.clear();
    return accepted;
}


bool QSGCanvas::event(QEvent *e)
{
    Q_D(QSGCanvas);

    if (e->type() == QEvent::User) {
        if (!d->thread->syncAlreadyHappened)
            d->thread->sync(false);
        else
            d->renderThreadAwakened = false;

        d->thread->syncAlreadyHappened = false;

        if (d->animationRunning && d->animationDriver) {
#ifdef THREAD_DEBUG
            qDebug("GUI: Advancing animations...\n");
#endif

            d->animationDriver->advance();

#ifdef THREAD_DEBUG
            qDebug("GUI: Animations advanced...\n");
#endif
        }

        return true;
    }

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

    return QGLWidget::event(e);
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

bool QSGCanvasPrivate::deliverInitialMousePressEvent(QSGItem *item, QGraphicsSceneMouseEvent *event)
{
    Q_Q(QSGCanvas);

    QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item);
    if (itemPrivate->opacity == 0.0)
        return false;

    if (itemPrivate->flags & QSGItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(event->scenePos());
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
        QPointF p = item->mapFromScene(event->scenePos());
        if (QRectF(0, 0, item->width(), item->height()).contains(p)) {
            sceneMouseEventForTransform(*event, itemPrivate->canvasToItemTransform());
            event->accept();
            mouseGrabberItem = item;
            q->sendEvent(item, event);
            if (event->isAccepted()) 
                return true;
            mouseGrabberItem->ungrabMouse();
            mouseGrabberItem = 0;
        }
    }

    return false;
}

bool QSGCanvasPrivate::deliverMouseEvent(QGraphicsSceneMouseEvent *sceneEvent)
{
    Q_Q(QSGCanvas);

    if (!mouseGrabberItem && 
         sceneEvent->type() == QEvent::GraphicsSceneMousePress &&
         (sceneEvent->button() & sceneEvent->buttons()) == sceneEvent->buttons()) {
        
        return deliverInitialMousePressEvent(rootItem, sceneEvent);
    }

    if (mouseGrabberItem) {
        QSGItemPrivate *mgPrivate = QSGItemPrivate::get(mouseGrabberItem);
        sceneMouseEventForTransform(*sceneEvent, mgPrivate->canvasToItemTransform());

        sceneEvent->accept();
        q->sendEvent(mouseGrabberItem, sceneEvent);
        if (sceneEvent->isAccepted())
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

    QGraphicsSceneMouseEvent sceneEvent(d->sceneMouseEventTypeFromMouseEvent(event));
    d->sceneMouseEventFromMouseEvent(sceneEvent, event);

    d->deliverMouseEvent(&sceneEvent);
    event->setAccepted(sceneEvent.isAccepted());
}

void QSGCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QSGCanvas);
    
#ifdef MOUSE_DEBUG
    qWarning() << "QSGCanvas::mouseReleaseEvent()" << event->pos() << event->button() << event->buttons();
#endif

    if (!d->mouseGrabberItem) {
        QGLWidget::mouseReleaseEvent(event);
        return;
    }

    QGraphicsSceneMouseEvent sceneEvent(d->sceneMouseEventTypeFromMouseEvent(event));
    d->sceneMouseEventFromMouseEvent(sceneEvent, event);

    d->deliverMouseEvent(&sceneEvent);
    event->setAccepted(sceneEvent.isAccepted());

    d->mouseGrabberItem = 0;
}

void QSGCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QSGCanvas);
    
#ifdef MOUSE_DEBUG
    qWarning() << "QSGCanvas::mouseDoubleClickEvent()" << event->pos() << event->button() << event->buttons();
#endif

    QGraphicsSceneMouseEvent sceneEvent(d->sceneMouseEventTypeFromMouseEvent(event));
    d->sceneMouseEventFromMouseEvent(sceneEvent, event);

    if (!d->mouseGrabberItem && (event->button() & event->buttons()) == event->buttons()) {
        if (d->deliverInitialMousePressEvent(d->rootItem, &sceneEvent))
            event->accept();
        else
            event->ignore();
        return;
    } 

    d->deliverMouseEvent(&sceneEvent);
    event->setAccepted(sceneEvent.isAccepted());
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
            d->lastMousePosition = event->pos();
        QPointF last = d->lastMousePosition;
        d->lastMousePosition = event->pos();

        bool accepted = event->isAccepted();
        bool delivered = d->deliverHoverEvent(d->rootItem, event->pos(), last, event->modifiers(), accepted);
        if (!delivered) {
            //take care of any exits
            accepted = d->clearHover();
        }
        event->setAccepted(accepted);
        return;
    }

    QGraphicsSceneMouseEvent sceneEvent(d->sceneMouseEventTypeFromMouseEvent(event));
    d->sceneMouseEventFromMouseEvent(sceneEvent, event);

    d->deliverMouseEvent(&sceneEvent);
    event->setAccepted(sceneEvent.isAccepted());
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
        if (!child->isEnabled())
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
                QList<QSGItem*> parents;
                QSGItem* parent = item;
                parents << item;
                while ((parent = parent->parentItem()))
                    parents << parent;

                //exit from previous (excepting ancestors)
                while (!hoverItems.isEmpty() && !parents.contains(hoverItems[0])){
                    sendHoverEvent(QEvent::HoverLeave, hoverItems[0], scenePos, lastScenePos, modifiers, accepted);
                    hoverItems.removeFirst();
                }

                if (!hoverItems.isEmpty() && hoverItems[0] == item){//Not entering a new Item
                    accepted = sendHoverEvent(QEvent::HoverMove, item, scenePos, lastScenePos, modifiers, accepted);
                } else {
                    //enter any ancestors that also wish to be hovered and aren't
                    int startIdx = -1;
                    if (!hoverItems.isEmpty())
                        startIdx = parents.indexOf(hoverItems[0]);
                    if (startIdx == -1)
                        startIdx = parents.count() - 1;

                    for (int i = startIdx; i >= 0; i--) {
                        if (QSGItemPrivate::get(parents[i])->hoverEnabled) {
                            hoverItems.prepend(parents[i]);
                            sendHoverEvent(QEvent::HoverEnter, parents[i], scenePos, lastScenePos, modifiers, accepted);
                        }
                    }

                    //enter new item
                    hoverItems.prepend(item);
                    accepted = sendHoverEvent(QEvent::HoverEnter, item, scenePos, lastScenePos, modifiers, accepted);
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
        QPointF p = item->mapFromScene(event->pos());
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

    QPointF p = item->mapFromScene(event->pos());
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
            touchEvent.setWidget(q);
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

bool QSGCanvasPrivate::sendFilteredMouseEvent(QSGItem *target, QSGItem *item, QGraphicsSceneMouseEvent *event)
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
    case QEvent::GraphicsSceneMousePress:
    case QEvent::GraphicsSceneMouseRelease:
    case QEvent::GraphicsSceneMouseDoubleClick:
    case QEvent::GraphicsSceneMouseMove:
        // XXX todo - should sendEvent be doing this?  how does it relate to forwarded events? 
        {
            QGraphicsSceneMouseEvent *se = static_cast<QGraphicsSceneMouseEvent *>(e);
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

    if (d->threadedRendering && d->thread && d->thread->isRunning()) {
        Q_ASSERT_X(QThread::currentThread() == QApplication::instance()->thread() || d->thread->inSync,
                   "QSGCanvas::update",
                   "Function can only be called from GUI thread or during QSGItem::updatePaintNode()");

        if (d->thread->inSync) {
            d->thread->isExternalUpdatePending = true;

        } else if (!d->renderThreadAwakened) {
#ifdef THREAD_DEBUG
            printf("GUI: doing update...\n");
#endif
            d->renderThreadAwakened = true;
            d->thread->lockInGui();
            d->thread->isExternalUpdatePending = true;
            if (d->thread->isRenderBlocked)
                d->thread->wake();
            d->thread->unlockInGui();
        }
    } else if (!d->animationDriver || !d->animationDriver->isRunning()) {
        update();
    }
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

void QSGCanvas::setRenderTarget(QGLFramebufferObject *fbo)
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
QGLFramebufferObject *QSGCanvas::renderTarget() const
{
    Q_D(const QSGCanvas);
    return d->renderTarget;
}


/*!
    Grabs the contents of the framebuffer and returns it as an image.

    This function might not work if the view is not visible.

    \warning Calling this function will cause performance problems.
 */
QImage QSGCanvas::grabFrameBuffer()
{
    Q_D(QSGCanvas);
    if (d->threadedRendering)
        return d->thread ? d->thread->grab() : QImage();
    else {
        // render a fresh copy of the scene graph in the current thread.
        d->renderSceneGraph(size());
        return QGLWidget::grabFrameBuffer(false);
    }
}


void QSGCanvasRenderThread::run()
{
#ifdef THREAD_DEBUG
    qDebug("QML Rendering Thread Started");
#endif

    renderer->makeCurrent();

    if (!d->context->isReady())
        d->initializeSceneGraph();

    
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
            QApplication::postEvent(renderer, new QEvent(QEvent::User));
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
        d->syncSceneGraph();
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

        d->renderSceneGraph(windowSize);

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

        renderer->swapBuffers();

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

        if (!d->animationRunning && !isExternalUpdatePending && !shouldExit && !doGrab) {
#ifdef THREAD_DEBUG
            printf("                RenderThread: nothing to do, going to sleep...\n");
#endif
            isRenderBlocked = true;
            wait();
            isRenderBlocked = false;
        }

        unlock();
    }

#ifdef THREAD_DEBUG
    printf("                RenderThread: exited... Good Night!\n");
#endif

    renderer->doneCurrent();

    lock();
    hasExited = true;
#ifdef THREAD_DEBUG
    printf("                RenderThread: waking GUI for final sleep..\n");
#endif
    wake();
    unlock();
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
    Q_ASSERT(d->threadedRendering);

    if (!guiAlreadyLocked)
        d->thread->lockInGui();

    d->renderThreadAwakened = false;

    d->polishItems();

    d->thread->wake();
    d->thread->wait();

    if (!guiAlreadyLocked)
        d->thread->unlockInGui();
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

    d->animationRunning = true;

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
    d->animationRunning = false;
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

    // paint is only called for the inital show. After that we will do all
    // drawing ourselves, so block future updates..
    renderer->setUpdatesEnabled(false);
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



void QSGCanvasRenderThread::startRenderThread()
{
#ifdef THREAD_DEBUG
    printf("GUI: Starting Render Thread\n");
#endif
    hasExited = false;
    shouldExit = false;
    isGuiBlocked = 0;
    isGuiBlockPending = false;

    renderer->doneCurrent();
    start();
}



void QSGCanvasRenderThread::stopRenderThread()
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
}



QImage QSGCanvasRenderThread::grab()
{
    if (!isRunning())
        return QImage();

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


#include "moc_qsgcanvas.cpp"

QT_END_NAMESPACE
