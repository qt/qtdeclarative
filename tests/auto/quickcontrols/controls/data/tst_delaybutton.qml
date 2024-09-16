// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "DelayButton"

    Component {
        id: defaultComponent

        DelayButton {}
    }

    Component {
        id: delayButton
        DelayButton {
            delay: 200
        }
    }

    Component {
        id: signalSequenceSpy
        SignalSequenceSpy {
            signals: ["pressed", "released", "canceled", "clicked", "toggled", "doubleClicked", "pressedChanged", "downChanged", "checkedChanged", "activated"]
        }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(defaultComponent, testCase)
        verify(control)
    }

    function test_mouse() {
        let control = createTemporaryObject(delayButton, testCase)
        verify(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        // click
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        "released",
                                        "clicked"]
        mouseClick(control)
        verify(sequenceSpy.success)

        // check
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        "activated"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        tryVerify(function() { return sequenceSpy.success})

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        ["checkedChanged", { "checked": true }],
                                        "released",
                                        "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // uncheck
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        ["checkedChanged", { "checked": false }],
                                        "released",
                                        "clicked"]
        mouseRelease(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // release outside
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed"]
        mousePress(control, control.width / 2, control.height / 2, Qt.LeftButton)
        compare(control.pressed, true)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }]]
        mouseMove(control, control.width * 2, control.height * 2, 0)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["canceled", { "pressed": false }]]
        mouseRelease(control, control.width * 2, control.height * 2, Qt.LeftButton)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // right button
        sequenceSpy.expectedSequence = []
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.pressed, false)

        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // double click
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        "released",
                                        "clicked",
                                        ["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        "doubleClicked",
                                        ["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        "released"]
        mouseDoubleClickSequence(control, control.width / 2, control.height / 2, Qt.LeftButton)
        verify(sequenceSpy.success)
    }

    function test_touch() {
        let control = createTemporaryObject(delayButton, testCase)
        verify(control)

        let touch = touchEvent(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        // click
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        "released",
                                        "clicked"]
        touch.press(0, control).commit()
        touch.release(0, control).commit()
        verify(sequenceSpy.success)

        // check
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        "activated"]
        // Don't want to double-click.
        wait(Application.styleHints.mouseDoubleClickInterval + 50)
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        tryVerify(function() { return sequenceSpy.success})

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        ["checkedChanged", { "checked": true }],
                                        "released",
                                        "clicked"]
        touch.release(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // uncheck
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed"]
        wait(Application.styleHints.mouseDoubleClickInterval + 50)
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        ["checkedChanged", { "checked": false }],
                                        "released",
                                        "clicked"]
        touch.release(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        // release outside
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed"]
        wait(Application.styleHints.mouseDoubleClickInterval + 50)
        touch.press(0, control, control.width / 2, control.height / 2).commit()
        compare(control.pressed, true)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }]]
        touch.move(0, control, control.width * 2, control.height * 2).commit()
        compare(control.pressed, false)
        verify(sequenceSpy.success)

        sequenceSpy.expectedSequence = [["canceled", { "pressed": false }]]
        touch.release(0, control, control.width * 2, control.height * 2).commit()
        compare(control.pressed, false)
        verify(sequenceSpy.success)
    }

    function test_keys() {
        let control = createTemporaryObject(delayButton, testCase)
        verify(control)

        control.forceActiveFocus()
        verify(control.activeFocus)

        let sequenceSpy = signalSequenceSpy.createObject(control, {target: control})

        // click
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        verify(sequenceSpy.success)

        // check
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        "activated"]
        keyPress(Qt.Key_Space)
        tryVerify(function() { return sequenceSpy.success})

        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        ["checkedChanged", { "checked": true }],
                                        "released",
                                        "clicked"]
        keyRelease(Qt.Key_Space)
        verify(sequenceSpy.success)

        // uncheck
        sequenceSpy.expectedSequence = [["pressedChanged", { "pressed": true }],
                                        ["downChanged", { "down": true }],
                                        "pressed",
                                        ["pressedChanged", { "pressed": false }],
                                        ["downChanged", { "down": false }],
                                        ["checkedChanged", { "checked": false }],
                                        "released",
                                        "clicked"]
        keyClick(Qt.Key_Space)
        verify(sequenceSpy.success)

        // no change
        sequenceSpy.expectedSequence = []
        // Not testing Key_Enter and Key_Return because QGnomeTheme uses them for
        // pressing buttons and the CI uses the QGnomeTheme platform theme.
        let keys = [Qt.Key_Escape, Qt.Key_Tab]
        for (let i = 0; i < keys.length; ++i) {
            sequenceSpy.reset()
            keyClick(keys[i])
            verify(sequenceSpy.success)
        }
    }

    function test_progress() {
        let control = createTemporaryObject(delayButton, testCase)
        verify(control)

        let progressSpy = signalSpy.createObject(control, {target: control, signalName: "progressChanged"})
        verify(progressSpy.valid)

        compare(control.progress, 0.0)
        mousePress(control)
        tryCompare(control, "progress", 1.0)
        verify(progressSpy.count > 0)
    }

    function test_baseline() {
        let control = createTemporaryObject(delayButton, testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
    }
}
