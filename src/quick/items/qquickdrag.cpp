/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qquickdrag_p.h"

#include <private/qquickitem_p.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <private/qquickitemchangelistener_p.h>
#include <private/qv8engine_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtQml/qqmlinfo.h>
#include <QtGui/qevent.h>

#ifndef QT_NO_DRAGANDDROP

QT_BEGIN_NAMESPACE

class QQuickDragAttachedPrivate : public QObjectPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickDragAttached)
public:
    static QQuickDragAttachedPrivate *get(QQuickDragAttached *attached) {
        return static_cast<QQuickDragAttachedPrivate *>(QObjectPrivate::get(attached)); }

    QQuickDragAttachedPrivate()
        : attachedItem(0)
        , mimeData(0)
        , proposedAction(Qt::MoveAction)
        , supportedActions(Qt::MoveAction | Qt::CopyAction | Qt::LinkAction)
        , active(false)
        , listening(false)
        , inEvent(false)
        , dragRestarted(false)
        , itemMoved(false)
        , eventQueued(false)
        , overrideActions(false)
    {
    }

    void itemGeometryChanged(QQuickItem *, const QRectF &, const QRectF &);
    void itemParentChanged(QQuickItem *, QQuickItem *parent);
    void updatePosition();
    void restartDrag();
    void deliverEnterEvent();
    void deliverMoveEvent();
    void deliverLeaveEvent();
    void deliverEvent(QQuickWindow *window, QEvent *event);
    void start() { start(supportedActions); }
    void start(Qt::DropActions supportedActions);
    void setTarget(QQuickItem *item);

    QQuickDragGrabber dragGrabber;

    QQmlGuard<QObject> source;
    QQmlGuard<QObject> target;
    QQmlGuard<QQuickWindow> window;
    QQuickItem *attachedItem;
    QQuickDragMimeData *mimeData;
    Qt::DropAction proposedAction;
    Qt::DropActions supportedActions;
    bool active : 1;
    bool listening : 1;
    bool inEvent : 1;
    bool dragRestarted : 1;
    bool itemMoved : 1;
    bool eventQueued : 1;
    bool overrideActions : 1;
    QPointF hotSpot;
    QStringList keys;
};

/*!
    \qmltype Drag
    \instantiates QQuickDrag
    \inqmlmodule QtQuick 2
    \ingroup qtquick-input
    \brief For specifying drag and drop events for moved Items

    Using the Drag attached property any Item can be made a source of drag and drop
    events within a scene.

    When a drag is \l active on an item any change in that item's position will
    generate a drag event that will be sent to any DropArea that intersects
    with the new position of the item.  Other items which implement drag and
    drop event handlers can also receive these events.

    The following snippet shows how an item can be dragged with a MouseArea.
    However, dragging is not limited to mouse drags, anything that can move an item
    can generate drag events, this can include touch events, animations and bindings.

    \snippet qml/drag.qml 0

    A drag can be terminated either by canceling it with Drag.cancel() or setting
    Drag.active to false, or it can be terminated with a drop event by calling
    Drag.drop().  If the drop event is accepted Drag.drop() will return the
    \l {supportedActions}{drop action} chosen by the recipient of the event,
    otherwise it will return Qt.IgnoreAction.

*/

void QQuickDragAttachedPrivate::itemGeometryChanged(QQuickItem *, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    if (newGeometry.topLeft() == oldGeometry.topLeft() || !active || itemMoved)
        return;
    updatePosition();
}

void QQuickDragAttachedPrivate::itemParentChanged(QQuickItem *, QQuickItem *)
{
    if (!active || dragRestarted)
        return;

    QQuickWindow *newWindow = attachedItem->window();

    if (window != newWindow)
        restartDrag();
    else if (window)
        updatePosition();
}

void QQuickDragAttachedPrivate::updatePosition()
{
    Q_Q(QQuickDragAttached);
    itemMoved = true;
    if (!eventQueued) {
        eventQueued = true;
        QCoreApplication::postEvent(q, new QEvent(QEvent::User));
    }
}

