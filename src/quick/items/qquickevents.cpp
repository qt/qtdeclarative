// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickevents_p_p.h"
#include <QtCore/qmap.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qinputdevice_p.h>
#include <QtGui/private/qevent_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickpointerhandler_p.h>
#include <QtQuick/private/qquickpointerhandler_p_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <private/qdebug_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcPointerEvents, "qt.quick.pointer.events")

/*!
    \qmltype KeyEvent
    \instantiates QQuickKeyEvent
    \inqmlmodule QtQuick
    \ingroup qtquick-input-events

    \brief Provides information about a key event.

    For example, the following changes the Item's state property when the Enter
    key is pressed:
    \qml
Item {
    focus: true
    Keys.onPressed: (event)=> { if (event.key == Qt.Key_Enter) state = 'ShowDetails'; }
}
    \endqml
*/

/*!
    \qmlproperty int QtQuick::KeyEvent::key

    This property holds the code of the key that was pressed or released.

    See \l {Qt::Key}{Qt.Key} for the list of keyboard codes. These codes are
    independent of the underlying window system. Note that this
    function does not distinguish between capital and non-capital
    letters; use the \l {KeyEvent::}{text} property for this purpose.

    A value of either 0 or \l {Qt::Key_unknown}{Qt.Key_Unknown} means that the event is not
    the result of a known key; for example, it may be the result of
    a compose sequence, a keyboard macro, or due to key event
    compression.
*/

/*!
    \qmlproperty string QtQuick::KeyEvent::text

    This property holds the Unicode text that the key generated.
    The text returned can be an empty string in cases where modifier keys,
    such as Shift, Control, Alt, and Meta, are being pressed or released.
    In such cases \c key will contain a valid value
*/

/*!
    \qmlproperty bool QtQuick::KeyEvent::isAutoRepeat

    This property holds whether this event comes from an auto-repeating key.
*/

/*!
    \qmlproperty quint32 QtQuick::KeyEvent::nativeScanCode

    This property contains the native scan code of the key that was pressed. It is
    passed through from QKeyEvent unchanged.

    \sa QKeyEvent::nativeScanCode()
*/

/*!
    \qmlproperty int QtQuick::KeyEvent::count

    This property holds the number of keys involved in this event. If \l KeyEvent::text
    is not empty, this is simply the length of the string.
*/

/*!
    \qmlproperty bool QtQuick::KeyEvent::accepted

    Setting \a accepted to true prevents the key event from being
    propagated to the item's parent.

    Generally, if the item acts on the key event then it should be accepted
    so that ancestor items do not also respond to the same event.
*/

/*!
    \qmlproperty int QtQuick::KeyEvent::modifiers

    This property holds the keyboard modifier flags that existed immediately
    before the event occurred.

    It contains a bitwise combination of numeric values (the same as in Qt::KeyboardModifier):

    \value Qt.NoModifier        No modifier key is pressed.
    \value Qt.ShiftModifier}    A Shift key on the keyboard is pressed.
    \value Qt.ControlModifier   A Ctrl key on the keyboard is pressed.
    \value Qt.AltModifier       An Alt key on the keyboard is pressed.
    \value Qt.MetaModifier      A Meta key on the keyboard is pressed.
    \value Qt.KeypadModifier    A keypad button is pressed.
    \value Qt.GroupSwitchModifier X11 only. A Mode_switch key on the keyboard is pressed.

    For example, to react to a Shift key + Enter key combination:
    \qml
    Item {
        focus: true
        Keys.onPressed: (event)=> {
            if ((event.key == Qt.Key_Enter) && (event.modifiers & Qt.ShiftModifier))
                doSomething();
        }
    }
    \endqml
*/

