// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpointerhandler_p.h"
#include "qquickpointerhandler_p_p.h"
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickhandlerpoint_p.h>
#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#include <QtGui/private/qinputdevice_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcPointerHandlerDispatch, "qt.quick.handler.dispatch")
Q_LOGGING_CATEGORY(lcPointerHandlerGrab, "qt.quick.handler.grab")
Q_LOGGING_CATEGORY(lcPointerHandlerActive, "qt.quick.handler.active")
Q_DECLARE_LOGGING_CATEGORY(lcHandlerParent)

/*!
    \qmltype PointerHandler
    \qmlabstract
    \since 5.10
    \instantiates QQuickPointerHandler
    \inqmlmodule QtQuick
    \brief Abstract handler for pointer events.

    PointerHandler is the base class Input Handler (not registered as a QML type) for
    events from any kind of pointing device (touch, mouse or graphics tablet).
*/

/*! \internal
    So far we only offer public QML API for Pointer Handlers, but we expect
    in some future version of Qt to have public C++ API as well. This will open
    up the possibility to instantiate handlers in custom items (which we should
    begin doing in Qt Quick Controls in the near future), and to subclass to make
    custom handlers (as TableView is already doing).

    To make a custom Pointer Handler, first try to choose the parent class
    according to your needs. If the gesture that you want to recognize could
    involve multiple touchpoints (even if it could start with only one point),
    subclass QQuickMultiPointHandler. If you are sure that you never want to
    handle more than one QEventPoint, subclass QQuickSinglePointHandler.
*/
QQuickPointerHandler::QQuickPointerHandler(QQuickItem *parent)
  : QQuickPointerHandler(*(new QQuickPointerHandlerPrivate), parent)
{
}

QQuickPointerHandler::QQuickPointerHandler(QQuickPointerHandlerPrivate &dd, QQuickItem *parent)
  : QObject(dd, parent)
{
    // When a handler is created in QML, the given parent is null, and we
    // depend on QQuickItemPrivate::data_append() later when it's added to an
    // item's DefaultProperty data property. But when a handler is created in
    // C++ with a parent item, data_append() won't be called, and the caller
    // shouldn't have to worry about it either.
    if (parent)
        QQuickItemPrivate::get(parent)->addPointerHandler(this);
}

QQuickPointerHandler::~QQuickPointerHandler()
{
    QQuickItem *parItem = parentItem();
    if (parItem) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(parItem);
        p->extra.value().pointerHandlers.removeOne(this);
    }
}

/*!
     \qmlproperty real PointerHandler::margin

     The margin beyond the bounds of the \l {PointerHandler::parent}{parent}
     item within which an \l eventPoint can activate this handler. For example, on
     a PinchHandler where the \l {PointerHandler::target}{target} is also the
     \c parent, it's useful to set this to a distance at least half the width
     of a typical user's finger, so that if the \c parent has been scaled down
     to a very small size, the pinch gesture is still possible.  Or, if a
     TapHandler-based button is placed near the screen edge, it can be used
     to comply with Fitts's Law: react to mouse clicks at the screen edge
     even though the button is visually spaced away from the edge by a few pixels.

     The default value is 0.

     \image pointerHandlerMargin.png
*/
qreal QQuickPointerHandler::margin() const
{
    Q_D(const QQuickPointerHandler);
    return d->m_margin;
}

void QQuickPointerHandler::setMargin(qreal pointDistanceThreshold)
{
    Q_D(QQuickPointerHandler);
    if (d->m_margin == pointDistanceThreshold)
        return;

    d->m_margin = pointDistanceThreshold;
    emit marginChanged();
}

/*!
    \qmlproperty int PointerHandler::dragThreshold
    \since 5.15

    The distance in pixels that the user must drag an \l eventPoint in order to
    have it treated as a drag gesture.

    The default value depends on the platform and screen resolution.
    It can be reset back to the default value by setting it to undefined.
    The behavior when a drag gesture begins varies in different handlers.
*/
int QQuickPointerHandler::dragThreshold() const
{
    Q_D(const QQuickPointerHandler);
    if (d->dragThreshold < 0)
        return qApp->styleHints()->startDragDistance();
    return d->dragThreshold;
}

void QQuickPointerHandler::setDragThreshold(int t)
{
    Q_D(QQuickPointerHandler);
    if (d->dragThreshold == t)
        return;

    if (t > std::numeric_limits<qint16>::max())
        qWarning() << "drag threshold cannot exceed" << std::numeric_limits<qint16>::max();
    d->dragThreshold = qint16(t);
    emit dragThresholdChanged();
}

void QQuickPointerHandler::resetDragThreshold()
{
    Q_D(QQuickPointerHandler);
    if (d->dragThreshold < 0)
        return;

    d->dragThreshold = -1;
    emit dragThresholdChanged();
}

