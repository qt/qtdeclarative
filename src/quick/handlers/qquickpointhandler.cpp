// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpointhandler_p.h"
#include <private/qquickdeliveryagent_p_p.h>
#include <private/qquickwindow_p.h>
#include <QDebug>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PointHandler
    \nativetype QQuickPointHandler
    \inherits SinglePointHandler
    \inqmlmodule QtQuick
    \ingroup qtquick-input-handlers
    \brief Handler for reacting to a single touchpoint.

    PointHandler can be used to show feedback about a touchpoint or the mouse
    position, or to otherwise react to pointer events.

    When a press event occurs, each instance of PointHandler chooses a
    single point which is not yet "taken" at that moment: if the press
    occurs within the bounds of the \l {PointerHandler::parent}, and
    no sibling PointHandler within the same \l {PointerHandler::parent}
    has yet acquired a passive grab on that point, and if the other
    constraints such as \l {PointerDeviceHandler::acceptedButtons}{acceptedButtons}, \l {PointerDeviceHandler::acceptedDevices}{acceptedDevices} etc.
    are satisfied, it's
    eligible, and the PointHandler then acquires a passive grab. In
    this way, the \l {PointerHandler::parent} acts like an exclusive
    group: there can be multiple instances of PointHandler, and the
    set of pressed touchpoints will be distributed among them. Each
    PointHandler which has chosen a point to track has its \l active
    property \c true. It then continues to track its chosen point
    until release: the properties of the \l point will be kept
    up-to-date. Any Item can bind to these properties, and thereby
    follow the point's movements.

    By being only a passive grabber, it has the ability to keep independent
    oversight of all movements. The passive grab cannot be stolen or overridden
    even when other gestures are detected and exclusive grabs occur.

    If your goal is orthogonal surveillance of eventpoints, an older
    alternative was QObject::installEventFilter(), but that has never been a
    built-in QtQuick feature: it requires some C++ code, such as a QQuickItem
    subclass. PointHandler is more efficient than that, because only pointer
    events will be delivered to it, during the course of normal event delivery
    in QQuickWindow; whereas an event filter needs to filter all QEvents of all
    types, and thus sets itself up as a potential event delivery bottleneck.

    One possible use case is to add this handler to a transparent Item which is
    on top of the rest of the scene (by having a high \l{Item::z} {z} value),
    so that when a point is freshly pressed, it will be delivered to that Item
    and its handlers first, providing the opportunity to take the passive grab
    as early as possible. Such an item (like a pane of glass over the whole UI)
    can be a convenient parent for other Items which visualize the kind of reactive
    feedback which must always be on top; and likewise it can be the parent for
    popups, popovers, dialogs and so on. If it will be used in that way, it can
    be helpful for your main.cpp to use QQmlContext::setContextProperty() to
    make the "glass pane" accessible by ID to the entire UI, so that other
    Items and PointHandlers can be reparented to it.

    \snippet pointerHandlers/pointHandler.qml 0

    Like all input handlers, a PointHandler has a \l target property, which
    may be used as a convenient place to put a point-tracking Item; but
    PointHandler will not automatically manipulate the \c target item in any way.
    You need to use bindings to make it react to the \l point.

    \note On macOS, PointHandler does not react to multiple fingers on the
    trackpad by default, although it does react to a pressed point (mouse position).
    That is because macOS can provide either native gesture recognition, or raw
    touchpoints, but not both. We prefer to use the native gesture event in
    PinchHandler, so we do not want to disable it by enabling touch. However
    MultiPointTouchArea does enable touch, thus disabling native gesture
    recognition within the entire window; so it's an alternative if you only
    want to react to all the touchpoints but do not require the smooth
    native-gesture experience.

    \sa MultiPointTouchArea, HoverHandler, {Qt Quick Examples - Pointer Handlers}
*/

QQuickPointHandler::QQuickPointHandler(QQuickItem *parent)
    : QQuickSinglePointHandler(parent)
{
    setIgnoreAdditionalPoints();
}

bool QQuickPointHandler::wantsEventPoint(const QPointerEvent *event, const QEventPoint &point)
{
    // On press, we want it unless a sibling of the same type also does.
    // If a synth-mouse press occurs, and we've already been interested in the original point, stay interested.
    const bool trackedPointId = QQuickSinglePointHandler::point().id() == point.id() &&
            QQuickSinglePointHandler::point().device() == point.device();
    if ((point.state() == QEventPoint::Pressed && QQuickSinglePointHandler::wantsEventPoint(event, point))
            || (QQuickDeliveryAgentPrivate::isSynthMouse(event) && trackedPointId)) {
        for (const QObject *grabber : event->passiveGrabbers(point)) {
            if (grabber && grabber != this && grabber->parent() == parent() &&
                    grabber->metaObject()->className() == metaObject()->className())
                return false;
        }
        return true;
    }
    // If we've already been interested in a point, stay interested, even if it has strayed outside bounds.
    return (point.state() != QEventPoint::Pressed && trackedPointId);
}

