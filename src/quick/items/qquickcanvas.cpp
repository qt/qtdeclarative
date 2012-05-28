/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickcanvas.h"
#include "qquickcanvas_p.h"

#include "qquickitem.h"
#include "qquickitem_p.h"
#include "qquickevents_p_p.h"

#include <QtQuick/private/qsgrenderer_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qsgflashnode_p.h>

#include <private/qquickwindowmanager_p.h>

#include <private/qguiapplication_p.h>
#include <QtGui/QInputMethod>

#include <private/qabstractanimation_p.h>

#include <QtGui/qpainter.h>
#include <QtGui/qevent.h>
#include <QtGui/qmatrix4x4.h>
#include <QtGui/qstylehints.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qabstractanimation.h>
#include <QtQml/qqmlincubator.h>

#include <QtQuick/private/qquickpixmapcache_p.h>

#include <private/qqmlprofilerservice_p.h>
#include <private/qqmlmemoryprofiler_p.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlTranslateTouchToMouse, QML_TRANSLATE_TOUCH_TO_MOUSE)

void QQuickCanvasPrivate::updateFocusItemTransform()
{
    Q_Q(QQuickCanvas);
    QQuickItem *focus = q->activeFocusItem();
    if (focus && qApp->focusObject() == focus)
        qApp->inputMethod()->setInputItemTransform(QQuickItemPrivate::get(focus)->itemToCanvasTransform());
}