/*!
    \since 5.15
    \qmlproperty Qt::CursorShape PointerHandler::cursorShape
    This property holds the cursor shape that will appear whenever the mouse is
    hovering over the \l parent item while \l active is \c true.

    The available cursor shapes are:
    \list
    \li Qt.ArrowCursor
    \li Qt.UpArrowCursor
    \li Qt.CrossCursor
    \li Qt.WaitCursor
    \li Qt.IBeamCursor
    \li Qt.SizeVerCursor
    \li Qt.SizeHorCursor
    \li Qt.SizeBDiagCursor
    \li Qt.SizeFDiagCursor
    \li Qt.SizeAllCursor
    \li Qt.BlankCursor
    \li Qt.SplitVCursor
    \li Qt.SplitHCursor
    \li Qt.PointingHandCursor
    \li Qt.ForbiddenCursor
    \li Qt.WhatsThisCursor
    \li Qt.BusyCursor
    \li Qt.OpenHandCursor
    \li Qt.ClosedHandCursor
    \li Qt.DragCopyCursor
    \li Qt.DragMoveCursor
    \li Qt.DragLinkCursor
    \endlist

    The default value is not set, which allows the \l {QQuickItem::cursor()}{cursor}
    of \l parent item to appear. This property can be reset to the same initial
    condition by setting it to undefined.

    \note When this property has not been set, or has been set to \c undefined,
    if you read the value it will return \c Qt.ArrowCursor.

    \sa Qt::CursorShape, QQuickItem::cursor(), HoverHandler::cursorShape
*/
#if QT_CONFIG(cursor)
Qt::CursorShape QQuickPointerHandler::cursorShape() const
{
    Q_D(const QQuickPointerHandler);
    return d->cursorShape;
}

void QQuickPointerHandler::setCursorShape(Qt::CursorShape shape)
{
    Q_D(QQuickPointerHandler);
    if (d->cursorSet && shape == d->cursorShape)
        return;
    d->cursorShape = shape;
    d->cursorSet = true;
    if (auto *parent = parentItem()) {
        QQuickItemPrivate *itemPriv = QQuickItemPrivate::get(parent);
        itemPriv->hasCursorHandler = true;
        itemPriv->setHasCursorInChild(true);
    }
    emit cursorShapeChanged();
}

void QQuickPointerHandler::resetCursorShape()
{
    Q_D(QQuickPointerHandler);
    if (!d->cursorSet)
        return;
    d->cursorShape = Qt::ArrowCursor;
    d->cursorSet = false;
    if (auto *parent = parentItem()) {
        QQuickItemPrivate *itemPriv = QQuickItemPrivate::get(parent);
        itemPriv->hasCursorHandler = false;
        itemPriv->setHasCursorInChild(itemPriv->hasCursor);
    }
    emit cursorShapeChanged();
}

bool QQuickPointerHandler::isCursorShapeExplicitlySet() const
{
    Q_D(const QQuickPointerHandler);
    return d->cursorSet;
}
#endif

/*!
    Notification that the grab has changed in some way which is relevant to this handler.
    The \a grabber (subject) will be the Input Handler whose state is changing,
    or null if the state change regards an Item.
    The \a transition (verb) tells what happened.
    The \a point (object) is the \l eventPoint that was grabbed or ungrabbed.
    QQuickDeliveryAgent calls this function.
    The Input Handler must react in whatever way is appropriate, and must
    emit the relevant signals (for the benefit of QML code).
    A subclass is allowed to override this virtual function, but must always
    call its parent class's implementation in addition to (usually after)
    whatever custom behavior it implements.
*/
void QQuickPointerHandler::onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition,
                                         QPointerEvent *event, QEventPoint &point)
{
    Q_UNUSED(event);
    qCDebug(lcPointerHandlerGrab) << point << transition << grabber;
    if (grabber == this) {
        bool wasCanceled = false;
        switch (transition) {
        case QPointingDevice::GrabPassive:
        case QPointingDevice::GrabExclusive:
            break;
        case QPointingDevice::CancelGrabPassive:
        case QPointingDevice::CancelGrabExclusive:
            wasCanceled = true; // the grab was stolen by something else
            Q_FALLTHROUGH();
        case QPointingDevice::UngrabPassive:
        case QPointingDevice::UngrabExclusive:
            setActive(false);
            point.setAccepted(false);
            if (auto par = parentItem()) {
                Q_D(const QQuickPointerHandler);
                par->setKeepMouseGrab(d->hadKeepMouseGrab);
                par->setKeepTouchGrab(d->hadKeepTouchGrab);
            }
            break;
        case QPointingDevice::OverrideGrabPassive:
            // Passive grab is still there, but we won't receive point updates right now.
            // No need to notify about this.
            return;
        }
        if (wasCanceled)
            emit canceled(point);
        emit grabChanged(transition, point);
    }
}