/*!
    \qmlmethod bool QtQuick::KeyEvent::matches(StandardKey matchKey)
    \since 5.2

    Returns \c true if the key event matches the given standard \a matchKey; otherwise returns \c false.

    \qml
    Item {
        focus: true
        Keys.onPressed: (event)=> {
            if (event.matches(StandardKey.Undo))
                myModel.undo();
            else if (event.matches(StandardKey.Redo))
                myModel.redo();
        }
    }
    \endqml

    \sa QKeySequence::StandardKey
*/
#if QT_CONFIG(shortcut)
bool QQuickKeyEvent::matches(QKeySequence::StandardKey matchKey) const
{
    // copying QKeyEvent::matches
    uint searchkey = (modifiers() | key()) & ~(Qt::KeypadModifier | Qt::GroupSwitchModifier);

    const QList<QKeySequence> bindings = QKeySequence::keyBindings(matchKey);
    return bindings.contains(QKeySequence(searchkey));
}
#endif


/*!
    \qmltype MouseEvent
    \instantiates QQuickMouseEvent
    \inqmlmodule QtQuick
    \ingroup qtquick-input-events

    \brief Provides information about a mouse event.

    The position of the mouse can be found via the \l {Item::x} {x} and \l {Item::y} {y} properties.
    The button that caused the event is available via the \l button property.

    \sa MouseArea
*/

/*!
    \internal
    \class QQuickMouseEvent
*/

/*!
    \qmlproperty real QtQuick::MouseEvent::x
    \qmlproperty real QtQuick::MouseEvent::y

    These properties hold the coordinates of the position supplied by the mouse event.
*/


/*!
    \qmlproperty bool QtQuick::MouseEvent::accepted

    Setting \a accepted to true prevents the mouse event from being
    propagated to items below this item.

    Generally, if the item acts on the mouse event then it should be accepted
    so that items lower in the stacking order do not also respond to the same event.
*/

/*!
    \qmlproperty enumeration QtQuick::MouseEvent::button

    This property holds the button that caused the event.  It can be one of:
    \list
    \li \l {Qt::LeftButton} {Qt.LeftButton}
    \li \l {Qt::RightButton} {Qt.RightButton}
    \li \l {Qt::MiddleButton} {Qt.MiddleButton}
    \endlist
*/

/*!
    \qmlproperty bool QtQuick::MouseEvent::wasHeld

    This property is true if the mouse button has been held pressed longer
    than the threshold (800ms).
*/

/*!
    \qmlproperty int QtQuick::MouseEvent::buttons

    This property holds the mouse buttons pressed when the event was generated.
    For mouse move events, this is all buttons that are pressed down. For mouse
    press and double click events this includes the button that caused the event.
    For mouse release events this excludes the button that caused the event.

    It contains a bitwise combination of:
    \list
    \li \l {Qt::LeftButton} {Qt.LeftButton}
    \li \l {Qt::RightButton} {Qt.RightButton}
    \li \l {Qt::MiddleButton} {Qt.MiddleButton}
    \endlist
*/

/*!
    \qmlproperty int QtQuick::MouseEvent::modifiers

    This property holds the keyboard modifier flags that existed immediately
    before the event occurred.

    It contains a bitwise combination of:
    \list
    \li \l {Qt::NoModifier} {Qt.NoModifier} - No modifier key is pressed.
    \li \l {Qt::ShiftModifier} {Qt.ShiftModifier} - A Shift key on the keyboard is pressed.
    \li \l {Qt::ControlModifier} {Qt.ControlModifier} - A Ctrl key on the keyboard is pressed.
    \li \l {Qt::AltModifier} {Qt.AltModifier} - An Alt key on the keyboard is pressed.
    \li \l {Qt::MetaModifier} {Qt.MetaModifier} - A Meta key on the keyboard is pressed.
    \li \l {Qt::KeypadModifier} {Qt.KeypadModifier} - A keypad button is pressed.
    \endlist

    For example, to react to a Shift key + Left mouse button click:
    \qml
    MouseArea {
        onClicked: (mouse)=> {
            if ((mouse.button == Qt.LeftButton) && (mouse.modifiers & Qt.ShiftModifier))
                doSomething();
        }
    }
    \endqml
*/

