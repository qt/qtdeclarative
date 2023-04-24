/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qquickhoverhandler_p.h"
#include <private/qquicksinglepointhandler_p_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcHoverHandler, "qt.quick.handler.hover")

/*!
    \qmltype HoverHandler
    \instantiates QQuickHoverHandler
    \inherits SinglePointHandler
    \inqmlmodule QtQuick
    \ingroup qtquick-input-handlers
    \brief Handler for mouse and tablet hover.

    HoverHandler detects a hovering mouse or tablet stylus cursor.

    A binding to the \l hovered property is the easiest way to react when the
    cursor enters or leaves the \l {PointerHandler::parent}{parent} Item.
    The \l {SinglePointHandler::point}{point} property provides more detail,
    including the cursor position. The
    \l {PointerDeviceHandler::acceptedDevices}{acceptedDevices},
    \l {PointerDeviceHandler::acceptedPointerTypes}{acceptedPointerTypes},
    and \l {PointerDeviceHandler::acceptedModifiers}{acceptedModifiers}
    properties can be used to narrow the behavior to detect hovering of
    specific kinds of devices or while holding a modifier key.

    The \l cursorShape property allows changing the cursor whenever
    \l hovered changes to \c true.

    \sa MouseArea, PointHandler
*/

QQuickHoverHandler::QQuickHoverHandler(QQuickItem *parent)
    : QQuickSinglePointHandler(parent)
{
    // Tell QQuickPointerDeviceHandler::wantsPointerEvent() to ignore button state
    d_func()->acceptedButtons = Qt::NoButton;
}

QQuickHoverHandler::~QQuickHoverHandler()
{
    if (auto parent = parentItem())
        QQuickItemPrivate::get(parent)->setHasHoverInChild(false);
}

void QQuickHoverHandler::componentComplete()
{
    QQuickSinglePointHandler::componentComplete();
    if (auto par = parentItem()) {
        par->setAcceptHoverEvents(true);
        QQuickItemPrivate::get(par)->setHasHoverInChild(true);
    }
}

bool QQuickHoverHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    QQuickEventPoint *point = event->point(0);
    if (QQuickPointerDeviceHandler::wantsPointerEvent(event) && wantsEventPoint(point) && parentContains(point)) {
        // assume this is a mouse or tablet event, so there's only one point
        setPointId(point->pointId());
        return true;
    }

    // Some hover events come from QQuickWindow::tabletEvent(). In between,
    // some hover events come from QQWindowPrivate::flushFrameSynchronousEvents(),
    // but those look like mouse events. If a particular HoverHandler instance
    // is filtering for tablet events only (e.g. by setting
    // acceptedDevices:PointerDevice.Stylus), those events should not cause
    // the hovered property to transition to false prematurely.
    // If a QQuickPointerTabletEvent caused the hovered property to become true,
    // then only another QQuickPointerTabletEvent can make it become false.
    if (!(m_hoveredTablet && event->asPointerMouseEvent()))
        setHovered(false);

    return false;
}

void QQuickHoverHandler::handleEventPoint(QQuickEventPoint *point)
{
    bool hovered = true;
    if (point->state() == QQuickEventPoint::Released &&
            point->pointerEvent()->device()->pointerType() == QQuickPointerDevice::Finger)
        hovered = false;
    else if (point->pointerEvent()->asPointerTabletEvent())
        m_hoveredTablet = true;
    setHovered(hovered);
    setPassiveGrab(point);
}

void QQuickHoverHandler::onGrabChanged(QQuickPointerHandler *grabber, QQuickEventPoint::GrabTransition transition, QQuickEventPoint *point)
{
    QQuickSinglePointHandler::onGrabChanged(grabber, transition, point);
    if (grabber == this && transition == QQuickEventPoint::CancelGrabPassive)
        setHovered(false);
}

/*!
    \qmlproperty bool QtQuick::HoverHandler::hovered
    \readonly

    Holds true whenever any pointing device cursor (mouse or tablet) is within
    the bounds of the \c parent Item, extended by the
    \l {PointerHandler::margin}{margin}, if any.
*/
void QQuickHoverHandler::setHovered(bool hovered)
{
    if (m_hovered != hovered) {
        qCDebug(lcHoverHandler) << objectName() << "hovered" << m_hovered << "->" << hovered;
        m_hovered = hovered;
        if (!hovered)
            m_hoveredTablet = false;
        emit hoveredChanged();
    }
}

/*!
    \internal
    \qmlproperty flags QtQuick::HoverHandler::acceptedButtons

    This property is not used in HoverHandler.
*/

