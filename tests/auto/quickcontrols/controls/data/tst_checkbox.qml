// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "CheckBox"

    Component {
        id: checkBox
        CheckBox { }
    }

    Component {
        id: signalSequenceSpy
        SignalSequenceSpy {
            signals: ["pressed", "released", "canceled", "clicked", "toggled", "pressedChanged", "checkedChanged", "checkStateChanged"]
        }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(checkBox, testCase)
        verify(control)
        compare(control.tristate, false)
        compare(control.checkState, Qt.Unchecked)
    }

    function test_text() {
        let control = createTemporaryObject(checkBox, testCase)
        verify(control)

        compare(control.text, "")
        control.text = "CheckBox"
        compare(control.text, "CheckBox")
        control.text = ""
        compare(control.text, "")
    }

    function test_checked() {
        let control = createTemporaryObject(checkBox, testCase)
        verify(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        sequenceSpy.expectedSequence = []
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["checkStateChanged", { "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "checked": true, "checkState": Qt.Checked }]]
        control.checked = true
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["checkStateChanged", { "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "checked": false, "checkState": Qt.Unchecked }]]
        control.checked = false
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)
    }

    function test_checkState() {
        let control = createTemporaryObject(checkBox, testCase)
        verify(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        sequenceSpy.expectedSequence = []
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["checkStateChanged", { "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "checked": true, "checkState": Qt.Checked }]]
        control.checkState = Qt.Checked
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["checkStateChanged", { "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "checked": false, "checkState": Qt.Unchecked }]]
        control.checkState = Qt.Unchecked
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)
    }

    function test_mouse() {
        let control = createTemporaryObject(checkBox, testCase)
        verify(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        // check
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // uncheck
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // release outside
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }]]
        mouseMove(control, control.width * 2, control.height * 2, 0)
        compare(control.pressed, false)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["canceled", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }]]
        mouseRelease(control, control.width * 2, control.height * 2, Qt.LeftButton)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // right button
        sequenceSpy.expectedSequence = []
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.pressed, false)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)
    }

    function test_touch() {
        let control = createTemporaryObject(checkBox, testCase)
        verify(control)

        let touch = touchEvent(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        // check
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed"]
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        touch.release(0, control, control.width / 2, control.height / 2).commit()
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // uncheck
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed"]
        // Don't want to double-click.
        wait(Qt.styleHints.mouseDoubleClickInterval + 50)
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        touch.release(0, control, control.width / 2, control.height / 2).commit()
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // release outside
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed"]
        wait(Qt.styleHints.mouseDoubleClickInterval + 50)
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }]]
        touch.move(0, control, control.width * 2, control.height * 2).commit()
        compare(control.pressed, false)
        verify(sequenceSpy.success)
        sequenceSpy.expectedSequence = [["canceled", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }]]
        touch.release(0, control, control.width * 2, control.height * 2).commit()
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        compare(control.pressed, false)
        verify(sequenceSpy.success)
    }

    function test_keys() {
        let control = createTemporaryObject(checkBox, testCase)
        verify(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        sequenceSpy.expectedSequence = []
        control.forceActiveFocus()
        verify(control.activeFocus)
        verify(sequenceSpy.success)

        // check
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(sequenceSpy.success)

        // uncheck
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "toggled",
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)

        // no change
        sequenceSpy.expectedSequence = []
        // Not testing Key_Enter and Key_Return because QGnomeTheme uses them for
        // pressing buttons and the CI uses the QGnomeTheme platform theme.
        let keys = [Qt.Key_Escape, Qt.Key_Tab]
        for (let i = 0; i < keys.length; ++i) {
            sequenceSpy.reset()
            keyClick(keys[i])
            compare(control.checked, false)
            verify(sequenceSpy.success)
        }
    }

    Component {
        id: checkedBoundBoxes
        Item {
            property CheckBox cb1: CheckBox { id: cb1 }
            property CheckBox cb2: CheckBox { id: cb2; checked: cb1.checked; enabled: false }
        }
    }

    function test_checked_binding() {
        let container = createTemporaryObject(checkedBoundBoxes, testCase)
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
    }

    Component {
        id: checkStateBoundBoxes
        Item {
            property CheckBox cb1: CheckBox { id: cb1 }
            property CheckBox cb2: CheckBox { id: cb2; checkState: cb1.checkState; enabled: false }
        }
    }

    function test_checkState_binding() {
        let container = createTemporaryObject(checkStateBoundBoxes, testCase)
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
        compare(container.cb1.checked, false)
        compare(container.cb1.checkState, Qt.PartiallyChecked)
        compare(container.cb2.checked, false)
        compare(container.cb2.checkState, Qt.PartiallyChecked)

        // note: since Qt Quick Controls 2.4 (Qt 5.11), CheckBox does not
        // force tristate when checkState is set to Qt.PartiallyChecked
        compare(container.cb1.tristate, false)
        compare(container.cb2.tristate, false)
    }

    function test_tristate() {
        let control = createTemporaryObject(checkBox, testCase, {tristate: true})

        let sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        sequenceSpy.expectedSequence = []
        control.forceActiveFocus()
        verify(control.activeFocus)

        compare(control.tristate, true)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)

        sequenceSpy.expectedSequence = [["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.PartiallyChecked }]]
        control.checkState = Qt.PartiallyChecked
        compare(control.checked, false)
        compare(control.checkState, Qt.PartiallyChecked)
        verify(sequenceSpy.success)

        // key: partial -> checked
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.PartiallyChecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.PartiallyChecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(sequenceSpy.success)

        // key: checked -> unchecked
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)

        // key: unchecked -> partial
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.PartiallyChecked }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, false)
        compare(control.checkState, Qt.PartiallyChecked)
        verify(sequenceSpy.success)

        // mouse: partial -> checked
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.PartiallyChecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.PartiallyChecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        "released",
                                        "clicked"]
        mouseClick(control)
        compare(control.checked, true)
        compare(control.checkState, Qt.Checked)
        verify(sequenceSpy.success)

        // mouse: checked -> unchecked
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true, "checkState": Qt.Checked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": true, "checkState": Qt.Checked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        "released",
                                        "clicked"]
        mouseClick(control)
        compare(control.checked, false)
        compare(control.checkState, Qt.Unchecked)
        verify(sequenceSpy.success)

        // mouse: unchecked -> partial
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false, "checkState": Qt.Unchecked }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false, "checked": false, "checkState": Qt.Unchecked }],
                                        ["checkStateChanged", { "pressed": false, "checked": false, "checkState": Qt.PartiallyChecked }],
                                        "released",
                                        "clicked"]
        mouseClick(control)
        compare(control.checked, false)
        compare(control.checkState, Qt.PartiallyChecked)
        verify(sequenceSpy.success)
    }

    function test_baseline() {
        let control = createTemporaryObject(checkBox, testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
    }

    Component {
        id: nextCheckStateBox
        CheckBox {
            tristate: true
            nextCheckState: function() {
                if (checkState === Qt.Checked)
                    return Qt.Unchecked
                else
                    return Qt.Checked
            }
        }
    }

    function test_nextCheckState_data() {
        return [
            { tag: "unchecked", checkState: Qt.Unchecked, expectedState: Qt.Checked },
            { tag: "partially-checked", checkState: Qt.PartiallyChecked, expectedState: Qt.Checked },
            { tag: "checked", checkState: Qt.Checked, expectedState: Qt.Unchecked }
        ]
    }

    function test_nextCheckState(data) {
        let control = createTemporaryObject(nextCheckStateBox, testCase)
        verify(control)

        // mouse
        control.checkState = data.checkState
        compare(control.checkState, data.checkState)
        mouseClick(control)
        compare(control.checkState, data.expectedState)

        // touch
        control.checkState = data.checkState
        compare(control.checkState, data.checkState)
        let touch = touchEvent(control)
        touch.press(0, control).commit().release(0, control).commit()
        compare(control.checkState, data.expectedState)

        // keyboard
        control.forceActiveFocus()
        tryCompare(control, "activeFocus", true)
        control.checkState = data.checkState
        compare(control.checkState, data.checkState)
        keyClick(Qt.Key_Space)
        compare(control.checkState, data.expectedState)
    }
}