/*!
    Acquire or give up a passive grab of the given \a point, according to the \a grab state.

    Unlike the exclusive grab, multiple Input Handlers can have passive grabs
    simultaneously. This means that each of them will receive further events
    when the \a point moves, and when it is finally released. Typically an
    Input Handler should acquire a passive grab as soon as a point is pressed,
    if the handler's constraints do not clearly rule out any interest in that
    point. For example, DragHandler needs a passive grab in order to watch the
    movement of a point to see whether it will be dragged past the drag
    threshold. When a handler is actively manipulating its \l target (that is,
    when \l active is true), it may be able to do its work with only a passive
    grab, or it may acquire an exclusive grab if the gesture clearly must not
    be interpreted in another way by another handler.
*/
void QQuickPointerHandler::setPassiveGrab(QPointerEvent *event, const QEventPoint &point, bool grab)
{
    qCDebug(lcPointerHandlerGrab) << this << point << grab << "via"
                                  << QQuickDeliveryAgentPrivate::currentOrItemDeliveryAgent(parentItem());
    if (grab) {
        event->addPassiveGrabber(point, this);
    } else {
        event->removePassiveGrabber(point, this);
    }
}

/*!
    Check whether it's OK to take an exclusive grab of the \a point.

    The default implementation will call approveGrabTransition() to check this
    handler's \l grabPermissions. If grabbing can be done only by taking over
    the exclusive grab from an Item, approveGrabTransition() checks the Item's
    \l keepMouseGrab or \l keepTouchGrab flags appropriately. If grabbing can
    be done only by taking over another handler's exclusive grab, canGrab()
    also calls approveGrabTransition() on the handler which is about to lose
    its grab. Either one can deny the takeover.
*/
bool QQuickPointerHandler::canGrab(QPointerEvent *event, const QEventPoint &point)
{
    QQuickPointerHandler *existingPhGrabber = qobject_cast<QQuickPointerHandler *>(event->exclusiveGrabber(point));
    return approveGrabTransition(event, point, this) &&
        (existingPhGrabber ? existingPhGrabber->approveGrabTransition(event, point, this) : true);
}