void QQuickDragAttachedPrivate::restartDrag()
{
    Q_Q(QQuickDragAttached);
    dragRestarted = true;
    if (!eventQueued) {
        eventQueued = true;
        QCoreApplication::postEvent(q, new QEvent(QEvent::User));
    }
}

void QQuickDragAttachedPrivate::deliverEnterEvent()
{
    dragRestarted = false;
    itemMoved = false;

    window = attachedItem->window();

    mimeData->m_source = source;
    if (!overrideActions)
        mimeData->m_supportedActions = supportedActions;
    mimeData->m_keys = keys;

    if (window) {
        QPoint scenePos = attachedItem->mapToScene(hotSpot).toPoint();
        QDragEnterEvent event(scenePos, mimeData->m_supportedActions, mimeData, Qt::NoButton, Qt::NoModifier);
        QQuickDropEventEx::setProposedAction(&event, proposedAction);
        deliverEvent(window, &event);
    }
}

void QQuickDragAttachedPrivate::deliverMoveEvent()
{
    Q_Q(QQuickDragAttached);

    itemMoved = false;
    if (window) {
        QPoint scenePos = attachedItem->mapToScene(hotSpot).toPoint();
        QDragMoveEvent event(scenePos, mimeData->m_supportedActions, mimeData, Qt::NoButton, Qt::NoModifier);
        QQuickDropEventEx::setProposedAction(&event, proposedAction);
        deliverEvent(window, &event);
        if (target != dragGrabber.target()) {
            target = dragGrabber.target();
            emit q->targetChanged();
        }
    }
}

void QQuickDragAttachedPrivate::deliverLeaveEvent()
{
    if (window) {
        QDragLeaveEvent event;
        deliverEvent(window, &event);
        window = 0;
    }
}

void QQuickDragAttachedPrivate::deliverEvent(QQuickWindow *window, QEvent *event)
{
    Q_ASSERT(!inEvent);
    inEvent = true;
    QQuickWindowPrivate::get(window)->deliverDragEvent(&dragGrabber, event);
    inEvent = false;
}

bool QQuickDragAttached::event(QEvent *event)
{
    Q_D(QQuickDragAttached);

    if (event->type() == QEvent::User) {
        d->eventQueued = false;
        if (d->dragRestarted) {
            d->deliverLeaveEvent();
            d->deliverEnterEvent();

            if (d->target != d->dragGrabber.target()) {
                d->target = d->dragGrabber.target();
                emit targetChanged();
            }
        } else if (d->itemMoved) {
            d->deliverMoveEvent();
        }
        return true;
    } else {
        return QObject::event(event);
    }
}

QQuickDragAttached::QQuickDragAttached(QObject *parent)
    : QObject(*new QQuickDragAttachedPrivate, parent)
{
    Q_D(QQuickDragAttached);
    d->attachedItem = qobject_cast<QQuickItem *>(parent);
    d->source = d->attachedItem;
}

QQuickDragAttached::~QQuickDragAttached()
{
    Q_D(QQuickDragAttached);
    delete d->mimeData;
}

/*!
    \qmlattachedproperty bool QtQuick2::Drag::active

    This property holds whether a drag event sequence is currently active.

    Setting this property to true will send a QDragEnter event to the scene
    with the item's current position.  Setting it to false will send a
    QDragLeave event.

    While a drag is active any change in an item's position will send a QDragMove
    event with item's new position to the scene.
*/

bool QQuickDragAttached::isActive() const
{
    Q_D(const QQuickDragAttached);
    return d->active;
}

void QQuickDragAttached::setActive(bool active)
{
    Q_D(QQuickDragAttached);
    if (d->active != active) {
        if (d->inEvent)
            qmlInfo(this) << "active cannot be changed from within a drag event handler";
        else if (active)
            d->start(d->supportedActions);
        else
            cancel();
    }
}