/*!
    \qmlproperty int QtQuick::MouseEvent::source
    \since 5.7
    \deprecated [6.2] Use \l {Qt Quick Input Handlers}{input handlers} with \l {PointerDeviceHandler::acceptedDevices}{acceptedDevices} set.

    This property holds the source of the mouse event.

    The mouse event source can be used to distinguish between genuine and
    artificial mouse events. When using other pointing devices such as
    touchscreens and graphics tablets, if the application does not make use of
    the actual touch or tablet events, mouse events may be synthesized by the
    operating system or by Qt itself.

    The value can be one of:

    \list
    \li \l{Qt::MouseEventNotSynthesized} {Qt.MouseEventNotSynthesized}
    - The most common value. On platforms where such information is
    available, this value indicates that the event represents a genuine
    mouse event from the system.

    \li \l{Qt::MouseEventSynthesizedBySystem} {Qt.MouseEventSynthesizedBySystem} - Indicates that the mouse event was
    synthesized from a touch or tablet event by the platform.

    \li \l{Qt::MouseEventSynthesizedByQt} {Qt.MouseEventSynthesizedByQt}
    - Indicates that the mouse event was synthesized from an unhandled
    touch or tablet event by Qt.

    \li \l{Qt::MouseEventSynthesizedByApplication} {Qt.MouseEventSynthesizedByApplication}
    - Indicates that the mouse event was synthesized by the application.
    This allows distinguishing application-generated mouse events from
    the ones that are coming from the system or are synthesized by Qt.
    \endlist

    For example, to react only to events which come from an actual mouse:
    \qml
    MouseArea {
        onPressed: (mouse)=> {
            if (mouse.source !== Qt.MouseEventNotSynthesized)
                mouse.accepted = false
        }

        onClicked: doSomething()
    }
    \endqml

    If the handler for the press event rejects the event, it will be propagated
    further, and then another Item underneath can handle synthesized events
    from touchscreens. For example, if a Flickable is used underneath (and the
    MouseArea is not a child of the Flickable), it can be useful for the
    MouseArea to handle genuine mouse events in one way, while allowing touch
    events to fall through to the Flickable underneath, so that the ability to
    flick on a touchscreen is retained. In that case the ability to drag the
    Flickable via mouse would be lost, but it does not prevent Flickable from
    receiving mouse wheel events.
*/

/*!
    \qmlproperty int QtQuick::MouseEvent::flags
    \since 5.11

    This property holds the flags that provide additional information about the
    mouse event.

    \list
    \li \l {Qt::MouseEventCreatedDoubleClick} {Qt.MouseEventCreatedDoubleClick}
    - Indicates that Qt has created a double click event from this event.
    This flag is set in the event originating from a button press, and not
    in the resulting double click event.
    \endlist
*/

/*!
    \qmltype WheelEvent
    \instantiates QQuickWheelEvent
    \inqmlmodule QtQuick
    \ingroup qtquick-input-events
    \brief Provides information about a mouse wheel event.

    The position of the mouse can be found via the \l x and \l y properties.

    \sa WheelHandler, MouseArea
*/

/*!
    \internal
    \class QQuickWheelEvent
*/

/*!
    \qmlproperty real QtQuick::WheelEvent::x
    \qmlproperty real QtQuick::WheelEvent::y

    These properties hold the coordinates of the position supplied by the wheel event.

    \sa QWheelEvent::position()
*/

/*!
    \qmlproperty bool QtQuick::WheelEvent::accepted

    Setting \a accepted to \c true prevents the wheel event from being
    propagated to items below the receiving item or handler.

    Generally, if the item acts on the wheel event, it should be accepted
    so that items lower in the stacking order do not also respond to the same event.

    \sa QWheelEvent::accepted
*/