/*!
    \qmlproperty flags QtQuick::HoverHandler::acceptedDevices

    The types of pointing devices that can activate the pointer handler.

    By default, this property is set to
    \l{QInputDevice::DeviceType}{PointerDevice.AllDevices}.
    If you set it to an OR combination of device types, it will ignore pointer
    events from the non-matching devices.

    For example, an item could be made to respond to mouse hover in one way,
    and stylus hover in another way, with two handlers:

    \snippet pointerHandlers/hoverMouseOrStylus.qml 0

    The available device types are as follows:

    \value PointerDevice.Mouse          A mouse.
    \value PointerDevice.TouchScreen    A touchscreen.
    \value PointerDevice.TouchPad       A touchpad or trackpad.
    \value PointerDevice.Stylus         A stylus on a graphics tablet.
    \value PointerDevice.Airbrush       An airbrush on a graphics tablet.
    \value PointerDevice.Puck           A digitizer with crosshairs, on a graphics tablet.
    \value PointerDevice.AllDevices     Any type of pointing device.

    \sa QInputDevice::DeviceType
*/

/*!
    \qmlproperty flags QtQuick::HoverHandler::acceptedPointerTypes

    The types of pointing instruments (generic, stylus, eraser, and so on)
    that can activate the pointer handler.

    By default, this property is set to
    \l {QPointingDevice::PointerType} {PointerDevice.AllPointerTypes}.
    If you set it to an OR combination of device types, it will ignore events
    from non-matching events.

    For example, you could provide feedback by changing the cursor depending on
    whether a stylus or eraser is hovering over a graphics tablet:

    \snippet pointerHandlers/hoverStylusOrEraser.qml 0

    The available pointer types are as follows:

    \value PointerDevice.Generic          A mouse or a device that emulates a mouse.
    \value PointerDevice.Finger           A finger on a touchscreen (hover detection is unlikely).
    \value PointerDevice.Pen              A stylus on a graphics tablet.
    \value PointerDevice.Eraser           An eraser on a graphics tablet.
    \value PointerDevice.Cursor           A digitizer with crosshairs, on a graphics tablet.
    \value PointerDevice.AllPointerTypes  Any type of pointing device.

    \sa QPointingDevice::PointerType
*/

/*!
    \qmlproperty flags QtQuick::HoverHandler::acceptedModifiers

    If this property is set, a hover event is handled only if the given keyboard
    modifiers are pressed. The event is ignored without the modifiers.

    This property is set to \c Qt.KeyboardModifierMask by default, resulting
    in handling hover events regardless of any modifier keys.

    For example, an \l[QML]{Item} could have two handlers of the same type, one
    of which is enabled only if the required keyboard modifiers are pressed:

    \snippet pointerHandlers/hoverModifiers.qml 0

    The available modifiers are as follows:

    \value Qt.NoModifier        No modifier key is allowed.
    \value Qt.ShiftModifier     A Shift key on the keyboard must be pressed.
    \value Qt.ControlModifier   A Ctrl key on the keyboard must be pressed.
    \value Qt.AltModifier       An Alt key on the keyboard must be pressed.
    \value Qt.MetaModifier      A Meta key on the keyboard must be pressed.
    \value Qt.KeypadModifier    A keypad button must be pressed.
    \value Qt.GroupSwitchModifier A Mode_switch key on the keyboard must be pressed.
                                X11 only (unless activated on Windows by a command line argument).
    \value Qt.KeyboardModifierMask The handler ignores modifier keys.

    \sa Qt::KeyboardModifier
*/

/*!
    \since 5.15
    \qmlproperty Qt::CursorShape QtQuick::HoverHandler::cursorShape
    This property holds the cursor shape that will appear whenever
    \l hovered is \c true and no other handler is overriding it.

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

    The default value of this property is not set, which allows any active
    handler on the same \l parentItem to determine the cursor shape.
    This property can be reset to the initial condition by setting it to
    \c undefined.

    If any handler with defined \c cursorShape is
    \l {PointerHandler::active}{active}, that cursor will appear.
    Else if the HoverHandler has a defined \c cursorShape, that cursor will appear.
    Otherwise, the \l {QQuickItem::cursor()}{cursor} of \l parentItem will appear.

    \note When this property has not been set, or has been set to \c undefined,
    if you read the value it will return \c Qt.ArrowCursor.

    \sa Qt::CursorShape, QQuickItem::cursor()
*/

QT_END_NAMESPACE

#include "moc_qquickhoverhandler_p.cpp"
