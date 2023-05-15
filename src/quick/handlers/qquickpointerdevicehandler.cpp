// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpointerdevicehandler_p_p.h"
#include <private/qquickitem_p.h>
#include <QMouseEvent>
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PointerDeviceHandler
    \qmlabstract
    \since 5.10
    \preliminary
    \instantiates QQuickPointerDeviceHandler
    \inherits PointerHandler
    \inqmlmodule QtQuick
    \brief Abstract handler for pointer events with device-specific constraints.

    An intermediate class (not registered as a QML type) for handlers which
    allow filtering based on device type, pointer type, or keyboard modifiers.
*/
QQuickPointerDeviceHandler::QQuickPointerDeviceHandler(QQuickItem *parent)
    : QQuickPointerHandler(*(new QQuickPointerDeviceHandlerPrivate), parent)
{
}

QQuickPointerDeviceHandler::QQuickPointerDeviceHandler(QQuickPointerDeviceHandlerPrivate &dd, QQuickItem *parent)
    : QQuickPointerHandler(dd, parent)
{
}

QInputDevice::DeviceTypes QQuickPointerDeviceHandler::acceptedDevices() const
{
    Q_D(const QQuickPointerDeviceHandler);
    return d->acceptedDevices;
}

QPointingDevice::PointerTypes QQuickPointerDeviceHandler::acceptedPointerTypes() const
{
    Q_D(const QQuickPointerDeviceHandler);
    return d->acceptedPointerTypes;
}

/*!
    \qmlproperty flags QtQuick::PointerDeviceHandler::acceptedButtons

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
Qt::MouseButtons QQuickPointerDeviceHandler::acceptedButtons() const
{
    Q_D(const QQuickPointerDeviceHandler);
    return d->acceptedButtons;
}

void QQuickPointerDeviceHandler::setAcceptedButtons(Qt::MouseButtons buttons)
{
    Q_D(QQuickPointerDeviceHandler);
    if (d->acceptedButtons == buttons)
        return;

    d->acceptedButtons = buttons;
    emit acceptedButtonsChanged();
}

Qt::KeyboardModifiers QQuickPointerDeviceHandler::acceptedModifiers() const
{
    Q_D(const QQuickPointerDeviceHandler);
    return d->acceptedModifiers;
}

/*!
    \qmlproperty flags PointerDeviceHandler::acceptedDevices

    The types of pointing devices that can activate this Pointer Handler.

    By default, this property is set to
    \l{QInputDevice::DeviceType}{PointerDevice.AllDevices}.
    If you set it to an OR combination of device types, it will ignore events
    from non-matching devices.

    For example, a control could be made to respond to mouse and stylus clicks
    in one way, and touchscreen taps in another way, with two handlers:

    \qml
    Item {
       TapHandler {
           acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus
           onTapped: console.log("clicked")
       }
       TapHandler {
           acceptedDevices: PointerDevice.TouchScreen
           onTapped: console.log("tapped")
       }
    }
    \endqml
*/
void QQuickPointerDeviceHandler::setAcceptedDevices(QPointingDevice::DeviceTypes acceptedDevices)
{
    Q_D(QQuickPointerDeviceHandler);
    if (d->acceptedDevices == acceptedDevices)
        return;

    d->acceptedDevices = acceptedDevices;
    emit acceptedDevicesChanged();
}

/*!
    \qmlproperty flags PointerDeviceHandler::acceptedPointerTypes

    The types of pointing instruments (finger, stylus, eraser, etc.)
    that can activate this Pointer Handler.

    By default, this property is set to
    \l {QPointingDevice::PointerType} {PointerDevice.AllPointerTypes}.
    If you set it to an OR combination of device types, it will ignore events
    from non-matching \l {PointerDevice}{devices}.

    For example, a control could be made to respond to mouse, touch, and stylus clicks
    in some way, but delete itself if tapped with an eraser tool on a graphics tablet,
    with two handlers:

    \qml
    Rectangle {
       id: rect
       TapHandler {
           acceptedPointerTypes: PointerDevice.Generic | PointerDevice.Finger | PointerDevice.Pen
           onTapped: console.log("clicked")
       }
       TapHandler {
           acceptedPointerTypes: PointerDevice.Eraser
           onTapped: rect.destroy()
       }
    }
    \endqml
*/
void QQuickPointerDeviceHandler::setAcceptedPointerTypes(QPointingDevice::PointerTypes acceptedPointerTypes)
{
    Q_D(QQuickPointerDeviceHandler);
    if (d->acceptedPointerTypes == acceptedPointerTypes)
        return;

    d->acceptedPointerTypes = acceptedPointerTypes;
    emit acceptedPointerTypesChanged();
}