void QQuickPointHandler::handleEventPoint(QPointerEvent *event, QEventPoint &point)
{
    switch (point.state()) {
    case QEventPoint::Pressed:
        if (acceptedButtons() == Qt::NoButton || !event->isSinglePointEvent() ||
                (static_cast<const QSinglePointEvent *>(event)->buttons() & acceptedButtons()) != Qt::NoButton) {
            setPassiveGrab(event, point);
            setActive(true);
        }
        break;
    case QEventPoint::Released:
        if (acceptedButtons() == Qt::NoButton || !event->isSinglePointEvent() ||
                (static_cast<const QSinglePointEvent *>(event)->buttons() & acceptedButtons()) == Qt::NoButton)
            setActive(false);
        break;
    default:
        break;
    }
    point.setAccepted(false); // Just lurking... don't interfere with propagation
    emit translationChanged();
    if (!QQuickDeliveryAgentPrivate::isSynthMouse(event))
        QQuickSinglePointHandler::handleEventPoint(event, point);
}

QVector2D QQuickPointHandler::translation() const
{
    return QVector2D(point().position() - point().pressPosition());
}

/*!
    \qmlproperty flags PointHandler::acceptedButtons

    The mouse buttons that can activate this PointHandler.

    By default, this property is set to \l {QtQuick::MouseEvent::button} {Qt.LeftButton}.
    It can be set to an OR combination of mouse buttons, and will ignore events
    in which other buttons are pressed or held. If it is set to \c Qt.NoButton,
    it means it does not care about buttons at all, and ignores synthetic
    mouse events that come from any device for which it's already handling
    an authentic \l eventPoint.

    \snippet pointerHandlers/pointHandlerAcceptedButtons.qml 0

    \note On a touchscreen, there are no buttons, so this property does not
    prevent PointHandler from reacting to touchpoints.

    \note By default, when this property holds \c Qt.LeftButton, if a
    non-mouse \l {PointerDevice} (such as a touchscreen or graphics tablet
    stylus) is allowed to generate synthetic mouse events, those usually
    indicate that the left mouse button is pressed, and those events can
    temporarily de-activate the PointHandler which was already reacting to an
    authentic \l eventPoint from the device. It's useful to declare
    \qml
    acceptedButtons: \c Qt.NoButton
    \endqml
    to avoid this issue. See also
    \l Qt::AA_SynthesizeMouseForUnhandledTouchEvents and
    \l Qt::AA_SynthesizeMouseForUnhandledTabletEvents.
*/

/*!
    \qmlproperty flags PointHandler::acceptedDevices

    The types of pointing devices that can activate this PointHandler.

    By default, this property is set to
    \l{QInputDevice::DeviceType}{PointerDevice.AllDevices}.
    If you set it to an OR combination of device types, it will ignore events
    from non-matching \l {PointerDevice}{devices}:

    \snippet pointerHandlers/pointHandler.qml 1
*/

/*!
    \qmlproperty flags PointHandler::acceptedPointerTypes

    The types of pointing instruments (finger, stylus, eraser, etc.)
    that can activate this PointHandler.

    By default, this property is set to
    \l {QPointingDevice::PointerType} {PointerDevice.AllPointerTypes}.
    If you set it to an OR combination of device types, it will ignore events
    from non-matching \l {PointerDevice}{devices}:

    \snippet pointerHandlers/pointHandlerCanvasDrawing.qml 0

    The \l {Qt Quick Examples - Pointer Handlers} includes a more complex example for
    drawing on a Canvas with a graphics tablet.
*/

/*!
    \qmlproperty flags PointHandler::acceptedModifiers

    If this property is set, PointHandler requires the given keyboard modifiers
    to be pressed in order to react to \l {PointerEvent}{PointerEvents}, and
    otherwise ignores them.

    If this property is set to \c Qt.KeyboardModifierMask (the default value),
    then PointHandler ignores the modifier keys.

    For example, an \l [QML] Item could have two handlers, one of which is
    enabled only if the required keyboard modifier is pressed:

    \snippet pointerHandlers/pointHandlerAcceptedModifiers.qml 0

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
    \readonly
    \qmlproperty bool PointHandler::active

    This holds \c true whenever the constraints are satisfied and this
    PointHandler is reacting. This means that it is keeping its properties
    up-to-date according to the movements of the \l {eventPoint}{eventPoints}
    that satisfy the constraints.
*/

/*!
    \internal
    \qmlproperty flags PointHandler::dragThreshold

    This property is not used in PointHandler.
*/

/*!
    \qmlproperty real PointHandler::margin

    The margin beyond the bounds of the \l {PointerHandler::parent}{parent}
    item within which an \l eventPoint can activate this handler.

    The default value is \c 0.

    \snippet pointerHandlers/pointHandlerMargin.qml 0
*/

/*!
    \qmlproperty real PointHandler::target

    A property that can conveniently hold an Item to be manipulated or to show
    feedback. Unlike other \l {Qt Quick Input Handlers}{Pointer Handlers},
    PointHandler does not do anything with the \c target on its own: you
    usually need to create reactive bindings to properties such as
    \l SinglePointHandler::point and \l PointHandler::active. If you declare
    an Item instance here, you need to explicitly set its \l {Item::}{parent},
    because PointHandler is not an Item.

    By default, it is the same as the \l {PointerHandler::}{parent}, the Item
    within which the handler is declared.
*/

QT_END_NAMESPACE

#include "moc_qquickpointhandler_p.cpp"