/*!
    Check this handler's rules to see if \l proposedGrabber will be allowed to take
    the exclusive grab.  This function may be called twice: once on the instance which
    will take the grab, and once on the instance which would thereby lose its grab,
    in case of a takeover scenario.
*/
bool QQuickPointerHandler::approveGrabTransition(QPointerEvent *event, const QEventPoint &point, QObject *proposedGrabber)
{
    Q_D(const QQuickPointerHandler);
    bool allowed = false;
    QObject* existingGrabber = event->exclusiveGrabber(point);
    if (proposedGrabber == this) {
        allowed = (existingGrabber == nullptr) || ((d->grabPermissions & CanTakeOverFromAnything) == CanTakeOverFromAnything);
        if (existingGrabber) {
            if (QQuickPointerHandler *existingPhGrabber = qobject_cast<QQuickPointerHandler *>(event->exclusiveGrabber(point))) {
                if (!allowed && (d->grabPermissions & CanTakeOverFromHandlersOfDifferentType) &&
                        existingPhGrabber->metaObject()->className() != metaObject()->className())
                    allowed = true;
                if (!allowed && (d->grabPermissions & CanTakeOverFromHandlersOfSameType) &&
                        existingPhGrabber->metaObject()->className() == metaObject()->className())
                    allowed = true;
            } else if ((d->grabPermissions & CanTakeOverFromItems)) {
                allowed = true;
                QQuickItem * existingItemGrabber = qobject_cast<QQuickItem *>(event->exclusiveGrabber(point));
                auto da = parentItem() ? QQuickItemPrivate::get(parentItem())->deliveryAgentPrivate()
                                       : QQuickDeliveryAgentPrivate::currentEventDeliveryAgent ? static_cast<QQuickDeliveryAgentPrivate *>(
                                            QQuickDeliveryAgentPrivate::get(QQuickDeliveryAgentPrivate::currentEventDeliveryAgent)) : nullptr;
                const bool isTouchMouse = (da && da->isDeliveringTouchAsMouse());
                if (existingItemGrabber &&
                        ((existingItemGrabber->keepMouseGrab() &&
                          (QQuickDeliveryAgentPrivate::isMouseEvent(event) || isTouchMouse)) ||
                         (existingItemGrabber->keepTouchGrab() && QQuickDeliveryAgentPrivate::isTouchEvent(event)))) {
                    allowed = false;
                    // If the handler wants to steal the exclusive grab from an Item, the Item can usually veto
                    // by having its keepMouseGrab flag set.  But an exception is if that Item is a parent that
                    // normally filters events (such as a Flickable): it needs to be possible for e.g. a
                    // DragHandler to operate on an Item inside a Flickable.  Flickable is aggressive about
                    // grabbing on press (for fear of missing updates), but DragHandler uses a passive grab
                    // at first and then expects to be able to steal the grab later on.  It cannot respect
                    // Flickable's wishes in that case, because then it would never have a chance.
                    if (existingItemGrabber->keepMouseGrab() &&
                            existingItemGrabber->filtersChildMouseEvents() && existingItemGrabber->isAncestorOf(parentItem())) {
                        Q_ASSERT(da);
                        if (isTouchMouse && point.id() == da->touchMouseId) {
                            qCDebug(lcPointerHandlerGrab) << this << "steals touchpoint" << point.id()
                                << "despite parent touch-mouse grabber with keepMouseGrab=true" << existingItemGrabber;
                            allowed = true;
                        }
                    }
                    if (!allowed) {
                        qCDebug(lcPointerHandlerGrab) << this << "wants to grab point" << point.id()
                                << "but declines to steal from grabber" << existingItemGrabber
                                << "with keepMouseGrab=" << existingItemGrabber->keepMouseGrab()
                                << "keepTouchGrab=" << existingItemGrabber->keepTouchGrab();
                    }
                }
            }
        }
    } else {
        // proposedGrabber is different: that means this instance will lose its grab
        if (proposedGrabber) {
            if ((d->grabPermissions & ApprovesTakeOverByAnything) == ApprovesTakeOverByAnything)
                allowed = true;
            if (!allowed && (d->grabPermissions & ApprovesTakeOverByHandlersOfDifferentType) &&
                    proposedGrabber->metaObject()->className() != metaObject()->className())
                allowed = true;
            if (!allowed && (d->grabPermissions & ApprovesTakeOverByHandlersOfSameType) &&
                    proposedGrabber->metaObject()->className() == metaObject()->className())
                allowed = true;
            if (!allowed && (d->grabPermissions & ApprovesTakeOverByItems) && proposedGrabber->inherits("QQuickItem"))
                allowed = true;
        } else {
            if (d->grabPermissions & ApprovesCancellation)
                allowed = true;
        }
    }
    qCDebug(lcPointerHandlerGrab) << "point" << Qt::hex << point.id() << "permission" <<
            QMetaEnum::fromType<GrabPermissions>().valueToKeys(grabPermissions()) <<
            ':' << this << (allowed ? "approved from" : "denied from") <<
            existingGrabber << "to" << proposedGrabber;
    return allowed;
}

/*!
    \qmlproperty flags QtQuick::PointerHandler::grabPermissions

    This property specifies the permissions when this handler's logic decides
    to take over the exclusive grab, or when it is asked to approve grab
    takeover or cancellation by another handler.

    \value PointerHandler.TakeOverForbidden
           This handler neither takes from nor gives grab permission to any type of Item or Handler.
    \value PointerHandler.CanTakeOverFromHandlersOfSameType
           This handler can take the exclusive grab from another handler of the same class.
    \value PointerHandler.CanTakeOverFromHandlersOfDifferentType
           This handler can take the exclusive grab from any kind of handler.
    \value PointerHandler.CanTakeOverFromItems
           This handler can take the exclusive grab from any type of Item.
    \value PointerHandler.CanTakeOverFromAnything
           This handler can take the exclusive grab from any type of Item or Handler.
    \value PointerHandler.ApprovesTakeOverByHandlersOfSameType
           This handler gives permission for another handler of the same class to take the grab.
    \value PointerHandler.ApprovesTakeOverByHandlersOfDifferentType
           This handler gives permission for any kind of handler to take the grab.
    \value PointerHandler.ApprovesTakeOverByItems
           This handler gives permission for any kind of Item to take the grab.
    \value PointerHandler.ApprovesCancellation
           This handler will allow its grab to be set to null.
    \value PointerHandler.ApprovesTakeOverByAnything
           This handler gives permission for any type of Item or Handler to take the grab.

    The default is
    \c {PointerHandler.CanTakeOverFromItems | PointerHandler.CanTakeOverFromHandlersOfDifferentType | PointerHandler.ApprovesTakeOverByAnything}
    which allows most takeover scenarios but avoids e.g. two PinchHandlers fighting
    over the same touchpoints.
*/
QQuickPointerHandler::GrabPermissions QQuickPointerHandler::grabPermissions() const
{
    Q_D(const QQuickPointerHandler);
    return static_cast<QQuickPointerHandler::GrabPermissions>(d->grabPermissions);
}

