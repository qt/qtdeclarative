/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qquicksinglepointhandler_p.h"

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(DBG_TOUCH_TARGET)

/*!
    \qmltype SinglePointHandler
    \qmlabstract
    \preliminary
    \instantiates QQuickSinglePointHandler
    \inherits PointerDeviceHandler
    \inqmlmodule Qt.labs.handlers
    \ingroup qtquick-handlers
    \brief Abstract handler for single-point Pointer Events.

    An intermediate class (not registered as a QML type)
    for the most common handlers: those which expect only a single point.
    wantsPointerEvent() will choose the first point which is inside the
    \l target item, and return true as long as the event contains that point.
    Override handleEventPoint() to implement a single-point handler.
*/

QQuickSinglePointHandler::QQuickSinglePointHandler(QObject *parent)
  : QQuickPointerDeviceHandler(parent)
  , m_acceptedButtons(Qt::LeftButton)
  , m_ignoreAdditionalPoints(false)
{
}

bool QQuickSinglePointHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    if (!QQuickPointerDeviceHandler::wantsPointerEvent(event))
        return false;
    if (event->device()->pointerType() != QQuickPointerDevice::Finger &&
            (event->buttons() & m_acceptedButtons) == 0 && (event->button() & m_acceptedButtons) == 0)
        return false;

    if (m_pointInfo.m_id) {
        // We already know which one we want, so check whether it's there.
        // It's expected to be an update or a release.
        // If we no longer want it, cancel the grab.
        int candidatePointCount = 0;
        QQuickEventPoint *point = nullptr;
        int c = event->pointCount();
        for (int i = 0; i < c; ++i) {
            QQuickEventPoint *p = event->point(i);
            if (wantsEventPoint(p)) {
                ++candidatePointCount;
                if (p->pointId() == m_pointInfo.m_id)
                    point = p;
            }
        }
        if (point) {
            if (candidatePointCount == 1 || (candidatePointCount > 1 && m_ignoreAdditionalPoints)) {
                point->setAccepted();
                return true;
            } else {
                point->cancelAllGrabs(this);
            }
        } else {
            qCWarning(DBG_TOUCH_TARGET) << this << "pointId" << hex << m_pointInfo.m_id
                << "is missing from current event, but was neither canceled nor released";
            return false;
        }
    } else {
        // We have not yet chosen a point; choose the first one for which wantsEventPoint() returns true.
        int candidatePointCount = 0;
        int c = event->pointCount();
        QQuickEventPoint *chosen = nullptr;
        for (int i = 0; i < c; ++i) {
            QQuickEventPoint *p = event->point(i);
            if (!p->exclusiveGrabber() && wantsEventPoint(p)) {
                if (!chosen)
                    chosen = p;
                ++candidatePointCount;
            }
        }
        if (chosen && candidatePointCount == 1) {
            m_pointInfo.m_id = chosen->pointId();
            chosen->setAccepted();
        }
    }
    return m_pointInfo.m_id;
}

