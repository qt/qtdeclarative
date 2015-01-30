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
    name: "CheckBox"

    SignalSpy {
        id: checkedSpy
        signalName: "checkedChanged"
    }

    SignalSpy {
        id: pressedSpy
        signalName: "pressedChanged"
    }

    SignalSpy {
        id: clickedSpy
        signalName: "clicked"
    }

    Component {
        id: checkBox
        CheckBox { }
    }

    function init() {
        verify(!checkedSpy.target)
        verify(!pressedSpy.target)
        verify(!clickedSpy.target)
        compare(checkedSpy.count, 0)
        compare(pressedSpy.count, 0)
        compare(clickedSpy.count, 0)
    }

    function cleanup() {
        checkedSpy.target = null
        pressedSpy.target = null
        clickedSpy.target = null
        checkedSpy.clear()
        pressedSpy.clear()
        clickedSpy.clear()
    }

    function test_defaults() {
        var control = checkBox.createObject(testCase)
        verify(control)
        verify(control.label)
        verify(control.indicator)
        compare(control.text, "")
        compare(control.pressed, false)
        compare(control.checked, false)
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)
        control.destroy()
    }

    function test_layoutDirection() {
        var control = checkBox.createObject(testCase)

        verify(!control.LayoutMirroring.enabled)
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)

        control.layoutDirection = Qt.RightToLeft
        compare(control.layoutDirection, Qt.RightToLeft)
        compare(control.effectiveLayoutDirection, Qt.RightToLeft)

        control.LayoutMirroring.enabled = true
        compare(control.layoutDirection, Qt.RightToLeft)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)

        control.layoutDirection = Qt.LeftToRight
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.RightToLeft)

        control.LayoutMirroring.enabled = false
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)

        control.destroy()
    }

    function test_text() {
        var control = checkBox.createObject(testCase)
        compare(control.text, "")
        control.text = "CheckBox"
        compare(control.text, "CheckBox")
        control.text = ""
        compare(control.text, "")
        control.destroy()
    }

    function test_checked() {
        var control = checkBox.createObject(testCase)

        checkedSpy.target = control
        verify(checkedSpy.valid)

        compare(control.checked, false)
        compare(checkedSpy.count, 0)

        control.checked = true
        compare(control.checked, true)
        compare(checkedSpy.count, 1)

        control.checked = false
        compare(control.checked, false)
        compare(checkedSpy.count, 2)

        control.destroy()
    }

    function test_mouse() {
        var control = checkBox.createObject(testCase)

        checkedSpy.target = control
        pressedSpy.target = control
        clickedSpy.target = control
        verify(checkedSpy.valid)
        verify(pressedSpy.valid)
        verify(clickedSpy.valid)

        // check
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(clickedSpy.count, 1)
        compare(checkedSpy.count, 1)
        compare(pressedSpy.count, 2)
        compare(control.checked, true)
        compare(control.pressed, false)

        // uncheck
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(clickedSpy.count, 2)
        compare(checkedSpy.count, 2)
        compare(pressedSpy.count, 4)
        compare(control.checked, false)
        compare(control.pressed, false)

        // release outside
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(pressedSpy.count, 5)
        compare(control.pressed, true)
        mouseMove(control, control.width * 2, control.height * 2, 0, Qt.LeftButton)
        compare(control.pressed, false)
        mouseRelease(control, control.width * 2, control.height * 2, Qt.LeftButton)
        compare(clickedSpy.count, 2)
        compare(checkedSpy.count, 2)
        compare(pressedSpy.count, 6)
        compare(control.checked, false)
        compare(control.pressed, false)

        // right button
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(pressedSpy.count, 6)
        compare(control.pressed, false)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(clickedSpy.count, 2)
        compare(checkedSpy.count, 2)
        compare(pressedSpy.count, 6)
        compare(control.checked, false)
        compare(control.pressed, false)

        control.destroy()
    }

    function test_keys() {
        var control = checkBox.createObject(testCase)

        checkedSpy.target = control
        clickedSpy.target = control
        verify(checkedSpy.valid)
        verify(clickedSpy.valid)

        control.forceActiveFocus()
        verify(control.activeFocus)

        // check
        keyClick(Qt.Key_Space)
        compare(clickedSpy.count, 1)
        compare(checkedSpy.count, 1)
        compare(control.checked, true)

        // uncheck
        keyClick(Qt.Key_Space)
        compare(clickedSpy.count, 2)
        compare(checkedSpy.count, 2)
        compare(control.checked, false)

        // no change
        var keys = [Qt.Key_Enter, Qt.Key_Return, Qt.Key_Escape, Qt.Key_Tab]
        for (var i = 0; i < keys.length; ++i) {
            keyClick(keys[i])
            compare(clickedSpy.count, 2)
            compare(checkedSpy.count, 2)
            compare(control.checked, false)
        }

        control.destroy()
    }

    Component {
        id: twoCheckBoxes
        Item {
            property CheckBox cb1: CheckBox { id: cb1 }
            property CheckBox cb2: CheckBox { id: cb2; checked: cb1.checked; enabled: false }
        }
    }

    function test_binding() {
        var container = twoCheckBoxes.createObject(testCase)

        compare(container.cb1.checked, false)
        compare(container.cb2.checked, false)

        container.cb1.checked = true
        compare(container.cb1.checked, true)
        compare(container.cb2.checked, true)

        container.cb1.checked = false
        compare(container.cb1.checked, false)
        compare(container.cb2.checked, false)

        container.destroy()
    }
}