void QQuickPointerHandler::setGrabPermissions(GrabPermissions grabPermission)
{
    Q_D(QQuickPointerHandler);
    if (d->grabPermissions == grabPermission)
        return;

    d->grabPermissions = grabPermission;
    emit grabPermissionChanged();
}

/*!
    Overridden only because QQmlParserStatus requires it.
*/
void QQuickPointerHandler::classBegin()
{
}

/*!
    Overridden from QQmlParserStatus to ensure that parentItem() sets its
    cursor if this handler's \l cursorShape property has been set.
*/
void QQuickPointerHandler::componentComplete()
{
    Q_D(const QQuickPointerHandler);
    if (d->cursorSet) {
        if (auto *parent = parentItem()) {
            QQuickItemPrivate *itemPriv = QQuickItemPrivate::get(parent);
            itemPriv->hasCursorHandler = true;
            itemPriv->setHasCursorInChild(true);
        }
    }
}

/*! \internal
    \deprecated You should handle the event during delivery by overriding
    handlePointerEventImpl() or QQuickSinglePointHandler::handleEventPoint().
    Therefore currentEvent() should not be needed. It is here only because
    onActiveChanged() does not take the event as an argument.
*/
QPointerEvent *QQuickPointerHandler::currentEvent()
{
    Q_D(const QQuickPointerHandler);
    return d->currentEvent;
}

/*!
    Acquire or give up the exclusive grab of the given \a point, according to
    the \a grab state, and subject to the rules: canGrab(), and the rule not to
    relinquish another handler's grab. Returns true if permission is granted,
    or if the exclusive grab has already been acquired or relinquished as
    specified. Returns false if permission is denied either by this handler or
    by the handler or item from which this handler would take over
*/
bool QQuickPointerHandler::setExclusiveGrab(QPointerEvent *ev, const QEventPoint &point, bool grab)
{
    if ((grab && ev->exclusiveGrabber(point) == this) || (!grab && ev->exclusiveGrabber(point) != this))
        return true;
    // TODO m_hadKeepMouseGrab m_hadKeepTouchGrab
    bool allowed = true;
    if (grab) {
        allowed = canGrab(ev, point);
    } else {
        QQuickPointerHandler *existingPhGrabber = qobject_cast<QQuickPointerHandler *>(ev->exclusiveGrabber(point));
        // Ask before allowing one handler to cancel another's grab
        if (existingPhGrabber && existingPhGrabber != this && !existingPhGrabber->approveGrabTransition(ev, point, nullptr))
            allowed = false;
    }
    qCDebug(lcPointerHandlerGrab) << point << (grab ? "grab" : "ungrab") << (allowed ? "allowed" : "forbidden") <<
        ev->exclusiveGrabber(point) << "->" << (grab ? this : nullptr);
    if (allowed)
        ev->setExclusiveGrabber(point, grab ? this : nullptr);
    return allowed;
}

/*!
    Cancel any existing grab of the given \a point.
*/
void QQuickPointerHandler::cancelAllGrabs(QPointerEvent *event, QEventPoint &point)
{
    qCDebug(lcPointerHandlerGrab) << point;
    if (event->exclusiveGrabber(point) == this) {
        event->setExclusiveGrabber(point, nullptr);
        onGrabChanged(this, QPointingDevice::CancelGrabExclusive, event, point);
    }
    if (event->removePassiveGrabber(point, this))
        onGrabChanged(this, QPointingDevice::CancelGrabPassive, event, point);
}

QPointF QQuickPointerHandler::eventPos(const QEventPoint &point) const
{
    return (target() ? target()->mapFromScene(point.scenePosition()) : point.scenePosition());
}

/*!
    Returns \c true if margin() > 0 and \a point is within the margin beyond
    QQuickItem::boundingRect(), or else returns QQuickItem::contains()
    QEventPoint::position() effectively (because parentContains(scenePosition)
    calls QQuickItem::mapFromScene()).
*/
bool QQuickPointerHandler::parentContains(const QEventPoint &point) const
{
    return parentContains(point.scenePosition());
}

