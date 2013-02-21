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

#include "qquickevents_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype KeyEvent
    \instantiates QQuickKeyEvent
    \inqmlmodule QtQuick 2
    \ingroup qtquick-input-events

    \brief Provides information about a key event

    For example, the following changes the Item's state property when the Enter
    key is pressed:
    \qml
Item {
    focus: true
    Keys.onPressed: { if (event.key == Qt.Key_Enter) state = 'ShowDetails'; }
}
    \endqml
*/

/*!
    \qmlproperty int QtQuick2::KeyEvent::key

    This property holds the code of the key that was pressed or released.

    See \l {Qt::Key}{Qt.Key} for the list of keyboard codes. These codes are
    independent of the underlying window system. Note that this
    function does not distinguish between capital and non-capital
    letters, use the text() function (returning the Unicode text the
    key generated) for this purpose.

    A value of either 0 or \l {Qt::Key_unknown}{Qt.Key_Unknown} means that the event is not
    the result of a known key; for example, it may be the result of
    a compose sequence, a keyboard macro, or due to key event
    compression.
*/

/*!
    \qmlproperty string QtQuick2::KeyEvent::text

    This property holds the Unicode text that the key generated.
    The text returned can be an empty string in cases where modifier keys,
    such as Shift, Control, Alt, and Meta, are being pressed or released.
    In such cases \c key will contain a valid value
*/

/*!
    \qmlproperty bool QtQuick2::KeyEvent::isAutoRepeat

    This property holds whether this event comes from an auto-repeating key.
*/

/*!
    \qmlproperty quint32 QtQuick2::KeyEvent::nativeScanCode

    This property contains the native scan code of the key that was pressed. It is
    passed through from QKeyEvent unchanged.

    \sa QKeyEvent::nativeScanCode()
*/

/*!
    \qmlproperty int QtQuick2::KeyEvent::count

    This property holds the number of keys involved in this event. If \l KeyEvent::text
    is not empty, this is simply the length of the string.
*/

/*!
    \qmlproperty bool QtQuick2::KeyEvent::accepted

    Setting \a accepted to true prevents the key event from being
    propagated to the item's parent.

    Generally, if the item acts on the key event then it should be accepted
    so that ancestor items do not also respond to the same event.
*/

/*!
    \qmlproperty int QtQuick2::KeyEvent::modifiers

    This property holds the keyboard modifier flags that existed immediately
    before the event occurred.

    It contains a bitwise combination of:
    \list
    \li Qt.NoModifier - No modifier key is pressed.
    \li Qt.ShiftModifier - A Shift key on the keyboard is pressed.
    \li Qt.ControlModifier - A Ctrl key on the keyboard is pressed.
    \li Qt.AltModifier - An Alt key on the keyboard is pressed.
    \li Qt.MetaModifier - A Meta key on the keyboard is pressed.
    \li Qt.KeypadModifier - A keypad button is pressed.
    \endlist

    For example, to react to a Shift key + Enter key combination:
    \qml
    Item {
        focus: true
        Keys.onPressed: {
            if ((event.key == Qt.Key_Enter) && (event.modifiers & Qt.ShiftModifier))
                doSomething();
        }
    }
    \endqml
*/


/*!
    \qmltype MouseEvent
    \instantiates QQuickMouseEvent
    \inqmlmodule QtQuick 2
    \ingroup qtquick-input-events

    \brief Provides information about a mouse event

    The position of the mouse can be found via the \l x and \l y properties.
    The button that caused the event is available via the \l button property.

    \sa MouseArea
*/

/*!
    \internal
    \class QQuickMouseEvent
*/

/*!
    \qmlproperty int QtQuick2::MouseEvent::x
    \qmlproperty int QtQuick2::MouseEvent::y

    These properties hold the coordinates of the position supplied by the mouse event.
*/


/*!
    \qmlproperty bool QtQuick2::MouseEvent::accepted

    Setting \a accepted to true prevents the mouse event from being
    propagated to items below this item.

    Generally, if the item acts on the mouse event then it should be accepted
    so that items lower in the stacking order do not also respond to the same event.
*/

/*!
    \qmlproperty enumeration QtQuick2::MouseEvent::button

    This property holds the button that caused the event.  It can be one of:
    \list
    \li Qt.LeftButton
    \li Qt.RightButton
    \li Qt.MiddleButton
    \endlist
*/

/*!
    \qmlproperty bool QtQuick2::MouseEvent::wasHeld

    This property is true if the mouse button has been held pressed longer the
    threshold (800ms).
*/