void QQuickSinglePointHandler::handlePointerEventImpl(QQuickPointerEvent *event)
{
    QQuickPointerDeviceHandler::handlePointerEventImpl(event);
    QQuickEventPoint *currentPoint = event->pointById(m_pointInfo.m_id);
    Q_ASSERT(currentPoint);
    if (!m_pointInfo.m_id || !currentPoint->isAccepted()) {
        reset();
    } else {
        if (event->asPointerTouchEvent()) {
            QQuickEventTouchPoint *tp = static_cast<QQuickEventTouchPoint *>(currentPoint);
            m_pointInfo.m_uniqueId = tp->uniqueId();
            m_pointInfo.m_rotation = tp->rotation();
            m_pointInfo.m_pressure = tp->pressure();
            m_pointInfo.m_ellipseDiameters = tp->ellipseDiameters();
        } else if (event->asPointerTabletEvent()) {
            // TODO
        } else {
            m_pointInfo.m_uniqueId = event->device()->uniqueId();
            m_pointInfo.m_rotation = 0;
            m_pointInfo.m_pressure = event->buttons() ? 1 : 0;
            m_pointInfo.m_ellipseDiameters = QSizeF();
        }
        m_pointInfo.m_position = currentPoint->position();
        m_pointInfo.m_scenePosition = currentPoint->scenePosition();
        if (currentPoint->state() == QQuickEventPoint::Updated)
            m_pointInfo.m_velocity = currentPoint->velocity();
        handleEventPoint(currentPoint);
        switch (currentPoint->state()) {
        case QQuickEventPoint::Pressed:
            m_pointInfo.m_pressPosition = currentPoint->position();
            m_pointInfo.m_scenePressPosition = currentPoint->scenePosition();
            m_pointInfo.m_pressedButtons = event->buttons();
            break;
        case QQuickEventPoint::Released:
            setExclusiveGrab(currentPoint, false);
            reset();
            break;
        default:
            m_pointInfo.m_pressedButtons = event->buttons();
            break;
        }
        emit pointChanged();
    }
}

bool QQuickSinglePointHandler::wantsEventPoint(QQuickEventPoint *point)
{
    return parentContains(point);
}

void QQuickSinglePointHandler::onGrabChanged(QQuickPointerHandler *grabber, QQuickEventPoint::GrabState stateChange, QQuickEventPoint *point)
{
    if (grabber != this)
        return;
    switch (stateChange) {
    case QQuickEventPoint::GrabExclusive:
        m_pointInfo.m_sceneGrabPosition = point->sceneGrabPosition();
        setActive(true);
        QQuickPointerHandler::onGrabChanged(grabber, stateChange, point);
        break;
    case QQuickEventPoint::GrabPassive:
        m_pointInfo.m_sceneGrabPosition = point->sceneGrabPosition();
        QQuickPointerHandler::onGrabChanged(grabber, stateChange, point);
        break;
    case QQuickEventPoint::OverrideGrabPassive:
        return; // don't emit
    case QQuickEventPoint::UngrabPassive:
    case QQuickEventPoint::UngrabExclusive:
    case QQuickEventPoint::CancelGrabPassive:
    case QQuickEventPoint::CancelGrabExclusive:
        // the grab is lost or relinquished, so the point is no longer relevant
        QQuickPointerHandler::onGrabChanged(grabber, stateChange, point);
        reset();
        break;
    }
    emit singlePointGrabChanged();
}

void QQuickSinglePointHandler::setIgnoreAdditionalPoints(bool v)
{
    m_ignoreAdditionalPoints = v;
}

void QQuickSinglePointHandler::moveTarget(QPointF pos, QQuickEventPoint *point)
{
    target()->setPosition(pos);
    m_pointInfo.m_scenePosition = point->scenePosition();
    m_pointInfo.m_position = target()->mapFromScene(m_pointInfo.m_scenePosition);
}

/*!
    \qmlproperty int QtQuick::SinglePointHandler::acceptedButtons

    The mouse buttons which can activate this Pointer Handler.

    By default, this property is set to \l {QtQuick::MouseEvent::button} {Qt.LeftButton}.
    It can be set to an OR combination of mouse buttons, and will ignore events
    from other buttons.

    For example, a control could be made to respond to left and right clicks
    in different ways, with two handlers:

    \qml
    Item {
        TapHandler {
            onTapped: console.log("left clicked")
        }
        TapHandler {
            acceptedButtons: Qt.RightButton
            onTapped: console.log("right clicked")
        }
    }
    \endqml

    \note Tapping on a touchscreen or tapping the stylus on a graphics tablet
    emulates clicking the left mouse button. This behavior can be altered via
    \l {PointerDeviceHandler::acceptedDevices}{acceptedDevices} or
    \l {PointerDeviceHandler::acceptedPointerTypes}{acceptedPointerTypes}.
*/
void QQuickSinglePointHandler::setAcceptedButtons(Qt::MouseButtons buttons)
{
    if (m_acceptedButtons == buttons)
        return;

    m_acceptedButtons = buttons;
    emit acceptedButtonsChanged();
}