/*!
    Returns \c true if \a scenePosition is within the margin() beyond
    QQuickItem::boundingRect() (if margin > 0), or parentItem() contains
    \a scenePosition according to QQuickItem::contains(). (So if the \l margin
    property is set, that overrides the bounds-check, and QQuickItem::contains()
    is not called.) As a precheck, it's also required that the window contains
    \a scenePosition mapped to global coordinates, if parentItem() is in a window.
*/
bool QQuickPointerHandler::parentContains(const QPointF &scenePosition) const
{
    if (QQuickItem *par = parentItem()) {
        if (par->window()) {
            QRect windowGeometry = par->window()->geometry();
            if (!par->window()->isTopLevel())
                windowGeometry = QRect(QWindowPrivate::get(par->window())->globalPosition(), par->window()->size());
            QPoint screenPosition = par->window()->mapToGlobal(scenePosition.toPoint());
            if (!windowGeometry.contains(screenPosition))
                return false;
        }
        QPointF p = par->mapFromScene(scenePosition);
        qreal m = margin();
        if (m > 0)
            return p.x() >= -m && p.y() >= -m && p.x() <= par->width() + m && p.y() <= par->height() + m;
        return par->contains(p);
    } else if (parent() && parent()->inherits("QQuick3DModel")) {
        // If the parent is from Qt Quick 3D, assume that
        // bounds checking was already done, as part of picking.
        return true;
    }
    return false;
}

/*!
     \qmlproperty bool QtQuick::PointerHandler::enabled

     If a PointerHandler is disabled, it will reject all events
     and no signals will be emitted.
*/
bool QQuickPointerHandler::enabled() const
{
    Q_D(const QQuickPointerHandler);
    return d->enabled;
}

void QQuickPointerHandler::setEnabled(bool enabled)
{
    Q_D(QQuickPointerHandler);
    if (d->enabled == enabled)
        return;

    d->enabled = enabled;
    d->onEnabledChanged();

    emit enabledChanged();
}

/*!
    \qmlproperty Item QtQuick::PointerHandler::target

    The Item which this handler will manipulate.

    By default, it is the same as the \l [QML] {parent}, the Item within which
    the handler is declared. However, it can sometimes be useful to set the
    target to a different Item, in order to handle events within one item
    but manipulate another; or to \c null, to disable the default behavior
    and do something else instead.
*/
QQuickItem *QQuickPointerHandler::target() const
{
    Q_D(const QQuickPointerHandler);
    if (!d->targetExplicitlySet)
        return parentItem();
    return d->target;
}

void QQuickPointerHandler::setTarget(QQuickItem *target)
{
    Q_D(QQuickPointerHandler);
    d->targetExplicitlySet = true;
    if (d->target == target)
        return;

    QQuickItem *oldTarget = d->target;
    d->target = target;
    onTargetChanged(oldTarget);
    emit targetChanged();
}

/*!
    \qmlproperty Item QtQuick::PointerHandler::parent

    The \l Item which is the scope of the handler; the Item in which it was
    declared. The handler will handle events on behalf of this Item, which
    means a pointer event is relevant if at least one of its
    \l {eventPoint}{eventPoints} occurs within the Item's interior. Initially
    \l [QML] {target} {target()} is the same, but it can be reassigned.

    \sa {target}, QObject::parent()
*/
/*! \internal
    We still haven't shipped official support for declaring handlers in
    QtQuick3D.Model objects. Many prerequisites are in place for that, so we
    should try to keep it working; but there are issues with getting
    DragHandler to drag its target intuitively in 3D space, for example.
    TapHandler would work well enough.

    \note When a handler is declared in a \l [QtQuick3D] {Model}{QtQuick3D.Model}
        object, the parent is not an Item, therefore this property is \c null.
*/
QQuickItem *QQuickPointerHandler::parentItem() const
{
    return qmlobject_cast<QQuickItem *>(QObject::parent());
}

void QQuickPointerHandler::setParentItem(QQuickItem *p)
{
    Q_D(QQuickPointerHandler);
    if (QObject::parent() == p)
        return;

    qCDebug(lcHandlerParent) << "reparenting handler" << this << ":" << parent() << "->" << p;
    auto *oldParent = static_cast<QQuickItem *>(QObject::parent());
    if (oldParent)
        QQuickItemPrivate::get(oldParent)->removePointerHandler(this);
    setParent(p);
    if (p)
        QQuickItemPrivate::get(p)->addPointerHandler(this);
    d->onParentChanged(oldParent, p);
    emit parentChanged();
}

/*! \internal
    Pointer Handlers do most of their work in implementations of virtual functions
    that are called directly from QQuickItem, not by direct event handling.
    But it's convenient to deliver TouchCancel events via QCoreApplication::sendEvent().
    Perhaps it will turn out that more events could be delivered this way.
*/
bool QQuickPointerHandler::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::TouchCancel: {
        auto te = static_cast<QTouchEvent *>(e);
        for (int i = 0; i < te->pointCount(); ++i)
            onGrabChanged(this, QPointingDevice::CancelGrabExclusive, te, te->point(i));
        return true;
        break;
    }
    default:
        return QObject::event(e);
        break;
    }
}

