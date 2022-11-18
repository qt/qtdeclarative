// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "ToolTip"

    Component {
        id: toolTip
        ToolTip { }
    }

    Component {
        id: mouseArea
        MouseArea { }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    QtObject {
        id: object
    }

    SignalSpy {
        id: sharedSpy
        target: ToolTip.toolTip
    }

    function test_defaults() {
        failOnWarning(/.?/)

        let control = createTemporaryObject(toolTip, testCase)
        verify(control)
    }

    function test_properties_data() {
        return [
            {tag: "text", property: "text", defaultValue: "", setValue: "Hello", signalName: "textChanged"},
            {tag: "delay", property: "delay", defaultValue: 0, setValue: 1000, signalName: "delayChanged"},
            {tag: "timeout", property: "timeout", defaultValue: -1, setValue: 2000, signalName: "timeoutChanged"}
        ]
    }

    function test_properties(data) {
        var control = createTemporaryObject(toolTip, testCase)
        verify(control)

        compare(control[data.property], data.defaultValue)

        var spy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: data.signalName})
        verify(spy.valid)

        control[data.property] = data.setValue
        compare(control[data.property], data.setValue)
        compare(spy.count, 1)
    }

    function test_attached_data() {
        return [
            {tag: "text", property: "text", defaultValue: "", setValue: "Hello", signalName: "textChanged"},
            {tag: "delay", property: "delay", defaultValue: 0, setValue: 1000, signalName: "delayChanged"},
            {tag: "timeout", property: "timeout", defaultValue: -1, setValue: 2000, signalName: "timeoutChanged"}
        ]
    }

    function test_attached(data) {
        var item1 = createTemporaryObject(mouseArea, testCase)
        verify(item1)

        var item2 = createTemporaryObject(mouseArea, testCase)
        verify(item2)

        // Reset the properties to the expected default values, in case
        // we're not the first test that uses attached properties to be run.
        var sharedTip = ToolTip.toolTip
        sharedTip[data.property] = data.defaultValue

        compare(item1.ToolTip[data.property], data.defaultValue)
        compare(item2.ToolTip[data.property], data.defaultValue)

        var spy1 = signalSpy.createObject(item1, {target: item1.ToolTip, signalName: data.signalName})
        verify(spy1.valid)

        var spy2 = signalSpy.createObject(item2, {target: item2.ToolTip, signalName: data.signalName})
        verify(spy2.valid)

        sharedSpy.signalName = data.signalName
        verify(sharedSpy.valid)
        sharedSpy.clear()

        // change attached properties while the shared tooltip is not visible
        item1.ToolTip[data.property] = data.setValue
        compare(item1.ToolTip[data.property], data.setValue)
        compare(spy1.count, 1)

        compare(spy2.count, 0)
        compare(item2.ToolTip[data.property], data.defaultValue)

        // the shared tooltip is not visible for item1, so the attached
        // property change should therefore not apply to the shared instance
        compare(sharedSpy.count, 0)
        compare(sharedTip[data.property], data.defaultValue)

        // show the shared tooltip for item2
        item2.ToolTip.visible = true
        verify(item2.ToolTip.visible)
        verify(sharedTip.visible)

        // change attached properties while the shared tooltip is visible
        item2.ToolTip[data.property] = data.setValue
        compare(item2.ToolTip[data.property], data.setValue)
        compare(spy2.count, 1)

        // the shared tooltip is visible for item2, so the attached
        // property change should apply to the shared instance
        compare(sharedSpy.count, 1)
        compare(sharedTip[data.property], data.setValue)
    }

    function test_delay_data() {
        return [
            {tag: "imperative:0", delay: 0, imperative: true},
            {tag: "imperative:100", delay: 100, imperative: true},
            {tag: "declarative:0", delay: 0, imperative: false},
            {tag: "declarative:100", delay: 100, imperative: false}
        ]
    }

    function test_delay(data) {
        var control = createTemporaryObject(toolTip, testCase, {delay: data.delay})

        compare(control.visible, false)
        if (data.imperative)
            control.open()
        else
            control.visible = true
        compare(control.visible, data.delay <= 0)
        tryCompare(control, "visible", true)
    }

    function test_timeout_data() {
        return [
            {tag: "imperative", imperative: true},
            {tag: "declarative", imperative: false}
        ]
    }

    function test_timeout(data) {
        var control = createTemporaryObject(toolTip, testCase, {timeout: 100})

        compare(control.visible, false)
        if (data.imperative)
            control.open()
        else
            control.visible = true
        compare(control.visible, true)
        // wait a bit to make sure that it's still visible
        wait(50)
        compare(control.visible, true)
        // re-arm for another 200 ms
        control.timeout = 200
        compare(control.visible, true)
        // ensure that it's still visible after 150 ms (where old timeout < 150 < new timeout)
        wait(150)
        compare(control.visible, true)
        tryCompare(control, "visible", false)
    }

    function test_warning() {
        ignoreWarning(new RegExp(".*QML QtObject: ToolTip must be attached to an Item"))
        ignoreWarning(new RegExp(".*: QML ToolTip: cannot find any window to open popup in."))
        object.ToolTip.show("") // don't crash (QTBUG-56243)
    }

    Component {
        id: toolTipWithExitTransition

        ToolTip {
            Component.onCompleted: contentItem.objectName = "contentItem"

            enter: Transition {
                NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 100 }
            }
            exit: Transition {
                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 500 }
            }
        }
    }

    function test_makeVisibleWhileExitTransitionRunning_data() {
        return [
            { tag: "imperative", imperative: true },
            { tag: "declarative", imperative: false }
        ]
    }

    function test_makeVisibleWhileExitTransitionRunning(data) {
        var control = createTemporaryObject(toolTipWithExitTransition, testCase)

        // Show, hide, and show the tooltip again. Its exit transition should
        // start and get cancelled, and then its enter transition should run.
        if (data.imperative)
            control.open()
        else
            control.visible = true
        tryCompare(control, "opacity", 1)

        if (data.imperative)
            control.close()
        else
            control.visible = false
        verify(control.exit.running)
        tryVerify(function() { return control.opacity < 1; })

        if (data.imperative)
            control.open()
        else
            control.visible = true
        tryCompare(control, "opacity", 1)
    }

    Component {
        id: buttonAndShortcutComponent

        Item {
            property alias shortcut: shortcut
            property alias button: button

            Shortcut {
                id: shortcut
                sequence: "A"
            }

            Button {
                id: button
                text: "Just a button"
                focusPolicy: Qt.NoFocus

                ToolTip.visible: button.hovered
                ToolTip.text: qsTr("Some helpful text")
            }
        }
    }

    function test_activateShortcutWhileToolTipVisible() {
        if ((Qt.platform.pluginName === "offscreen")
            || (Qt.platform.pluginName === "minimal"))
            skip("Mouse hovering not functional on offscreen/minimal platforms")

        // Window shortcuts (the default context for Shortcut) require the window to have focus.
        var window = testCase.Window.window
        verify(window)
        window.requestActivate()
        tryCompare(window, "active", true)

        var root = createTemporaryObject(buttonAndShortcutComponent, testCase)
        verify(root)

        mouseMove(root.button, root.button.width / 2, root.button.height / 2)
        tryCompare(root.button.ToolTip.toolTip, "visible", true)

        var shortcutActivatedSpy = signalSpy.createObject(root, { target: root.shortcut, signalName: "activated" })
        verify(shortcutActivatedSpy.valid)
        keyPress(Qt.Key_A)
        compare(shortcutActivatedSpy.count, 1)
    }

    Component {
        id: hoverComponent
        MouseArea {
            id: hoverArea
            property alias tooltip: tooltip
            hoverEnabled: true
            width: testCase.width
            height: testCase.height
            ToolTip {
                id: tooltip
                x: 10; y: 10
                width: 10; height: 10
                visible: hoverArea.containsMouse
            }
        }
    }

    // QTBUG-63644
    function test_hover() {
        var root = createTemporaryObject(hoverComponent, testCase)
        verify(root)

        var tooltip = root.tooltip
        verify(tooltip)

        for (var pos = 0; pos <= 25; pos += 5) {
            mouseMove(root, pos, pos)
            verify(tooltip.visible)
        }
    }

    Component {
        id: nonAttachedToolTipComponent
        ToolTip { }
    }

    function test_nonAttachedToolTipShowAndHide() {
        var tip = createTemporaryObject(nonAttachedToolTipComponent, testCase)
        verify(tip)
        tip.show("hello");
        verify(tip.visible)
        verify(tip.text === "hello")
        tip.hide()
        tryCompare(tip, "visible", false)
        tip.show("delay", 200)
        verify(tip.visible)
        tryCompare(tip, "visible", false)
    }

    Component {
        id: timeoutButtonRowComponent

        Row {
            Button {
                text: "Timeout: 1"
                ToolTip.text: text
                ToolTip.visible: down
                ToolTip.timeout: 1
            }

            Button {
                text: "Timeout: -1"
                ToolTip.text: text
                ToolTip.visible: down
            }
        }
    }

    // QTBUG-74226
    function test_attachedTimeout() {
        var row = createTemporaryObject(timeoutButtonRowComponent, testCase)
        verify(row)

        // Press the button that has no timeout; it should stay visible.
        var button2 = row.children[1]
        mousePress(button2)
        compare(button2.down, true)
        tryCompare(button2.ToolTip.toolTip, "opened", true)

        // Wait a bit to make sure that it's still visible.
        wait(50)
        compare(button2.ToolTip.toolTip.opened, true)

        // Release and should close.
        mouseRelease(button2)
        compare(button2.down, false)
        tryCompare(button2.ToolTip, "visible", false)

        // Now, press the first button that does have a timeout; it should close on its own eventually.
        var button1 = row.children[0]
        mousePress(button1)
        compare(button1.down, true)
        // We use a short timeout to speed up the test, but tryCompare(...opened, true) then
        // fails because the dialog has already been hidden by that point, so just check that it's
        // immediately visible, which is more or less the same thing.
        compare(button1.ToolTip.visible, true)
        tryCompare(button1.ToolTip, "visible", false)
        mouseRelease(button2)

        // Now, hover over the second button again. It should still stay visible until the mouse is released.
        mousePress(button2)
        compare(button2.down, true)
        tryCompare(button2.ToolTip.toolTip, "opened", true)

        // Wait a bit to make sure that it's still visible.
        wait(50)
        compare(button2.ToolTip.toolTip.opened, true)

        // Release and should close.
        mouseRelease(button2)
        compare(button2.down, false)
        tryCompare(button2.ToolTip, "visible", false)
    }

    Component {
        id: wrapComponent

        Item {
            ToolTip.text: "This is some very very very very very very very very very very very very"
                + " very very very very very very very very very very very very very very"
                + " very very very very very very very very very very very very long text"
        }
    }

    // QTBUG-62350
    function test_wrap() {
        var item = createTemporaryObject(wrapComponent, testCase)
        verify(item)

        // Avoid "cannot find window to popup in" warning that can occur if it's made visible too early.
        item.ToolTip.visible = true
        tryCompare(item.ToolTip.toolTip, "opened", true)
        compare(item.ToolTip.toolTip.contentItem.wrapMode, Text.Wrap)
        verify(item.ToolTip.toolTip.contentItem.width < item.ToolTip.toolTip.contentItem.implicitWidth)
    }

    function test_timeoutAfterOpened() {
        let control = createTemporaryObject(toolTipWithExitTransition, testCase, { timeout: 1, exit: null })
        verify(control)

        let openedSpy = createTemporaryObject(signalSpy, testCase, { target: control, signalName: "opened" })
        verify(openedSpy.valid)

        control.show("Test")
        tryCompare(openedSpy, "count", 1)
    }

    Component {
        id: buttonDownToolTipComponent

        Button {
            required property string toolTipText

            ToolTip.text: toolTipText
            ToolTip.visible: down
        }
    }

    function test_attachedSizeBug() {
        let shortTextButton = createTemporaryObject(buttonDownToolTipComponent, testCase,
            { toolTipText: "Short text" })
        verify(shortTextButton)

        let longTextButton = createTemporaryObject(buttonDownToolTipComponent, testCase,
            { x: shortTextButton.width, toolTipText: "Some reeeeeeaaaaaaallly looooooooooongggggggg text" })
        verify(longTextButton)

        shortTextButton.ToolTip.toolTip.background.objectName = "ToolTipBackground"
        shortTextButton.ToolTip.toolTip.contentItem.objectName = "ToolTipText"

        // Show the tooltip with long text.
        mousePress(longTextButton)
        tryCompare(longTextButton.ToolTip.toolTip, "opened", true)
        const longTextToolTipImplicitWidth = longTextButton.ToolTip.toolTip.implicitWidth
        mouseRelease(longTextButton)
        tryCompare(longTextButton.ToolTip.toolTip, "visible", false)

        // Show the tooltip with short text.
        mousePress(shortTextButton)
        tryCompare(shortTextButton.ToolTip.toolTip, "opened", true)
        mouseRelease(shortTextButton)
        tryCompare(shortTextButton.ToolTip.toolTip, "visible", false)

        // Show the tooltip with long text again. It should have its original width.
        mousePress(longTextButton)
        tryCompare(longTextButton.ToolTip.toolTip, "opened", true)
        compare(longTextButton.ToolTip.toolTip.implicitWidth, longTextToolTipImplicitWidth)
        mouseRelease(longTextButton)
        tryCompare(longTextButton.ToolTip.toolTip, "visible", false)
    }
}