class QQuickCanvasIncubationController : public QObject, public QQmlIncubationController
{
public:
    QQuickCanvasIncubationController(QQuickCanvasPrivate *canvas)
    : m_canvas(canvas), m_eventSent(false) {}

protected:
    virtual bool event(QEvent *e)
    {
        if (e->type() == QEvent::User) {
            Q_ASSERT(m_eventSent);
            volatile bool *amtp = m_canvas->windowManager->allowMainThreadProcessing();
            while (incubatingObjectCount()) {
                if (amtp)
                    incubateWhile(amtp, 2);
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
        // If no animations are running, the renderer may be waiting
        m_canvas->windowManager->wakeup();
    }

private:
    QQuickCanvasPrivate *m_canvas;
    bool m_eventSent;
};

#ifndef QT_NO_ACCESSIBILITY
QAccessibleInterface *QQuickCanvas::accessibleRoot() const
{
    return QAccessible::queryAccessibleInterface(const_cast<QQuickCanvas*>(this));
}
#endif


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


// #define FOCUS_DEBUG
// #define MOUSE_DEBUG
// #define TOUCH_DEBUG
// #define DIRTY_DEBUG

#ifdef FOCUS_DEBUG
void printFocusTree(QQuickItem *item, QQuickItem *scope = 0, int depth = 1);
#endif

QQuickItem::UpdatePaintNodeData::UpdatePaintNodeData()
: transformNode(0)
{
}

QQuickRootItem::QQuickRootItem()
{
}

/*! \reimp */
void QQuickCanvas::exposeEvent(QExposeEvent *)
{
    Q_D(QQuickCanvas);
    d->windowManager->exposureChanged(this);
}

/*! \reimp */
void QQuickCanvas::resizeEvent(QResizeEvent *)
{
    Q_D(QQuickCanvas);
    d->windowManager->resize(this, size());
}

/*! \reimp */
void QQuickCanvas::showEvent(QShowEvent *)
{
    d_func()->windowManager->show(this);
}

/*! \reimp */
void QQuickCanvas::hideEvent(QHideEvent *)
{
    d_func()->windowManager->hide(this);
}

/*! \reimp */
void QQuickCanvas::focusOutEvent(QFocusEvent *)
{
    Q_D(QQuickCanvas);
    d->rootItem->setFocus(false);
}

/*! \reimp */
void QQuickCanvas::focusInEvent(QFocusEvent *)
{
    Q_D(QQuickCanvas);
    d->rootItem->setFocus(true);
    d->updateFocusItemTransform();
}


void QQuickCanvasPrivate::polishItems()
{
    int maxPolishCycles = 100000;

    while (!itemsToPolish.isEmpty() && --maxPolishCycles > 0) {
        QSet<QQuickItem *> itms = itemsToPolish;
        itemsToPolish.clear();

        for (QSet<QQuickItem *>::iterator it = itms.begin(); it != itms.end(); ++it) {
            QQuickItem *item = *it;
            QQuickItemPrivate::get(item)->polishScheduled = false;
            item->updatePolish();
        }
    }

    if (maxPolishCycles == 0)
        qWarning("QQuickCanvas: possible QQuickItem::polish() loop");

    updateFocusItemTransform();
}

/**
 * This parameter enables that this canvas can be rendered without
 * being shown on screen. This feature is very limited in what it supports.
 *
 * There needs to be another window actually showing that we can make current
 * to get a surface to make current AND for this feature to be useful
 * one needs to hook into beforeRender() and set the render tareget.
 *
 */
void QQuickCanvasPrivate::setRenderWithoutShowing(bool render)
{
    if (render == renderWithoutShowing)
        return;

    Q_Q(QQuickCanvas);
    renderWithoutShowing = render;

    if (render)
        windowManager->show(q);
    else
        windowManager->hide(q);
}


/*!
 * Schedules the canvas to render another frame.
 *
 * Calling QQuickCanvas::update() differs from QQuickItem::update() in that
 * it always triggers a repaint, regardless of changes in the underlying
 * scene graph or not.
 */
void QQuickCanvas::update()
{
    Q_D(QQuickCanvas);
    d->windowManager->update(this);
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

void QQuickCanvasPrivate::syncSceneGraph()
{
    QML_MEMORY_SCOPE_STRING("SceneGraph");
    Q_Q(QQuickCanvas);

    emit q->beforeSynchronizing();
    if (!renderer) {
        forceUpdate(rootItem);

        QSGRootNode *rootNode = new QSGRootNode;
        rootNode->appendChildNode(QQuickItemPrivate::get(rootItem)->itemNode());
        renderer = context->createRenderer();
        renderer->setRootNode(rootNode);
    }

    updateDirtyNodes();

    // Copy the current state of clearing from canvas into renderer.
    renderer->setClearColor(clearColor);
    QSGRenderer::ClearMode mode = QSGRenderer::ClearStencilBuffer | QSGRenderer::ClearDepthBuffer;
    if (clearBeforeRendering)
        mode |= QSGRenderer::ClearColorBuffer;
    renderer->setClearMode(mode);
}


void QQuickCanvasPrivate::renderSceneGraph(const QSize &size)
{
    QML_MEMORY_SCOPE_STRING("SceneGraph");
    Q_Q(QQuickCanvas);
    emit q->beforeRendering();
    int fboId = 0;
    renderer->setDeviceRect(QRect(QPoint(0, 0), size));
    if (renderTargetId) {
        fboId = renderTargetId;
        renderer->setViewportRect(QRect(QPoint(0, 0), renderTargetSize));
    } else {
        renderer->setViewportRect(QRect(QPoint(0, 0), size));
    }
    renderer->setProjectionMatrixToDeviceRect();

    context->renderNextFrame(renderer, fboId);
    emit q->afterRendering();
}

QQuickCanvasPrivate::QQuickCanvasPrivate()
    : rootItem(0)
    , activeFocusItem(0)
    , mouseGrabberItem(0)
    , touchMouseId(-1)
    , touchMousePressTimestamp(0)
    , renderWithoutShowing(false)
    , dirtyItemList(0)
    , context(0)
    , renderer(0)
    , windowManager(0)
    , clearColor(Qt::white)
    , clearBeforeRendering(true)
    , persistentGLContext(false)
    , persistentSceneGraph(false)
    , renderTarget(0)
    , renderTargetId(0)
    , incubationController(0)
{
}

QQuickCanvasPrivate::~QQuickCanvasPrivate()
{
}

void QQuickCanvasPrivate::init(QQuickCanvas *c)
{
    q_ptr = c;

    Q_Q(QQuickCanvas);

    rootItem = new QQuickRootItem;
    QQmlEngine::setObjectOwnership(rootItem, QQmlEngine::CppOwnership);
    QQuickItemPrivate *rootItemPrivate = QQuickItemPrivate::get(rootItem);
    rootItemPrivate->canvas = q;
    rootItemPrivate->canvasRefCount = 1;
    rootItemPrivate->flags |= QQuickItem::ItemIsFocusScope;

    // In the absence of a focus in event on some platforms assume the window will
    // be activated immediately and set focus on the rootItem
    // ### Remove when QTBUG-22415 is resolved.
    //It is important that this call happens after the rootItem has a canvas..
    rootItem->setFocus(true);

    windowManager = QQuickWindowManager::instance();
    context = windowManager->sceneGraphContext();
    q->setSurfaceType(QWindow::OpenGLSurface);
    q->setFormat(context->defaultSurfaceFormat());

    QObject::connect(context, SIGNAL(initialized()), q, SIGNAL(sceneGraphInitialized()), Qt::DirectConnection);
    QObject::connect(context, SIGNAL(invalidated()), q, SIGNAL(sceneGraphInvalidated()), Qt::DirectConnection);
    QObject::connect(context, SIGNAL(invalidated()), q, SLOT(cleanupSceneGraph()), Qt::DirectConnection);
}

QQmlListProperty<QObject> QQuickCanvasPrivate::data()
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

static QQuickMouseEventEx touchToMouseEvent(QEvent::Type type, const QTouchEvent::TouchPoint &p)
{
    QQuickMouseEventEx me(type, p.pos(), p.scenePos(), p.screenPos(),
            Qt::LeftButton, Qt::LeftButton, 0);
    me.setVelocity(p.velocity());
    return me;
}

void QQuickCanvasPrivate::translateTouchToMouse(QTouchEvent *event)
{
    if (event->type() == QEvent::TouchCancel) {
        touchMouseId = -1;
        if (mouseGrabberItem)
            mouseGrabberItem->ungrabMouse();
        return;
    }
    for (int i = 0; i < event->touchPoints().count(); ++i) {
        QTouchEvent::TouchPoint p = event->touchPoints().at(i);
        if (touchMouseId == -1 && p.state() & Qt::TouchPointPressed) {
            bool doubleClick = event->timestamp() - touchMousePressTimestamp
                            < static_cast<ulong>(qApp->styleHints()->mouseDoubleClickInterval());
            touchMousePressTimestamp = event->timestamp();
            // Store the id already here and restore it to -1 if the event does not get
            // accepted. Cannot defer setting the new value because otherwise if the event
            // handler spins the event loop all subsequent moves and releases get lost.
            touchMouseId = p.id();
            QQuickMouseEventEx me = touchToMouseEvent(QEvent::MouseButtonPress, p);
            me.setTimestamp(event->timestamp());
            me.setAccepted(false);
            me.setCapabilities(event->device()->capabilities());
            deliverMouseEvent(&me);
            if (me.isAccepted())
                event->setAccepted(true);
            else
                touchMouseId = -1;
            if (doubleClick && me.isAccepted()) {
                touchMousePressTimestamp = 0;
                QQuickMouseEventEx me = touchToMouseEvent(QEvent::MouseButtonDblClick, p);
                me.setTimestamp(event->timestamp());
                me.setAccepted(false);
                me.setCapabilities(event->device()->capabilities());
                if (!mouseGrabberItem) {
                    if (deliverInitialMousePressEvent(rootItem, &me))
                        event->setAccepted(true);
                    else
                        touchMouseId = -1;
                } else {
                    deliverMouseEvent(&me);
                    if (me.isAccepted())
                        event->setAccepted(true);
                    else
                        touchMouseId = -1;
                }
            }
            if (touchMouseId != -1)
                break;
        } else if (p.id() == touchMouseId) {
            if (p.state() & Qt::TouchPointMoved) {
                QQuickMouseEventEx me = touchToMouseEvent(QEvent::MouseMove, p);
                me.setTimestamp(event->timestamp());
                me.setCapabilities(event->device()->capabilities());
                if (!mouseGrabberItem) {
                    if (lastMousePosition.isNull())
                        lastMousePosition = me.windowPos();
                    QPointF last = lastMousePosition;
                    lastMousePosition = me.windowPos();

                    bool accepted = me.isAccepted();
                    bool delivered = deliverHoverEvent(rootItem, me.windowPos(), last, me.modifiers(), accepted);
                    if (!delivered) {
                        //take care of any exits
                        accepted = clearHover();
                    }
                    me.setAccepted(accepted);
                    break;
                }

                deliverMouseEvent(&me);
            } else if (p.state() & Qt::TouchPointReleased) {
                touchMouseId = -1;
                if (!mouseGrabberItem)
                    return;
                QQuickMouseEventEx me = touchToMouseEvent(QEvent::MouseButtonRelease, p);
                me.setTimestamp(event->timestamp());
                me.setCapabilities(event->device()->capabilities());
                deliverMouseEvent(&me);
                if (mouseGrabberItem)
                    mouseGrabberItem->ungrabMouse();
            }
            break;
        }
    }
}

void QQuickCanvasPrivate::transformTouchPoints(QList<QTouchEvent::TouchPoint> &touchPoints, const QTransform &transform)
{
    QMatrix4x4 transformMatrix(transform);
    for (int i=0; i<touchPoints.count(); i++) {
        QTouchEvent::TouchPoint &touchPoint = touchPoints[i];
        touchPoint.setRect(transform.mapRect(touchPoint.sceneRect()));
        touchPoint.setStartPos(transform.map(touchPoint.startScenePos()));
        touchPoint.setLastPos(transform.map(touchPoint.lastScenePos()));
        touchPoint.setVelocity(transformMatrix.mapVector(touchPoint.velocity()).toVector2D());
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

        if (i == 0)
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
    if (item == rootItem || (scopePrivate->activeFocus && item->isEnabled())) {
        oldActiveFocusItem = activeFocusItem;
        newActiveFocusItem = item;
        while (newActiveFocusItem->isFocusScope()
               && newActiveFocusItem->scopedFocusItem()
               && newActiveFocusItem->scopedFocusItem()->isEnabled()) {
            newActiveFocusItem = newActiveFocusItem->scopedFocusItem();
        }

        if (oldActiveFocusItem) {
#ifndef QT_NO_IM
            qApp->inputMethod()->commit();
#endif

            activeFocusItem = 0;
            QFocusEvent event(QEvent::FocusOut, Qt::OtherFocusReason);
            q->sendEvent(oldActiveFocusItem, &event);

            QQuickItem *afi = oldActiveFocusItem;
            while (afi && afi != scope) {
                if (QQuickItemPrivate::get(afi)->activeFocus) {
                    QQuickItemPrivate::get(afi)->activeFocus = false;
                    changed << afi;
                }
                afi = afi->parentItem();
            }
        }
    }

    if (item != rootItem && !(options & DontChangeSubFocusItem)) {
        QQuickItem *oldSubFocusItem = scopePrivate->subFocusItem;
        if (oldSubFocusItem) {
            QQuickItemPrivate::get(oldSubFocusItem)->focus = false;
            changed << oldSubFocusItem;
        }

        QQuickItemPrivate::get(item)->updateSubFocusItem(scope, true);
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

        QFocusEvent event(QEvent::FocusIn, Qt::OtherFocusReason);
        q->sendEvent(newActiveFocusItem, &event);
    }

    emit q->focusObjectChanged(activeFocusItem);

    if (!changed.isEmpty())
        notifyFocusChangesRecur(changed.data(), changed.count() - 1);
}

void QQuickCanvasPrivate::clearFocusInScope(QQuickItem *scope, QQuickItem *item, FocusOptions options)
{
    Q_Q(QQuickCanvas);

    Q_ASSERT(item);
    Q_ASSERT(scope || item == rootItem);

#ifdef FOCUS_DEBUG
    qWarning() << "QQuickCanvasPrivate::clearFocusInScope():";
    qWarning() << "    scope:" << (QObject *)scope;
    qWarning() << "    item:" << (QObject *)item;
    qWarning() << "    activeFocusItem:" << (QObject *)activeFocusItem;
#endif

    QQuickItemPrivate *scopePrivate = 0;
    if (scope) {
        scopePrivate = QQuickItemPrivate::get(scope);
        if ( !scopePrivate->subFocusItem )
            return;//No focus, nothing to do.
    }

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
        qApp->inputMethod()->commit();
#endif

        activeFocusItem = 0;
        QFocusEvent event(QEvent::FocusOut, Qt::OtherFocusReason);
        q->sendEvent(oldActiveFocusItem, &event);

        QQuickItem *afi = oldActiveFocusItem;
        while (afi && afi != scope) {
            if (QQuickItemPrivate::get(afi)->activeFocus) {
                QQuickItemPrivate::get(afi)->activeFocus = false;
                changed << afi;
            }
            afi = afi->parentItem();
        }
    }

    if (item != rootItem && !(options & DontChangeSubFocusItem)) {
        QQuickItem *oldSubFocusItem = scopePrivate->subFocusItem;
        if (oldSubFocusItem && !(options & DontChangeFocusProperty)) {
            QQuickItemPrivate::get(oldSubFocusItem)->focus = false;
            changed << oldSubFocusItem;
        }

        QQuickItemPrivate::get(item)->updateSubFocusItem(scope, false);

    } else if (!(options & DontChangeFocusProperty)) {
        QQuickItemPrivate::get(item)->focus = false;
        changed << item;
    }

    if (newActiveFocusItem) {
        Q_ASSERT(newActiveFocusItem == scope);
        activeFocusItem = scope;

        QFocusEvent event(QEvent::FocusIn, Qt::OtherFocusReason);
        q->sendEvent(newActiveFocusItem, &event);
    }

    emit q->focusObjectChanged(activeFocusItem);

    if (!changed.isEmpty())
        notifyFocusChangesRecur(changed.data(), changed.count() - 1);
}

void QQuickCanvasPrivate::notifyFocusChangesRecur(QQuickItem **items, int remaining)
{
    QQmlGuard<QQuickItem> item(*items);

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
    \ingroup qtquick-visual-types
    \brief Creates a new top-level window

    The Window object creates a new top-level window for a QtQuick scene. It automatically sets up the
    window for use with QtQuick 2.0 graphical elements.

    To use this element, you will need to import the module with the following line:
    \code
    import QtQuick.Window 2.0
    \endcode

    Restricting this import will allow you to have a QML environment without access to window system features.
*/
/*!
    \class QQuickCanvas
    \since QtQuick 2.0
    \brief The QQuickCanvas class provides the canvas for displaying a graphical QML scene

    QQuickCanvas provides the graphical scene management needed to interact with and display
    a scene of QQuickItems.

    A QQuickCanvas always has a single invisible root item. To add items to this canvas,
    reparent the items to the root item or to an existing item in the scene.

    For easily displaying a scene from a QML file, see \l{QQuickView}.

    \section1 Scene Graph and Rendering

    The QQuickCanvas uses a scene graph on top of OpenGL to render. This scene graph is disconnected
    from the QML scene and potentially lives in another thread, depending on the platform
    implementation. Since the rendering scene graph lives independently from the QML scene, it can
    also be completely released without affecting the state of the QML scene.

    The sceneGraphInitialized() signal is emitted on the rendering thread before the QML scene is
    rendered to the screen for the first time. If the rendering scene graph has been released
    the signal will be emitted again before the next frame is rendered.

    The rendering of each frame is broken down into the following
    steps, in the given order:

    \list

    \li The QQuickCanvas::beforeSynchronizing() signal is emitted.
    Applications can make direct connections (Qt::DirectConnection)
    to this signal to do any preparation required before calls to
    QQuickItem::updatePaintNode().

    \li Synchronzation of the QML state into the scene graph. This is
    done by calling the QQuickItem::updatePaintNode() function on all
    items that have changed since the previous frame. When a dedicated
    rendering thread is used, the GUI thread is blocked during this
    synchroniation. This is the only time the QML items and the nodes
    in the scene graph interact.

    \li The canvas to be rendered is made current using
    QOpenGLContext::makeCurrent().

    \li The QQuickCanvas::beforeRendering() signal is
    emitted. Applications can make direct connections
    (Qt::DirectConnection) to this signal to use custom OpenGL calls
    which will then stack visually beneath the QML scene.

    \li Items that have specified QSGNode::UsesPreprocess, will have their
    QSGNode::preprocess() function invoked.

    \li The QQuickCanvas is cleared according to what is specified
    using QQuickCanvas::setClearBeforeRendering() and
    QQuickCanvas::setClearColor().

    \li The scene graph is rendered.

    \li The QQuickCanvas::afterRendering() signal is
    emitted. Applications can make direct connections
    (Qt::DirectConnection) to this signal to use custom OpenGL calls
    which will then stack visually over the QML scene.

    \li The rendered frame is swapped and QQuickCanvas::frameSwapped()
    is emitted.

    \endlist

    All of the above happen on the rendering thread, when applicable.

    While the scene graph is being rendered on the rendering thread, the GUI will process animations
    for the next frame. This means that as long as users are not using scene graph API
    directly, the added complexity of a rendering thread can be completely ignored.

    When a QQuickCanvas is programatically hidden with hide() or setVisible(false), it will
    stop rendering and its scene graph and OpenGL context might be released. The
    sceneGraphInvalidated() signal will be emitted when this happens.

    \warning It is crucial that OpenGL operations and interaction with the scene graph happens
    exclusively on the rendering thread, primarily during the updatePaintNode() phase.

    \warning As signals related to rendering might be emitted from the rendering thread,
    connections should be made using Qt::DirectConnection


    \section2 Resource Management

    QML will typically try to cache images, scene graph nodes, etc to improve performance, but in
    some low-memory scenarios it might be required to aggressively release these resources. The
    releaseResources() can be used to force clean up of certain resources. Calling releaseResources()
    may result in the entire scene graph and its OpenGL context being deleted. The
    sceneGraphInvalidated() signal will be emitted when this happens.

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

    d->windowManager->canvasDestroyed(this);

    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    delete d->incubationController; d->incubationController = 0;

    delete d->rootItem; d->rootItem = 0;
}



/*!
    This function tries to release redundant resources currently held by the QML scene.

    Calling this function might result in the scene graph and the OpenGL context used
    for rendering being released to release graphics memory. If this happens, the
    sceneGraphInvalidated() signal will be called, allowing users to clean up their
    own graphics resources. The setPersistentOpenGLContext() and setPersistentSceneGraph()
    functions can be used to prevent this from happening, if handling the cleanup is
    not feasible in the application, at the cost of higher memory usage.

    \sa sceneGraphInvalidated(), setPersistentOpenGLContext(), setPersistentSceneGraph().
 */

void QQuickCanvas::releaseResources()
{
    Q_D(QQuickCanvas);
    d->windowManager->releaseResources();
    QQuickPixmap::purgeCache();
}



/*!
    Controls whether the OpenGL context can be released as a part of a call to
    releaseResources().

    The OpenGL context might still be released when the user makes an explicit
    call to hide().

    \sa setPersistentSceneGraph()
 */

void QQuickCanvas::setPersistentOpenGLContext(bool persistent)
{
    Q_D(QQuickCanvas);
    d->persistentGLContext = persistent;
}


/*!
    Returns whether the OpenGL context can be released as a part of a call to
    releaseResources().
 */

bool QQuickCanvas::isPersistentOpenGLContext() const
{
    Q_D(const QQuickCanvas);
    return d->persistentGLContext;
}



/*!
    Controls whether the scene graph nodes and resources can be released as a
    part of a call to releaseResources().

    The scene graph nodes and resources might still be released when the user
    makes an explicit call to hide().

    \sa setPersistentOpenGLContext()
 */

void QQuickCanvas::setPersistentSceneGraph(bool persistent)
{
    Q_D(QQuickCanvas);
    d->persistentSceneGraph = persistent;
}



/*!
    Returns whether the scene graph nodes and resources can be released as a part
    of a call to releaseResources().
 */

bool QQuickCanvas::isPersistentSceneGraph() const
{
    Q_D(const QQuickCanvas);
    return d->persistentSceneGraph;
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

QObject *QQuickCanvas::focusObject() const
{
    Q_D(const QQuickCanvas);

    if (d->activeFocusItem)
        return d->activeFocusItem;
    return const_cast<QQuickCanvas*>(this);
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

    QPointF pos = QGuiApplicationPrivate::lastCursorPosition;; // ### refactor: q->mapFromGlobal(QCursor::pos());

    bool accepted = false;
    foreach (QQuickItem* item, hoverItems)
        accepted = sendHoverEvent(QEvent::HoverLeave, item, pos, pos, QGuiApplication::keyboardModifiers(), true) || accepted;
    hoverItems.clear();
    return accepted;
}

/*! \reimp */
bool QQuickCanvas::event(QEvent *e)
{
    Q_D(QQuickCanvas);

    switch (e->type()) {

    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
    {
        QTouchEvent *touch = static_cast<QTouchEvent *>(e);
        d->translateTouchEvent(touch);
        d->deliverTouchEvent(touch);
        if (qmlTranslateTouchToMouse())
            d->translateTouchToMouse(touch);

        return touch->isAccepted();
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
    case QEvent::FocusAboutToChange:
        if (d->activeFocusItem)
            qGuiApp->inputMethod()->commit();
        break;
    default:
        break;
    }

    return QWindow::event(e);
}

/*! \reimp */
void QQuickCanvas::keyPressEvent(QKeyEvent *e)
{
    Q_D(QQuickCanvas);

    if (d->activeFocusItem)
        sendEvent(d->activeFocusItem, e);
}

/*! \reimp */
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
    if (itemPrivate->opacity() == 0.0)
        return false;

    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(event->windowPos());
        if (!item->contains(p))
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

    if (itemPrivate->acceptedMouseButtons() & event->button()) {
        QPointF p = item->mapFromScene(event->windowPos());
        if (item->contains(p)) {
            QMouseEvent me(event->type(), p, event->windowPos(), event->screenPos(),
                           event->button(), event->buttons(), event->modifiers());
            me.setTimestamp(event->timestamp());
            me.accept();
            item->grabMouse();
            q->sendEvent(item, &me);
            event->setAccepted(me.isAccepted());
            if (me.isAccepted())
                return true;
            if (mouseGrabberItem)
                mouseGrabberItem->ungrabMouse();
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
         (event->buttons() & event->button()) == event->buttons()) {
        if (deliverInitialMousePressEvent(rootItem, event))
            event->accept();
        else
            event->ignore();
        return event->isAccepted();
    }

    if (mouseGrabberItem) {
        QQuickItemPrivate *mgPrivate = QQuickItemPrivate::get(mouseGrabberItem);
        const QTransform &transform = mgPrivate->canvasToItemTransform();
        QQuickMouseEventEx me(event->type(), transform.map(event->windowPos()),
                                event->windowPos(), event->screenPos(),
                                event->button(), event->buttons(), event->modifiers());
        QQuickMouseEventEx *eventEx = QQuickMouseEventEx::extended(event);
        if (eventEx) {
            me.setVelocity(QMatrix4x4(transform).mapVector(eventEx->velocity()).toVector2D());
            me.setCapabilities(eventEx->capabilities());
        }
        me.setTimestamp(event->timestamp());
        me.accept();
        q->sendEvent(mouseGrabberItem, &me);
        event->setAccepted(me.isAccepted());
        if (me.isAccepted())
            return true;
    }

    return false;
}

/*! \reimp */
void QQuickCanvas::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickCanvas);
    if (qmlTranslateTouchToMouse())
        return; // We are using touch events
#ifdef MOUSE_DEBUG
    qWarning() << "QQuickCanvas::mousePressEvent()" << event->pos() << event->button() << event->buttons();
#endif

    d->deliverMouseEvent(event);
}

/*! \reimp */
void QQuickCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickCanvas);
    if (qmlTranslateTouchToMouse())
        return; // We are using touch events
#ifdef MOUSE_DEBUG
    qWarning() << "QQuickCanvas::mouseReleaseEvent()" << event->pos() << event->button() << event->buttons();
#endif

    if (!d->mouseGrabberItem) {
        QWindow::mouseReleaseEvent(event);
        return;
    }

    d->deliverMouseEvent(event);
    if (d->mouseGrabberItem)
        d->mouseGrabberItem->ungrabMouse();
}

/*! \reimp */
void QQuickCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QQuickCanvas);
    if (qmlTranslateTouchToMouse())
        return; // We are using touch events

#ifdef MOUSE_DEBUG
    qWarning() << "QQuickCanvas::mouseDoubleClickEvent()" << event->pos() << event->button() << event->buttons();
#endif

