/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
import QtQuick.Controls 2.3

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "AbstractButton"

    Component {
        id: button
        AbstractButton {}
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

    function test_text() {
        var control = createTemporaryObject(button, testCase);
        verify(control);

        compare(control.text, "");
        control.text = "Button";
        compare(control.text, "Button");
        control.text = "";
        compare(control.text, "");
    }

    function test_baseline() {
        var control = createTemporaryObject(button, testCase, {padding: 6})
        verify(control)
        compare(control.baselineOffset, 0)
        control.contentItem = item.createObject(control, {baselineOffset: 12})
        compare(control.baselineOffset, 18)
    }

    function test_implicitSize() {
        var control = createTemporaryObject(button, testCase)
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

    function test_pressAndHold() {
        var control = createTemporaryObject(button, testCase, {checkable: true})
        verify(control)

        var pressAndHoldSpy = signalSpy.createObject(control, {target: control, signalName: "pressAndHold"})
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
            Keys.onPressed: lastKeyPress = event.key
            Keys.onReleased: lastKeyRelease = event.key
        }
    }

    function test_keyEvents_data() {
        return [
            { tag: "space", key: Qt.Key_Space, result: -1 },
            { tag: "backspace", key: Qt.Key_Backspace, result: Qt.Key_Backspace }
        ]
    }

    function test_keyEvents(data) {
        var container = createTemporaryObject(keyCatcher, testCase)
        verify(container)

        var control = button.createObject(container)
        verify(control)

        control.forceActiveFocus()
        verify(control.activeFocus)

        keyPress(data.key)
        compare(container.lastKeyPress, data.result)

        keyRelease(data.key)
        compare(container.lastKeyRelease, data.result)
    }

    function test_icon() {
        var control = createTemporaryObject(button, testCase)
        verify(control)
        compare(control.icon.name, "")
        compare(control.icon.source, "")
        compare(control.icon.width, 0)
        compare(control.icon.height, 0)
        compare(control.icon.color, "#00000000")

        var iconSpy = signalSpy.createObject(control, { target: control, signalName: "iconChanged"} )
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

    Component {
        id: actionButton
        AbstractButton {
            action: Action {
                text: "Default"
                icon.name: "default"
                icon.source: "qrc:/icons/default.png"
                checkable: true
                checked: true
                enabled: false
            }
        }
    }

    function test_action() {
        var control = createTemporaryObject(actionButton, testCase)
        verify(control)

        // initial values
        compare(control.text, "Default")
        compare(control.icon.name, "default")
        compare(control.icon.source, "qrc:/icons/default.png")
        compare(control.checkable, true)
        compare(control.checked, true)
        compare(control.enabled, false)

        // changes via action
        control.action.text = "Action"
        control.action.icon.name = "action"
        control.action.icon.source = "qrc:/icons/action.png"
        control.action.checkable = false
        control.action.checked = false
        control.action.enabled = true
        compare(control.text, "Action") // propagates
        compare(control.icon.name, "action") // propagates
        compare(control.icon.source, "qrc:/icons/action.png") // propagates
        compare(control.checkable, false) // propagates
        compare(control.checked, false) // propagates
        compare(control.enabled, true) // propagates

        // changes via button
        control.text = "Button"
        control.icon.name = "button"
        control.icon.source = "qrc:/icons/button.png"
        control.checkable = true
        control.checked = true
        control.enabled = false
        compare(control.text, "Button")
        compare(control.icon.name, "button")
        compare(control.icon.source, "qrc:/icons/button.png")
        compare(control.checkable, true)
        compare(control.checked, true)
        compare(control.enabled, false)
        compare(control.action.text, "Action") // does NOT propagate
        compare(control.action.icon.name, "action") // does NOT propagate
        compare(control.action.icon.source, "qrc:/icons/action.png") // does NOT propagate
        compare(control.action.checkable, true) // propagates
        compare(control.action.checked, true) // propagates
        compare(control.action.enabled, true) // does NOT propagate
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
        var control = createTemporaryObject(actionButton, testCase, {"enabled": data.button, "action.enabled": data.action})
        verify(control)

        compare(control.enabled, data.button)
        compare(control.action.enabled, data.action)

        var buttonSpy = signalSpy.createObject(control, {target: control, signalName: "clicked"})
        verify(buttonSpy.valid)

        var actionSpy = signalSpy.createObject(control, {target: control.action, signalName: "triggered"})
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

        var control = createTemporaryObject(button, testCase)
        verify(control)

        control.text = "&Hello"
        compare(control.text, "&Hello")

        var clickSpy = signalSpy.createObject(control, {target: control, signalName: "clicked"})
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

        control.action = action.createObject(control, {text: "&Action"})

        var actionSpy = signalSpy.createObject(control, {target: control.action, signalName: "triggered"})
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
        var group = createTemporaryObject(actionGroup, testCase)
        verify(group)

        var button1 = createTemporaryObject(button, testCase, {action: group.actions[0], width: 10, height: 10})
        var button2 = createTemporaryObject(button, testCase, {action: group.actions[1], width: 10, height: 10, y: 10})
        var button3 = createTemporaryObject(button, testCase, {action: group.actions[2], width: 10, height: 10, y: 20})

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
}
