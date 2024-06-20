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
    name: "AbstractButton"

    Component {
        id: defaultComponent

        AbstractButton {}
    }

    Component {
        id: button
        AbstractButton {
            width: 100
            height: 50
        }
    }

    Component {
        id: item
        Item { }
    }

    Component {
        id: action
        Action { }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    property var expectedPressSignals: [
        ["activeFocusChanged", { "activeFocus": true }],
        ["pressedChanged", { "pressed": true }],
        ["downChanged", { "down": true }],
        "pressed"
    ]

    property var expectedReleaseSignals: [
        ["pressedChanged", { "pressed": false }],
        ["downChanged", { "down": false }],
        "released",
        "clicked"
    ]

    property var expectedClickSignals

    property var expectedCheckableClickSignals: [
        ["activeFocusChanged", { "activeFocus": true }],
        ["pressedChanged", { "pressed": true }],
        ["downChanged", { "down": true }],
        "pressed",
        ["pressedChanged", { "pressed": false }],
        ["downChanged", { "down": false }],
        ["checkedChanged", { "checked": true }],
        "toggled",
        "released",
        "clicked"
    ]

    function initTestCase() {
        // AbstractButton has TabFocus on macOS, not StrongFocus.
        if (Qt.platform.os === "osx") {
            expectedPressSignals.splice(0, 1)
            expectedCheckableClickSignals.splice(0, 1)
        }

        expectedClickSignals = [...expectedPressSignals, ...expectedReleaseSignals]
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(defaultComponent, testCase)
        verify(control)
    }

    function test_text() {
        let control = createTemporaryObject(button, testCase);
        verify(control);

        compare(control.text, "");
        control.text = "Button";
        compare(control.text, "Button");
        control.text = "";
        compare(control.text, "");
    }

    function test_baseline() {
        let control = createTemporaryObject(button, testCase, {padding: 6})
        verify(control)
        compare(control.baselineOffset, 0)
        control.contentItem = item.createObject(control, {baselineOffset: 12})
        compare(control.baselineOffset, 18)
    }

    function test_implicitSize() {
        let control = createTemporaryObject(button, testCase)
        verify(control)

        compare(control.implicitWidth, 0)
        compare(control.implicitHeight, 0)

        control.contentItem = item.createObject(control, {implicitWidth: 10, implicitHeight: 20})
        compare(control.implicitWidth, 10)
        compare(control.implicitHeight, 20)

        control.background = item.createObject(control, {implicitWidth: 20, implicitHeight: 30})
        compare(control.implicitWidth, 20)
        compare(control.implicitHeight, 30)

        control.padding = 100
        compare(control.implicitWidth, 210)
        compare(control.implicitHeight, 220)
    }

    function test_pressPoint_data() {
        return [
            { tag: "mouse", mouse: true },
            { tag: "touch", touch: true }
        ]
    }

    function test_pressPoint(data) {
        let control = createTemporaryObject(button, testCase, {width: 100, height: 40})
        verify(control)

        let pressXChanges = 0
        let pressYChanges = 0

        let pressXSpy = signalSpy.createObject(control, {target: control, signalName: "pressXChanged"})
        verify(pressXSpy.valid)

        let pressYSpy = signalSpy.createObject(control, {target: control, signalName: "pressYChanged"})
        verify(pressYSpy.valid)

        compare(control.pressX, 0)
        compare(control.pressY, 0)

        let touch = data.touch ? touchEvent(control) : null

        if (data.touch)
            touch.press(0, control, control.width / 2, control.height / 2).commit()
        else
            mousePress(control, control.width / 2, control.height / 2)
        compare(control.pressX, control.width / 2)
        compare(control.pressY, control.height / 2)
        compare(pressXSpy.count, ++pressXChanges)
        compare(pressYSpy.count, ++pressYChanges)

        if (data.touch)
            touch.move(0, control, control.width / 2, control.height / 2).commit()
        else
            mouseMove(control, control.width / 2, control.height / 2)
        compare(control.pressX, control.width / 2)
        compare(control.pressY, control.height / 2)
        compare(pressXSpy.count, pressXChanges)
        compare(pressYSpy.count, pressYChanges)

        if (data.touch)
            touch.move(0, control, control.width / 4, control.height / 4).commit()
        else
            mouseMove(control, control.width / 4, control.height / 4)
        compare(control.pressX, control.width / 4)
        compare(control.pressY, control.height / 4)
        compare(pressXSpy.count, ++pressXChanges)
        compare(pressYSpy.count, ++pressYChanges)

        if (data.touch)
            touch.move(0, control, 0, 0).commit()
        else
            mouseMove(control, 0, 0)
        compare(control.pressX, 0)
        compare(control.pressY, 0)
        compare(pressXSpy.count, ++pressXChanges)
        compare(pressYSpy.count, ++pressYChanges)

        if (data.touch)
            touch.move(0, control, -control.width / 2, -control.height / 2).commit()
        else
            mouseMove(control, -control.width / 2, -control.height / 2)
        compare(control.pressX, -control.width / 2)
        compare(control.pressY, -control.height / 2)
        compare(pressXSpy.count, ++pressXChanges)
        compare(pressYSpy.count, ++pressYChanges)

        if (data.touch)
            touch.release(0, control, -control.width / 2, -control.height / 2).commit()
        else
            mouseRelease(control, -control.width / 2, -control.height / 2)
        compare(control.pressX, -control.width / 2)
        compare(control.pressY, -control.height / 2)
        compare(pressXSpy.count, pressXChanges)
        compare(pressYSpy.count, pressYChanges)

        if (data.touch)
            touch.press(0, control, control.width - 1, control.height - 1).commit()
        else
            mousePress(control, control.width - 1, control.height - 1)
        compare(control.pressX, control.width - 1)
        compare(control.pressY, control.height - 1)
        compare(pressXSpy.count, ++pressXChanges)
        compare(pressYSpy.count, ++pressYChanges)

        if (data.touch)
            touch.move(0, control, control.width + 1, control.height + 1).commit()
        else
            mouseMove(control, control.width + 1, control.height + 1)
        compare(control.pressX, control.width + 1)
        compare(control.pressY, control.height + 1)
        compare(pressXSpy.count, ++pressXChanges)
        compare(pressYSpy.count, ++pressYChanges)

        if (data.touch)
            touch.release(0, control, control.width + 2, control.height + 2).commit()
        else
            mouseRelease(control, control.width + 2, control.height + 2)
        compare(control.pressX, control.width + 2)
        compare(control.pressY, control.height + 2)
        compare(pressXSpy.count, ++pressXChanges)
        compare(pressYSpy.count, ++pressYChanges)
    }

    function test_pressAndHold() {
        let control = createTemporaryObject(button, testCase, {checkable: true})
        verify(control)

        let pressAndHoldSpy = signalSpy.createObject(control, {target: control, signalName: "pressAndHold"})
        verify(pressAndHoldSpy.valid)

        mousePress(control)
        pressAndHoldSpy.wait()
        compare(control.checked, false)

        mouseRelease(control)
        compare(control.checked, false)
    }

    Component {
        id: keyCatcher
        Item {
            property int lastKeyPress: -1
            property int lastKeyRelease: -1
            Keys.onPressed: function (event) { lastKeyPress = event.key }
            Keys.onReleased: function (event) { lastKeyRelease = event.key }
        }
    }

    function test_keyEvents_data() {
        return [
            { tag: "space", key: Qt.Key_Space, result: -1 },
            { tag: "backspace", key: Qt.Key_Backspace, result: Qt.Key_Backspace }
        ]
    }

    function test_keyEvents(data) {
        let container = createTemporaryObject(keyCatcher, testCase)
        verify(container)

        let control = button.createObject(container)
        verify(control)

        control.forceActiveFocus()
        verify(control.activeFocus)

        keyPress(data.key)
        compare(container.lastKeyPress, data.result)

        keyRelease(data.key)
        compare(container.lastKeyRelease, data.result)
    }

    function test_icon() {
        let control = createTemporaryObject(button, testCase)
        verify(control)
        compare(control.icon.name, "")
        compare(control.icon.source, "")
        compare(control.icon.width, 0)
        compare(control.icon.height, 0)
        compare(control.icon.color, "#00000000")

        let iconSpy = signalSpy.createObject(control, { target: control, signalName: "iconChanged"} )
        verify(iconSpy.valid)

        control.icon.name = "test-name"
        compare(control.icon.name, "test-name")
        compare(iconSpy.count, 1)

        control.icon.source = "qrc:/test-source"
        compare(control.icon.source, "qrc:/test-source")
        compare(iconSpy.count, 2)

        control.icon.width = 32
        compare(control.icon.width, 32)
        compare(iconSpy.count, 3)

        control.icon.height = 32
        compare(control.icon.height, 32)
        compare(iconSpy.count, 4)

        control.icon.color = "#ff0000"
        compare(control.icon.color, "#ff0000")
        compare(iconSpy.count, 5)
    }

    function test_action_data() {
        return [
            { tag: "implicit text", property: "text",
                                    initButton: undefined, initAction: "Action",
                                    assignExpected: "Action", assignChanged: true,
                                    resetExpected: "", resetChanged: true },
            { tag: "explicit text", property: "text",
                                    initButton: "Button", initAction: "Action",
                                    assignExpected: "Button", assignChanged: false,
                                    resetExpected: "Button", resetChanged: false },
            { tag: "empty button text", property: "text",
                                    initButton: "", initAction: "Action",
                                    assignExpected: "", assignChanged: false,
                                    resetExpected: "", resetChanged: false },
            { tag: "empty action text", property: "text",
                                    initButton: "Button", initAction: "",
                                    assignExpected: "Button", assignChanged: false,
                                    resetExpected: "Button", resetChanged: false },
            { tag: "empty both text", property: "text",
                                    initButton: undefined, initAction: "",
                                    assignExpected: "", assignChanged: false,
                                    resetExpected: "", resetChanged: false },

            { tag: "modify button text", property: "text",
                                    initButton: undefined, initAction: "Action",
                                    assignExpected: "Action", assignChanged: true,
                                    modifyButton: "Button2",
                                    modifyButtonExpected: "Button2", modifyButtonChanged: true,
                                    resetExpected: "Button2", resetChanged: false },
            { tag: "modify implicit action text", property: "text",
                                    initButton: undefined, initAction: "Action",
                                    assignExpected: "Action", assignChanged: true,
                                    modifyAction: "Action2",
                                    modifyActionExpected: "Action2", modifyActionChanged: true,
                                    resetExpected: "", resetChanged: true },
            { tag: "modify explicit action text", property: "text",
                                    initButton: "Button", initAction: "Action",
                                    assignExpected: "Button", assignChanged: false,
                                    modifyAction: "Action2",
                                    modifyActionExpected: "Button", modifyActionChanged: false,
                                    resetExpected: "Button", resetChanged: false },
        ]
    }

    function test_action(data) {
        let control = createTemporaryObject(button, testCase)
        verify(control)
        control[data.property] = data.initButton

        let act = action.createObject(control)
        act[data.property] = data.initAction

        let spy = signalSpy.createObject(control, {target: control, signalName: data.property + "Changed"})
        verify(spy.valid)

        // assign action
        spy.clear()
        control.action = act
        compare(control[data.property], data.assignExpected)
        compare(spy.count, data.assignChanged ? 1 : 0)

        // modify button
        if (data.hasOwnProperty("modifyButton")) {
            spy.clear()
            control[data.property] = data.modifyButton
            compare(control[data.property], data.modifyButtonExpected)
            compare(spy.count, data.modifyButtonChanged ? 1 : 0)
        }

        // modify action
        if (data.hasOwnProperty("modifyAction")) {
            spy.clear()
            act[data.property] = data.modifyAction
            compare(control[data.property], data.modifyActionExpected)
            compare(spy.count, data.modifyActionChanged ? 1 : 0)
        }

        // reset action
        spy.clear()
        control.action = null
        compare(control[data.property], data.resetExpected)
        compare(spy.count, data.resetChanged ? 1 : 0)
    }

    function test_actionIcon_data() {
        let data = []

        // Save duplicating the rows by reusing them with different properties of the same type.
        // This means that the first loop will test icon.name and the second one will test icon.source.
        let stringPropertyValueSuffixes = [
            { propertyName: "name", valueSuffix: "IconName" },
            { propertyName: "source", valueSuffix: "IconSource" }
        ]

        for (let i = 0; i < stringPropertyValueSuffixes.length; ++i) {
            let propertyName = stringPropertyValueSuffixes[i].propertyName
            let valueSuffix = stringPropertyValueSuffixes[i].valueSuffix

            let buttonPropertyValue = "Button" + valueSuffix
            let buttonPropertyValue2 = "Button" + valueSuffix + "2"
            let actionPropertyValue = "Action" + valueSuffix
            let actionPropertyValue2 = "Action" + valueSuffix + "2"

            data.push({ tag: "implicit " + propertyName, property: propertyName,
                initButton: undefined, initAction: actionPropertyValue,
                assignExpected: actionPropertyValue, assignChanged: true,
                resetExpected: "", resetChanged: true })
            data.push({ tag: "explicit " + propertyName, property: propertyName,
                initButton: buttonPropertyValue, initAction: actionPropertyValue,
                assignExpected: buttonPropertyValue, assignChanged: false,
                resetExpected: buttonPropertyValue, resetChanged: false })
            data.push({ tag: "empty button " + propertyName, property: propertyName,
                initButton: "", initAction: actionPropertyValue,
                assignExpected: "", assignChanged: false,
                resetExpected: "", resetChanged: false })
            data.push({ tag: "empty action " + propertyName, property: propertyName,
                initButton: buttonPropertyValue, initAction: "",
                assignExpected: buttonPropertyValue, assignChanged: false,
                resetExpected: buttonPropertyValue, resetChanged: false })
            data.push({ tag: "empty both " + propertyName, property: propertyName,
                initButton: undefined, initAction: "",
                assignExpected: "", assignChanged: false,
                resetExpected: "", resetChanged: false })
            data.push({ tag: "modify button " + propertyName, property: propertyName,
                initButton: undefined, initAction: actionPropertyValue,
                assignExpected: actionPropertyValue, assignChanged: true,
                modifyButton: buttonPropertyValue2,
                modifyButtonExpected: buttonPropertyValue2, modifyButtonChanged: true,
                resetExpected: buttonPropertyValue2, resetChanged: false })
            data.push({ tag: "modify implicit action " + propertyName, property: propertyName,
                initButton: undefined, initAction: actionPropertyValue,
                assignExpected: actionPropertyValue, assignChanged: true,
                modifyAction: actionPropertyValue2,
                modifyActionExpected: actionPropertyValue2, modifyActionChanged: true,
                resetExpected: "", resetChanged: true })
            data.push({ tag: "modify explicit action " + propertyName, property: propertyName,
                initButton: buttonPropertyValue, initAction: actionPropertyValue,
                assignExpected: buttonPropertyValue, assignChanged: false,
                modifyAction: actionPropertyValue2,
                modifyActionExpected: buttonPropertyValue, modifyActionChanged: false,
                resetExpected: buttonPropertyValue, resetChanged: false })
        }

        let intPropertyNames = [
            "width",
            "height",
        ]

        for (let i = 0; i < intPropertyNames.length; ++i) {
            let propertyName = intPropertyNames[i]

            let buttonPropertyValue = 20
            let buttonPropertyValue2 = 21
            let actionPropertyValue = 40
            let actionPropertyValue2 = 41
            let defaultValue = 0

            data.push({ tag: "implicit " + propertyName, property: propertyName,
                initButton: undefined, initAction: actionPropertyValue,
                assignExpected: actionPropertyValue, assignChanged: true,
                resetExpected: defaultValue, resetChanged: true })
            data.push({ tag: "explicit " + propertyName, property: propertyName,
                initButton: buttonPropertyValue, initAction: actionPropertyValue,
                assignExpected: buttonPropertyValue, assignChanged: false,
                resetExpected: buttonPropertyValue, resetChanged: false })
            data.push({ tag: "default button " + propertyName, property: propertyName,
                initButton: defaultValue, initAction: actionPropertyValue,
                assignExpected: defaultValue, assignChanged: false,
                resetExpected: defaultValue, resetChanged: false })
            data.push({ tag: "default action " + propertyName, property: propertyName,
                initButton: buttonPropertyValue, initAction: defaultValue,
                assignExpected: buttonPropertyValue, assignChanged: false,
                resetExpected: buttonPropertyValue, resetChanged: false })
            data.push({ tag: "default both " + propertyName, property: propertyName,
                initButton: undefined, initAction: defaultValue,
                assignExpected: defaultValue, assignChanged: false,
                resetExpected: defaultValue, resetChanged: false })
            data.push({ tag: "modify button " + propertyName, property: propertyName,
                initButton: undefined, initAction: actionPropertyValue,
                assignExpected: actionPropertyValue, assignChanged: true,
                modifyButton: buttonPropertyValue2,
                modifyButtonExpected: buttonPropertyValue2, modifyButtonChanged: true,
                resetExpected: buttonPropertyValue2, resetChanged: false })
            data.push({ tag: "modify implicit action " + propertyName, property: propertyName,
                initButton: undefined, initAction: actionPropertyValue,
                assignExpected: actionPropertyValue, assignChanged: true,
                modifyAction: actionPropertyValue2,
                modifyActionExpected: actionPropertyValue2, modifyActionChanged: true,
                resetExpected: defaultValue, resetChanged: true })
            data.push({ tag: "modify explicit action " + propertyName, property: propertyName,
                initButton: buttonPropertyValue, initAction: actionPropertyValue,
                assignExpected: buttonPropertyValue, assignChanged: false,
                modifyAction: actionPropertyValue2,
                modifyActionExpected: buttonPropertyValue, modifyActionChanged: false,
                resetExpected: buttonPropertyValue, resetChanged: false })
        }

        let propertyName = "color"
        let buttonPropertyValue = "#aa0000"
        let buttonPropertyValue2 = "#ff0000"
        let actionPropertyValue = "#0000aa"
        let actionPropertyValue2 = "#0000ff"
        let defaultValue = "#00000000"

        data.push({ tag: "implicit " + propertyName, property: propertyName,
            initButton: undefined, initAction: actionPropertyValue,
            assignExpected: actionPropertyValue, assignChanged: true,
            resetExpected: defaultValue, resetChanged: true })
        data.push({ tag: "explicit " + propertyName, property: propertyName,
            initButton: buttonPropertyValue, initAction: actionPropertyValue,
            assignExpected: buttonPropertyValue, assignChanged: false,
            resetExpected: buttonPropertyValue, resetChanged: false })
        data.push({ tag: "default button " + propertyName, property: propertyName,
            initButton: defaultValue, initAction: actionPropertyValue,
            assignExpected: defaultValue, assignChanged: false,
            resetExpected: defaultValue, resetChanged: false })
        data.push({ tag: "default action " + propertyName, property: propertyName,
            initButton: buttonPropertyValue, initAction: defaultValue,
            assignExpected: buttonPropertyValue, assignChanged: false,
            resetExpected: buttonPropertyValue, resetChanged: false })
        data.push({ tag: "default both " + propertyName, property: propertyName,
            initButton: undefined, initAction: defaultValue,
            assignExpected: defaultValue, assignChanged: false,
            resetExpected: defaultValue, resetChanged: false })
        data.push({ tag: "modify button " + propertyName, property: propertyName,
            initButton: undefined, initAction: actionPropertyValue,
            assignExpected: actionPropertyValue, assignChanged: true,
            modifyButton: buttonPropertyValue2,
            modifyButtonExpected: buttonPropertyValue2, modifyButtonChanged: true,
            resetExpected: buttonPropertyValue2, resetChanged: false })
        data.push({ tag: "modify implicit action " + propertyName, property: propertyName,
            initButton: undefined, initAction: actionPropertyValue,
            assignExpected: actionPropertyValue, assignChanged: true,
            modifyAction: actionPropertyValue2,
            modifyActionExpected: actionPropertyValue2, modifyActionChanged: true,
            resetExpected: defaultValue, resetChanged: true })
        data.push({ tag: "modify explicit action " + propertyName, property: propertyName,
            initButton: buttonPropertyValue, initAction: actionPropertyValue,
            assignExpected: buttonPropertyValue, assignChanged: false,
            modifyAction: actionPropertyValue2,
            modifyActionExpected: buttonPropertyValue, modifyActionChanged: false,
            resetExpected: buttonPropertyValue, resetChanged: false })

        return data;
    }

    function test_actionIcon(data) {
        let control = createTemporaryObject(button, testCase)
        verify(control)
        control.icon[data.property] = data.initButton

        let act = action.createObject(control)
        act.icon[data.property] = data.initAction

        let spy = signalSpy.createObject(control, {target: control, signalName: "iconChanged"})
        verify(spy.valid)

        // assign action
        spy.clear()
        control.action = act
        compare(control.icon[data.property], data.assignExpected)
        compare(spy.count, data.assignChanged ? 1 : 0)

        // modify button
        if (data.hasOwnProperty("modifyButton")) {
            spy.clear()
            control.icon[data.property] = data.modifyButton
            compare(control.icon[data.property], data.modifyButtonExpected)
            compare(spy.count, data.modifyButtonChanged ? 1 : 0)
        }

        // modify action
        if (data.hasOwnProperty("modifyAction")) {
            spy.clear()
            act.icon[data.property] = data.modifyAction
            compare(control.icon[data.property], data.modifyActionExpected)
            compare(spy.count, data.modifyActionChanged ? 1 : 0)
        }

        // reset action
        spy.clear()
        control.action = null
        compare(control.icon[data.property], data.resetExpected)
        compare(spy.count, data.resetChanged ? 1 : 0)
    }

    Component {
        id: actionButton
        AbstractButton {
            width: 100
            height: 50
            action: Action {
                text: "Default"
                icon.name: checked ? "checked" : "unchecked"
                icon.source: "qrc:/icons/default.png"
                checkable: true
                checked: true
                enabled: false
            }
        }
    }

    function test_actionButton() {
        let control = createTemporaryObject(actionButton, testCase)
        verify(control)

        // initial values
        compare(control.text, "Default")
        compare(control.checkable, true)
        compare(control.checked, true)
        compare(control.enabled, false)
        compare(control.icon.name, "checked")

        let textSpy = signalSpy.createObject(control, { target: control, signalName: "textChanged" })
        verify(textSpy.valid)

        // changes via action
        control.action.text = "Action"
        control.action.checkable = false
        control.action.checked = false
        control.action.enabled = true
        compare(control.text, "Action") // propagates
        compare(control.checkable, false) // propagates
        compare(control.checked, false) // propagates
        compare(control.enabled, true) // propagates
        compare(control.icon.name, "unchecked") // propagates
        compare(textSpy.count, 1)

        // changes via button
        control.text = "Button"
        control.checkable = true
        control.checked = true
        control.enabled = false
        control.icon.name = "default"
        compare(control.text, "Button")
        compare(control.checkable, true)
        compare(control.checked, true)
        compare(control.enabled, false)
        compare(control.icon.name, "default")
        compare(control.action.text, "Action") // does NOT propagate
        compare(control.action.checkable, true) // propagates
        compare(control.action.checked, true) // propagates
        compare(control.action.enabled, true) // does NOT propagate
        compare(control.action.icon.name, control.action.checked ? "checked" : "unchecked") // does NOT propagate
        compare(textSpy.count, 2)

        // remove the action so that only the button's properties are left
        control.action = null
        compare(control.text, "Button")
        compare(control.icon.name, "default")
        compare(textSpy.count, 2)

        // setting an action while button has a particular property set
        // shouldn't cause a change in the button's effective property value
        let secondAction = createTemporaryObject(action, testCase)
        verify(secondAction)
        secondAction.text = "SecondAction"
        control.action = secondAction
        compare(control.text, "Button")
        compare(textSpy.count, 2)

        // test setting an action whose properties aren't set
        let thirdAction = createTemporaryObject(action, testCase)
        verify(thirdAction)
        control.action = thirdAction
        compare(control.text, "Button")
        compare(textSpy.count, 2)
    }

    Component {
        id: checkableButton
        AbstractButton {
            width: 100
            height: 50
            checkable: true
            action: Action {}
        }
    }

    function test_checkable_button() {
        let control = createTemporaryObject(checkableButton, testCase)
        verify(control)
        control.checked = false
        control.forceActiveFocus()
        verify(control.activeFocus)
        verify(!control.checked)
        verify(!control.action.checked)

        keyPress(Qt.Key_Space)
        keyRelease(Qt.Key_Space)

        compare(control.action.checked, true)
        compare(control.checked, true)

        keyPress(Qt.Key_Space)

        compare(control.action.checked, true)
        compare(control.checked, true)

        keyRelease(Qt.Key_Space)

        compare(control.action.checked, false)
        compare(control.checked, false)

        let checkedSpy = signalSpy.createObject(control, {target: control.action, signalName: "checkedChanged"})
        let toggledSpy = signalSpy.createObject(control, {target: control, signalName: "toggled"})
        let actionToggledSpy = signalSpy.createObject(control, {target: control.action, signalName: "toggled"})

        verify(checkedSpy.valid)
        verify(toggledSpy.valid)
        verify(actionToggledSpy.valid)

        mousePress(control)

        compare(control.action.checked, false)
        compare(control.checked, false)

        mouseRelease(control)

        checkedSpy.wait()
        compare(checkedSpy.count, 1)
        compare(actionToggledSpy.count, 1)
        compare(toggledSpy.count, 1)

        compare(control.action.checked, true)
        compare(control.checked, true)

        mousePress(control)
        mouseRelease(control)

        compare(control.checked, false)
        compare(control.action.checked, false)
    }

    function test_trigger_data() {
        return [
            {tag: "click", click: true, button: true, action: true, clicked: true, triggered: true},
            {tag: "click disabled button", click: true, button: false, action: true, clicked: false, triggered: false},
            {tag: "click disabled action", click: true, button: true, action: false, clicked: true, triggered: false},
            {tag: "trigger", trigger: true, button: true, action: true, clicked: true, triggered: true},
            {tag: "trigger disabled button", trigger: true, button: false, action: true, clicked: false, triggered: true},
            {tag: "trigger disabled action", trigger: true, button: true, action: false, clicked: false, triggered: false}
        ]
    }

    function test_trigger(data) {
        let control = createTemporaryObject(actionButton, testCase, {"action.enabled": data.action, "enabled": data.button})
        verify(control)

        compare(control.enabled, data.button)
        compare(control.action.enabled, data.action)

        let buttonSpy = signalSpy.createObject(control, {target: control, signalName: "clicked"})
        verify(buttonSpy.valid)

        let actionSpy = signalSpy.createObject(control, {target: control.action, signalName: "triggered"})
        verify(actionSpy.valid)

        if (data.click)
            mouseClick(control)
        else if (data.trigger)
            control.action.trigger()

        compare(buttonSpy.count, data.clicked ? 1 : 0)
        compare(actionSpy.count, data.triggered ? 1 : 0)
    }

    function test_mnemonic() {
        if (Qt.platform.os === "osx" || Qt.platform.os === "macos")
            skip("Mnemonics are not used on macOS")

        let control = createTemporaryObject(button, testCase)
        verify(control)

        control.text = "&Hello"
        compare(control.text, "&Hello")

        let clickSpy = signalSpy.createObject(control, {target: control, signalName: "clicked"})
        verify(clickSpy.valid)

        keyClick(Qt.Key_H, Qt.AltModifier)
        compare(clickSpy.count, 1)

        control.visible = false
        keyClick(Qt.Key_H, Qt.AltModifier)
        compare(clickSpy.count, 1)

        control.visible = true
        keyClick(Qt.Key_H, Qt.AltModifier)
        compare(clickSpy.count, 2)

        control.text = "Te&st"
        compare(control.text, "Te&st")

        keyClick(Qt.Key_H, Qt.AltModifier)
        compare(clickSpy.count, 2)

        keyClick(Qt.Key_S, Qt.AltModifier)
        compare(clickSpy.count, 3)

        control.visible = false
        control.text = "&Hidden"
        keyClick(Qt.Key_H, Qt.AltModifier)
        compare(clickSpy.count, 3)

        control.visible = true
        keyClick(Qt.Key_H, Qt.AltModifier)
        compare(clickSpy.count, 4)

        control.text = undefined
        control.action = action.createObject(control, {text: "&Action"})

        let actionSpy = signalSpy.createObject(control, {target: control.action, signalName: "triggered"})
        verify(actionSpy.valid)

        keyClick(Qt.Key_A, Qt.AltModifier)
        compare(actionSpy.count, 1)
        compare(clickSpy.count, 5)

        // ungrab on destruction (don't crash)
        control.Component.onDestruction.connect(function() { control = null })
        control.destroy()
        wait(0)
        verify(!control)
        keyClick(Qt.Key_H, Qt.AltModifier)
    }

    Component {
        id: actionGroup
        ActionGroup {
            Action { id: action1; checkable: true; checked: true }
            Action { id: action2; checkable: true }
            Action { id: action3; checkable: true }
        }
    }

    function test_actionGroup() {
        let group = createTemporaryObject(actionGroup, testCase)
        verify(group)

        let button1 = createTemporaryObject(button, testCase, {action: group.actions[0], width: 10, height: 10})
        let button2 = createTemporaryObject(button, testCase, {action: group.actions[1], width: 10, height: 10, y: 10})
        let button3 = createTemporaryObject(button, testCase, {action: group.actions[2], width: 10, height: 10, y: 20})

        verify(button1)
        compare(button1.checked, true)
        compare(button1.action.checked, true)

        verify(button2)
        compare(button2.checked, false)
        compare(button2.action.checked, false)

        verify(button3)
        compare(button3.checked, false)
        compare(button3.action.checked, false)

        mouseClick(button2)

        compare(button1.checked, false)
        compare(button1.action.checked, false)

        compare(button2.checked, true)
        compare(button2.action.checked, true)

        compare(button3.checked, false)
        compare(button3.action.checked, false)
    }

    function test_clickedAfterLongPress() {
        let control = createTemporaryObject(button, testCase, { text: "Hello" })
        verify(control)

        let clickedSpy = signalSpy.createObject(control, { target: control, signalName: "clicked" })
        verify(clickedSpy.valid)

        mousePress(control)
        // Ensure that clicked is emitted when no handler is defined for the pressAndHold() signal.
        // Note that even though signal spies aren't considered in QObject::isSignalConnected(),
        // we can't use one here to check for pressAndHold(), because otherwise clicked() won't be emitted.
        wait(Application.styleHints.mousePressAndHoldInterval + 100)
        mouseRelease(control)
        compare(clickedSpy.count, 1)
    }

    function test_doubleClick() {
        let control = createTemporaryObject(button, testCase, { text: "Hello" })
        verify(control)

        let pressedSpy = signalSpy.createObject(control, { target: control, signalName: "pressed" })
        verify(pressedSpy.valid)

        let releasedSpy = signalSpy.createObject(control, { target: control, signalName: "released" })
        verify(releasedSpy.valid)

        let clickedSpy = signalSpy.createObject(control, { target: control, signalName: "clicked" })
        verify(clickedSpy.valid)

        let doubleClickedSpy = signalSpy.createObject(control, { target: control, signalName: "doubleClicked" })
        verify(doubleClickedSpy.valid)

        mouseDoubleClickSequence(control)
        compare(pressedSpy.count, 2)
        compare(releasedSpy.count, 2)
        compare(clickedSpy.count, 1)
        compare(doubleClickedSpy.count, 1)

        let touch = touchEvent(control)
        touch.press(0, control)
        touch.commit()
        compare(pressedSpy.count, 3)
        compare(releasedSpy.count, 2)
        compare(clickedSpy.count, 1)
        compare(doubleClickedSpy.count, 1)

        touch.release(0, control)
        touch.commit()
        compare(pressedSpy.count, 3)
        compare(releasedSpy.count, 3)
        compare(clickedSpy.count, 2)
        compare(doubleClickedSpy.count, 1)

        touch.press(0, control)
        touch.commit()
        compare(pressedSpy.count, 4)
        compare(releasedSpy.count, 3)
        compare(clickedSpy.count, 2)
        compare(doubleClickedSpy.count, 1)

        touch.release(0, control)
        touch.commit()
        compare(pressedSpy.count, 4)
        compare(releasedSpy.count, 4)
        compare(clickedSpy.count, 2)
        compare(doubleClickedSpy.count, 2)
    }

    // It should be possible to quickly click a button whose doubleClicked signal
    // is not connected to anything.
    function test_fastClick() {
        let control = createTemporaryObject(button, testCase, { text: "Hello" })
        verify(control)

        let pressedSpy = signalSpy.createObject(control, { target: control, signalName: "pressed" })
        verify(pressedSpy.valid)

        let releasedSpy = signalSpy.createObject(control, { target: control, signalName: "released" })
        verify(releasedSpy.valid)

        let clickedSpy = signalSpy.createObject(control, { target: control, signalName: "clicked" })
        verify(clickedSpy.valid)

        // Can't listen to doubleClicked because it would cause it to be emitted.
        // We instead just check that clicked is emitted twice.

        mouseDoubleClickSequence(control)
        compare(pressedSpy.count, 2)
        compare(releasedSpy.count, 2)
        compare(clickedSpy.count, 2)

        let touch = touchEvent(control)
        touch.press(0, control)
        touch.commit()
        compare(pressedSpy.count, 3)
        compare(releasedSpy.count, 2)
        compare(clickedSpy.count, 2)

        touch.release(0, control)
        touch.commit()
        compare(pressedSpy.count, 3)
        compare(releasedSpy.count, 3)
        compare(clickedSpy.count, 3)

        touch.press(0, control)
        touch.commit()
        compare(pressedSpy.count, 4)
        compare(releasedSpy.count, 3)
        compare(clickedSpy.count, 3)

        touch.release(0, control)
        touch.commit()
        compare(pressedSpy.count, 4)
        compare(releasedSpy.count, 4)
        compare(clickedSpy.count, 4)
    }

    function test_checkedShouldNotSetCheckable() {
         let control = createTemporaryObject(button, testCase, { checked: true })
         verify(control)

         verify(!control.checkable)
    }

    function test_rightMouseButton() {
        let control = createTemporaryObject(button, testCase)
        verify(control)

        let pressedSpy = signalSpy.createObject(control, { target: control, signalName: "pressed" })
        verify(pressedSpy.valid)

        let releasedSpy = signalSpy.createObject(control, { target: control, signalName: "released" })
        verify(releasedSpy.valid)

        let clickedSpy = signalSpy.createObject(control, { target: control, signalName: "clicked" })
        verify(clickedSpy.valid)

        // button should not react on the right mouse button by defualt
        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)

        compare(pressedSpy.count, 0)
        compare(releasedSpy.count, 0)
        compare(clickedSpy.count, 0)

        // QTBUG-116289 - adding a HoverHandler into the button should not affect the handling of the right mouse button
        let hoverHandler = createTemporaryQmlObject("import QtQuick; HoverHandler {}", control)
        verify(hoverHandler)
        compare(hoverHandler.target, control)

        mousePress(control, control.width / 2, control.height / 2, Qt.RightButton)
        mouseRelease(control, control.width / 2, control.height / 2, Qt.RightButton)

        compare(pressedSpy.count, 0)
        compare(releasedSpy.count, 0)
        compare(clickedSpy.count, 0)
    }

    Component {
        id: signalSequenceSpy
        SignalSequenceSpy {
            // List all signals, even ones we might not be interested in for a particular test,
            // so that it can catch unwanted ones and fail the test.
            signals: ["pressed", "released", "canceled", "clicked", "toggled", "doubleClicked",
                "pressedChanged", "downChanged", "checkedChanged", "activeFocusChanged"]
        }
    }

    function test_click() {
        let control = createTemporaryObject(button, testCase)
        verify(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, { target: control })
        sequenceSpy.expectedSequence = testCase.expectedClickSignals
        control.click()
        verify(sequenceSpy.success)
    }

    function test_clickCheckableButton() {
        let control = createTemporaryObject(button, testCase, { checkable: true })
        verify(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, { target: control })
        sequenceSpy.expectedSequence = testCase.expectedCheckableClickSignals
        control.click()
        verify(sequenceSpy.success)
    }

    function test_animateClick() {
        let control = createTemporaryObject(button, testCase)
        verify(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, { target: control })
        sequenceSpy.expectedSequence = testCase.expectedClickSignals
        control.animateClick()
        tryVerify(() => { return sequenceSpy.success }, 1000)
    }

    function test_animateClickCheckableButton() {
        let control = createTemporaryObject(button, testCase, { checkable: true })
        verify(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, { target: control })
        sequenceSpy.expectedSequence = testCase.expectedCheckableClickSignals
        control.animateClick()
        tryVerify(() => { return sequenceSpy.success }, 1000)
    }

    function test_animateClickTwice() {
        let control = createTemporaryObject(button, testCase)
        verify(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, { target: control })
        sequenceSpy.expectedSequence = testCase.expectedPressSignals
        // Check that calling it again before it finishes works as expected.
        control.animateClick()
        verify(sequenceSpy.success)
        // Let the timer progress a bit.
        wait(0)
        sequenceSpy.expectedSequence = testCase.expectedReleaseSignals
        control.animateClick()
        tryVerify(() => { return sequenceSpy.success }, 1000)
    }

    function test_clickOnDisabledButton() {
        let control = createTemporaryObject(button, testCase, { enabled: false })
        verify(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, { target: control })
        sequenceSpy.expectedSequence = []
        control.click()
        verify(sequenceSpy.success)
    }

    function test_animateClickOnDisabledButton() {
        let control = createTemporaryObject(button, testCase, { enabled: false })
        verify(control)

        let sequenceSpy = signalSequenceSpy.createObject(control, { target: control })
        sequenceSpy.expectedSequence = []
        control.animateClick()
        verify(sequenceSpy.success)
    }

    Component {
        id: destroyOnPressButtonComponent

        AbstractButton {
            width: 100
            height: 50

            onPressed: destroy(this)
        }
    }

    function test_clickDestroyOnPress() {
        let control = createTemporaryObject(destroyOnPressButtonComponent, testCase)
        verify(control)

        // Parent it to the testCase, otherwise it will be destroyed when the control is.
        let destructionSpy = createTemporaryObject(signalSpy, testCase,
            { target: control.Component, signalName: "destruction"  })
        verify(destructionSpy.valid)

        let sequenceSpy = signalSequenceSpy.createObject(control, { target: control })
        sequenceSpy.expectedSequence = testCase.expectedClickSignals
        // Shouldn't crash, etc. Note that destroy() isn't synchronous, and so
        // the destruction will happen after the release.
        control.click()
        verify(sequenceSpy.success)
        tryCompare(destructionSpy, "count", 1)
    }

    function test_animateClickDestroyOnPress() {
        let control = createTemporaryObject(destroyOnPressButtonComponent, testCase)
        verify(control)

        // Parent it to the testCase, otherwise it will be destroyed when the control is.
        let destructionSpy = createTemporaryObject(signalSpy, testCase,
            { target: control.Component, signalName: "destruction"  })
        verify(destructionSpy.valid)

        let sequenceSpy = signalSequenceSpy.createObject(control, { target: control })
        sequenceSpy.expectedSequence = testCase.expectedPressSignals
        // Shouldn't crash, etc. Note that destroy() isn't synchronous, but it is processed
        // on the next frame, so should always come before the release's 100 ms delay.
        control.animateClick()
        verify(sequenceSpy.success)
        tryCompare(destructionSpy, "count", 1)
    }
}