/*! \internal
    The entry point to handle the \a event: it's called from
    QQuickItemPrivate::handlePointerEvent(), begins with wantsPointerEvent(),
    and calls handlePointerEventImpl() if that returns \c true.
*/
void QQuickPointerHandler::handlePointerEvent(QPointerEvent *event)
{
    Q_D(QQuickPointerHandler);
    bool wants = wantsPointerEvent(event);
    qCDebug(lcPointerHandlerDispatch) << metaObject()->className() << objectName()
                                      << "on" << parent()->metaObject()->className() << parent()->objectName()
                                      << (wants ? "WANTS" : "DECLINES") << event;
    d->currentEvent = event;
    if (wants) {
        handlePointerEventImpl(event);
        d->lastEventTime = event->timestamp();
    } else {
#if QT_CONFIG(gestures)
        if (event->type() != QEvent::NativeGesture)
#endif
            setActive(false);
        for (int i = 0; i < event->pointCount(); ++i) {
            auto &pt = event->point(i);
            if (event->exclusiveGrabber(pt) == this && pt.state() != QEventPoint::Stationary)
                event->setExclusiveGrabber(pt, nullptr);
        }
    }
    d->currentEvent = nullptr;
    QQuickPointerHandlerPrivate::deviceDeliveryTargets(event->device()).append(this);
}

/*!
    It is the responsibility of this function to decide whether the \a event
    could be relevant at all to this handler, as a preliminary check.

    Returns \c true if this handler would like handlePointerEventImpl() to be called.
    If it returns \c false, the handler will be deactivated: \c setActive(false)
    will be called, and any remaining exclusive grab will be relinquished,
    as a fail-safe.

    If you override this function, you should call the immediate parent class
    implementation (and return \c false if it returns \c false); that in turn
    calls its parent class implementation, and so on.
    QQuickSinglePointHandler::wantsPointerEvent() and
    QQuickMultiPointHandler::wantsPointerEvent() call wantsEventPoint(), which
    is also virtual. You usually can get the behavior you want by subclassing
    the appropriate handler type, overriding
    QQuickSinglePointHandler::handleEventPoint() or handlePointerEventImpl(),
    and perhaps overriding wantsEventPoint() if needed.

    \sa wantsEventPoint(), QQuickPointerDeviceHandler::wantsPointerEvent(),
        QQuickMultiPointHandler::wantsPointerEvent(), QQuickSinglePointHandler::wantsPointerEvent()
 */
bool QQuickPointerHandler::wantsPointerEvent(QPointerEvent *event)
{
    Q_D(const QQuickPointerHandler);
    Q_UNUSED(event);
    return d->enabled;
}

/*!
    Returns \c true if the given \a point (as part of \a event) could be
    relevant at all to this handler, as a preliminary check.

    If you override this function, you should call the immediate parent class
    implementation (and return \c false if it returns \c false); that in turn
    calls its parent class implementation, and so on.

    In particular, the bounds checking is done here: the base class
    QQuickPointerHandler::wantsEventPoint() calls parentContains(point)
    (which allows the flexibility promised by margin(), QQuickItem::contains()
    and QQuickItem::containmentMask()). Pointer Handlers can receive
    QEventPoints that are outside the parent item's bounds: this allows some
    flexibility for dealing with multi-point gestures in which one or more
    fingers have strayed outside the bounds, and yet the gesture is still
    unambiguously intended for the target() item.

    You should not generally react to the \a event or \a point here, but it's
    ok to set state to remember what needs to be done in your overridden
    handlePointerEventImpl() or QQuickSinglePointHandler::handleEventPoint().
*/
bool QQuickPointerHandler::wantsEventPoint(const QPointerEvent *event, const QEventPoint &point)
{
    Q_UNUSED(event);
    bool ret = event->exclusiveGrabber(point) == this ||
            event->passiveGrabbers(point).contains(this) || parentContains(point);
    qCDebug(lcPointerHandlerDispatch) << Qt::hex << point.id() << "@" << point.scenePosition()
                                      << metaObject()->className() << objectName() << ret;
    return ret;
}

/*!
    \readonly
    \qmlproperty bool QtQuick::PointerHandler::active

    This holds \c true whenever this Input Handler has taken sole responsibility
    for handing one or more \l {eventPoint}{eventPoints}, by successfully taking an
    exclusive grab of those points. This means that it is keeping its properties
    up-to-date according to the movements of those eventPoints and actively
    manipulating its \l target (if any).
*/
bool QQuickPointerHandler::active() const
{
    Q_D(const QQuickPointerHandler);
    return d->active;
}

void QQuickPointerHandler::setActive(bool active)
{
    Q_D(QQuickPointerHandler);
    if (d->active != active) {
        qCDebug(lcPointerHandlerActive) << this << d->active << "->" << active;
        d->active = active;
        onActiveChanged();
        emit activeChanged();
    }
}