/*!
    \qmlproperty flags PointerDeviceHandler::acceptedModifiers

    If this property is set, it will require the given keyboard modifiers to
    be pressed in order to react to pointer events, and otherwise ignore them.

    If this property is set to \c Qt.KeyboardModifierMask (the default value),
    then the PointerHandler ignores the modifier keys.

    For example, an \l [QML] Item could have two handlers of the same type,
    one of which is enabled only if the required keyboard modifiers are
    pressed:

    \qml
    Item {
       TapHandler {
           acceptedModifiers: Qt.ControlModifier
           onTapped: console.log("control-tapped")
       }
       TapHandler {
           acceptedModifiers: Qt.NoModifier
           onTapped: console.log("tapped")
       }
    }
    \endqml

    If you set \c acceptedModifiers to an OR combination of modifier keys,
    it means \e all of those modifiers must be pressed to activate the handler:

    \qml
    Item {
       TapHandler {
           acceptedModifiers: Qt.ControlModifier | Qt.AltModifier | Qt.ShiftModifier
           onTapped: console.log("control-alt-shift-tapped")
       }
    }
    \endqml

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

    If you need even more complex behavior than can be achieved with
    combinations of multiple handlers with multiple modifier flags, you can
    check the modifiers in JavaScript code:

    \qml
    Item {
        TapHandler {
            onTapped:
                switch (point.modifiers) {
                case Qt.ControlModifier | Qt.AltModifier:
                    console.log("CTRL+ALT");
                    break;
                case Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier:
                    console.log("CTRL+META+ALT");
                    break;
                default:
                    console.log("other modifiers", point.modifiers);
                    break;
                }
        }
    }
    \endqml

    \sa Qt::KeyboardModifier
*/
void QQuickPointerDeviceHandler::setAcceptedModifiers(Qt::KeyboardModifiers acceptedModifiers)
{
    Q_D(QQuickPointerDeviceHandler);
    if (d->acceptedModifiers == acceptedModifiers)
        return;

    d->acceptedModifiers = acceptedModifiers;
    emit acceptedModifiersChanged();
}

bool QQuickPointerDeviceHandler::wantsPointerEvent(QPointerEvent *event)
{
    Q_D(QQuickPointerDeviceHandler);
    if (!QQuickPointerHandler::wantsPointerEvent(event))
        return false;
    qCDebug(lcPointerHandlerDispatch) << objectName()
        << "checking device type" << d->acceptedDevices
        << "pointer type" << d->acceptedPointerTypes
        << "modifiers" << d->acceptedModifiers;
    if (!d->acceptedDevices.testFlag(event->device()->type()))
        return false;
    if (!d->acceptedPointerTypes.testFlag(event->pointingDevice()->pointerType()))
        return false;
    if (d->acceptedModifiers != Qt::KeyboardModifierMask && event->modifiers() != d->acceptedModifiers)
        return false;
    // Some handlers (HoverHandler, PinchHandler) set acceptedButtons to Qt::NoButton to indicate that button state is irrelevant.
    if (event->pointingDevice()->type() != QPointingDevice::DeviceType::TouchScreen &&
            acceptedButtons() != Qt::NoButton && event->type() != QEvent::Wheel &&
            (static_cast<QSinglePointEvent *>(event)->buttons() & acceptedButtons()) == 0 &&
            (static_cast<QSinglePointEvent *>(event)->button() & acceptedButtons()) == 0)
        return false;
    return true;
}

QT_END_NAMESPACE

#include "moc_qquickpointerdevicehandler_p.cpp"
