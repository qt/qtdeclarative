// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickdraghandler_p.h"
#include <private/qquickwindow_p.h>
#include <private/qquickmultipointhandler_p_p.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

static const qreal DragAngleToleranceDegrees = 10;

Q_STATIC_LOGGING_CATEGORY(lcDragHandler, "qt.quick.handler.drag")

/*!
    \qmltype DragHandler
    \nativetype QQuickDragHandler
    \inherits MultiPointHandler
    \inqmlmodule QtQuick
    \ingroup qtquick-input-handlers
    \brief Handler for dragging.

    DragHandler is a handler that is used to interactively move an Item.
    Like other Input Handlers, by default it is fully functional, and
    manipulates its \l {PointerHandler::target} {target}.

    \snippet pointerHandlers/dragHandler.qml 0

    It has properties to restrict the range of dragging.

    If it is declared within one Item but is assigned a different
    \l {PointerHandler::target} {target}, then it handles events within the
    bounds of the \l {PointerHandler::parent} {parent} Item but
    manipulates the \c target Item instead:

    \snippet pointerHandlers/dragHandlerDifferentTarget.qml 0

    A third way to use it is to set \l {PointerHandler::target} {target} to
    \c null and react to property changes in some other way:

    \snippet pointerHandlers/dragHandlerNullTarget.qml 0

    If minimumPointCount and maximumPointCount are set to values larger than 1,
    the user will need to drag that many fingers in the same direction to start
    dragging. A multi-finger drag gesture can be detected independently of both
    a (default) single-finger DragHandler and a PinchHandler on the same Item,
    and thus can be used to adjust some other feature independently of the
    usual pinch behavior: for example adjust a tilt transformation, or adjust
    some other numeric value, if the \c target is set to null. But if the
    \c target is an Item, \c centroid is the point at which the drag begins and
    to which the \c target will be moved (subject to constraints).

    DragHandler can be used together with the \l Drag attached property to
    implement drag-and-drop.

    \sa Drag, MouseArea, {Qt Quick Examples - Pointer Handlers}
*/

QQuickDragHandler::QQuickDragHandler(QQuickItem *parent)
    : QQuickMultiPointHandler(parent, 1, 1)
{
}

QPointF QQuickDragHandler::targetCentroidPosition()
{
    QPointF pos = centroid().position();
    if (auto par = parentItem()) {
        if (target() != par)
            pos = par->mapToItem(target(), pos);
    }
    return pos;
}

void QQuickDragHandler::onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition, QPointerEvent *event, QEventPoint &point)
{
    QQuickMultiPointHandler::onGrabChanged(grabber, transition, event, point);
    if (grabber == this && transition == QPointingDevice::GrabExclusive && target()) {
        // In case the grab got handed over from another grabber, we might not get the Press.

        auto isDescendant = [](QQuickItem *parent, QQuickItem *target) {
            return parent && (target != parent) && !target->isAncestorOf(parent);
        };
        if (m_snapMode == SnapAlways
            || (m_snapMode == SnapIfPressedOutsideTarget && !m_pressedInsideTarget)
            || (m_snapMode == SnapAuto && !m_pressedInsideTarget && isDescendant(parentItem(), target()))
            ) {
                m_pressTargetPos = QPointF(target()->width(), target()->height()) / 2;
        } else if (m_pressTargetPos.isNull()) {
            m_pressTargetPos = targetCentroidPosition();
        }
    }
}

/*!
    \qmlproperty enumeration QtQuick::DragHandler::snapMode

    This property holds the snap mode.

    The snap mode configures snapping of the \l target item's center to the \l eventPoint.

    Possible values:
    \value DragHandler.NoSnap Never snap
    \value DragHandler.SnapAuto The \l target snaps if the \l eventPoint was pressed outside of the \l target
                                item \e and the \l target is a descendant of \l {PointerHandler::}{parent} item (default)
    \value DragHandler.SnapWhenPressedOutsideTarget The \l target snaps if the \l eventPoint was pressed outside of the \l target
    \value DragHandler.SnapAlways Always snap
*/
QQuickDragHandler::SnapMode QQuickDragHandler::snapMode() const
{
    return m_snapMode;
}

