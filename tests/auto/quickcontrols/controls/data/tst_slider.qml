// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "Slider"

    Component {
        id: signalSpy
        SignalSpy { }
    }

    Component {
        id: slider
        Slider { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(slider, testCase)
        verify(control)

        compare(control.stepSize, 0)
        compare(control.snapMode, Slider.NoSnap)
        compare(control.orientation, Qt.Horizontal)
        compare(control.horizontal, true)
        compare(control.vertical, false)
    }

    function test_value() {
        let control = createTemporaryObject(slider, testCase)
        verify(control)

        compare(control.value, 0.0)
        control.value = 0.5
        compare(control.value, 0.5)
        control.value = 1.0
        compare(control.value, 1.0)
        control.value = -1.0
        compare(control.value, 0.0)
        control.value = 2.0
        compare(control.value, 1.0)
    }

    function test_range() {
        let control = createTemporaryObject(slider, testCase, {from: 0, to: 100, value: 50})
        verify(control)

        compare(control.from, 0)
        compare(control.to, 100)
        compare(control.value, 50)
        compare(control.position, 0.5)

        control.value = 1000
        compare(control.value, 100)
        compare(control.position, 1)

        control.value = -1
        compare(control.value, 0)
        compare(control.position, 0)

        control.from = 25
        compare(control.from, 25)
        compare(control.value, 25)
        compare(control.position, 0)

        control.to = 75
        compare(control.to, 75)
        compare(control.value, 25)
        compare(control.position, 0)

        control.value = 50
        compare(control.value, 50)
        compare(control.position, 0.5)
    }

    function test_inverted() {
        let control = createTemporaryObject(slider, testCase, {from: 1.0, to: -1.0})
        verify(control)

        compare(control.from, 1.0)
        compare(control.to, -1.0)
        compare(control.value, 0.0)
        compare(control.position, 0.5)

        control.value = 2.0
        compare(control.value, 1.0)
        compare(control.position, 0.0)

        control.value = -2.0
        compare(control.value, -1.0)
        compare(control.position, 1.0)

        control.value = 0.0
        compare(control.value, 0.0)
        compare(control.position, 0.5)
    }

    function test_position() {
        let control = createTemporaryObject(slider, testCase)
        verify(control)

        compare(control.value, 0.0)
        compare(control.position, 0.0)

        control.value = 0.25
        compare(control.value, 0.25)
        compare(control.position, 0.25)

        control.value = 0.75
        compare(control.value, 0.75)
        compare(control.position, 0.75)
    }

    function test_visualPosition() {
        let control = createTemporaryObject(slider, testCase)
        verify(control)

        compare(control.value, 0.0)
        compare(control.visualPosition, 0.0)

        control.value = 0.25
        compare(control.value, 0.25)
        compare(control.visualPosition, 0.25)

        // RTL locale
        control.locale = Qt.locale("ar_EG")
        compare(control.visualPosition, 0.25)

        // RTL locale + LayoutMirroring
        control.LayoutMirroring.enabled = true
        compare(control.visualPosition, 0.75)

        // LTR locale + LayoutMirroring
        control.locale = Qt.locale("en_US")
        compare(control.visualPosition, 0.75)

        // LTR locale
        control.LayoutMirroring.enabled = false
        compare(control.visualPosition, 0.25)

        // LayoutMirroring
        control.LayoutMirroring.enabled = true
        compare(control.visualPosition, 0.75)
    }

    function test_floatingPointPrecision() {
        let control = createTemporaryObject(slider, testCase, {
            from: 0, to: 1, stepSize: 0.1, value: 0.0
        })
        verify(control)

        compare(control.value, 0.0)
        compare(control.position, 0.0)

        // programmatically increase and decrease the value
        for (let i = 1; i < 4; i++) {
            control.increase()
            compare(control.value, i * 0.1)
            compare(control.position, i * 0.1)
        }
        let value = control.value
        for (let i = 1; i < 4; i++) {
            control.decrease()
            compare(control.value, value - i * 0.1)
            compare(control.position, value - i * 0.1)
        }
        // Compare as strings to avoid a fuzzy compare; we want an exact match.
        compare("" + control.value, "" + 0)
        compare("" + control.position, "" + 0)

        // small delta should not change the value
        let smallDelta = 1e-13
        control.value = 0.5 + smallDelta
        compare(control.value, 0.5)
        compare(control.position, 0.5)

        // set a value very close to 0
        control.value = 1e-12
        compare("" + control.value, "" + 0)
        compare("" + control.position, "" + 0)

        // set value to 0
        control.value = 0
        compare("" + control.value, "" + 0)
        compare("" + control.position, "" + 0)
    }

    function test_orientation() {
        let control = createTemporaryObject(slider, testCase)
        verify(control)

        compare(control.orientation, Qt.Horizontal)
        compare(control.horizontal, true)
        compare(control.vertical, false)
        verify(control.width > control.height)

        control.orientation = Qt.Vertical
        compare(control.orientation, Qt.Vertical)
        compare(control.horizontal, false)
        compare(control.vertical, true)
        verify(control.width < control.height)
    }

    function test_mouse_data() {
        return [
            { tag: "horizontal", orientation: Qt.Horizontal, live: false },
            { tag: "vertical", orientation: Qt.Vertical, live: false },
            { tag: "horizontal:live", orientation: Qt.Horizontal, live: true },
            { tag: "vertical:live", orientation: Qt.Vertical, live: true }
        ]
    }

    function test_mouse(data) {
        let control = createTemporaryObject(slider, testCase, {orientation: data.orientation, live: data.live})
        verify(control)

        let pressedCount = 0
        let movedCount = 0

        let pressedSpy = signalSpy.createObject(control, {target: control, signalName: "pressedChanged"})
        verify(pressedSpy.valid)

        let movedSpy = signalSpy.createObject(control, {target: control, signalName: "moved"})
        verify(movedSpy.valid)

        mousePress(control, 0, control.height - 1, Qt.LeftButton)
        compare(pressedSpy.count, ++pressedCount)
        compare(movedSpy.count, movedCount)
        compare(control.pressed, true)
        compare(control.value, 0.0)
        compare(control.position, 0.0)

        // mininum on the left in horizontal vs. at the bottom in vertical
        mouseMove(control, -control.width, 2 * control.height, 0)
        compare(pressedSpy.count, pressedCount)
        compare(movedSpy.count, movedCount)
        compare(control.pressed, true)
        compare(control.value, 0.0)
        compare(control.position, 0.0)

        mouseMove(control, control.width * 0.5, control.height * 0.5, 0)
        compare(pressedSpy.count, pressedCount)
        compare(movedSpy.count, ++movedCount)
        compare(control.pressed, true)
        compare(control.value, data.live ? 0.5 : 0.0)
        compare(control.position, 0.5)

        mouseRelease(control, control.width * 0.5, control.height * 0.5, Qt.LeftButton)
        compare(pressedSpy.count, ++pressedCount)
        compare(movedSpy.count, movedCount)
        compare(control.pressed, false)
        compare(control.value, 0.5)
        compare(control.position, 0.5)

        mousePress(control, control.width - 1, 0, Qt.LeftButton)
        compare(pressedSpy.count, ++pressedCount)
        compare(movedSpy.count, ++movedCount)
        compare(control.pressed, true)
        compare(control.value, data.live ? 1.0 : 0.5)
        compare(control.position, 1.0)

        // maximum on the right in horizontal vs. at the top in vertical
        mouseMove(control, control.width * 2, -control.height, 0)
        compare(pressedSpy.count, pressedCount)
        compare(movedSpy.count, movedCount)
        compare(control.pressed, true)
        compare(control.value, data.live ? 1.0 : 0.5)
        compare(control.position, 1.0)

        mouseMove(control, control.width * 0.75, control.height * 0.25, 0)
        compare(pressedSpy.count, pressedCount)
        compare(movedSpy.count, ++movedCount)
        compare(control.pressed, true)
        compare(control.value, data.live ? control.position : 0.5)
        verify(control.position >= 0.75)

        mouseRelease(control, control.width * 0.25, control.height * 0.75, Qt.LeftButton)
        compare(pressedSpy.count, ++pressedCount)
        compare(movedSpy.count, ++movedCount)
        compare(control.pressed, false)
        compare(control.value, control.position)
        verify(control.value <= 0.25 && control.value >= 0.0)
        verify(control.position <= 0.25 && control.position >= 0.0)

        // QTBUG-53846
        mouseClick(control, control.width * 0.5, control.height * 0.5, Qt.LeftButton)
        compare(movedSpy.count, ++movedCount)
        compare(pressedSpy.count, pressedCount += 2)
        compare(control.value, 0.5)
        compare(control.position, 0.5)
    }

    function test_touch_data() {
        return [
            { tag: "horizontal", orientation: Qt.Horizontal, live: false },
            { tag: "vertical", orientation: Qt.Vertical, live: false },
            { tag: "horizontal:live", orientation: Qt.Horizontal, live: true },
            { tag: "vertical:live", orientation: Qt.Vertical, live: true }
        ]
    }

    function test_touch(data) {
        let control = createTemporaryObject(slider, testCase, {orientation: data.orientation, live: data.live})
        verify(control)

        let pressedCount = 0
        let movedCount = 0

        let pressedSpy = signalSpy.createObject(control, {target: control, signalName: "pressedChanged"})
        verify(pressedSpy.valid)

        let movedSpy = signalSpy.createObject(control, {target: control, signalName: "moved"})
        verify(movedSpy.valid)

        let touch = touchEvent(control)
        touch.press(0, control, 0, 0).commit()
        compare(pressedSpy.count, ++pressedCount)
        compare(movedSpy.count, movedCount)
        compare(control.pressed, true)
        compare(control.value, 0.0)
        compare(control.position, 0.0)

        // mininum on the left in horizontal vs. at the bottom in vertical
        touch.move(0, control, -control.width, 2 * control.height, 0).commit()
        compare(pressedSpy.count, pressedCount)
        compare(movedSpy.count, movedCount)
        compare(control.pressed, true)
        compare(control.value, 0.0)
        compare(control.position, 0.0)

        touch.move(0, control, control.width * 0.5, control.height * 0.5, 0).commit()
        compare(pressedSpy.count, pressedCount)
        compare(movedSpy.count, ++movedCount)
        compare(control.pressed, true)
        compare(control.value, data.live ? 0.5 : 0.0)
        compare(control.position, 0.5)

        touch.release(0, control, control.width * 0.5, control.height * 0.5).commit()
        compare(pressedSpy.count, ++pressedCount)
        compare(movedSpy.count, movedCount)
        compare(control.pressed, false)
        compare(control.value, 0.5)
        compare(control.position, 0.5)

        touch.press(0, control, control.width - 1, control.height - 1).commit()
        compare(pressedSpy.count, ++pressedCount)
        compare(movedSpy.count, movedCount)
        compare(control.pressed, true)
        compare(control.value, 0.5)
        compare(control.position, 0.5)

        // maximum on the right in horizontal vs. at the top in vertical
        touch.move(0, control, control.width * 2, -control.height, 0).commit()
        compare(pressedSpy.count, pressedCount)
        compare(movedSpy.count, ++movedCount)
        compare(control.pressed, true)
        compare(control.value, data.live ? 1.0 : 0.5)
        compare(control.position, 1.0)

        touch.move(0, control, control.width * 0.75, control.height * 0.25, 0).commit()
        compare(pressedSpy.count, pressedCount)
        compare(movedSpy.count, ++movedCount)
        compare(control.pressed, true)
        compare(control.value, data.live ? control.position : 0.5)
        verify(control.position >= 0.75)

        touch.release(0, control, control.width * 0.25, control.height * 0.75).commit()
        compare(pressedSpy.count, ++pressedCount)
        compare(movedSpy.count, ++movedCount)
        compare(control.pressed, false)
        compare(control.value, control.position)
        verify(control.value <= 0.25 && control.value >= 0.0)
        verify(control.position <= 0.25 && control.position >= 0.0)

        // QTBUG-53846
        touch.press(0, control).commit().release(0, control).commit()
        compare(movedSpy.count, ++movedCount)
        compare(pressedSpy.count, pressedCount += 2)
        compare(control.value, 0.5)
        compare(control.position, 0.5)
    }

    function test_multiTouch() {
        let control1 = createTemporaryObject(slider, testCase, {live: false})
        verify(control1)

        let pressedCount1 = 0
        let movedCount1 = 0

        let pressedSpy1 = signalSpy.createObject(control1, {target: control1, signalName: "pressedChanged"})
        verify(pressedSpy1.valid)

        let movedSpy1 = signalSpy.createObject(control1, {target: control1, signalName: "moved"})
        verify(movedSpy1.valid)

        let touch = touchEvent(control1)
        touch.press(0, control1, 0, 0).commit().move(0, control1, control1.width, control1.height).commit()

        compare(pressedSpy1.count, ++pressedCount1)
        compare(movedSpy1.count, ++movedCount1)
        compare(control1.pressed, true)
        compare(control1.position, 1.0)

        // second touch point on the same control is ignored
        touch.stationary(0).press(1, control1, 0, 0).commit()
        touch.stationary(0).move(1, control1).commit()
        touch.stationary(0).release(1).commit()

        compare(pressedSpy1.count, pressedCount1)
        compare(movedSpy1.count, movedCount1)
        compare(control1.pressed, true)
        compare(control1.position, 1.0)

        let control2 = createTemporaryObject(slider, testCase, {y: control1.height, live: false})
        verify(control2)

        let pressedCount2 = 0
        let movedCount2 = 0

        let pressedSpy2 = signalSpy.createObject(control2, {target: control2, signalName: "pressedChanged"})
        verify(pressedSpy2.valid)

        let movedSpy2 = signalSpy.createObject(control2, {target: control2, signalName: "moved"})
        verify(movedSpy2.valid)

        // press the second slider
        touch.stationary(0).press(2, control2, 0, 0).commit()

        compare(pressedSpy2.count, ++pressedCount2)
        compare(movedSpy2.count, movedCount2)
        compare(control2.pressed, true)
        compare(control2.position, 0.0)

        compare(pressedSpy1.count, pressedCount1)
        compare(movedSpy1.count, movedCount1)
        compare(control1.pressed, true)
        compare(control1.position, 1.0)

        // move both sliders
        touch.move(0, control1).move(2, control2).commit()

        compare(pressedSpy2.count, pressedCount2)
        compare(movedSpy2.count, ++movedCount2)
        compare(control2.pressed, true)
        compare(control2.position, 0.5)
        compare(control2.value, 0.0)

        compare(pressedSpy1.count, pressedCount1)
        compare(movedSpy1.count, ++movedCount1)
        compare(control1.pressed, true)
        compare(control1.position, 0.5)
        compare(control1.value, 0.0)

        // release both sliders
        touch.release(0, control1).release(2, control2).commit()

        compare(pressedSpy2.count, ++pressedCount2)
        compare(movedSpy2.count, movedCount2)
        compare(control2.pressed, false)
        compare(control2.position, 0.5)
        compare(control2.value, 0.5)

        compare(pressedSpy1.count, ++pressedCount1)
        compare(movedSpy1.count, movedCount1)
        compare(control1.pressed, false)
        compare(control1.position, 0.5)
        compare(control1.value, 0.5)
    }

    function test_keys_data() {
        return [
            { tag: "horizontal", orientation: Qt.Horizontal, decrease: Qt.Key_Left, increase: Qt.Key_Right },
            { tag: "vertical", orientation: Qt.Vertical, decrease: Qt.Key_Down, increase: Qt.Key_Up }
        ]
    }

    function test_keys(data) {
        let control = createTemporaryObject(slider, testCase, {orientation: data.orientation})
        verify(control)

        let pressedCount = 0
        let movedCount = 0

        let pressedSpy = signalSpy.createObject(control, {target: control, signalName: "pressedChanged"})
        verify(pressedSpy.valid)

        let movedSpy = signalSpy.createObject(control, {target: control, signalName: "moved"})
        verify(movedSpy.valid)

        control.forceActiveFocus()
        verify(control.activeFocus)

        let oldValue = 0.0
        control.value = 0.5

        for (let d1 = 1; d1 <= 10; ++d1) {
            oldValue = control.value
            keyPress(data.decrease)
            compare(control.pressed, true)
            compare(pressedSpy.count, ++pressedCount)
            if (oldValue !== control.value)
                compare(movedSpy.count, ++movedCount)

            compare(control.value, Math.max(0.0, 0.5 - d1 * 0.1))
            compare(control.value, control.position)

            keyRelease(data.decrease)
            compare(control.pressed, false)
            compare(pressedSpy.count, ++pressedCount)
            compare(movedSpy.count, movedCount)
        }

        for (let i1 = 1; i1 <= 20; ++i1) {
            oldValue = control.value
            keyPress(data.increase)
            compare(control.pressed, true)
            compare(pressedSpy.count, ++pressedCount)
            if (oldValue !== control.value)
                compare(movedSpy.count, ++movedCount)

            compare(control.value, Math.min(1.0, 0.0 + i1 * 0.1))
            compare(control.value, control.position)

            keyRelease(data.increase)
            compare(control.pressed, false)
            compare(pressedSpy.count, ++pressedCount)
            compare(movedSpy.count, movedCount)
        }

        control.stepSize = 0.25

        for (let d2 = 1; d2 <= 10; ++d2) {
            oldValue = control.value
            keyPress(data.decrease)
            compare(control.pressed, true)
            compare(pressedSpy.count, ++pressedCount)
            if (oldValue !== control.value)
                compare(movedSpy.count, ++movedCount)

            compare(control.value, Math.max(0.0, 1.0 - d2 * 0.25))
            compare(control.value, control.position)

            keyRelease(data.decrease)
            compare(control.pressed, false)
            compare(pressedSpy.count, ++pressedCount)
            compare(movedSpy.count, movedCount)
        }

        for (let i2 = 1; i2 <= 10; ++i2) {
            oldValue = control.value
            keyPress(data.increase)
            compare(control.pressed, true)
            compare(pressedSpy.count, ++pressedCount)
            if (oldValue !== control.value)
                compare(movedSpy.count, ++movedCount)

            compare(control.value, Math.min(1.0, 0.0 + i2 * 0.25))
            compare(control.value, control.position)

            keyRelease(data.increase)
            compare(control.pressed, false)
            compare(pressedSpy.count, ++pressedCount)
            compare(movedSpy.count, movedCount)
        }
    }

    function test_padding() {
        // test with "unbalanced" paddings (left padding != right padding) to ensure
        // that the slider position calculation is done taking padding into account
        // ==> the position is _not_ 0.5 in the middle of the control
        let control = createTemporaryObject(slider, testCase, {leftPadding: 10, rightPadding: 20, live: false})
        verify(control)

        let pressedSpy = signalSpy.createObject(control, {target: control, signalName: "pressedChanged"})
        verify(pressedSpy.valid)

        mousePress(control, 0, 0, Qt.LeftButton)
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        compare(control.value, 0.0)
        compare(control.position, 0.0)
        compare(control.visualPosition, 0.0)

        mouseMove(control, control.leftPadding + control.availableWidth * 0.5, control.height * 0.5, 0)
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        compare(control.value, 0.0)
        compare(control.position, 0.5)
        compare(control.visualPosition, 0.5)

        mouseMove(control, control.width * 0.5, control.height * 0.5, 0)
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        compare(control.value, 0.0)
        verify(control.position > 0.5)
        verify(control.visualPosition > 0.5)

        mouseRelease(control, control.leftPadding + control.availableWidth * 0.5, control.height * 0.5, Qt.LeftButton)
        compare(pressedSpy.count, 2)
        compare(control.pressed, false)
        compare(control.value, 0.5)
        compare(control.position, 0.5)
        compare(control.visualPosition, 0.5)

        // RTL
        control.value = 0
        control.locale = Qt.locale("ar_EG")

        mousePress(control, 0, 0, Qt.LeftButton)
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        compare(control.value, 0.0)
        compare(control.position, 0.0)
        compare(control.visualPosition, 0.0)

        mouseMove(control, control.leftPadding + control.availableWidth * 0.5, control.height * 0.5, 0)
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        compare(control.value, 0.0)
        compare(control.position, 0.5)
        compare(control.visualPosition, 0.5)

        mouseMove(control, control.width * 0.5, control.height * 0.5, 0)
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        compare(control.value, 0.0)
        verify(control.position > 0.5)
        verify(control.visualPosition > 0.5)

        mouseRelease(control, control.leftPadding + control.availableWidth * 0.5, control.height * 0.5, Qt.LeftButton)
        compare(pressedSpy.count, 4)
        compare(control.pressed, false)
        compare(control.value, 0.5)
        compare(control.position, 0.5)
        compare(control.visualPosition, 0.5)
    }

    function calcMousePos(control, t) {
        t = Math.min(Math.max(t, 0.0), 1.0);
        return control.leftPadding + control.handle.width * 0.5 + t * (control.availableWidth - control.handle.width)
    }

    function snapModeData(immediate) {
        return [
            { tag: "NoSnap", snapMode: Slider.NoSnap, from: 0, to: 2, values: [0, 0, 0.25], positions: [0, 0.1, 0.1] },
            { tag: "SnapAlways (0..2)", snapMode: Slider.SnapAlways, from: 0, to: 2, values: [0.0, 0.0, 0.2], positions: [0.0, 0.1, 0.1] },
            { tag: "SnapAlways (1..3)", snapMode: Slider.SnapAlways, from: 1, to: 3, values: [1.0, 1.0, 1.2], positions: [0.0, 0.1, 0.1] },
            { tag: "SnapAlways (-1..1)", snapMode: Slider.SnapAlways, from: -1, to: 1, values: [0.0, 0.0, -0.8], positions: [immediate ? 0.0 : 0.5, 0.1, 0.1] },
            { tag: "SnapAlways (1..-1)", snapMode: Slider.SnapAlways, from: 1, to: -1, values: [0.0, 0.0,  0.8], positions: [immediate ? 0.0 : 0.5, 0.1, 0.1] },
            { tag: "SnapOnRelease (0..2)", snapMode: Slider.SnapOnRelease, from: 0, to: 2, values: [0.0, 0.0, 0.2], positions: [0.0, 0.1, 0.1] },
            { tag: "SnapOnRelease (1..3)", snapMode: Slider.SnapOnRelease, from: 1, to: 3, values: [1.0, 1.0, 1.2], positions: [0.0, 0.1, 0.1] },
            { tag: "SnapOnRelease (-1..1)", snapMode: Slider.SnapOnRelease, from: -1, to: 1, values: [0.0, 0.0, -0.8], positions: [immediate ? 0.0 : 0.5, 0.1, 0.1] },
            { tag: "SnapOnRelease (1..-1)", snapMode: Slider.SnapOnRelease, from: 1, to: -1, values: [0.0, 0.0,  0.8], positions: [immediate ? 0.0 : 0.5, 0.1, 0.1] },
            // Live
            { tag: "SnapAlwaysLive1", snapMode: Slider.SnapAlways, from: 0, to: 1, value: 0, stepSize: 1, live: true, sliderPos: 0.6, values: [0, 1, 1], positions: [0, 1, 1] },
            { tag: "SnapAlwaysLive2", snapMode: Slider.SnapAlways, from: 0, to: 1, value: 0, stepSize: 1, live: true, sliderPos: 0.4, values: [0, 0, 0], positions: [0, 0, 0] },
            { tag: "NoSnapLive1", snapMode: Slider.NoSnap, from: 0, to: 1, value: 0, stepSize: 1, live: true, sliderPos: 0.6, values: [0, 1, 1], positions: [0, 0.6, 0.6] },
            { tag: "NoSnapLive2", snapMode: Slider.NoSnap, from: 0, to: 1, value: 0, stepSize: 1, live: true, sliderPos: 0.4, values: [0, 0, 0], positions: [0, 0.4, 0.4] },
            { tag: "SnapOnReleaseLive1", snapMode: Slider.SnapOnRelease, from: 0, to: 1, value: 0, stepSize: 1, live: true, sliderPos: 0.6, values: [0, 1, 1], positions: [0, 0.6, 1] },
            { tag: "SnapOnReleaseLive2", snapMode: Slider.SnapOnRelease, from: 0, to: 1, value: 0, stepSize: 1, live: true, sliderPos: 0.4, values: [0, 0, 0], positions: [0, 0.4, 0] },
        ]
    }

    function testSnapMode(data, useMouse) {
        let live = data.live !== undefined ? data.live : false
        let stepSize = data.stepSize !== undefined ? data.stepSize : 0.2
        let sliderPos = data.sliderPos !== undefined ? data.sliderPos : 0.1
        let fuzz = 0.05

        let control = createTemporaryObject(slider, testCase, {live: live, snapMode: data.snapMode, from: data.from, to: data.to, stepSize: stepSize})
        verify(control)

        // The test assumes there is no drag threshold for neither mouse or touch.
        // But by default, touch has a threshold and mouse doesn't.
        // In order to not get a test fail if we're trying to move the slider handle
        // on a very narrow slider, we ensure the same width of all sliders
        control.width = testCase.width

        let touch = useMouse ? null : touchEvent(control)

        if (useMouse)
            mousePress(control, calcMousePos(control, 0.0))
        else
            touch.press(0, control, calcMousePos(control, 0.0)).commit()

        fuzzyCompare(control.value, data.values[0], fuzz)
        fuzzyCompare(control.position, data.positions[0], fuzz)

        if (useMouse)
            mouseMove(control, calcMousePos(control, sliderPos))
        else
            touch.move(0, control, calcMousePos(control, sliderPos)).commit()

        fuzzyCompare(control.value, data.values[1], fuzz)
        fuzzyCompare(control.position, data.positions[1], fuzz)

        if (useMouse)
            mouseRelease(control, calcMousePos(control, sliderPos))
        else
            touch.release(0, control, calcMousePos(control, sliderPos)).commit()

        fuzzyCompare(control.value, data.values[2], fuzz)
        fuzzyCompare(control.position, data.positions[2], fuzz)
    }

    function test_snapMode_touch_data() {
        return snapModeData(false)
    }

    function test_snapMode_touch(data) {
        return testSnapMode(data, false)
    }

    function test_snapMode_mouse_data() {
        return snapModeData(true)
    }

    function test_snapMode_mouse(data) {
        return testSnapMode(data, true)
    }

    function test_wheel_data() {
        return [
            { tag: "horizontal", orientation: Qt.Horizontal, dx: 120, dy: 0 },
            { tag: "vertical", orientation: Qt.Vertical, dx: 0, dy: 120 },
            { tag: "mostlyHorizontal", orientation: Qt.Horizontal, dx: 120, dy: 1 },
            { tag: "mostlyVertical", orientation: Qt.Vertical, dx: 1, dy: 120 },
        ]
    }

    function test_wheel(data) {
        let control = createTemporaryObject(slider, testCase, {wheelEnabled: true, orientation: data.orientation})
        verify(control)

        let movedCount = 0
        let movedSpy = signalSpy.createObject(control, {target: control, signalName: "moved"})
        verify(movedSpy.valid)

        compare(control.value, 0.0)

        mouseWheel(control, control.width / 2, control.height / 2, data.dx, data.dy)
        compare(movedSpy.count, ++movedCount)
        compare(control.value, 0.1)
        compare(control.position, 0.1)

        control.stepSize = 0.2

        mouseWheel(control, control.width / 2, control.height / 2, data.dx, data.dy)
        compare(movedSpy.count, ++movedCount)
        compare(control.value, 0.3)
        compare(control.position, 0.3)

        control.stepSize = 10.0

        mouseWheel(control, control.width / 2, control.height / 2, -data.dx, -data.dy)
        compare(movedSpy.count, ++movedCount)
        compare(control.value, 0.0)
        compare(control.position, 0.0)

        // no change
        mouseWheel(control, control.width / 2, control.height / 2, -data.dx, -data.dy)
        compare(movedSpy.count, movedCount)
        compare(control.value, 0.0)
        compare(control.position, 0.0)

        control.to = 10.0
        control.stepSize = 5.0

        mouseWheel(control, control.width / 2, control.height / 2, data.dx, data.dy)
        compare(movedSpy.count, ++movedCount)
        compare(control.value, 5.0)
        compare(control.position, 0.5)

        mouseWheel(control, control.width / 2, control.height / 2, 0.5 * data.dx, 0.5 * data.dy)
        compare(movedSpy.count, ++movedCount)
        compare(control.value, 7.5)
        compare(control.position, 0.75)

        mouseWheel(control, control.width / 2, control.height / 2, -data.dx, -data.dy)
        compare(movedSpy.count, ++movedCount)
        compare(control.value, 2.5)
        compare(control.position, 0.25)
    }

    function test_wheelPropagation_data() {
        return [
            { tag: "horizontal", orientation: Qt.Horizontal, dx: 120, dy: 0 },
            { tag: "vertical", orientation: Qt.Vertical, dx: 0, dy: 120 }
        ]
    }

    Component {
        id: mouseAreaComponent
        MouseArea {}
    }

    function test_wheelPropagation(data) {
        let mouseArea = createTemporaryObject(mouseAreaComponent, testCase, { width: parent.width, height: parent.height })
        verify(mouseArea)

        let mouseAreaWheelSpy = signalSpy.createObject(mouseArea, { target: mouseArea, signalName: "wheel" })
        verify(mouseAreaWheelSpy.valid)

        let control = createTemporaryObject(slider, mouseArea,
            { wheelEnabled: true, orientation: data.orientation, stepSize: 1 })
        verify(control)
        compare(control.value, 0.0)

        let movedCount = 0
        let movedSpy = signalSpy.createObject(control, { target: control, signalName: "moved" })
        verify(movedSpy.valid)

        // Scroll the handle to the edge.
        mouseWheel(control, control.width / 2, control.height / 2, data.dx, data.dy)
        compare(control.value, 1.0)
        compare(control.position, 1.0)
        compare(movedSpy.count, ++movedCount)
        compare(mouseAreaWheelSpy.count, 0)

        // Scroll again; the wheel event shouldn't go through to the MouseArea parent.
        mouseWheel(control, control.width / 2, control.height / 2, data.dx, data.dy)
        compare(control.value, 1.0)
        compare(control.position, 1.0)
        compare(movedSpy.count, movedCount)
        compare(mouseAreaWheelSpy.count, 0)

        // Scroll the handle to the other edge.
        mouseWheel(control, control.width / 2, control.height / 2, -data.dx, -data.dy)
        compare(control.value, 0.0)
        compare(control.position, 0.0)
        compare(movedSpy.count, ++movedCount)
        compare(mouseAreaWheelSpy.count, 0)

        // Scroll again; the wheel event shouldn't go through to the MouseArea parent.
        mouseWheel(control, control.width / 2, control.height / 2, -data.dx, -data.dy)
        compare(control.value, 0.0)
        compare(control.position, 0.0)
        compare(movedSpy.count, movedCount)
        compare(mouseAreaWheelSpy.count, 0)
    }

    function test_valueAt_data() {
        return [
            { tag: "0.0..1.0", properties: { from: 0.0, to: 1.0 }, values: [0.0, 0.2, 0.5, 1.0] },
            { tag: "0..100", properties: { from: 0, to: 100 }, values: [0, 20, 50, 100] },
            { tag: "100..-100", properties: { from: 100, to: -100 }, values: [100, 60, 0, -100] },
            { tag: "-7..7", properties: { from: -7, to: 7, stepSize: 1.0 }, values: [-7.0, -4.0, 0.0, 7.0] },
            { tag: "-3..7", properties: { from: -3, to: 7, stepSize: 5.0 }, values: [-3.0, -3.0, 2.0, 7.0] },
        ]
    }

    function test_valueAt(data) {
        let control = createTemporaryObject(slider, testCase, data.properties)
        verify(control)

        compare(control.valueAt(0.0), data.values[0])
        compare(control.valueAt(0.2), data.values[1])
        compare(control.valueAt(0.5), data.values[2])
        compare(control.valueAt(1.0), data.values[3])
    }

    function test_nullHandle() {
        let control = createTemporaryObject(slider, testCase)
        verify(control)

        control.handle = null

        mousePress(control)
        verify(control.pressed, true)

        mouseRelease(control)
        compare(control.pressed, false)
    }

    function test_touchDragThreshold_data() {
        let d1 = 3; let d2 = 7;
        return [
            { tag: "horizontal", orientation: Qt.Horizontal, dx1: d1, dy1: 0, dx2: d2, dy2: 0 },
            { tag: "vertical", orientation: Qt.Vertical, dx1: 0, dy1: -d1, dx2: 0, dy2: -d2 },
            { tag: "horizontal2", orientation: Qt.Horizontal, dx1: -d1, dy1: 0, dx2: -d2, dy2: 0 },
            { tag: "vertical2", orientation: Qt.Vertical, dx1: 0, dy1: d1, dx2: 0, dy2: d2 }
        ]
    }

    function test_touchDragThreshold(data) {
        let control = createTemporaryObject(slider, testCase, {touchDragThreshold: 10, live: true, orientation: data.orientation, value: 0.5})
        verify(control)
        compare(control.touchDragThreshold, 10)

        let valueChangedCount = 0
        let valueChangedSpy = signalSpy.createObject(control, {target: control, signalName: "touchDragThresholdChanged"})
        verify(valueChangedSpy.valid)

        control.touchDragThreshold = undefined
        compare(control.touchDragThreshold, -1) // reset to -1
        compare(valueChangedSpy.count, ++valueChangedCount)

        let t = 5
        control.touchDragThreshold = t
        compare(control.touchDragThreshold, t)
        compare(valueChangedSpy.count, ++valueChangedCount)

        control.touchDragThreshold = t
        compare(control.touchDragThreshold, t)
        compare(valueChangedSpy.count, valueChangedCount)

        let pressedCount = 0
        let movedCount = 0

        let pressedSpy = signalSpy.createObject(control, {target: control, signalName: "pressedChanged"})
        verify(pressedSpy.valid)

        let movedSpy = signalSpy.createObject(control, {target: control, signalName: "moved"})
        verify(movedSpy.valid)

        let touch = touchEvent(control)
        let x0 = control.handle.x + control.handle.width * 0.5
        let y0 = control.handle.y + control.handle.height * 0.5
        touch.press(0, control, x0, y0).commit()
        compare(pressedSpy.count, ++pressedCount)
        compare(movedSpy.count, movedCount)
        compare(control.pressed, true)

        touch.move(0, control, x0 + data.dx1, y0 + data.dy1).commit()
        compare(pressedSpy.count, pressedCount)
        compare(movedSpy.count, movedCount) // shouldn't move
        compare(control.pressed, true)

        touch.move(0, control, x0 + data.dx2, y0 + data.dy2).commit()
        compare(pressedSpy.count, pressedCount)
        compare(movedSpy.count, ++movedCount)
        compare(control.pressed, true)

        touch.release(0, control, x0 + data.dx2, y0 + data.dy2).commit()
    }

    function test_nullTexture() {
        failOnWarning("No QSGTexture provided from updateSampledImage(). This is wrong.")
        let control = createTemporaryObject(slider, testCase, {width: -100})
        verify(control)
        control.visible = true
        waitForRendering(control)
    }

    function test_clickFocus() {
        let control = createTemporaryObject(slider, testCase)
        verify(control)

        mouseClick(control)
        if (Qt.platform.os === "osx")
            verify(!control.activeFocus)
        else
            verify(control.activeFocus)
    }
}