void QQuickSinglePointHandler::reset()
{
    setActive(false);
    m_pointInfo.reset();
}

/*!
    \readonly
    \qmlproperty HandlerPoint QtQuick::SinglePointHandler::point

    The event point currently being handled. When no point is currently being
    handled, this object is reset to default values (all coordinates are 0).
*/

/*!
    \qmltype HandlerPoint
    \instantiates QQuickHandlerPoint
    \inqmlmodule Qt.labs.handlers
    \ingroup qtquick-handlers
    \brief An event point.

    A QML representation of a QQuickEventPoint.

    It's possible to make bindings to properties of a \l SinglePointHandler's
    current point. For example:

    \snippet pointerHandlers/dragHandlerNullTarget.qml 0

    The point is kept up-to-date when the DragHandler is actively responding to
    an EventPoint; but when the point is released, or the current point is
    being handled by a different handler, \c position.x and \c position.y are 0.

    \note This is practically identical to QtQuick::EventPoint; however an
    EventPoint is a long-lived QObject which is invalidated between gestures
    and reused for subsequent event deliveries. Continuous bindings to its
    properties are not possible, and an individual handler cannot rely on it
    outside the period when that point is part of an active gesture which that
    handler is handling. HandlerPoint is a Q_GADGET that the handler owns.
    This allows you to make lifetime bindings to its properties.

    \sa SinglePointHandler::point
*/

QQuickHandlerPoint::QQuickHandlerPoint()
    : m_id(0)
    , m_rotation(0)
    , m_pressure(0)
{}

void QQuickHandlerPoint::reset()
{
    m_id = 0;
    m_uniqueId = QPointingDeviceUniqueId();
    m_position = QPointF();
    m_scenePosition = QPointF();
    m_pressPosition = QPointF();
    m_scenePressPosition = QPointF();
    m_sceneGrabPosition = QPointF();
    m_velocity = QVector2D();
    m_rotation = 0;
    m_pressure = 0;
    m_ellipseDiameters = QSizeF();
    m_pressedButtons = Qt::NoButton;
}

/*!
    \readonly
    \qmlproperty int QtQuick::HandlerPoint::id
    \brief The ID number of the point

    During a touch gesture, from the time that the first finger is pressed
    until the last finger is released, each touchpoint will have a unique ID
    number. Likewise, if input from multiple devices occurs (for example
    simultaneous mouse and touch presses), all the current event points from
    all the devices will have unique IDs.

    \note Do not assume that id numbers start at zero or that they are
    sequential. Such an assumption is often false due to the way the underlying
    drivers work.

    \sa QTouchEvent::TouchPoint::id
*/

/*!
    \readonly
    \qmlproperty PointingDeviceUniqueId QtQuick::HandlerPoint::uniqueId
    \brief The unique ID of the point, if any

    This is normally empty, because touchscreens cannot uniquely identify fingers.

    On some types of touchscreens, especially those using TUIO drivers,
    it's possible to use recognizable physical tokens (fiducial objects)
    in addition to fingers.  So if this point is a touch point, and
    uniqueId is set, it is the identifier for such an object.

    On a graphics tablet, each type of stylus or other tool often has a unique
    ID or serial number, which can be useful to respond in different ways to
    different tools.

    Interpreting the contents of this ID requires knowledge of the hardware and
    drivers in use.

    \sa QTabletEvent::uniqueId, QtQuick::TouchPoint::uniqueId, QtQuick::EventTouchPoint::uniqueId
*/

/*!
    \readonly
    \qmlproperty QPointF QtQuick::HandlerPoint::position
    \brief The position within the \c parent Item

    This is the position of the event point relative to the bounds of
    the \l {PointerHandler::parent} {parent}.
*/