void QQuickDragHandler::setSnapMode(QQuickDragHandler::SnapMode mode)
{
    if (mode == m_snapMode)
        return;
    m_snapMode = mode;
    emit snapModeChanged();
}

void QQuickDragHandler::onActiveChanged()
{
    QQuickMultiPointHandler::onActiveChanged();
    const bool curActive = active();
    m_xAxis.onActiveChanged(curActive, 0);
    m_yAxis.onActiveChanged(curActive, 0);
    if (curActive) {
        if (auto parent = parentItem()) {
            if (QQuickDeliveryAgentPrivate::isTouchEvent(currentEvent()))
                parent->setKeepTouchGrab(true);
            // tablet and mouse are treated the same by Item's legacy event handling, and
            // touch becomes synth-mouse for Flickable, so we need to prevent stealing
            // mouse grab too, whenever dragging occurs in an enabled direction
            parent->setKeepMouseGrab(true);
        }
    } else {
        m_pressTargetPos = QPointF();
        m_pressedInsideTarget = false;
        m_pressedInsideParent = false;
        if (auto parent = parentItem()) {
            parent->setKeepTouchGrab(false);
            parent->setKeepMouseGrab(false);
        }
    }
}

bool QQuickDragHandler::wantsPointerEvent(QPointerEvent *event)
{
    if (!QQuickMultiPointHandler::wantsPointerEvent(event))
        /* Do handle other events than we would normally care about
           while we are still doing a drag; otherwise we would suddenly
           become inactive when a wheel event arrives during dragging.
           This extra condition needs to be kept in sync with
           handlePointerEventImpl */
        if (!active())
            return false;

#if QT_CONFIG(gestures)
    if (event->type() == QEvent::NativeGesture)
       return false;
#endif

    if (event->isBeginEvent()) {
        // At least one point must be pressed in the parent item
        if (event->isSinglePointEvent()) {
            m_pressedInsideParent = parentContains(event->points().first());
        } else {
            for (int i = 0; !m_pressedInsideParent && i < event->pointCount(); ++i) {
                auto &p = event->point(i);
                if (p.state() == QEventPoint::Pressed && parentContains(p))
                    m_pressedInsideParent = true;
            }
        }
    }

    if (!m_pressedInsideParent)
        return false;

    return true;
}