/*!
    \qmlattachedproperty Object QtQuick2::Drag::source

    This property holds an object that is identified to recipients of drag events as
    the source of the events.  By default this is the item Drag property is attached to.

    Changing the source while a drag is active will reset the sequence of drag events by
    sending a drag leave event followed by a drag enter event with the new source.
*/

QObject *QQuickDragAttached::source() const
{
    Q_D(const QQuickDragAttached);
    return d->source;
}

void QQuickDragAttached::setSource(QObject *item)
{
    Q_D(QQuickDragAttached);
    if (d->source != item) {
        d->source = item;
        if (d->active)
            d->restartDrag();
        emit sourceChanged();
    }
}

void QQuickDragAttached::resetSource()
{
    Q_D(QQuickDragAttached);
    if (d->source != d->attachedItem) {
        d->source = d->attachedItem;
        if (d->active)
            d->restartDrag();
        emit sourceChanged();
    }
}

/*!
    \qmlattachedproperty Object QtQuick2::Drag::target

    While a drag is active this property holds the last object to accept an
    enter event from the dragged item, if the current drag position doesn't
    intersect any accepting targets it is null.

    When a drag is not active this property holds the object that accepted
    the drop event that ended the drag, if no object accepted the drop or
    the drag was canceled the target will then be null.
*/

QObject *QQuickDragAttached::target() const
{
    Q_D(const QQuickDragAttached);
    return d->target;
}

/*!
    \qmlattachedproperty QPointF QtQuick2::Drag::hotSpot

    This property holds the drag position relative to the top left of the item.

    By default this is (0, 0).

    Changes to hotSpot trigger a new drag move with the updated position.
*/

QPointF QQuickDragAttached::hotSpot() const
{
    Q_D(const QQuickDragAttached);
    return d->hotSpot;
}

void QQuickDragAttached::setHotSpot(const QPointF &hotSpot)
{
    Q_D(QQuickDragAttached);
    if (d->hotSpot != hotSpot) {
        d->hotSpot = hotSpot;

        if (d->active)
            d->updatePosition();

        emit hotSpotChanged();
    }
}

/*!
    \qmlattachedproperty stringlist QtQuick2::Drag::keys

    This property holds a list of keys that can be used by a DropArea to filter drag events.

    Changing the keys while a drag is active will reset the sequence of drag events by
    sending a drag leave event followed by a drag enter event with the new source.
*/

QStringList QQuickDragAttached::keys() const
{
    Q_D(const QQuickDragAttached);
    return d->keys;
}

void QQuickDragAttached::setKeys(const QStringList &keys)
{
    Q_D(QQuickDragAttached);
    if (d->keys != keys) {
        d->keys = keys;
        if (d->active)
            d->restartDrag();
        emit keysChanged();
    }
}

/*!
    \qmlattachedproperty flags QtQuick2::Drag::supportedActions

    This property holds return values of Drag.drop() supported by the drag source.

    Changing the supportedActions while a drag is active will reset the sequence of drag
    events by sending a drag leave event followed by a drag enter event with the new source.
*/

Qt::DropActions QQuickDragAttached::supportedActions() const
{
    Q_D(const QQuickDragAttached);
    return d->supportedActions;
}

void QQuickDragAttached::setSupportedActions(Qt::DropActions actions)
{
    Q_D(QQuickDragAttached);
    if (d->supportedActions != actions) {
        d->supportedActions = actions;
        if (d->active)
            d->restartDrag();
        emit supportedActionsChanged();
    }
}

/*!
    \qmlattachedproperty enumeration QtQuick2::Drag::proposedAction

    This property holds an action that is recommended by the drag source as a
    return value from Drag.drop().

    Changes to proposedAction will trigger a move event with the updated proposal.
*/

Qt::DropAction QQuickDragAttached::proposedAction() const
{
    Q_D(const QQuickDragAttached);
    return d->proposedAction;
}

void QQuickDragAttached::setProposedAction(Qt::DropAction action)
{
    Q_D(QQuickDragAttached);
    if (d->proposedAction != action) {
        d->proposedAction = action;
        // The proposed action shouldn't affect whether a drag is accepted
        // so leave/enter events are excessive, but the target should still
        // updated.
        if (d->active)
            d->updatePosition();
        emit proposedActionChanged();
    }
}

