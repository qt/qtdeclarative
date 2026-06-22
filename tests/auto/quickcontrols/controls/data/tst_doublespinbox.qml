// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "DoubleSpinBox"

    Component {
        id: signalSpy
        SignalSpy { }
    }

    Component {
        id: doubleSpinBox
        DoubleSpinBox { }
    }

    Component {
        id: mouseArea
        MouseArea { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(doubleSpinBox, testCase)
        verify(control)

        compare(control.from, 0.0)
        compare(control.to, 99.99)
        compare(control.value, 0.0)
        compare(control.stepSize, 1.0)
        compare(control.decimals, 2)
        compare(control.editable, false)
        compare(control.up.pressed, false)
        compare(control.up.indicator.enabled, true)
        compare(control.down.pressed, false)
        compare(control.down.indicator.enabled, false)
    }

    function test_value() {
        let control = createTemporaryObject(doubleSpinBox, testCase)
        verify(control)

        compare(control.value, 0)
        control.value = 50.5
        compare(control.value, 50.5)
        control.value = 99.99
        compare(control.value, 99.99)
        control.value = -99.99
        compare(control.value, 0)
        control.value = 100.0
        compare(control.value, 99.99)
    }

    function test_range() {
        let control = createTemporaryObject(doubleSpinBox, testCase, {from: 0.0, to: 100.0, value: 50.5})
        verify(control)

        compare(control.from, 0.0)
        compare(control.to, 100.0)
        compare(control.value, 50.5)
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, true)

        control.value = 1000
        compare(control.value, 100.0)
        compare(control.up.indicator.enabled, false)
        compare(control.down.indicator.enabled, true)

        control.wrap = true
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, true)

        control.value = -1
        compare(control.value, 0)
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, true)

        control.from = 15
        compare(control.from, 15)
        compare(control.value, 15)
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, true)

        control.wrap = false
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, false)

        control.value = 20
        compare(control.from, 15)
        compare(control.value, 20)
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, true)

        control.from = 20
        compare(control.from, 20)
        compare(control.value, 20)
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, false)

        control.to = 75
        compare(control.to, 75)
        compare(control.value, 20)
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, false)

        control.value = 50
        compare(control.value, 50)
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, true)

        control.to = 50
        compare(control.to, 50)
        compare(control.value, 50)
        compare(control.up.indicator.enabled, false)
        compare(control.down.indicator.enabled, true)

        control.to = 40
        compare(control.to, 40)
        compare(control.value, 40)
        compare(control.up.indicator.enabled, false)
        compare(control.down.indicator.enabled, true)

        control.value = 24.013
        compare(control.value, 24.01)
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, true)

        control.to = 30.013
        compare(control.to, 30.01)
        compare(control.value, 24.01)
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, true)

        control.from = 25.377
        compare(control.from, 25.38)
        compare(control.value, 25.38)
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, false)
    }

    function test_inverted() {
        let control = createTemporaryObject(doubleSpinBox, testCase, {from: 100, to: -100})
        verify(control)

        compare(control.from, 100)
        compare(control.to, -100)
        compare(control.value, 0)
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, true)

        control.value = 200
        compare(control.value, 100)
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, false)

        control.value = -200
        compare(control.value, -100)
        compare(control.up.indicator.enabled, false)
        compare(control.down.indicator.enabled, true)

        control.value = 0
        compare(control.value, 0)
        compare(control.up.indicator.enabled, true)
        compare(control.down.indicator.enabled, true)
    }

    function test_mouse_data() {
        return [
            { tag: "up", button: "up", value: 50, enabled: true, hold: false, modified: 1, stepSize: 0.1, expected: 50.1 },
            { tag: "down", button: "down", value: 50, enabled: true, hold: false, modified: 1, stepSize: 0.1, expected: 49.9 },
            { tag: "up:disabled", button: "up", value: 99.99, enabled: false, hold: false, modified: 0, stepSize: 0.1, expected: 99.99 },
            { tag: "down:disabled", button: "down", value: 0, enabled: false, hold: false, modified: 0, stepSize: 0.1, expected: 0 },
            { tag: "up:hold", button: "up", value: 95, enabled: true, hold: true, modified: 4, stepSize: 0.1, expected: 95.4 },
            { tag: "down:hold", button: "down", value: 5, enabled: true, hold: true, modified: 5, stepSize: 0.1, expected: 4.5 }
        ]
    }

    function test_mouse(data) {
        let control = createTemporaryObject(doubleSpinBox, testCase, {value: data.value, stepSize: data.stepSize})
        verify(control)

        let button = control[data.button]
        verify(button)

        let pressedSpy = signalSpy.createObject(control, {target: button, signalName: "pressedChanged"})
        verify(pressedSpy.valid)

        let valueModifiedSpy = signalSpy.createObject(control, {target: control, signalName: "valueModified"})
        verify(valueModifiedSpy.valid)

        mousePress(button.indicator)
        compare(pressedSpy.count, data.enabled ? 1 : 0)
        compare(button.pressed, data.enabled)
        compare(control.value, data.value)
        compare(valueModifiedSpy.count, 0)

        if (data.hold)
            tryCompare(control, "value", data.expected)

        mouseRelease(button.indicator)
        compare(pressedSpy.count, data.enabled ? 2 : 0)
        compare(button.pressed, false)
        compare(control.value, data.expected)
        compare(valueModifiedSpy.count, data.modified)
    }

    function test_keys_data() {
        return [
            { tag: "0.1", properties: { from: 1, to: 10, value: 1, stepSize: 0.1 }, upSteps: [1.1,1.2,1.3], downSteps: [1.2,1.1,1.0,1.0] },
            { tag: "0.2", properties: { from: 1, to: 10, value: 10, stepSize: 0.2 }, upSteps: [10,10], downSteps: [9.8,9.6,9.4] },
            { tag: "wrap0.1", properties: { wrap: true, from: 1, to: 10, value: 1, stepSize: 0.1 }, upSteps: [1.1,1.2], downSteps: [1.1,1.0,10,9.9] },
            { tag: "wrap25.5", properties: { wrap: true, from: 0, to: 99.5, value: 45, stepSize: 25.5 }, upSteps: [70.5,96.0,0,25.5], downSteps: [0,99.5,74] }
        ]
    }

    function test_keys(data) {
        let control = createTemporaryObject(doubleSpinBox, testCase, data.properties)
        verify(control)

        let upPressedCount = 0
        let downPressedCount = 0
        let valueModifiedCount = 0

        let upPressedSpy = signalSpy.createObject(control, {target: control.up, signalName: "pressedChanged"})
        verify(upPressedSpy.valid)

        let downPressedSpy = signalSpy.createObject(control, {target: control.down, signalName: "pressedChanged"})
        verify(downPressedSpy.valid)

        let valueModifiedSpy = signalSpy.createObject(control, {target: control, signalName: "valueModified"})
        verify(valueModifiedSpy.valid)

        control.forceActiveFocus()
        verify(control.activeFocus)

        for (let u = 0; u < data.upSteps.length; ++u) {
            let wasUpEnabled = control.wrap || control.value < control.to
            keyPress(Qt.Key_Up)
            compare(control.up.pressed, wasUpEnabled)
            compare(control.down.pressed, false)
            if (wasUpEnabled) {
                ++upPressedCount
                ++valueModifiedCount
            }
            compare(upPressedSpy.count, upPressedCount)
            compare(valueModifiedSpy.count, valueModifiedCount)

            compare(control.value, data.upSteps[u])

            keyRelease(Qt.Key_Up)
            compare(control.down.pressed, false)
            compare(control.up.pressed, false)
            if (wasUpEnabled)
                ++upPressedCount
            compare(upPressedSpy.count, upPressedCount)
            compare(valueModifiedSpy.count, valueModifiedCount)
        }

        for (let d = 0; d < data.downSteps.length; ++d) {
            let wasDownEnabled = control.wrap || control.value > control.from
            keyPress(Qt.Key_Down)
            compare(control.down.pressed, wasDownEnabled)
            compare(control.up.pressed, false)
            if (wasDownEnabled) {
                ++downPressedCount
                ++valueModifiedCount
            }
            compare(downPressedSpy.count, downPressedCount)
            compare(valueModifiedSpy.count, valueModifiedCount)

            compare(control.value, data.downSteps[d])

            keyRelease(Qt.Key_Down)
            compare(control.down.pressed, false)
            compare(control.up.pressed, false)
            if (wasDownEnabled)
                ++downPressedCount
            compare(downPressedSpy.count, downPressedCount)
            compare(valueModifiedSpy.count, valueModifiedCount)
        }
    }


    function test_decimals() {
        let control = createTemporaryObject(doubleSpinBox, testCase, data.properties)
        verify(control)

        control.locale = Qt.locale("en_EN") // forcing locale because double can be written differently because of current locale

        let valueChangedSpy = signalSpy.createObject(control, {target: control, signalName: "valueChanged"})
        verify(valueChangedSpy.valid)

        let valueModifiedSpy = signalSpy.createObject(control, {target: control, signalName: "valueModified"})
        verify(valueModifiedSpy.valid)

        let displayTextChangedSpy = signalSpy.createObject(control, {target: control, signalName: "displayTextChanged"})
        verify(displayTextChangedSpy.valid)

        control.from = 0
        control.to = 5
        compare(control.from, 0)
        compare(control.to, 5)

        compare(control.displayText, "0.00")

        control.decimals = 3
        compare(control.displayText, "0.000")
        compare(control.value, 0)
        compare(displayTextChangedSpy.count, 1)
        compare(valueModifiedSpy.count, 0)
        compare(valueChangedSpy.count, 0)

        control.value = 0.004
        compare(control.displayText, "0.004")
        compare(control.value, 0.004)
        compare(displayTextChangedSpy.count, 2)
        compare(valueModifiedSpy.count, 0)
        compare(valueChangedSpy.count, 1)

        control.decimals = 2
        compare(control.displayText, "0.00")
        compare(control.value, 0)
        compare(displayTextChangedSpy.count, 3)
        compare(valueModifiedSpy.count, 0)
        compare(valueChangedSpy.count, 2)
    }

    function test_locale() {
        let control = createTemporaryObject(doubleSpinBox, testCase)
        verify(control)

        control.locale = Qt.locale("ar_EG") // Arabic, Egypt
        control.decimals = 0

        let numbers = ["٠", "١", "٢", "٣", "٤", "٥", "٦", "٧", "٨", "٩"]
        for (let i = 0; i < 10; ++i) {
            control.value = i
            compare(control.contentItem.text, numbers[i])
        }
    }

    function test_baseline() {
        let control = createTemporaryObject(doubleSpinBox, testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
    }

    function test_focus() {
        let control = createTemporaryObject(doubleSpinBox, testCase, {from: 10, to: 1000, value: 100, focus: true})
        verify(control)

        control.forceActiveFocus()
        compare(control.activeFocus, true)

        compare(control.from, 10)
        compare(control.to, 1000)
        compare(control.value, 100)

        control.focus = false
        compare(control.activeFocus, false)

        compare(control.from, 10)
        compare(control.to, 1000)
        compare(control.value, 100)
    }

    function test_initialFocus() {
        let window = testCase.Window.window
        verify(window)
        compare(window.activeFocusItem, window.contentItem)

        let control = createTemporaryObject(doubleSpinBox, testCase, { editable: true, focus: true })
        verify(control)
        tryCompare(control.contentItem, "activeFocus", true)
    }

    function test_editable() {
        let control = createTemporaryObject(doubleSpinBox, testCase)
        verify(control)

        control.locale = Qt.locale("en_EN") // forcing locale because double can be written differently because of current locale

        let valueModifiedSpy = signalSpy.createObject(control, {target: control, signalName: "valueModified"})
        verify(valueModifiedSpy.valid)

        let displayTextChangedSpy = signalSpy.createObject(control, {target: control, signalName: "displayTextChanged"})
        verify(displayTextChangedSpy.valid)

        control.from = 0
        control.to = 5
        compare(control.from, 0)
        compare(control.to, 5)

        control.contentItem.forceActiveFocus()
        compare(control.contentItem.activeFocus, true)

        compare(control.editable, false)
        control.contentItem.selectAll()
        compare(control.displayText, "0.00")
        keyClick(Qt.Key_5)
        keyClick(Qt.Key_Return)
        compare(control.displayText, "0.00")
        compare(control.value, 0)
        compare(valueModifiedSpy.count, 0)

        control.editable = true
        compare(control.editable, true)
        control.contentItem.selectAll()
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_5)
        compare(control.displayText, "5") // .00 will only be added once Return is pressed
        keyClick(Qt.Key_Return)
        compare(control.displayText, "5.00")
        compare(control.value, 5)
        compare(valueModifiedSpy.count, 1)
        compare(displayTextChangedSpy.count, 3) //0.00->5->5.00

        keyClick(Qt.Key_Backspace)
        compare(control.displayText, "5.0")
        compare(control.value, 5)
        compare(displayTextChangedSpy.count, 4) //0.00->5->5.0
        keyClick(Qt.Key_4)
        compare(control.displayText, "5.04")
        compare(control.value, 5)
        compare(valueModifiedSpy.count, 1)
        compare(displayTextChangedSpy.count, 5) //0.00->5->5.00->5.0->5.04->5.00
        keyClick(Qt.Key_Return) //will set the value to maximum = 5
        compare(control.displayText, "5.00")
        compare(control.value, 5)
        compare(valueModifiedSpy.count, 1) // 0->5
        compare(displayTextChangedSpy.count, 6) //0.00->5->5.00->5.0->5.04->5.00
    }

    function test_qtbug64151() {
        // Slightly modified example from QTBUG-64151. We use displayText
        // instead of contentItem.text as a workaround.
        let control = createTemporaryObject(doubleSpinBox, testCase)
        verify(control)

        control.locale = Qt.locale("en_EN")

        control.from = 0
        control.to = 2000
        control.value = 2000
        control.editable = true

        compare(control.displayText, "2,000.00")

        control.contentItem.forceActiveFocus()
        compare(control.contentItem.activeFocus, true)
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_Return)

        compare(control.displayText, "200.00")
        compare(control.valueFromText(control.contentItem.text, control.locale), 200)
        compare(control.value, 200)

        control.contentItem.forceActiveFocus()
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_0)
        keyClick(Qt.Key_Return)
        compare(control.displayText, "2,000.00")
    }

    function test_wheel_data() {
        return [
            { tag: "0.1", properties: { from: 1, to: 10, value: 1, stepSize: 0.1 }, upSteps: [1.1,1.2,1.3], downSteps: [1.2,1.1,1.0,1.0] },
            { tag: "0.2", properties: { from: 1, to: 10, value: 10, stepSize: 0.2 }, upSteps: [10,10], downSteps: [9.8,9.6,9.4] },
            { tag: "wrap0.1", properties: { wrap: true, from: 1, to: 10, value: 1, stepSize: 0.1 }, upSteps: [1.1,1.2], downSteps: [1.1,1.0,10,9.9] },
            { tag: "wrap25.5", properties: { wrap: true, from: 0, to: 99.5, value: 45, stepSize: 25.5 }, upSteps: [70.5,96.0,0,25.5], downSteps: [0,99.5,74] }
        ]
    }

    function test_wheel(data) {
        let ma = createTemporaryObject(mouseArea, testCase, {width: 100, height: 100})
        verify(ma)

        data.properties.wheelEnabled = true
        let control = doubleSpinBox.createObject(ma, data.properties)
        verify(control)

        let valueModifiedCount = 0
        let valueModifiedSpy = signalSpy.createObject(control, {target: control, signalName: "valueModified"})
        verify(valueModifiedSpy.valid)

        let delta = 120

        let spy = signalSpy.createObject(ma, {target: ma, signalName: "wheel"})
        verify(spy.valid)

        for (let u = 0; u < data.upSteps.length; ++u) {
            let wasUpEnabled = control.wrap || control.value < control.to
            mouseWheel(control, control.width / 2, control.height / 2, delta, delta)
            if (wasUpEnabled)
                ++valueModifiedCount
            compare(valueModifiedSpy.count, valueModifiedCount)
            compare(spy.count, 0) // no propagation
            compare(control.value, data.upSteps[u])
        }

        for (let d = 0; d < data.downSteps.length; ++d) {
            let wasDownEnabled = control.wrap || control.value > control.from
            mouseWheel(control, control.width / 2, control.height / 2, -delta, -delta)
            if (wasDownEnabled)
                ++valueModifiedCount
            compare(valueModifiedSpy.count, valueModifiedCount)
            compare(spy.count, 0) // no propagation
            compare(control.value, data.downSteps[d])
        }
    }

    function test_initiallyDisabledIndicators_data() {
        return [
            { tag: "down disabled", from: 0, value: 0, to: 99, upEnabled: true, downEnabled: false },
            { tag: "up disabled", from: 0, value: 99, to: 99, upEnabled: false, downEnabled: true },
            { tag: "inverted, down disabled", from: 99, value: 99, to: 0, upEnabled: true, downEnabled: false },
            { tag: "inverted, up disabled", from: 99, value: 0, to: 0, upEnabled: false, downEnabled: true }
        ]
    }

    function test_initiallyDisabledIndicators(data) {
        let control = createTemporaryObject(doubleSpinBox, testCase, { from: data.from, value: data.value, to: data.to })
        verify(control)

        compare(control.up.indicator.enabled, data.upEnabled)
        compare(control.down.indicator.enabled, data.downEnabled)
    }

    function test_hover_data() {
        return [
            { tag: "up:true", button: "up", hoverEnabled: true, value: 50 },
            { tag: "up:false", button: "up", hoverEnabled: false, value: 50 },
            { tag: "up:max", button: "up", hoverEnabled: true, value: 99 },
            { tag: "down:true", button: "down", hoverEnabled: true, value: 50 },
            { tag: "down:false", button: "down", hoverEnabled: false, value: 50 },
            { tag: "down:min", button: "down", hoverEnabled: true, value: 0 }
        ]
    }

    function test_hover(data) {
        let control = createTemporaryObject(doubleSpinBox, testCase, {hoverEnabled: data.hoverEnabled, value: data.value})
        verify(control)

        let button = control[data.button]
        compare(control.hovered, false)
        compare(button.hovered, false)

        mouseMove(control, button.indicator.x + button.indicator.width / 2, button.indicator.y + button.indicator.height / 2)
        compare(control.hovered, data.hoverEnabled)
        compare(button.hovered, data.hoverEnabled && button.indicator.enabled)

        mouseMove(control, button.indicator.x - 1, button.indicator.y - 1)
        compare(button.hovered, false)
    }

    function test_hoverWhilePressed_data() {
        return [
            { tag: "up" },
            { tag: "down" },
        ]
    }

    // QTBUG-74688
    function test_hoverWhilePressed(data) {
        let control = createTemporaryObject(doubleSpinBox, testCase, { hoverEnabled: true, value: 50 })
        verify(control)

        let button = control[data.tag]
        compare(control.hovered, false)
        compare(button.hovered, false)

        // Hover over the indicator. It should be hovered.
        let buttonXCenter = button.indicator.x + button.indicator.width / 2
        let buttonYCenter = button.indicator.y + button.indicator.height / 2
        mouseMove(control, buttonXCenter, buttonYCenter)
        compare(button.hovered, true)

        // Press on the indicator and then move the mouse outside of it.
        mousePress(control, buttonXCenter, buttonYCenter)
        compare(button.hovered, true)
        mouseMove(control, buttonXCenter - button.indicator.width, buttonYCenter - button.indicator.height)
        // It should not be pressed or hovered.
        compare(button.pressed, false)
        compare(button.hovered, false)

        mouseRelease(control)
    }

    function test_valueFromText_data() {
        return [
            { tag: "editable", editable: true },
            { tag: "non-editable", editable: false }
        ]
    }

    function test_valueFromText(data) {
        let control = createTemporaryObject(doubleSpinBox, testCase, {editable: data.editable})
        verify(control)

        control.forceActiveFocus()
        verify(control.activeFocus)

        let valueFromTextCalls = 0
        control.valueFromText = function(text, locale) {
            ++valueFromTextCalls
            return Number.fromLocaleString(locale, text);
        }

        keyClick(Qt.Key_Enter)
        compare(valueFromTextCalls, data.editable ? 1 : 0)

        keyClick(Qt.Key_Return)
        compare(valueFromTextCalls, data.editable ? 2 : 0)

        control.focus = false
        compare(valueFromTextCalls, data.editable ? 3 : 0)
    }

    function test_callDefaultValueFromText() {
        let control = createTemporaryObject(doubleSpinBox, testCase)
        verify(control)
        compare(control.valueFromText("123", control.locale), 123)
    }

    function test_autoRepeat() {
        let control = createTemporaryObject(doubleSpinBox, testCase)
        verify(control)

        compare(control.value, 0)

        let valueSpy = signalSpy.createObject(control, {target: control, signalName: "valueChanged"})
        verify(valueSpy.valid)

        let countBefore = 0

        // repeat up
        mousePress(control.up.indicator)
        verify(control.up.pressed)
        compare(valueSpy.count, 0)
        valueSpy.wait()
        valueSpy.wait()
        countBefore = valueSpy.count
        mouseRelease(control.up.indicator)
        verify(!control.up.pressed)
        compare(valueSpy.count, countBefore)

        valueSpy.clear()

        // repeat down
        mousePress(control.down.indicator)
        verify(control.down.pressed)
        compare(valueSpy.count, 0)
        valueSpy.wait()
        valueSpy.wait()
        countBefore = valueSpy.count
        mouseRelease(control.down.indicator)
        verify(!control.down.pressed)
        compare(valueSpy.count, countBefore)

        mousePress(control.up.indicator)
        verify(control.up.pressed)
        valueSpy.wait()

        // move inside during repeat -> continue repeat (QTBUG-57085)
        mouseMove(control.up.indicator, control.up.indicator.width / 4, control.up.indicator.height / 4)
        verify(control.up.pressed)
        valueSpy.wait()

        valueSpy.clear()

        // move outside during repeat -> stop repeat
        mouseMove(control.up.indicator, -1, -1)
        verify(!control.up.pressed)
        // NOTE: The following wait() is NOT a reliable way to test that the
        // auto-repeat timer is not running, but there's no way dig into the
        // private APIs from QML. If this test ever fails in the future, it
        // indicates that the auto-repeat timer logic is broken.
        wait(125)
        compare(valueSpy.count, 0)

        mouseRelease(control.up.indicator, -1, -1)
        verify(!control.up.pressed)
    }

    function test_initialValue() {
        let control = createTemporaryObject(doubleSpinBox, testCase, {from: 1000, to: 10000})
        verify(control)
        compare(control.value, 1000)
    }

    Component {
        id: sizeBox
        DoubleSpinBox {
            from: 0
            to: items.length - 1

            property var items: ["Small", "Medium", "Large"]

            validator: RegularExpressionValidator {
                regularExpression: new RegExp("(Small|Medium|Large)", "i")
            }

            textFromValue: function(value, decimals) {
                return items[value];
            }

            valueFromText: function(text) {
                for (let i = 0; i < items.length; ++i) {
                    if (items[i].toLowerCase().indexOf(text.toLowerCase()) === 0)
                        return i
                }
                return sb.value
            }
        }
    }

    function test_textFromValue_data() {
        return [
            { tag: "default", component: doubleSpinBox, values: [0, 10, 99], displayTexts: ["0.00", "10.00", "99.00"] },
            { tag: "custom", component: sizeBox, values: [0, 1, 2], displayTexts: ["Small", "Medium", "Large"] }
        ]
    }

    function test_textFromValue(data) {
        let control = createTemporaryObject(data.component, testCase)
        verify(control)

        control.locale = Qt.locale("en_EN") // forcing locale because double can be written differently because of current locale

        for (let i = 0; i < data.values.length; ++i) {
            control.value = data.values[i]
            compare(control.value, data.values[i])
            compare(control.displayText, data.displayTexts[i])
        }
    }

    function test_callDefaultTextFromValue() {
        let control = createTemporaryObject(doubleSpinBox, testCase)
        verify(control)
        compare(control.textFromValue(123, control.locale), "123")
    }

    Component {
        id: overriddenSpinBox
        DoubleSpinBox {
            value: 50
            up.indicator: Rectangle {
                property string s: "this is the one"
            }
        }
    }

    function test_indicatorOverridden() {
        let control = createTemporaryObject(overriddenSpinBox, testCase)
        verify(control)
        compare(control.up.indicator.s, "this is the one");
    }

    Component {
        id: overriddenSpinBoxWithIds
        DoubleSpinBox {
            value: 50
            up.indicator: Rectangle {
                id: uhOh1
                property string s: "up"
            }
            down.indicator: Rectangle {
                id: uhOh2
                property string s: "down"
            }
        }
    }

    function test_indicatorOverriddenWithIds() {
        let control = createTemporaryObject(overriddenSpinBoxWithIds, testCase)
        verify(control)
        // Specifying an id will result in both the default indicator implementations
        // and the custom ones being created, but it shouldn't result in any TypeErrors.
        compare(control.up.indicator.s, "up");
        compare(control.down.indicator.s, "down");
    }

    function test_valueEnterFromOutsideRange() {
        // Check that changing from 2 to 99 goes to 98 then changing to 99 puts it back to 98
        let control = createTemporaryObject(doubleSpinBox, testCase, {from: 2, to: 98, value: 2, editable: true})
        verify(control)

        control.locale = Qt.locale("en_EN") // forcing locale because double can be written differently because of current locale

        control.forceActiveFocus()
        verify(control.activeFocus)

        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_9)
        keyClick(Qt.Key_9)
        keyClick(Qt.Key_Return)
        compare(control.value, 98)
        compare(control.displayText, "98.00")
        compare(control.contentItem.text, "98.00")

        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_Backspace)
        keyClick(Qt.Key_9)
        keyClick(Qt.Key_Return)
        compare(control.value, 98)
        compare(control.displayText, "98.00")
        compare(control.contentItem.text, "98.00")
    }

    function test_pressedBeforeIncrementOrDecrement(data) {
        let control = createTemporaryObject(doubleSpinBox, testCase, {from: -8, to: 8, value: 0})
        verify(control)

        control.forceActiveFocus()
        verify(control.activeFocus)

        // up, down
        control.stepSize = Qt.binding(() => control.up.pressed ? 2 : 1)
        keyClick(Qt.Key_Up) // +2
        compare(control.value, 2)
        mouseClick(control.up.indicator) // +2
        compare(control.value, 4)
        keyClick(Qt.Key_Down) // -1
        compare(control.value, 3)
        mouseClick(control.down.indicator) // -1
        compare(control.value, 2)

        // down, up
        control.stepSize = Qt.binding(() => control.down.pressed ? 2 : 1)
        keyClick(Qt.Key_Down) // -2
        compare(control.value, 0)
        mouseClick(control.down.indicator) // -2
        compare(control.value, -2)
        keyClick(Qt.Key_Up) // +1
        compare(control.value, -1)
        mouseClick(control.up.indicator) // +1
        compare(control.value, 0)
    }

    function test_nullValidator() {
        let control = createTemporaryObject(doubleSpinBox, testCase, { validator: null })
        verify(control)
    }

    function test_increaseAndDecrease() {
        let control = createTemporaryObject(doubleSpinBox, testCase, {from: 0, to: 100, value: 50})
        verify(control)
        compare(control.value, 50)
        control.increase()
        compare(control.value, 51)
        control.decrease()
        compare(control.value, 50)
    }
}
