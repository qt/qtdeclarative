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
    name: "Switch"

    Component {
        id: swtch
        Switch { }
    }

    Component {
        id: signalSequenceSpy
        SignalSequenceSpy {
            signals: ["pressed", "released", "canceled", "clicked", "toggled", "pressedChanged", "checkedChanged"]
        }
    }

    function test_defaults() {
        failOnWarning(/.?/)

        let control = createTemporaryObject(swtch, testCase)
        verify(control)
    }

    function test_text() {
        var control = createTemporaryObject(swtch, testCase)
        verify(control)

        compare(control.text, "")
        control.text = "Switch"
        compare(control.text, "Switch")
        control.text = ""
        compare(control.text, "")
    }

    function test_checked() {
        var control = createTemporaryObject(swtch, testCase)
        verify(control)

        compare(control.checked, false)

        var spy = signalSequenceSpy.createObject(control, {target: control})
        spy.expectedSequence = [["checkedChanged", { "checked": true }]]
        control.checked = true
        compare(control.checked, true)
        verify(spy.success)

        spy.expectedSequence = [["checkedChanged", { "checked": false }]]
        control.checked = false
        compare(control.checked, false)
        verify(spy.success)
    }

    function test_pressed_data() {
        return [
            { tag: "indicator", x: 15 },
            { tag: "background", x: 5 }
        ]
    }

    function test_pressed(data) {
        var control = createTemporaryObject(swtch, testCase, {padding: 10})
        verify(control)

        // stays pressed when dragged outside
        compare(control.pressed, false)
        mousePress(control, data.x, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        mouseMove(control, -1, control.height / 2)
        compare(control.pressed, true)
        mouseRelease(control, -1, control.height / 2, Qt.LeftButton)
        compare(control.pressed, false)
    }

    function test_mouse() {
        var control = createTemporaryObject(swtch, testCase)
        verify(control)

        // check
        var spy = signalSequenceSpy.createObject(control, {target: control})
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(spy.success)
        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }],
                                ["checkedChanged", { "pressed": false, "checked": true }],
                                "toggled",
                                "released",
                                "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(spy.success)

        // uncheck
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(spy.success)
        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true }],
                                ["checkedChanged", { "pressed": false, "checked": false }],
                                "toggled",
                                "released",
                                "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, false)
        compare(control.pressed, false)
        verify(spy.success)

        // release on the right
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(spy.success)
        mouseMove(control, control.width * 2, control.height / 2, 0)
        compare(control.pressed, true)
        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }],
                                ["checkedChanged", { "pressed": false, "checked": true }],
                                "toggled",
                                "released",
                                "clicked"]
        mouseRelease(control, control.width * 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(spy.success)

        // release on the left
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(spy.success)
        mouseMove(control, -control.width, control.height / 2, 0)
        compare(control.pressed, true)
        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true }],
                                ["checkedChanged", { "pressed": false, "checked": false }],
                                "toggled",
                                "released",
                                "clicked"]
        mouseRelease(control, -control.width, control.height / 2, Qt.LeftButton)
        compare(control.checked, false)
        compare(control.pressed, false)
        verify(spy.success)

        // release in the middle
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                "pressed"]
        mousePress(control, 0, 0, Qt.LeftButton)
        compare(control.pressed, true)
        verify(spy.success)
        mouseMove(control, control.width / 2, control.height / 2, 0, Qt.LeftButton)
        compare(control.pressed, true)
        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }],
                                "released",
                                "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.checked, false)
        compare(control.pressed, false)
        tryCompare(control, "position", 0) // QTBUG-57944
        verify(spy.success)

        // right button
        spy.expectedSequence = []
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.pressed, false)
        verify(spy.success)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.checked, false)
        compare(control.pressed, false)
        verify(spy.success)
    }

    function test_touch() {
        var control = createTemporaryObject(swtch, testCase)
        verify(control)

        var touch = touchEvent(control)

        // check
        var spy = signalSequenceSpy.createObject(control, {target: control})
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                "pressed"]
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(spy.success)
        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }],
                                ["checkedChanged", { "pressed": false, "checked": true }],
                                "toggled",
                                "released",
                                "clicked"]
        touch.release(0, control, control.width / 2, control.height / 2).commit()
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(spy.success)

        // uncheck
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                "pressed"]
        // Don't want to double-click.
        wait(Qt.styleHints.mouseDoubleClickInterval + 50)
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(spy.success)
        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true }],
                                ["checkedChanged", { "pressed": false, "checked": false }],
                                "toggled",
                                "released",
                                "clicked"]
        touch.release(0, control, control.width / 2, control.height / 2).commit()
        compare(control.checked, false)
        compare(control.pressed, false)
        verify(spy.success)

        // release on the right
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                "pressed"]
        wait(Qt.styleHints.mouseDoubleClickInterval + 50)
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(spy.success)
        touch.move(0, control, control.width * 2, control.height / 2).commit()
        compare(control.pressed, true)
        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }],
                                ["checkedChanged", { "pressed": false, "checked": true }],
                                "toggled",
                                "released",
                                "clicked"]
        touch.release(0, control, control.width * 2, control.height / 2).commit()
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(spy.success)

        // release on the left
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                "pressed"]
        wait(Qt.styleHints.mouseDoubleClickInterval + 50)
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(spy.success)
        touch.move(0, control, -control.width, control.height / 2).commit()
        compare(control.pressed, true)
        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true }],
                                ["checkedChanged", { "pressed": false, "checked": false }],
                                "toggled",
                                "released",
                                "clicked"]
        touch.release(0, control, -control.width, control.height / 2).commit()
        compare(control.checked, false)
        compare(control.pressed, false)
        verify(spy.success)

        // release in the middle
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                "pressed"]
        wait(Qt.styleHints.mouseDoubleClickInterval + 50)
        touch.press(0, control, 0, 0).commit()
        compare(control.pressed, true)
        verify(spy.success)
        touch.move(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }],
                                "released",
                                "clicked"]
        touch.release(0, control, control.width / 2, control.height / 2).commit()
        compare(control.checked, false)
        compare(control.pressed, false)
        tryCompare(control, "position", 0) // QTBUG-57944
        verify(spy.success)
    }

    function test_mouseDrag() {
        var control = createTemporaryObject(swtch, testCase, {leftPadding: 100, rightPadding: 100})
        verify(control)

        var spy = signalSequenceSpy.createObject(control, {target: control})
        compare(control.position, 0.0)
        compare(control.checked, false)
        compare(control.pressed, false)

        // press-drag-release inside the indicator
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                "pressed"]
        mousePress(control.indicator, 0)
        compare(control.position, 0.0)
        compare(control.checked, false)
        compare(control.pressed, true)
        verify(spy.success)

        mouseMove(control.indicator, control.width - 1)
        compare(control.position, 1.0)
        compare(control.checked, false)
        compare(control.pressed, true)

        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }],
                                ["checkedChanged", { "pressed": false, "checked": true }],
                                "toggled",
                                "released",
                                "clicked"]
        mouseRelease(control.indicator, control.indicator.width - 1)
        compare(control.position, 1.0)
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(spy.success)

        // press-drag-release outside the indicator
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                "pressed"]
        mousePress(control, 0)
        compare(control.position, 1.0)
        compare(control.checked, true)
        compare(control.pressed, true)
        verify(spy.success)

        mouseMove(control, control.width - control.rightPadding)
        compare(control.position, 1.0)
        compare(control.checked, true)
        compare(control.pressed, true)

        mouseMove(control, control.width / 2)
        compare(control.position, 0.5)
        compare(control.checked, true)
        compare(control.pressed, true)

        mouseMove(control, control.leftPadding)
        compare(control.position, 0.0)
        compare(control.checked, true)
        compare(control.pressed, true)

        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true }],
                                ["checkedChanged", { "pressed": false, "checked": false }],
                                "toggled",
                                "released",
                                "clicked"]
        mouseRelease(control, control.width - 1)
        compare(control.position, 0.0)
        compare(control.checked, false)
        compare(control.pressed, false)
        verify(spy.success)

        // press-drag-release from and to outside the indicator
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                "pressed"]
        mousePress(control, control.width - 1)
        compare(control.position, 0.0)
        compare(control.checked, false)
        compare(control.pressed, true)
        verify(spy.success)

        mouseMove(control, control.width - control.rightPadding)
        compare(control.position, 0.0)
        compare(control.checked, false)
        compare(control.pressed, true)

        mouseMove(control, control.width / 2)
        compare(control.position, 0.5)
        compare(control.checked, false)
        compare(control.pressed, true)

        mouseMove(control, control.width - control.rightPadding)
        compare(control.position, 1.0)
        compare(control.checked, false)
        compare(control.pressed, true)

        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }],
                                ["checkedChanged", { "pressed": false, "checked": true }],
                                "toggled",
                                "released",
                                "clicked"]
        mouseRelease(control, control.width - 1)
        compare(control.position, 1.0)
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(spy.success)
    }

    function test_touchDrag() {
        var control = createTemporaryObject(swtch, testCase, {leftPadding: 100, rightPadding: 100})
        verify(control)

        var touch = touchEvent(control)

        var spy = signalSequenceSpy.createObject(control, {target: control})
        compare(control.position, 0.0)
        compare(control.checked, false)
        compare(control.pressed, false)

        // press-drag-release inside the indicator
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                "pressed"]
        touch.press(0, control.indicator, 0).commit()
        compare(control.position, 0.0)
        compare(control.checked, false)
        compare(control.pressed, true)
        verify(spy.success)

        touch.move(0, control.indicator, control.width).commit()
        compare(control.position, 1.0)
        compare(control.checked, false)
        compare(control.pressed, true)

        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }],
                                ["checkedChanged", { "pressed": false, "checked": true }],
                                "toggled",
                                "released",
                                "clicked"]
        touch.release(0, control.indicator, control.indicator.width).commit()
        compare(control.position, 1.0)
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(spy.success)

        // press-drag-release outside the indicator
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                "pressed"]
        // Don't want to double-click.
        wait(Qt.styleHints.mouseDoubleClickInterval + 50)
        touch.press(0, control, 0).commit()
        compare(control.position, 1.0)
        compare(control.checked, true)
        compare(control.pressed, true)
        verify(spy.success)

        touch.move(0, control, control.width - control.rightPadding).commit()
        compare(control.position, 1.0)
        compare(control.checked, true)
        compare(control.pressed, true)

        touch.move(0, control, control.width / 2).commit()
        compare(control.position, 0.5)
        compare(control.checked, true)
        compare(control.pressed, true)

        touch.move(0, control, control.leftPadding).commit()
        compare(control.position, 0.0)
        compare(control.checked, true)
        compare(control.pressed, true)

        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": true }],
                                ["checkedChanged", { "pressed": false, "checked": false }],
                                "toggled",
                                "released",
                                "clicked"]
        touch.release(0, control, control.width).commit()
        compare(control.position, 0.0)
        compare(control.checked, false)
        compare(control.pressed, false)
        verify(spy.success)

        // press-drag-release from and to outside the indicator
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                "pressed"]
        wait(Qt.styleHints.mouseDoubleClickInterval + 50)
        touch.press(0, control, control.width - 1).commit()
        compare(control.position, 0.0)
        compare(control.checked, false)
        compare(control.pressed, true)
        verify(spy.success)

        touch.move(0, control, control.width - control.rightPadding).commit()
        compare(control.position, 0.0)
        compare(control.checked, false)
        compare(control.pressed, true)

        touch.move(0, control, control.width / 2).commit()
        compare(control.position, 0.5)
        compare(control.checked, false)
        compare(control.pressed, true)

        touch.move(0, control, control.width - control.rightPadding).commit()
        compare(control.position, 1.0)
        compare(control.checked, false)
        compare(control.pressed, true)

        spy.expectedSequence = [["pressedChanged", { "pressed": false, "checked": false }],
                                ["checkedChanged", { "pressed": false, "checked": true }],
                                "toggled",
                                "released",
                                "clicked"]
        touch.release(0, control, control.width).commit()
        compare(control.position, 1.0)
        compare(control.checked, true)
        compare(control.pressed, false)
        verify(spy.success)
    }

    function test_keys() {
        var control = createTemporaryObject(swtch, testCase)
        verify(control)

        control.forceActiveFocus()
        verify(control.activeFocus)

        // check
        var spy = signalSequenceSpy.createObject(control, {target: control})
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": false }],
                                "pressed",
                                ["pressedChanged", { "pressed": false, "checked": false }],
                                ["checkedChanged", { "pressed": false, "checked": true }],
                                "toggled",
                                "released",
                                "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, true)
        verify(spy.success)

        // uncheck
        spy.expectedSequence = [["pressedChanged", { "pressed": true, "checked": true }],
                                "pressed",
                                ["pressedChanged", { "pressed": false, "checked": true }],
                                ["checkedChanged", { "pressed": false, "checked": false }],
                                "toggled",
                                "released",
                                "clicked"]
        keyClick(Qt.Key_Space)
        compare(control.checked, false)
        verify(spy.success)

        // no change
        spy.expectedSequence = []
        // Not testing Key_Enter and Key_Return because QGnomeTheme uses them for
        // pressing buttons and the CI uses the QGnomeTheme platform theme.
        var keys = [Qt.Key_Escape, Qt.Key_Tab]
        for (var i = 0; i < keys.length; ++i) {
            keyClick(keys[i])
            compare(control.checked, false)
            verify(spy.success)
        }
    }

    Component {
        id: twoSwitches
        Item {
            property Switch sw1: Switch { id: sw1 }
            property Switch sw2: Switch { id: sw2; checked: sw1.checked; enabled: false }
        }
    }

    function test_binding() {
        var container = createTemporaryObject(twoSwitches, testCase)
        verify(container)

        compare(container.sw1.checked, false)
        compare(container.sw2.checked, false)

        container.sw1.checked = true
        compare(container.sw1.checked, true)
        compare(container.sw2.checked, true)

        container.sw1.checked = false
        compare(container.sw1.checked, false)
        compare(container.sw2.checked, false)
    }

    function test_baseline() {
        var control = createTemporaryObject(swtch, testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
    }

    function test_focus() {
        var control = createTemporaryObject(swtch, testCase)
        verify(control)

        verify(!control.activeFocus)
        mouseClick(control.indicator)
        // should not get activeFocus on mouseClick on macOS
        compare(control.activeFocus, Qt.platform.os !== "osx" && Qt.platform.os !== "macos")
    }

    Component {
        id: deletionOrder1
        Item {
            Image { id: innerImage }
            Switch { indicator: innerImage }
        }
    }

    Component {
        id: deletionOrder2
        Item {
            Switch { indicator: innerImage }
            Image { id: innerImage }
        }
    }

    function test_deletionOrder() {
        var control1 = createTemporaryObject(deletionOrder1, testCase)
        verify(control1)
        var control2 = createTemporaryObject(deletionOrder2, testCase)
        verify(control2)
    }
}