/*!
    \qmlproperty int QtQuick2::MouseEvent::buttons

    This property holds the mouse buttons pressed when the event was generated.
    For mouse move events, this is all buttons that are pressed down. For mouse
    press and double click events this includes the button that caused the event.
    For mouse release events this excludes the button that caused the event.

    It contains a bitwise combination of:
    \list
    \li Qt.LeftButton
    \li Qt.RightButton
    \li Qt.MiddleButton
    \endlist
*/

/*!
    \qmlproperty int QtQuick2::MouseEvent::modifiers

    This property holds the keyboard modifier flags that existed immediately
    before the event occurred.

    It contains a bitwise combination of:
    \list
    \li Qt.NoModifier - No modifier key is pressed.
    \li Qt.ShiftModifier - A Shift key on the keyboard is pressed.
    \li Qt.ControlModifier - A Ctrl key on the keyboard is pressed.
    \li Qt.AltModifier - An Alt key on the keyboard is pressed.
    \li Qt.MetaModifier - A Meta key on the keyboard is pressed.
    \li Qt.KeypadModifier - A keypad button is pressed.
    \endlist

    For example, to react to a Shift key + Left mouse button click:
    \qml
    MouseArea {
        onClicked: {
            if ((mouse.button == Qt.LeftButton) && (mouse.modifiers & Qt.ShiftModifier))
                doSomething();
        }
    }
    \endqml
*/


/*!
    \qmltype WheelEvent
    \instantiates QQuickWheelEvent
    \inqmlmodule QtQuick 2
    \ingroup qtquick-input-events
    \brief Provides information about a mouse wheel event

    The position of the mouse can be found via the \l x and \l y properties.

    \sa MouseArea
*/

/*!
    \internal
    \class QQuickWheelEvent
*/

/*!
    \qmlproperty int QtQuick2::WheelEvent::x
    \qmlproperty int QtQuick2::WheelEvent::y

    These properties hold the coordinates of the position supplied by the wheel event.
*/

/*!
    \qmlproperty bool QtQuick2::WheelEvent::accepted

    Setting \a accepted to true prevents the wheel event from being
    propagated to items below this item.

    Generally, if the item acts on the wheel event then it should be accepted
    so that items lower in the stacking order do not also respond to the same event.
*/

/*!
    \qmlproperty int QtQuick2::WheelEvent::buttons

    This property holds the mouse buttons pressed when the wheel event was generated.

    It contains a bitwise combination of:
    \list
    \li Qt.LeftButton
    \li Qt.RightButton
    \li Qt.MiddleButton
    \endlist
*/

/*!
    \qmlproperty point QtQuick2::WheelEvent::angleDelta

    This property holds the distance that the wheel is rotated in wheel degrees.
    The x and y cordinate of this property holds the delta in horizontal and
    vertical orientation.

    A positive value indicates that the wheel was rotated up/right;
    a negative value indicates that the wheel was rotated down/left.

    Most mouse types work in steps of 15 degrees, in which case the delta value is a
    multiple of 120; i.e., 120 units * 1/8 = 15 degrees.
*/

/*!
    \qmlproperty point QtQuick2::WheelEvent::pixelDelta

    This property holds the delta in screen pixels and is available in plataforms that
    have high-resolution trackpads, such as Mac OS X.
    The x and y cordinate of this property holds the delta in horizontal and
    vertical orientation. The value should be used directly to scroll content on screen.

    For platforms without high-resolution trackpad support, pixelDelta will always be (0,0),
    and angleDelta should be used instead.
*/

/*!
    \qmlproperty int QtQuick2::WheelEvent::modifiers

    This property holds the keyboard modifier flags that existed immediately
    before the event occurred.

    It contains a bitwise combination of:
    \list
    \li Qt.NoModifier - No modifier key is pressed.
    \li Qt.ShiftModifier - A Shift key on the keyboard is pressed.
    \li Qt.ControlModifier - A Ctrl key on the keyboard is pressed.
    \li Qt.AltModifier - An Alt key on the keyboard is pressed.
    \li Qt.MetaModifier - A Meta key on the keyboard is pressed.
    \li Qt.KeypadModifier - A keypad button is pressed.
    \endlist

    For example, to react to a Control key pressed during the wheel event:
    \qml
    MouseArea {
        onWheel: {
            if (wheel.modifiers & Qt.ControlModifier) {
                if (wheel.angleDelta.y > 0)
                    zoomIn();
                else
                    zoomOut();
            }
        }
    }
    \endqml
*/

QT_END_NAMESPACE
