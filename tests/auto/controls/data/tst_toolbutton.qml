/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.2
import QtTest 1.0
import QtQuick.Controls 2.0

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "ToolButton"

    SignalSpy {
        id: pressedSpy
        signalName: "pressedChanged"
    }

    SignalSpy {
        id: clickedSpy
        signalName: "clicked"
    }

    Component {
        id: toolButton
        ToolButton { }
    }

    function init() {
        verify(!pressedSpy.target)
        verify(!clickedSpy.target)
        compare(pressedSpy.count, 0)
        compare(clickedSpy.count, 0)
    }

    function cleanup() {
        pressedSpy.target = null
        clickedSpy.target = null
        pressedSpy.clear()
        clickedSpy.clear()
    }

    function test_defaults() {
        var control = toolButton.createObject(testCase)
        verify(control)
        verify(control.label)
        compare(control.text, "")
        compare(control.pressed, false)
        control.destroy()
    }

    function test_text() {
        var control = toolButton.createObject(testCase)
        compare(control.text, "")
        control.text = "ToolButton"
        compare(control.text, "ToolButton")
        control.text = ""
        compare(control.text, "")
        control.destroy()
    }

    function test_mouse() {
        var control = toolButton.createObject(testCase)

        pressedSpy.target = control
        clickedSpy.target = control
        verify(pressedSpy.valid)
        verify(clickedSpy.valid)

        // check
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftToolButton)
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftToolButton)
        compare(clickedSpy.count, 1)
        compare(pressedSpy.count, 2)
        compare(control.pressed, false)

        // uncheck
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftToolButton)
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftToolButton)
        compare(clickedSpy.count, 2)
        compare(pressedSpy.count, 4)
        compare(control.pressed, false)

        // release outside
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftToolButton)
        compare(pressedSpy.count, 5)
        compare(control.pressed, true)
        mouseMove(control, control.width * 2, control.height * 2, 0, Qt.LeftToolButton)
        compare(control.pressed, false)
        mouseRelease(control, control.width * 2, control.height * 2, Qt.LeftToolButton)
        compare(clickedSpy.count, 2)
        compare(pressedSpy.count, 6)
        compare(control.pressed, false)

        // right button
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(pressedSpy.count, 6)
        compare(control.pressed, false)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(clickedSpy.count, 2)
        compare(pressedSpy.count, 6)
        compare(control.pressed, false)

        control.destroy()
    }

    function test_keys() {
        var control = toolButton.createObject(testCase)

        clickedSpy.target = control
        verify(clickedSpy.valid)

        control.forceActiveFocus()
        verify(control.activeFocus)

        // check
        keyClick(Qt.Key_Space)
        compare(clickedSpy.count, 1)

        // uncheck
        keyClick(Qt.Key_Space)
        compare(clickedSpy.count, 2)

        // no change
        var keys = [Qt.Key_Enter, Qt.Key_Return, Qt.Key_Escape, Qt.Key_Tab]
        for (var i = 0; i < keys.length; ++i) {
            keyClick(keys[i])
            compare(clickedSpy.count, 2)
        }

        control.destroy()
    }
}