void QQuickDragHandler::handlePointerEventImpl(QPointerEvent *event)
{
    if (active() && !QQuickMultiPointHandler::wantsPointerEvent(event))
        return; // see QQuickDragHandler::wantsPointerEvent; we don't want to handle those events

    QQuickMultiPointHandler::handlePointerEventImpl(event);
    event->accept(); // just the event, not the points

    if (active()) {
        // Calculate drag delta, taking into account the axis enabled constraint
        // i.e. if xAxis is not enabled, then ignore the horizontal component of the actual movement
        QVector2D accumulatedDragDelta = QVector2D(centroid().scenePosition() - centroid().scenePressPosition());
        if (!m_xAxis.enabled())
            accumulatedDragDelta.setX(0);
        if (!m_yAxis.enabled())
            accumulatedDragDelta.setY(0);
        setActiveTranslation(accumulatedDragDelta);
    } else {
        // Check that all points have been dragged past the drag threshold,
        // to the extent that the constraints allow,
        // and in approximately the same direction
        qreal minAngle =  361;
        qreal maxAngle = -361;
        bool allOverThreshold = QQuickDeliveryAgentPrivate::isTouchEvent(event) ?
                static_cast<QTouchEvent *>(event)->touchPointStates() != QEventPoint::Released :
                !event->isEndEvent();
        QVector<QEventPoint> chosenPoints;

        if (event->isBeginEvent())
            m_pressedInsideTarget = target() && currentPoints().size() > 0;

        for (const QQuickHandlerPoint &p : std::as_const(currentPoints())) {
            if (!allOverThreshold)
                break;
            auto point = event->pointById(p.id());
            Q_ASSERT(point);
            chosenPoints << *point;
            setPassiveGrab(event, *point);
            // Calculate drag delta, taking into account the axis enabled constraint
            // i.e. if xAxis is not enabled, then ignore the horizontal component of the actual movement
            auto const mapFromScene = [this](auto const &scenePos) {
                return target() ? target()->mapFromScene(scenePos) : scenePos;
            };
            QVector2D accumulatedDragDelta = QVector2D(mapFromScene(point->scenePosition())
                                                       - mapFromScene(point->scenePressPosition()));
            if (!m_xAxis.enabled()) {
                // If horizontal dragging is disallowed, but the user is dragging
                // mostly horizontally, then don't activate.
                if (qAbs(accumulatedDragDelta.x()) > qAbs(accumulatedDragDelta.y()))
                    accumulatedDragDelta.setY(0);
                accumulatedDragDelta.setX(0);
            }
            if (!m_yAxis.enabled()) {
                // If vertical dragging is disallowed, but the user is dragging
                // mostly vertically, then don't activate.
                if (qAbs(accumulatedDragDelta.y()) > qAbs(accumulatedDragDelta.x()))
                    accumulatedDragDelta.setX(0);
                accumulatedDragDelta.setY(0);
            }
            qreal angle = std::atan2(accumulatedDragDelta.y(), accumulatedDragDelta.x()) * 180 / M_PI;
            bool overThreshold = d_func()->dragOverThreshold(accumulatedDragDelta);
            qCDebug(lcDragHandler) << "movement" << accumulatedDragDelta << "angle" << angle << "of point" << point
                                   << "pressed @" << point->scenePressPosition() << "over threshold?" << overThreshold;
            minAngle = qMin(angle, minAngle);
            maxAngle = qMax(angle, maxAngle);
            if (allOverThreshold && !overThreshold)
                allOverThreshold = false;

            if (event->isBeginEvent()) {
                // m_pressedInsideTarget should stay true iff ALL points in which DragHandler is interested
                // have been pressed inside the target() Item.  (E.g. in a Slider the parent might be the
                // whole control while the target is just the knob.)
                if (target()) {
                    const QPointF localPressPos = target()->mapFromScene(point->scenePressPosition());
                    m_pressedInsideTarget &= target()->contains(localPressPos);
                    m_pressTargetPos = targetCentroidPosition();
                }
                // QQuickDeliveryAgentPrivate::deliverToPassiveGrabbers() skips subsequent delivery if the event is filtered.
                // That affects behavior for mouse but not for touch, because Flickable behaves differently in the mouse case.
                // So we have to compensate by accepting the event here to avoid any parent Flickable from
                // getting the event via direct delivery and grabbing too soon.
                if (QQuickDeliveryAgentPrivate::isMouseEvent(event))
                    point->setAccepted(true); // stop propagation iff it's a mouse event
            }
        }
        if (allOverThreshold) {
            qreal angleDiff = maxAngle - minAngle;
            if (angleDiff > 180)
                angleDiff = 360 - angleDiff;
            qCDebug(lcDragHandler) << "angle min" << minAngle << "max" << maxAngle << "range" << angleDiff;
            if (angleDiff < DragAngleToleranceDegrees && grabPoints(event, chosenPoints))
                setActive(true);
        }
    }
    if (active() && target() && target()->parentItem()) {
        const QPointF newTargetTopLeft = targetCentroidPosition() - m_pressTargetPos;
        const QPointF xformOrigin = target()->transformOriginPoint();
        const QPointF targetXformOrigin = newTargetTopLeft + xformOrigin;
        QPointF pos = target()->parentItem()->mapFromItem(target(), targetXformOrigin);
        pos -= xformOrigin;
        QPointF targetItemPos = target()->position();
        if (!m_xAxis.enabled())
            pos.setX(targetItemPos.x());
        if (!m_yAxis.enabled())
            pos.setY(targetItemPos.y());
        enforceAxisConstraints(&pos);
        moveTarget(pos);
    }
}

