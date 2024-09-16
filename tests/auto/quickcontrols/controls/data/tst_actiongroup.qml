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
    name: "ActionGroup"

    Component {
        id: actionGroup
        ActionGroup { }
    }

    Component {
        id: nonExclusiveGroup
        ActionGroup { exclusive: false }
    }

    Component {
        id: declarativeGroup
        ActionGroup {
            Action { text: "First" }
            Action { text: "Second" }
            Action { text: "Third" }
        }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_null() {
        let group = createTemporaryObject(actionGroup, testCase)
        verify(group)

        group.addAction(null)
        group.removeAction(null)
    }

    Component {
        id: action
        Action { }
    }

    function test_defaults() {
        let group = createTemporaryObject(actionGroup, testCase)
        verify(group)
        compare(group.actions.length, 0)
        compare(group.checkedAction, null)
        compare(group.exclusive, true)
    }

    function test_current() {
        let group = createTemporaryObject(actionGroup, testCase)
        verify(group)

        let checkedActionSpy = createTemporaryObject(signalSpy, testCase, {target: group, signalName: "checkedActionChanged"})
        verify(checkedActionSpy.valid)
        verify(!group.checkedAction)

        let action1 = createTemporaryObject(action, testCase, {checked: true})
        let action2 = createTemporaryObject(action, testCase, {checked: false})
        let action3 = createTemporaryObject(action, testCase, {checked: true, objectName: "3"})

        // add checked
        group.addAction(action1)
        compare(group.checkedAction, action1)
        compare(action1.checked, true)
        compare(action2.checked, false)
        compare(action3.checked, true)
        compare(checkedActionSpy.count, 1)

        // add non-checked
        group.addAction(action2)
        compare(group.checkedAction, action1)
        compare(action1.checked, true)
        compare(action2.checked, false)
        compare(action3.checked, true)
        compare(checkedActionSpy.count, 1)

        // add checked
        group.addAction(action3)
        compare(group.checkedAction, action3)
        compare(action1.checked, false)
        compare(action2.checked, false)
        compare(action3.checked, true)
        compare(checkedActionSpy.count, 2)

        // change current
        group.checkedAction = action2
        compare(group.checkedAction, action2)
        compare(action1.checked, false)
        compare(action2.checked, true)
        compare(action3.checked, false)
        compare(checkedActionSpy.count, 3)

        // check
        action1.checked = true
        compare(group.checkedAction, action1)
        compare(action1.checked, true)
        compare(action2.checked, false)
        compare(action3.checked, false)
        compare(checkedActionSpy.count, 4)

        // remove non-checked
        group.removeAction(action2)
        compare(group.checkedAction, action1)
        compare(action1.checked, true)
        compare(action2.checked, false)
        compare(action3.checked, false)
        compare(checkedActionSpy.count, 4)

        // remove checked
        group.removeAction(action1)
        verify(!group.checkedAction)
        compare(action1.checked, false)
        compare(action2.checked, false)
        compare(action3.checked, false)
        compare(checkedActionSpy.count, 5)
    }

    function test_actions() {
        let group = createTemporaryObject(actionGroup, testCase)
        verify(group)

        let actionsSpy = createTemporaryObject(signalSpy, testCase, {target: group, signalName: "actionsChanged"})
        verify(actionsSpy.valid)

        compare(group.actions.length, 0)
        compare(group.checkedAction, null)

        let action1 = createTemporaryObject(action, testCase, {checked: true})
        let action2 = createTemporaryObject(action, testCase, {checked: false})

        group.actions = [action1, action2]
        compare(group.actions.length, 2)
        compare(group.actions[0], action1)
        compare(group.actions[1], action2)
        compare(group.checkedAction, action1)
        compare(actionsSpy.count, 2)

        let action3 = createTemporaryObject(action, testCase, {checked: true})

        group.addAction(action3)
        compare(group.actions.length, 3)
        compare(group.actions[0], action1)
        compare(group.actions[1], action2)
        compare(group.actions[2], action3)
        compare(group.checkedAction, action3)
        compare(actionsSpy.count, 3)

        group.removeAction(action1)
        compare(group.actions.length, 2)
        compare(group.actions[0], action2)
        compare(group.actions[1], action3)
        compare(group.checkedAction, action3)
        compare(actionsSpy.count, 4)

        group.actions = []
        compare(group.actions.length, 0)
        tryCompare(group, "checkedAction", null)
        compare(actionsSpy.count, 5)
    }

    function test_declarative() {
        let group = createTemporaryObject(declarativeGroup, testCase)
        verify(group)

        compare(group.actions.length, 3)
        compare(group.actions[0].text, "First")
        compare(group.actions[1].text, "Second")
        compare(group.actions[2].text, "Third")
    }

    function test_triggered_data() {
        return [
            {tag: "exclusive", exclusive: true},
            {tag: "non-exclusive", exclusive: false}
        ]
    }

    function test_triggered(data) {
        let group = createTemporaryObject(actionGroup, testCase, {exclusive: data.exclusive})
        verify(group)

        let triggeredSpy = createTemporaryObject(signalSpy, testCase, {target: group, signalName: "triggered"})
        verify(triggeredSpy.valid)

        let action1 = createTemporaryObject(action, testCase)
        let action2 = createTemporaryObject(action, testCase)

        group.addAction(action1)
        group.addAction(action2)

        action1.triggered()
        compare(triggeredSpy.count, 1)
        compare(triggeredSpy.signalArguments[0][0], action1)

        action2.triggered()
        compare(triggeredSpy.count, 2)
        compare(triggeredSpy.signalArguments[1][0], action2)
    }

    Component {
        id: attachedGroup
        Item {
            property ActionGroup group: ActionGroup { id: group }
            property Action action1: Action { ActionGroup.group: group }
            property Action action2: Action { ActionGroup.group: group }
            property Action action3: Action { ActionGroup.group: group }
        }
    }

    function test_attached() {
        let container = createTemporaryObject(attachedGroup, testCase)
        verify(container)

        verify(!container.group.checkedAction)

        container.action1.checked = true
        compare(container.group.checkedAction, container.action1)
        compare(container.action1.checked, true)
        compare(container.action2.checked, false)
        compare(container.action3.checked, false)

        container.action2.checked = true
        compare(container.group.checkedAction, container.action2)
        compare(container.action1.checked, false)
        compare(container.action2.checked, true)
        compare(container.action3.checked, false)

        container.action3.checked = true
        compare(container.group.checkedAction, container.action3)
        compare(container.action1.checked, false)
        compare(container.action2.checked, false)
        compare(container.action3.checked, true)
    }

    function test_actionDestroyed() {
        let group = createTemporaryObject(actionGroup, testCase)
        verify(group)

        let actionsSpy = createTemporaryObject(signalSpy, testCase, {target: group, signalName: "actionsChanged"})
        verify(actionsSpy.valid)

        let action1 = createTemporaryObject(action, testCase, {objectName: "action1", checked: true})

        group.addAction(action1)
        compare(group.actions.length, 1)
        compare(group.actions[0], action1)
        compare(group.checkedAction, action1)
        compare(actionsSpy.count, 1)

        action1.destroy()
        wait(0)
        compare(group.actions.length, 0)
        compare(group.checkedAction, null)
        compare(actionsSpy.count, 2)
    }

    function test_nonExclusive() {
        let group = createTemporaryObject(nonExclusiveGroup, testCase)
        verify(group)

        let action1 = createTemporaryObject(action, testCase, {checked: true})
        group.addAction(action1)
        compare(action1.checked, true)
        compare(group.checkedAction, null)

        let action2 = createTemporaryObject(action, testCase, {checked: true})
        group.addAction(action2)
        compare(action1.checked, true)
        compare(action2.checked, true)
        compare(group.checkedAction, null)

        action1.checked = false
        compare(action1.checked, false)
        compare(action2.checked, true)
        compare(group.checkedAction, null)

        action2.checked = false
        compare(action1.checked, false)
        compare(action2.checked, false)
        compare(group.checkedAction, null)

        action1.checked = true
        compare(action1.checked, true)
        compare(action2.checked, false)
        compare(group.checkedAction, null)

        action2.checked = true
        compare(action1.checked, true)
        compare(action2.checked, true)
        compare(group.checkedAction, null)
    }

    function test_enabled() {
        let group = createTemporaryObject(actionGroup, testCase)
        verify(group)

        compare(group.enabled, true)

        let action1 = createTemporaryObject(action, testCase)
        let action2 = createTemporaryObject(action, testCase)
        compare(action1.enabled, true)
        compare(action2.enabled, true)

        let action1Spy = createTemporaryObject(signalSpy, testCase, {target: action1, signalName: "enabledChanged"})
        let action2Spy = createTemporaryObject(signalSpy, testCase, {target: action2, signalName: "enabledChanged"})
        verify(action1Spy.valid && action2Spy.valid)

        group.addAction(action1)
        compare(action1.enabled, true)
        compare(action2.enabled, true)
        compare(action1Spy.count, 0)
        compare(action2Spy.count, 0)

        group.enabled = false
        compare(action1.enabled, false)
        compare(action2.enabled, true)
        compare(action1Spy.count, 1)
        compare(action1Spy.signalArguments[0][0], false)
        compare(action2Spy.count, 0)

        group.addAction(action2)
        compare(action1.enabled, false)
        compare(action2.enabled, false)
        compare(action1Spy.count, 1)
        compare(action2Spy.count, 1)
        compare(action2Spy.signalArguments[0][0], false)

        action1.enabled = false
        compare(action1.enabled, false)
        compare(action1Spy.count, 2)
        compare(action1Spy.signalArguments[1][0], false)
        compare(action2Spy.count, 1)

        group.enabled = true
        compare(action1.enabled, false)
        compare(action2.enabled, true)
        compare(action1Spy.count, 2)
        compare(action2Spy.count, 2)
        compare(action2Spy.signalArguments[1][0], true)
    }

    Component {
        id: checkBoxes
        Item {
            property ActionGroup group: ActionGroup { id: group }
            property CheckBox control1: CheckBox { action: Action { ActionGroup.group: group } }
            property CheckBox control2: CheckBox { action: Action { ActionGroup.group: group } }
            property CheckBox control3: CheckBox { action: Action { ActionGroup.group: group } }
        }
    }

    Component {
        id: radioButtons
        Item {
            property ActionGroup group: ActionGroup { id: group }
            property RadioButton control1: RadioButton { action: Action { ActionGroup.group: group } }
            property RadioButton control2: RadioButton { action: Action { ActionGroup.group: group } }
            property RadioButton control3: RadioButton { action: Action { ActionGroup.group: group } }
        }
    }

    Component {
        id: switches
        Item {
            property ActionGroup group: ActionGroup { id: group }
            property Switch control1: Switch { action: Action { ActionGroup.group: group } }
            property Switch control2: Switch { action: Action { ActionGroup.group: group } }
            property Switch control3: Switch { action: Action { ActionGroup.group: group } }
        }
    }

    function test_controls_data() {
        return [
            { tag: "CheckBox", component: checkBoxes },
            { tag: "RadioButton", component: radioButtons },
            { tag: "Switch", component: switches },
        ]
    }

    function test_controls(data) {
        let container = createTemporaryObject(data.component, testCase)
        verify(container)

        verify(!container.group.checkedAction)

        container.control1.checked = true
        compare(container.group.checkedAction, container.control1.action)
        compare(container.control1.checked, true)
        compare(container.control2.checked, false)
        compare(container.control3.checked, false)

        container.control2.checked = true
        compare(container.group.checkedAction, container.control2.action)
        compare(container.control1.checked, false)
        compare(container.control2.checked, true)
        compare(container.control3.checked, false)

        container.control3.checked = true
        compare(container.group.checkedAction, container.control3.action)
        compare(container.control1.checked, false)
        compare(container.control2.checked, false)
        compare(container.control3.checked, true)
    }

    Component {
        id: exclusiveMenus
        Column {
            property ActionGroup group: ActionGroup { id: group }
            property alias control1: control1
            property alias control2: control2
            property alias control3: control3
            MenuItem {
                id: control1
                action: Action {
                    checkable: true
                    ActionGroup.group: group
                }
            }
            MenuItem {
                id: control2
                action: Action {
                    checkable: true
                    ActionGroup.group: group
                }
            }
            MenuItem {
                id: control3
                action: Action {
                    checkable: true
                    ActionGroup.group: group
                }
            }
        }
    }

    function test_exclusiveActionButtons() {
        const container = createTemporaryObject(exclusiveMenus, testCase)
        verify(container)

        verify(!container.group.checkedAction)

        mouseClick(container.control1)
        compare(container.group.checkedAction, container.control1.action)
        compare(container.control1.checked, true)
        compare(container.control2.checked, false)
        compare(container.control3.checked, false)

        mouseClick(container.control2)
        compare(container.group.checkedAction, container.control2.action)
        compare(container.control1.checked, false)
        compare(container.control2.checked, true)
        compare(container.control3.checked, false)

        mouseClick(container.control3)
        compare(container.group.checkedAction, container.control3.action)
        compare(container.control1.checked, false)
        compare(container.control2.checked, false)
        compare(container.control3.checked, true)

        // here comes the tricky part: clicking on checked control must not uncheck it
        mouseClick(container.control3)
        compare(container.group.checkedAction, container.control3.action)
        compare(container.control1.checked, false)
        compare(container.control2.checked, false)
        compare(container.control3.checked, true)
    }
}