void QQuickDragAttachedPrivate::start(Qt::DropActions supportedActions)
{
    Q_Q(QQuickDragAttached);
    Q_ASSERT(!active);

    if (!mimeData)
        mimeData = new QQuickDragMimeData;
    if (!listening) {
        QQuickItemPrivate::get(attachedItem)->addItemChangeListener(
                this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Parent);
        listening = true;
    }

    mimeData->m_supportedActions = supportedActions;
    active = true;
    itemMoved = false;
    dragRestarted = false;

    deliverEnterEvent();

    if (target != dragGrabber.target()) {
        target = dragGrabber.target();
        emit q->targetChanged();
    }

    emit q->activeChanged();
}

/*!
    \qmlattachedmethod void QtQuick2::Drag::start(flags supportedActions)

    Starts sending drag events.

    The optional \a supportedActions argument can be used to override the \l supportedActions
    property for the started sequence.
*/

void QQuickDragAttached::start(QQmlV8Function *args)
{
    Q_D(QQuickDragAttached);
    if (d->inEvent) {
        qmlInfo(this) << "start() cannot be called from within a drag event handler";
        return;
    }

    if (d->active)
        cancel();

    d->overrideActions = false;
    Qt::DropActions supportedActions = d->supportedActions;
    // check arguments for supportedActions, maybe data?
    if (args->Length() >= 1) {
        v8::Local<v8::Value> v = (*args)[0];
        if (v->IsInt32()) {
            supportedActions = Qt::DropActions(v->Int32Value());
            d->overrideActions = true;
        }
    }

    d->start(supportedActions);
}

/*!
    \qmlattachedmethod enumeration QtQuick2::Drag::drop()

    Ends a drag sequence by sending a drop event to the target item.

    Returns the action accepted by the target item.  If the target item or a parent doesn't accept
    the drop event then Qt.IgnoreAction will be returned.

    The returned drop action may be one of:

    \list
    \li Qt.CopyAction Copy the data to the target
    \li Qt.MoveAction Move the data from the source to the target
    \li Qt.LinkAction Create a link from the source to the target.
    \li Qt.IgnoreAction Ignore the action (do nothing with the data).
    \endlist

*/

int QQuickDragAttached::drop()
{
    Q_D(QQuickDragAttached);
    Qt::DropAction acceptedAction = Qt::IgnoreAction;

    if (d->inEvent) {
        qmlInfo(this) << "drop() cannot be called from within a drag event handler";
        return acceptedAction;
    }

    if (d->itemMoved)
        d->deliverMoveEvent();

    if (!d->active)
        return acceptedAction;
    d->active = false;

    QObject *target = 0;

    if (d->window) {
        QPoint scenePos = d->attachedItem->mapToScene(d->hotSpot).toPoint();

        QDropEvent event(
                scenePos, d->mimeData->m_supportedActions, d->mimeData, Qt::NoButton, Qt::NoModifier);
        QQuickDropEventEx::setProposedAction(&event, d->proposedAction);
        d->deliverEvent(d->window, &event);

        if (event.isAccepted()) {
            acceptedAction = event.dropAction();
            target = d->dragGrabber.target();
        }
    }

    if (d->target != target) {
        d->target = target;
        emit targetChanged();
    }

    emit activeChanged();
    return acceptedAction;
}

/*!
    \qmlattachedmethod void QtQuick2::Drag::cancel()

    Ends a drag sequence.
*/

void QQuickDragAttached::cancel()
{
    Q_D(QQuickDragAttached);

    if (d->inEvent) {
        qmlInfo(this) << "cancel() cannot be called from within a drag event handler";
        return;
    }

    if (!d->active)
        return;
    d->active = false;
    d->deliverLeaveEvent();

    if (d->target) {
        d->target = 0;
        emit targetChanged();
    }

    emit activeChanged();
}

QT_END_NAMESPACE

#endif // QT_NO_DRAGANDDROP