void QQuickDragHandler::enforceAxisConstraints(QPointF *localPos)
{
    if (m_xAxis.enabled())
        localPos->setX(qBound(m_xAxis.minimum(), localPos->x(), m_xAxis.maximum()));
    if (m_yAxis.enabled())
        localPos->setY(qBound(m_yAxis.minimum(), localPos->y(), m_yAxis.maximum()));
}

void QQuickDragHandler::setPersistentTranslation(const QVector2D &trans)
{
    if (trans == persistentTranslation())
        return;

    m_xAxis.updateValue(m_xAxis.activeValue(), trans.x());
    m_yAxis.updateValue(m_yAxis.activeValue(), trans.y());
    emit translationChanged({});
}

void QQuickDragHandler::setActiveTranslation(const QVector2D &trans)
{
    if (trans == activeTranslation())
        return;

    const QVector2D delta = trans - activeTranslation();
    m_xAxis.updateValue(trans.x(), m_xAxis.persistentValue() + delta.x(), delta.x());
    m_yAxis.updateValue(trans.y(), m_yAxis.persistentValue() + delta.y(), delta.y());

    qCDebug(lcDragHandler) << "translation: delta" << delta
                           << "active" << trans << "accumulated" << persistentTranslation();
    emit translationChanged(delta);
}

/*!
    \qmlpropertygroup QtQuick::DragHandler::xAxis
    \qmlproperty real QtQuick::DragHandler::xAxis.minimum
    \qmlproperty real QtQuick::DragHandler::xAxis.maximum
    \qmlproperty bool QtQuick::DragHandler::xAxis.enabled
    \qmlproperty real QtQuick::DragHandler::xAxis.activeValue

    \c xAxis controls the constraints for horizontal dragging.

    \c minimum is the minimum acceptable value of \l {Item::x}{x} to be
    applied to the \l {PointerHandler::target} {target}.
    \c maximum is the maximum acceptable value of \l {Item::x}{x} to be
    applied to the \l {PointerHandler::target} {target}.
    If \c enabled is true, horizontal dragging is allowed.
    \c activeValue is the same as \l {QtQuick::DragHandler::activeTranslation}{activeTranslation.x}.

    The \c activeValueChanged signal is emitted when \c activeValue changes, to
    provide the increment by which it changed.
    This is intended for incrementally adjusting one property via multiple handlers.
*/

/*!
    \qmlpropertygroup QtQuick::DragHandler::yAxis
    \qmlproperty real QtQuick::DragHandler::yAxis.minimum
    \qmlproperty real QtQuick::DragHandler::yAxis.maximum
    \qmlproperty bool QtQuick::DragHandler::yAxis.enabled
    \qmlproperty real QtQuick::DragHandler::yAxis.activeValue

    \c yAxis controls the constraints for vertical dragging.

    \c minimum is the minimum acceptable value of \l {Item::y}{y} to be
    applied to the \l {PointerHandler::target} {target}.
    \c maximum is the maximum acceptable value of \l {Item::y}{y} to be
    applied to the \l {PointerHandler::target} {target}.
    If \c enabled is true, vertical dragging is allowed.
    \c activeValue is the same as \l {QtQuick::DragHandler::activeTranslation}{activeTranslation.y}.

    The \c activeValueChanged signal is emitted when \c activeValue changes, to
    provide the increment by which it changed.
    This is intended for incrementally adjusting one property via multiple handlers:

    \snippet pointerHandlers/rotateViaWheelOrDrag.qml 0
*/

/*!
    \readonly
    \qmlproperty vector2d QtQuick::DragHandler::translation
    \deprecated [6.2] Use activeTranslation
*/