/*!
    \qmlproperty int QtQuick::WheelEvent::buttons

    This property holds the mouse buttons pressed when the wheel event was generated.

    It contains a bitwise combination of:
    \list
    \li \l {Qt::LeftButton} {Qt.LeftButton}
    \li \l {Qt::RightButton} {Qt.RightButton}
    \li \l {Qt::MiddleButton} {Qt.MiddleButton}
    \endlist

    \sa QWheelEvent::buttons()
*/

/*!
    \qmlproperty point QtQuick::WheelEvent::angleDelta

    This property holds the relative amount that the wheel was rotated, in
    eighths of a degree. The \c x and \c y coordinates of this property hold
    the delta in horizontal and vertical orientations, respectively.

    A positive value indicates that the wheel was rotated up/right;
    a negative value indicates that the wheel was rotated down/left.

    Most mouse types work in steps of \c 15 degrees, in which case the delta value is a
    multiple of \c 120; i.e., \c {120 units * 1/8 = 15 degrees}.

    \sa QWheelEvent::angleDelta()
*/

/*!
    \qmlproperty point QtQuick::WheelEvent::pixelDelta

    This property holds the delta in screen pixels and is available in platforms that
    have high-resolution \l {QInputDevice::DeviceType::TouchPad}{trackpads}, such as \macos.
    The \c x and \c y coordinates of this property hold the delta in horizontal
    and vertical orientations, respectively. The values can be used directly to
    scroll content on screen.

    For platforms without \l {QInputDevice::Capability::PixelScroll}{high-resolution trackpad}
    support, pixelDelta will always be \c {(0,0)}, and \l angleDelta should be used instead.

    \sa QWheelEvent::pixelDelta()
*/

/*!
    \qmlproperty int QtQuick::WheelEvent::modifiers

    This property holds the keyboard modifier flags that existed immediately
    before the event occurred.

    It contains a bitwise combination of:
    \list
    \li \l {Qt::NoModifier} {Qt.NoModifier} - No modifier key is pressed.
    \li \l {Qt::ShiftModifier} {Qt.ShiftModifier} - A Shift key on the keyboard is pressed.
    \li \l {Qt::ControlModifier} {Qt.ControlModifier} - A Ctrl key on the keyboard is pressed.
    \li \l {Qt::AltModifier} {Qt.AltModifier} - An Alt key on the keyboard is pressed.
    \li \l {Qt::MetaModifier} {Qt.MetaModifier} - A Meta key on the keyboard is pressed.
    \li \l {Qt::KeypadModifier} {Qt.KeypadModifier} - A keypad button is pressed.
    \endlist

    For example, to react to a Control key pressed during the wheel event:
    \qml
    WheelHandler {
        onWheel: (wheel)=> {
            if (wheel.modifiers & Qt.ControlModifier) {
                adjustZoom(wheel.angleDelta.y / 120);
            }
        }
    }
    \endqml

    \sa QWheelEvent::modifiers()
*/

/*!
    \qmlproperty bool QtQuick::WheelEvent::inverted

    Returns whether the delta values delivered with the event are inverted.

    Normally, a vertical wheel will produce a WheelEvent with positive delta
    values if the top of the wheel is rotating away from the hand operating it.
    Similarly, a horizontal wheel movement will produce a QWheelEvent with
    positive delta values if the top of the wheel is moved to the left.

    However, on some platforms this is configurable, so that the same
    operations described above will produce negative delta values (but with the
    same magnitude). For instance, in a QML component (such as a tumbler or a
    slider) where it is appropriate to synchronize the movement or rotation of
    an item with the direction of the wheel, regardless of the system settings,
    the wheel event handler can use the inverted property to decide whether to
    negate the angleDelta or pixelDelta values.

    \note Many platforms provide no such information. On such platforms
    \c inverted always returns \c false.

    \sa QWheelEvent::inverted()
*/

QT_END_NAMESPACE

#include "moc_qquickevents_p_p.cpp"