    if (!d->mouseGrabberItem && (event->buttons() & event->button()) == event->buttons()) {
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

/*! \reimp */
void QQuickCanvas::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickCanvas);
    if (qmlTranslateTouchToMouse())
        return; // We are using touch events
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
    if (itemPrivate->opacity() == 0.0)
        return false;

    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(scenePos);
        if (!item->contains(p))
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
        if (item->contains(p)) {
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
    if (itemPrivate->opacity() == 0.0)
        return false;

    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        QPointF p = item->mapFromScene(event->posF());
        if (!item->contains(p))
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

    if (item->contains(p)) {
        QWheelEvent wheel(p, p, event->pixelDelta(), event->angleDelta(), event->delta(),
                          event->orientation(), event->buttons(), event->modifiers());
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
/*! \reimp */
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
    else if (event->type() == QEvent::TouchCancel)
        qWarning("touchCancelEvent");
#endif

    Q_Q(QQuickCanvas);

    QHash<QQuickItem *, QList<QTouchEvent::TouchPoint> > updatedPoints;

    if (event->type() == QTouchEvent::TouchBegin) {     // all points are new touch points
        QSet<int> acceptedNewPoints;
        deliverTouchPoints(rootItem, event, event->touchPoints(), &acceptedNewPoints, &updatedPoints);
        if (acceptedNewPoints.count() > 0)
            event->accept();
        else
            event->ignore();
        return event->isAccepted();
    }

    if (event->type() == QTouchEvent::TouchCancel) {
        // A TouchCancel event will typically not contain any points.
        // Deliver it to all items that have active touches.
        QSet<QQuickItem *> cancelDelivered;
        foreach (QQuickItem *item, itemForTouchPointId) {
            if (cancelDelivered.contains(item))
                continue;
            cancelDelivered.insert(item);
            q->sendEvent(item, event);
        }
        // The next touch event can only be a TouchBegin so clean up.
        itemForTouchPointId.clear();
        return true;
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
        else
            event->ignore();
    } else
        event->ignore();

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

    if (itemPrivate->opacity() == 0.0)
        return false;

    if (itemPrivate->flags & QQuickItem::ItemClipsChildrenToShape) {
        for (int i=0; i<newPoints.count(); i++) {
            QPointF p = item->mapFromScene(newPoints[i].scenePos());
            if (!item->contains(p))
                return false;
        }
    }

    QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();
    for (int ii = children.count() - 1; ii >= 0; --ii) {
        QQuickItem *child = children.at(ii);
        if (!child->isEnabled() || !child->isVisible())
            continue;
        if (deliverTouchPoints(child, event, newPoints, acceptedNewPoints, updatedPoints))
            return true;
    }

    QList<QTouchEvent::TouchPoint> matchingPoints;
    if (newPoints.count() > 0 && acceptedNewPoints->count() < newPoints.count()) {
        for (int i=0; i<newPoints.count(); i++) {
            if (acceptedNewPoints->contains(newPoints[i].id()))
                continue;
            QPointF p = item->mapFromScene(newPoints[i].scenePos());
            if (item->contains(p))
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
            touchEvent.setWindow(event->window());
            touchEvent.setTarget(item);
            touchEvent.setDevice(event->device());
            touchEvent.setModifiers(event->modifiers());
            touchEvent.setTouchPointStates(eventStates);
            touchEvent.setTouchPoints(eventPoints);
            touchEvent.setTimestamp(event->timestamp());

            for (int i = 0; i < matchingPoints.count(); ++i)
                itemForTouchPointId[matchingPoints[i].id()] = item;

            touchEvent.accept();
            q->sendEvent(item, &touchEvent);

            if (touchEvent.isAccepted()) {
                for (int i = 0; i < matchingPoints.count(); ++i)
                    acceptedNewPoints->insert(matchingPoints[i].id());
            } else {
                for (int i = 0; i < matchingPoints.count(); ++i)
                    if (itemForTouchPointId.value(matchingPoints[i].id()) == item)
                        itemForTouchPointId.remove(matchingPoints[i].id());
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
                    if ((**grabItem)->contains(p)) {
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
    if (itemPrivate->opacity() == 0.0 || !item->isVisible() || !item->isEnabled())
        return false;

    QPointF p = item->mapFromScene(event->pos());
    if (item->contains(p)) {
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
    case QEvent::UngrabMouse:
        if (!d->sendFilteredMouseEvent(item->parentItem(), item, e)) {
            e->accept();
            item->mouseUngrabEvent();
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
    case QEvent::TouchCancel:
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

        if (p->extra.isAllocated()) {
            p->extra->opacityNode = 0;
            p->extra->clipNode = 0;
            p->extra->rootNode = 0;
        }

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
    QSet<QQuickItem *>::const_iterator it = parentlessItems.begin();
    for (; it != parentlessItems.end(); ++it)
        cleanupNodesOnShutdown(*it);
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
        (dirty & QQuickItemPrivate::Size && itemPriv->origin() != QQuickItem::TopLeft &&
         (itemPriv->scale() != 1. || itemPriv->rotation() != 0.))) {

        QMatrix4x4 matrix;

        if (itemPriv->x != 0. || itemPriv->y != 0.)
            matrix.translate(itemPriv->x, itemPriv->y);

        for (int ii = itemPriv->transforms.count() - 1; ii >= 0; --ii)
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

    bool clipEffectivelyChanged = (dirty & (QQuickItemPrivate::Clip | QQuickItemPrivate::Canvas)) &&
                                  ((item->clip() == false) != (itemPriv->clipNode() == 0));
    int effectRefCount = itemPriv->extra.isAllocated()?itemPriv->extra->effectRefCount:0;
    bool effectRefEffectivelyChanged = (dirty & (QQuickItemPrivate::EffectReference | QQuickItemPrivate::Canvas)) &&
                                  ((effectRefCount == 0) != (itemPriv->rootNode() == 0));

    if (clipEffectivelyChanged) {
        QSGNode *parent = itemPriv->opacityNode() ? (QSGNode *) itemPriv->opacityNode() :
                                                    (QSGNode *)itemPriv->itemNode();
        QSGNode *child = itemPriv->rootNode() ? (QSGNode *)itemPriv->rootNode() :
                                                (QSGNode *)itemPriv->groupNode;

        if (item->clip()) {
            Q_ASSERT(itemPriv->clipNode() == 0);
            itemPriv->extra.value().clipNode = new QQuickDefaultClipNode(item->clipRect());
            itemPriv->clipNode()->update();

            if (child)
                parent->removeChildNode(child);
            parent->appendChildNode(itemPriv->clipNode());
            if (child)
                itemPriv->clipNode()->appendChildNode(child);

        } else {
            Q_ASSERT(itemPriv->clipNode() != 0);
            parent->removeChildNode(itemPriv->clipNode());
            if (child)
                itemPriv->clipNode()->removeChildNode(child);
            delete itemPriv->clipNode();
            itemPriv->extra->clipNode = 0;
            if (child)
                parent->appendChildNode(child);
        }
    }

    if (dirty & QQuickItemPrivate::ChildrenUpdateMask)
        itemPriv->childContainerNode()->removeAllChildNodes();

    if (effectRefEffectivelyChanged) {
        QSGNode *parent = itemPriv->clipNode();
        if (!parent)
            parent = itemPriv->opacityNode();
        if (!parent)
            parent = itemPriv->itemNode();
        QSGNode *child = itemPriv->groupNode;

        if (itemPriv->extra.isAllocated() && itemPriv->extra->effectRefCount) {
            Q_ASSERT(itemPriv->rootNode() == 0);
            itemPriv->extra->rootNode = new QSGRootNode;

            if (child)
                parent->removeChildNode(child);
            parent->appendChildNode(itemPriv->rootNode());
            if (child)
                itemPriv->rootNode()->appendChildNode(child);
        } else {
            Q_ASSERT(itemPriv->rootNode() != 0);
            parent->removeChildNode(itemPriv->rootNode());
            if (child)
                itemPriv->rootNode()->removeChildNode(child);
            delete itemPriv->rootNode();
            itemPriv->extra->rootNode = 0;
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
            if (!childPrivate->explicitVisible &&
                (!childPrivate->extra.isAllocated() || !childPrivate->extra->effectRefCount))
                continue;
            if (childPrivate->itemNode()->parent())
                childPrivate->itemNode()->parent()->removeChildNode(childPrivate->itemNode());

            itemPriv->childContainerNode()->appendChildNode(childPrivate->itemNode());
        }

        QSGNode *beforePaintNode = itemPriv->groupNode ? itemPriv->groupNode->lastChild() : 0;
        if (beforePaintNode || itemPriv->extra.isAllocated())
            itemPriv->extra.value().beforePaintNode = beforePaintNode;

        if (itemPriv->paintNode)
            itemPriv->childContainerNode()->appendChildNode(itemPriv->paintNode);

        for (; ii < orderedChildren.count(); ++ii) {
            QQuickItemPrivate *childPrivate = QQuickItemPrivate::get(orderedChildren.at(ii));
            if (!childPrivate->explicitVisible &&
                (!childPrivate->extra.isAllocated() || !childPrivate->extra->effectRefCount))
                continue;
            if (childPrivate->itemNode()->parent())
                childPrivate->itemNode()->parent()->removeChildNode(childPrivate->itemNode());

            itemPriv->childContainerNode()->appendChildNode(childPrivate->itemNode());
        }
    }

    if ((dirty & QQuickItemPrivate::Size) && itemPriv->clipNode()) {
        itemPriv->clipNode()->setRect(item->clipRect());
        itemPriv->clipNode()->update();
    }

    if (dirty & (QQuickItemPrivate::OpacityValue | QQuickItemPrivate::Visible
                 | QQuickItemPrivate::HideReference | QQuickItemPrivate::Canvas))
    {
        qreal opacity = itemPriv->explicitVisible && (!itemPriv->extra.isAllocated() || itemPriv->extra->hideRefCount == 0)
                      ? itemPriv->opacity() : qreal(0);

        if (opacity != 1 && !itemPriv->opacityNode()) {
            itemPriv->extra.value().opacityNode = new QSGOpacityNode;

            QSGNode *parent = itemPriv->itemNode();
            QSGNode *child = itemPriv->clipNode();
            if (!child)
                child = itemPriv->rootNode();
            if (!child)
                child = itemPriv->groupNode;

            if (child)
                parent->removeChildNode(child);
            parent->appendChildNode(itemPriv->opacityNode());
            if (child)
                itemPriv->opacityNode()->appendChildNode(child);
        }
        if (itemPriv->opacityNode())
            itemPriv->opacityNode()->setOpacity(opacity);
    }

    if (dirty & QQuickItemPrivate::ContentUpdateMask) {

        if (itemPriv->flags & QQuickItem::ItemHasContents) {
            updatePaintNodeData.transformNode = itemPriv->itemNode();
            itemPriv->paintNode = item->updatePaintNode(itemPriv->paintNode, &updatePaintNodeData);

            Q_ASSERT(itemPriv->paintNode == 0 ||
                     itemPriv->paintNode->parent() == 0 ||
                     itemPriv->paintNode->parent() == itemPriv->childContainerNode());

            if (itemPriv->paintNode && itemPriv->paintNode->parent() == 0) {
                if (itemPriv->extra.isAllocated() && itemPriv->extra->beforePaintNode)
                    itemPriv->childContainerNode()->insertChildNodeAfter(itemPriv->paintNode, itemPriv->extra->beforePaintNode);
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
        itemPriv->opacityNode(),
        itemPriv->clipNode(),
        itemPriv->rootNode(),
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
    d->windowManager->maybeUpdate(this);
}

void QQuickCanvas::cleanupSceneGraph()
{
    Q_D(QQuickCanvas);

    if (!d->renderer)
        return;

    delete d->renderer->rootNode();
    delete d->renderer;

    d->renderer = 0;
}

/*!
    Returns the opengl context used for rendering.

    If the scene graph is not ready, this function will return 0.

    \sa sceneGraphInitialized(), sceneGraphInvalidated()
 */

QOpenGLContext *QQuickCanvas::openglContext() const
{
    Q_D(const QQuickCanvas);
    if (d->context->isReady())
        return d->context->glContext();
    return 0;
}


/*!
    \fn void QSGContext::sceneGraphInitialized()

    This signal is emitted when the scene graph has been initialized.

    This signal will be emitted from the scene graph rendering thread.

 */


/*!
    \fn void QSGContext::sceneGraphInvalidated()

    This signal is emitted when the scene graph has been invalidated.

    This signal implies that the opengl rendering context used
    has been invalidated and all user resources tied to that context
    should be released.

    This signal will be emitted from the scene graph rendering thread.
 */


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
    if (fbo) {
        d->renderTargetId = fbo->handle();
        d->renderTargetSize = fbo->size();
    } else {
        d->renderTargetId = 0;
        d->renderTargetSize = QSize();
    }
}

/*!
    \overload
 */
void QQuickCanvas::setRenderTarget(uint fboId, const QSize &size)
{
    Q_D(QQuickCanvas);
    if (d->context && d->context && QThread::currentThread() != d->context->thread()) {
        qWarning("QQuickCanvas::setRenderThread: Cannot set render target from outside the rendering thread");
        return;
    }

    d->renderTargetId = fboId;
    d->renderTargetSize = size;

    // Unset any previously set instance...
    d->renderTarget = 0;
}


/*!
    Returns the FBO id of the render target when set; otherwise returns 0.
 */
uint QQuickCanvas::renderTargetId() const
{
    Q_D(const QQuickCanvas);
    return d->renderTargetId;
}

/*!
    Returns the size of the currently set render target; otherwise returns an enpty size.
 */
QSize QQuickCanvas::renderTargetSize() const
{
    Q_D(const QQuickCanvas);
    return d->renderTargetSize;
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
    return d->windowManager->grab(this);
}

/*!
    Returns an incubation controller that splices incubation between frames
    for this canvas. QQuickView automatically installs this controller for you,
    otherwise you will need to install it yourself using \l{QQmlEngine::setIncubationController}

    The controller is owned by the canvas and will be destroyed when the canvas
    is deleted.
*/
QQmlIncubationController *QQuickCanvas::incubationController() const
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

    \warning Since this signal is emitted from the scene graph rendering thread, the
    receiver should be on the scene graph thread or the connection should be Qt::DirectConnection.

    \warning Make very sure that a signal handler for beforeRendering leaves the GL
    context in the same state as it was when the signal handler was entered. Failing to
    do so can result in the scene not rendering properly.
*/

/*!
    \fn void QQuickCanvas::afterRendering()

    This signal is emitted after the scene has completed rendering, before swapbuffers is called.

    This signal can be used to paint using raw GL on top of QML content,
    or to do screen scraping of the current frame buffer.

    The GL context used for rendering the scene graph will be bound at this point.

    \warning Since this signal is emitted from the scene graph rendering thread, the
    receiver should be on the scene graph thread or the connection should be Qt::DirectConnection.

    \warning Make very sure that a signal handler for afterRendering() leaves the GL
    context in the same state as it was when the signal handler was entered. Failing to
    do so can result in the scene not rendering properly.
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

    Depending on the underlying implementation of the scene graph, the returned
    texture may be part of an atlas. For code to be portable across implementations
    one should always use the texture coordinates returned from
    QSGTexture::normalizedTextureSubRect() when building geometry.

    \warning This function will return 0 if the scene graph has not yet been
    initialized.

    \warning The returned texture is not memory managed by the scene graph and
    must be explicitely deleted by the caller on the rendering thread.
    This is acheived by deleting the texture from a QSGNode destructor
    or by using deleteLater() in the case where the texture already has affinity
    to the rendering thread.

    This function can be called from any thread.

    \sa sceneGraphInitialized()
 */

QSGTexture *QQuickCanvas::createTextureFromImage(const QImage &image) const
{
    Q_D(const QQuickCanvas);
    if (d->context && d->context->isReady())
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
    if (d->context && d->context->isReady()) {
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
    Q_D(QQuickCanvas);
    if (color == d->clearColor)
        return;

    d->clearColor = color;
    emit clearColorChanged(color);
}



/*!
    Returns the color used to clear the opengl context.
 */

QColor QQuickCanvas::clearColor() const
{
    return d_func()->clearColor;
}



#include "moc_qquickcanvas.cpp"

QT_END_NAMESPACE