/*!
    This function can be overridden to implement whatever behavior a specific
    subclass is intended to have:
    \list
        \li Handle all the event's QPointerEvent::points() for which
        wantsEventPoint() already returned \c true.
        \li Call setPassiveGrab() setExclusiveGrab() or cancelAllGrabs() as
        necessary.
        \li Call QEvent::accept() to stop propagation, or ignore() to allow it
        to keep going.
    \endlist
*/
void QQuickPointerHandler::handlePointerEventImpl(QPointerEvent *event)
{
    Q_UNUSED(event);
}

/*!
    \qmlsignal QtQuick::PointerHandler::grabChanged(PointerDevice::GrabTransition transition, eventPoint point)

    This signal is emitted when the grab has changed in some way which is
    relevant to this handler.

    The \a transition (verb) tells what happened.
    The \a point (object) is the point that was grabbed or ungrabbed.

    Valid values for \a transition are:

    \value PointerDevice.GrabExclusive
        This handler has taken primary responsibility for handling the \a point.
    \value PointerDevice.UngrabExclusive
        This handler has given up its previous exclusive grab.
    \value PointerDevice.CancelGrabExclusive
        This handler's exclusive grab has been taken over or cancelled.
    \value PointerDevice.GrabPassive
        This handler has acquired a passive grab, to monitor the \a point.
    \value PointerDevice.UngrabPassive
        This handler has given up its previous passive grab.
    \value PointerDevice.CancelGrabPassive
        This handler's previous passive grab has terminated abnormally.
*/

/*!
    \qmlsignal QtQuick::PointerHandler::canceled(eventPoint point)

    If this handler has already grabbed the given \a point, this signal is
    emitted when the grab is stolen by a different Pointer Handler or Item.
*/

QQuickPointerHandlerPrivate::QQuickPointerHandlerPrivate()
  : grabPermissions(QQuickPointerHandler::CanTakeOverFromItems |
                      QQuickPointerHandler::CanTakeOverFromHandlersOfDifferentType |
                      QQuickPointerHandler::ApprovesTakeOverByAnything)
  , cursorShape(Qt::ArrowCursor)
  , enabled(true)
  , active(false)
  , targetExplicitlySet(false)
  , hadKeepMouseGrab(false)
  , hadKeepTouchGrab(false)
  , cursorSet(false)
{
}

/*! \internal
    Returns \c true if the movement delta \a d in pixels along the \a axis
    exceeds QQuickPointerHandler::dragThreshold() \e or QEventPoint::velocity()
    exceeds QStyleHints::startDragVelocity().
*/
template <typename TEventPoint>
bool QQuickPointerHandlerPrivate::dragOverThreshold(qreal d, Qt::Axis axis, const TEventPoint &p) const
{
    Q_Q(const QQuickPointerHandler);
    QStyleHints *styleHints = qApp->styleHints();
    bool overThreshold = qAbs(d) > q->dragThreshold();
    const bool dragVelocityLimitAvailable = (styleHints->startDragVelocity() > 0);
    if (!overThreshold && dragVelocityLimitAvailable) {
        qreal velocity = qreal(axis == Qt::XAxis ? p.velocity().x() : p.velocity().y());
        overThreshold |= qAbs(velocity) > styleHints->startDragVelocity();
    }
    return overThreshold;
}

/*!
    Returns \c true if the movement \a delta in pixels exceeds
    QQuickPointerHandler::dragThreshold().
*/
bool QQuickPointerHandlerPrivate::dragOverThreshold(QVector2D delta) const
{
    Q_Q(const QQuickPointerHandler);
    const float threshold = q->dragThreshold();
    return qAbs(delta.x()) > threshold || qAbs(delta.y()) > threshold;
}

/*!
    Returns \c true if the movement \a delta in pixels (calculated as
    QEventPoint::scenePosition() - QEventPoint::scenePressPosition())
    expressed in \a point exceeds QQuickPointerHandler::dragThreshold().
*/
bool QQuickPointerHandlerPrivate::dragOverThreshold(const QEventPoint &point) const
{
    QPointF delta = point.scenePosition() - point.scenePressPosition();
    return (dragOverThreshold(delta.x(), Qt::XAxis, point) ||
            dragOverThreshold(delta.y(), Qt::YAxis, point));
}

QVector<QObject *> &QQuickPointerHandlerPrivate::deviceDeliveryTargets(const QInputDevice *device)
{
    return QQuickDeliveryAgentPrivate::deviceExtra(device)->deliveryTargets;
}

QT_END_NAMESPACE

#include "moc_qquickpointerhandler_p.cpp"
