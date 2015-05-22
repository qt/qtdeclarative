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

            property ControlSpy spy: ControlSpy {
                target: control
                signals: ["pressed", "released", "canceled", "clicked", "pressedChanged", "checkedChanged"]
            }
        }
    }

    function init() {
    }

    function cleanup() {
    }

    function test_defaults() {
        var control = checkBox.createObject(testCase)
        verify(control)
        verify(control.label)
        verify(control.indicator)
        compare(control.text, "")
        compare(control.pressed, false)
        compare(control.checked, false)
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

        control.spy.expectedSequence = []
        compare(control.checked, false)
        verify(control.spy.success)

        control.spy.expectedSequence = [["checkedChanged", { "checked": true }]]
        control.checked = true
        compare(control.checked, true)
        verify(control.spy.success)

        control.spy.expectedSequence = [["checkedChanged", { "checked": false }]]
        control.checked = false
        compare(control.checked, false)
        verify(control.spy.success)

        control.destroy()
    }

    function test_mouse() {
        var control = checkBox.createObject(testCase)

        // check
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(control.spy.success)

        control.spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }],
                                        "released",
                                        "clicked",
                                        ["checkedChanged", { "pressed": false, "checked": true }]]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(control.spy.success)

        // uncheck
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(control.spy.success)
        control.spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true }],
                                        "released",
                                        "clicked",
                                        ["checkedChanged", { "pressed": false, "checked": false }]]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, false)
        compare(control.pressed, false)
        verify(control.spy.success)

        // release outside
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(control.spy.success)
        control.spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }]]
        mouseMove(control, control.width * 2, control.height * 2, 0, Qt.LeftButton)
        compare(control.pressed, false)
        verify(control.spy.success)
        control.spy.expectedSequence = [["canceled", { "pressed": false, "checked": false }]]
        mouseRelease(control, control.width * 2, control.height * 2, Qt.LeftButton)
        compare(control.checked, false)
        compare(control.pressed, false)
        verify(control.spy.success)

        // right button
        control.spy.expectedSequence = []
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.pressed, false)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.checked, false)
        compare(control.pressed, false)
        verify(control.spy.success)

        control.destroy()
    }

    function test_keys() {
        var control = checkBox.createObject(testCase)

        control.spy.expectedSequence = []
        control.forceActiveFocus()
        verify(control.activeFocus)
        verify(control.spy.success)

        // check
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false }],
                                        "released",
                                        "clicked",
                                        ["checkedChanged", { "pressed": false, "checked": true }]]
        keyClick(Qt.Key_Space)
        compare(control.checked, true)
        verify(control.spy.success)

        // uncheck
        control.spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": true }],
                                        "released",
                                        "clicked",
                                        ["checkedChanged", { "pressed": false, "checked": false }]]
        keyClick(Qt.Key_Space)
        compare(control.checked, false)
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