/*!
    \qmlproperty vector2d QtQuick::DragHandler::persistentTranslation

    The translation to be applied to the \l target if it is not \c null.
    Otherwise, bindings can be used to do arbitrary things with this value.
    While the drag gesture is being performed, \l activeTranslation is
    continuously added to it; after the gesture ends, it stays the same.
*/

/*!
    \readonly
    \qmlproperty vector2d QtQuick::DragHandler::activeTranslation

    The translation while the drag gesture is being performed.
    It is \c {0, 0} when the gesture begins, and increases as the event
    point(s) are dragged downward and to the right. After the gesture ends, it
    stays the same; and when the next drag gesture begins, it is reset to
    \c {0, 0} again.
*/

/*!
    \qmlproperty flags QtQuick::DragHandler::acceptedButtons

    The mouse buttons that can activate this DragHandler.

    By default, this property is set to
    \l {QtQuick::MouseEvent::button} {Qt.LeftButton}.
    It can be set to an OR combination of mouse buttons, and will ignore events
    from other buttons.

    For example, if a component (such as TextEdit) already handles
    left-button drags in its own way, it can be augmented with a
    DragHandler that does something different when dragged via the
    right button:

    \snippet pointerHandlers/dragHandlerAcceptedButtons.qml 0
*/

/*!
    \qmlproperty flags DragHandler::acceptedDevices

    The types of pointing devices that can activate this DragHandler.

    By default, this property is set to
    \l{QInputDevice::DeviceType}{PointerDevice.AllDevices}.
    If you set it to an OR combination of device types, it will ignore events
    from non-matching devices.

    \note Not all platforms are yet able to distinguish mouse and touchpad; and
    on those that do, you often want to make mouse and touchpad behavior the same.
*/

/*!
    \qmlproperty flags DragHandler::acceptedModifiers

    If this property is set, it will require the given keyboard modifiers to
    be pressed in order to react to pointer events, and otherwise ignore them.

    For example, two DragHandlers can perform two different drag-and-drop
    operations, depending on whether the \c Control modifier is pressed:

    \snippet pointerHandlers/draggableGridView.qml entire

    If this property is set to \c Qt.KeyboardModifierMask (the default value),
    then the DragHandler ignores the modifier keys.

    If you set \c acceptedModifiers to an OR combination of modifier keys,
    it means \e all of those modifiers must be pressed to activate the handler.

    The available modifiers are as follows:

    \value NoModifier       No modifier key is allowed.
    \value ShiftModifier    A Shift key on the keyboard must be pressed.
    \value ControlModifier  A Ctrl key on the keyboard must be pressed.
    \value AltModifier      An Alt key on the keyboard must be pressed.
    \value MetaModifier     A Meta key on the keyboard must be pressed.
    \value KeypadModifier   A keypad button must be pressed.
    \value GroupSwitchModifier X11 only (unless activated on Windows by a command line argument).
                            A Mode_switch key on the keyboard must be pressed.
    \value KeyboardModifierMask The handler does not care which modifiers are pressed.

    \sa Qt::KeyboardModifier
*/

/*!
    \qmlproperty flags DragHandler::acceptedPointerTypes

    The types of pointing instruments (finger, stylus, eraser, etc.)
    that can activate this DragHandler.

    By default, this property is set to
    \l {QPointingDevice::PointerType} {PointerDevice.AllPointerTypes}.
    If you set it to an OR combination of device types, it will ignore events
    from non-matching \l {PointerDevice}{devices}.
*/

/*!
    \qmlproperty real DragHandler::margin

    The margin beyond the bounds of the \l {PointerHandler::parent}{parent}
    item within which an \l eventPoint can activate this handler. For example,
    you can make it easier to drag small items by allowing the user to drag
    from a position nearby:

    \snippet pointerHandlers/dragHandlerMargin.qml draggable
*/

QT_END_NAMESPACE

#include "moc_qquickdraghandler_p.cpp"
