/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
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

    Component {
        id: checkBox
        CheckBox {
            id: control

            property SignalSequenceSpy spy: SignalSequenceSpy {
                target: control
                signals: ["pressed", "released", "canceled", "clicked", "pressedChanged", "checkedChanged", "checkStateChanged"]
            }
        }
    }

    function test_text() {
        var control = checkBox.createObject(testCase)
        verify(control)

        compare(control.text, "")
        control.text = "CheckBox"
        compare(control.text, "CheckBox")
        control.text = ""
        compare(control.text, "")

        control.destroy()
    }

    function test_checked() {
        var control = checkBox.createObject(testCase)
        verify(control)

        control.spy.expectedSequence = []
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(control.spy.success)

        control.spy.expectedSequence = [["checkStateChanged", { "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "checked": true, "checkState": Qt.Checked }]]
        control.checked = true
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(control.spy.success)

        control.spy.expectedSequence = [["checkStateChanged", { "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "checked": false, "checkState": Qt.Unchecked }]]
        control.checked = false
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(control.spy.success)

        control.destroy()
    }

    function test_checkState() {
        var control = checkBox.createObject(testCase)
        verify(control)

        control.spy.expectedSequence = []
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(control.spy.success)

        control.spy.expectedSequence = [["checkStateChanged", { "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "checked": true, "checkState": Qt.Checked }]]
        control.checkState = Qt.Checked
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(control.spy.success)

        control.spy.expectedSequence = [["checkStateChanged", { "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "checked": false, "checkState": Qt.Unchecked }]]
        control.checkState = Qt.Unchecked
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(control.spy.success)

        control.destroy()
    }

    function test_mouse() {
        var control = checkBox.createObject(testCase)
        verify(control)

        // check
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(control.spy.success)

        control.spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "released",
                                        "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        compare(control.pressed, false)
        verify(control.spy.success)

        // uncheck
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(control.spy.success)
        control.spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "released",
                                        "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        compare(control.pressed, false)
        verify(control.spy.success)

        // release outside
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(control.spy.success)
        control.spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }]]
        mouseMove(control, control.width * 2, control.height * 2, 0, Qt.LeftButton)
        compare(control.pressed, false)
        verify(control.spy.success)
        control.spy.expectedSequence = [["canceled", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }]]
        mouseRelease(control, control.width * 2, control.height * 2, Qt.LeftButton)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        compare(control.pressed, false)
        verify(control.spy.success)

        // right button
        control.spy.expectedSequence = []
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.pressed, false)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        compare(control.pressed, false)
        verify(control.spy.success)

        control.destroy()
    }

    function test_keys() {
        var control = checkBox.createObject(testCase)
        verify(control)

        control.spy.expectedSequence = []
        control.forceActiveFocus()
        verify(control.activeFocus)
        verify(control.spy.success)

        // check
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(control.spy.success)

        // uncheck
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(control.spy.success)

        // no change
        control.spy.expectedSequence = []
        var keys = [Qt.Key_Enter, Qt.Key_Return, Qt.Key_Escape, Qt.Key_Tab]
        for (var i = 0; i < keys.length; ++i) {
            control.spy.reset()
            keyClick(keys[i])
            compare(control.checked, false)
            verify(control.spy.success)
        }

        control.destroy()
    }

    Component {
        id: checkedBoundBoxes
        Item {
            property CheckBox cb1: CheckBox { id: cb1 }
            property CheckBox cb2: CheckBox { id: cb2; checked: cb1.checked; enabled: false }
        }
    }

    function test_checked_binding() {
        var container = checkedBoundBoxes.createObject(testCase)
        verify(container)

        compare(container.cb1.checked, false)
        compare(container.cb1.checkState, Qt.Unchecked)
        compare(container.cb2.checked, false)
        compare(container.cb2.checkState, Qt.Unchecked)

        container.cb1.checked = true
        compare(container.cb1.checked, true)
        compare(container.cb1.checkState, Qt.Checked)
        compare(container.cb2.checked, true)
        compare(container.cb2.checkState, Qt.Checked)

        container.cb1.checked = false
        compare(container.cb1.checked, false)
        compare(container.cb1.checkState, Qt.Unchecked)
        compare(container.cb2.checked, false)
        compare(container.cb2.checkState, Qt.Unchecked)

        container.destroy()
    }

    Component {
        id: checkStateBoundBoxes
        Item {
            property CheckBox cb1: CheckBox { id: cb1 }
            property CheckBox cb2: CheckBox { id: cb2; checkState: cb1.checkState; enabled: false }
        }
    }

    function test_checkState_binding() {
        var container = checkStateBoundBoxes.createObject(testCase)
        verify(container)

        compare(container.cb1.checked, false)
        compare(container.cb1.checkState, Qt.Unchecked)
        compare(container.cb2.checked, false)
        compare(container.cb2.checkState, Qt.Unchecked)

        container.cb1.checkState = Qt.Checked
        compare(container.cb1.checked, true)
        compare(container.cb1.checkState, Qt.Checked)
        compare(container.cb2.checked, true)
        compare(container.cb2.checkState, Qt.Checked)

        container.cb1.checkState = Qt.Unchecked
        compare(container.cb1.checked, false)
        compare(container.cb1.checkState, Qt.Unchecked)
        compare(container.cb2.checked, false)
        compare(container.cb2.checkState, Qt.Unchecked)

        compare(container.cb1.tristate, false)
        compare(container.cb2.tristate, false)

        container.cb1.checkState = Qt.PartiallyChecked
        compare(container.cb1.checked, true)
        compare(container.cb1.checkState, Qt.PartiallyChecked)
        compare(container.cb2.checked, true)
        compare(container.cb2.checkState, Qt.PartiallyChecked)

        compare(container.cb1.tristate, true)
        compare(container.cb2.tristate, true)

        container.destroy()
    }

    function test_tristate() {
        var control = checkBox.createObject(testCase)

        control.spy.expectedSequence = []
        control.forceActiveFocus()
        verify(control.activeFocus)

        compare(control.tristate, false)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)

        control.spy.expectedSequence = [["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.PartiallyChecked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.PartiallyChecked }]]
        control.checkState = Qt.PartiallyChecked
        compare(control.tristate, true)
        compare(control.checked, true)
        compare(control.checkState, Qt.PartiallyChecked)
        verify(control.spy.success)

        // key: partial -> checked
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.PartiallyChecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.PartiallyChecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(control.spy.success)

        // key: checked -> unchecked
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(control.spy.success)

        // key: unchecked -> partial
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.PartiallyChecked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.PartiallyChecked }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, true)
        compare(control.checkState, Qt.PartiallyChecked)
        verify(control.spy.success)

        // mouse: partial -> checked
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.PartiallyChecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.PartiallyChecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "released",
                                        "clicked"]
        mouseClick(control)
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(control.spy.success)

        // mouse: checked -> unchecked
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "released",
                                        "clicked"]
        mouseClick(control)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(control.spy.success)

        // mouse: unchecked -> partial
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.PartiallyChecked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.PartiallyChecked }],
                                        "released",
                                        "clicked"]
        mouseClick(control)
        compare(control.checked, true)
        compare(control.checkState, Qt.PartiallyChecked)
        verify(control.spy.success)

        control.destroy()
    }

    function test_baseline() {
        var control = checkBox.createObject(testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
        control.destroy()
    }
}