/*!
    \readonly
    \qmlproperty QPointF QtQuick::HandlerPoint::scenePosition
    \brief The position within the scene

    This is the position of the event point relative to the bounds of the Qt
    Quick scene (typically the whole window).
*/

/*!
    \readonly
    \qmlproperty QPointF QtQuick::HandlerPoint::pressPosition
    \brief The pressed position within the \c parent Item

    This is the position at which this point was pressed, relative to the
    bounds of the \l {PointerHandler::parent} {parent}.
*/

/*!
    \readonly
    \qmlproperty QPointF QtQuick::HandlerPoint::scenePressPosition
    \brief The pressed position within the scene

    This is the position at which this point was pressed, in the coordinate
    system of the \l {Qt Quick Scene Graph}{scene graph}.
*/

/*!
    \readonly
    \qmlproperty QPointF QtQuick::HandlerPoint::sceneGrabPosition
    \brief The grabbed position within the scene

    If this point has been grabbed by a Pointer Handler or an Item, it means
    that object has taken sole responsibility for handling the movement and the
    release if this point. In that case, this is the position at which the grab
    occurred, in the coordinate system of the \l {Qt Quick Scene Graph}{scene graph}.
*/

/*!
    \readonly
    \qmlproperty enum QtQuick::HandlerPoint::pressedButtons
    \brief Which mouse or stylus buttons are currently pressed

    \sa MouseArea::pressedButtons
*/

/*!
    \readonly
    \qmlproperty QVector2D QtQuick::HandlerPoint::velocity
    \brief A vector representing the average speed and direction of movement

    This is a velocity vector pointing in the direction of movement, in logical
    pixels per second. It has x and y components, at least one of which will be
    nonzero when this point is in motion. It holds the average recent velocity:
    how fast and in which direction the event point has been moving recently.

    \sa QtQuick::EventPoint::velocity, QtQuick::TouchPoint::velocity, QTouchEvent::TouchPoint::velocity
*/

/*!
    \readonly
    \qmlproperty qreal QtQuick::HandlerPoint::rotation

    This property holds the rotation angle of the stylus on a graphics tablet
    or the contact patch of a touchpoint on a touchscreen.

    It is valid only with certain tablet stylus devices and touchscreens that
    can measure the rotation angle. Otherwise, it will be zero.
*/

/*!
    \readonly
    \qmlproperty qreal QtQuick::HandlerPoint::pressure

    This property tells how hard the user is pressing the stylus on a graphics
    tablet or the finger against a touchscreen, in the range from \c 0 (no
    measurable pressure) to \c 1.0 (maximum pressure which the device can
    measure).

    It is valid only with certain tablets and touchscreens that can measure
    pressure. Otherwise, it will be zero.
*/

/*!
    \readonly
    \qmlproperty size QtQuick::HandlerPoint::ellipseDiameters

    This property holds the diameters of the contact patch, if the event
    comes from a touchpoint and the device provides this information.

    A touchpoint is modeled as an elliptical area where the finger is pressed
    against the touchscreen. (In fact, it could also be modeled as a bitmap;
    but in that case we expect an elliptical bounding estimate to be fitted to
    the contact patch before the event is sent.) The harder the user presses,
    the larger the contact patch; so, these diameters provide an alternate way
    of detecting pressure, in case the device does not include a separate
    pressure sensor. The ellipse is centered on \l scenePosition (\l position
    in the PointerHandler's Item's local coordinates). The \l rotation property
    provides the rotation of the ellipse, if known. It is expected that if the
    \l rotation is zero, the \l {QSize::height}{height} is the larger dimension
    (the major axis), because of the usual hand position, reaching upward or
    outward across the surface.

    If the contact patch is unknown, or the device is not a touchscreen,
    these values will be zero.

    \sa QtQuick::EventTouchPoint::ellipseDiameters, QtQuick::TouchPoint::ellipseDiameters, QTouchEvent::TouchPoint::ellipseDiameters
*/

QT_END_NAMESPACE
