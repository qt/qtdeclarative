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
    name: "ButtonGroup"

    Component {
        id: buttonGroup
        ButtonGroup { }
    }

    Component {
        id: nonExclusiveGroup
        ButtonGroup { exclusive: false }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    Component {
        id: button
        Button { }
    }

    Component {
        id: nonCheckable
        QtObject { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let group = createTemporaryObject(buttonGroup, testCase)
        verify(group)
        compare(group.buttons.length, 0)
        compare(group.checkedButton, null)
        compare(group.exclusive, true)
        compare(group.checkState, Qt.Unchecked)
    }

    function test_null() {
        let group = createTemporaryObject(buttonGroup, testCase)
        verify(group)

        group.addButton(null)
        group.removeButton(null)
    }

    function test_current() {
        let group = createTemporaryObject(buttonGroup, testCase)
        verify(group)

        let checkedButtonSpy = createTemporaryObject(signalSpy, testCase, {target: group, signalName: "checkedButtonChanged"})
        verify(checkedButtonSpy.valid)
        verify(!group.checkedButton)

        let button1 = createTemporaryObject(button, testCase, {checked: true})
        let button2 = createTemporaryObject(button, testCase, {checked: false})
        let button3 = createTemporaryObject(button, testCase, {checked: true, objectName: "3"})

        // add checked
        group.addButton(button1)
        compare(group.checkedButton, button1)
        compare(button1.checked, true)
        compare(button2.checked, false)
        compare(button3.checked, true)
        compare(checkedButtonSpy.count, 1)

        // add non-checked
        group.addButton(button2)
        compare(group.checkedButton, button1)
        compare(button1.checked, true)
        compare(button2.checked, false)
        compare(button3.checked, true)
        compare(checkedButtonSpy.count, 1)

        // add checked
        group.addButton(button3)
        compare(group.checkedButton, button3)
        compare(button1.checked, false)
        compare(button2.checked, false)
        compare(button3.checked, true)
        compare(checkedButtonSpy.count, 2)

        // change current
        group.checkedButton = button2
        compare(group.checkedButton, button2)
        compare(button1.checked, false)
        compare(button2.checked, true)
        compare(button3.checked, false)
        compare(checkedButtonSpy.count, 3)

        // check
        button1.checked = true
        compare(group.checkedButton, button1)
        compare(button1.checked, true)
        compare(button2.checked, false)
        compare(button3.checked, false)
        compare(checkedButtonSpy.count, 4)

        // remove non-checked
        group.removeButton(button2)
        compare(group.checkedButton, button1)
        compare(button1.checked, true)
        compare(button2.checked, false)
        compare(button3.checked, false)
        compare(checkedButtonSpy.count, 4)

        // remove checked
        group.removeButton(button1)
        verify(!group.checkedButton)
        compare(button1.checked, false)
        compare(button2.checked, false)
        compare(button3.checked, false)
        compare(checkedButtonSpy.count, 5)
    }

    function test_buttons() {
        let group = createTemporaryObject(buttonGroup, testCase)
        verify(group)

        let buttonsSpy = createTemporaryObject(signalSpy, testCase, {target: group, signalName: "buttonsChanged"})
        verify(buttonsSpy.valid)

        compare(group.buttons.length, 0)
        compare(group.checkedButton, null)

        let button1 = createTemporaryObject(button, testCase, {checked: true})
        let button2 = createTemporaryObject(button, testCase, {checked: false})

        group.buttons = [button1, button2]
        compare(group.buttons.length, 2)
        compare(group.buttons[0], button1)
        compare(group.buttons[1], button2)
        compare(group.checkedButton, button1)
        compare(buttonsSpy.count, 2)

        let button3 = createTemporaryObject(button, testCase, {checked: true})

        group.addButton(button3)
        compare(group.buttons.length, 3)
        compare(group.buttons[0], button1)
        compare(group.buttons[1], button2)
        compare(group.buttons[2], button3)
        compare(group.checkedButton, button3)
        compare(buttonsSpy.count, 3)

        group.removeButton(button1)
        compare(group.buttons.length, 2)
        compare(group.buttons[0], button2)
        compare(group.buttons[1], button3)
        compare(group.checkedButton, button3)
        compare(buttonsSpy.count, 4)

        group.buttons = []
        compare(group.buttons.length, 0)
        tryCompare(group, "checkedButton", null)
        compare(buttonsSpy.count, 5)
    }

    function test_clicked_data() {
        return [
            {tag: "exclusive", exclusive: true},
            {tag: "non-exclusive", exclusive: false}
        ]
    }

    function test_clicked(data) {
        let group = createTemporaryObject(buttonGroup, testCase, {exclusive: data.exclusive})
        verify(group)

        let clickedSpy = createTemporaryObject(signalSpy, testCase, {target: group, signalName: "clicked"})
        verify(clickedSpy.valid)

        let button1 = createTemporaryObject(button, testCase)
        let button2 = createTemporaryObject(button, testCase)

        group.addButton(button1)
        group.addButton(button2)

        button1.clicked()
        compare(clickedSpy.count, 1)
        compare(clickedSpy.signalArguments[0][0], button1)

        button2.clicked()
        compare(clickedSpy.count, 2)
        compare(clickedSpy.signalArguments[1][0], button2)
    }

    Component {
        id: checkBoxes
        Item {
            property ButtonGroup group: ButtonGroup { id: group }
            property CheckBox control1: CheckBox { ButtonGroup.group: group }
            property CheckBox control2: CheckBox { ButtonGroup.group: group }
            property CheckBox control3: CheckBox { ButtonGroup.group: group }
        }
    }

    Component {
        id: radioButtons
        Item {
            property ButtonGroup group: ButtonGroup { id: group }
            property RadioButton control1: RadioButton { ButtonGroup.group: group }
            property RadioButton control2: RadioButton { ButtonGroup.group: group }
            property RadioButton control3: RadioButton { ButtonGroup.group: group }
        }
    }

    Component {
        id: switches
        Item {
            property ButtonGroup group: ButtonGroup { id: group }
            property Switch control1: Switch { ButtonGroup.group: group }
            property Switch control2: Switch { ButtonGroup.group: group }
            property Switch control3: Switch { ButtonGroup.group: group }
        }
    }

    Component {
        id: childControls
        Item {
            id: container
            property ButtonGroup group: ButtonGroup { id: group; buttons: container.children }
            property alias control1: control1
            property alias control2: control2
            property alias control3: control3
            CheckBox { id: control1 }
            RadioButton { id: control2 }
            Switch { id: control3 }
        }
    }

    function test_controls_data() {
        return [
            { tag: "CheckBox", component: checkBoxes },
            { tag: "RadioButton", component: radioButtons },
            { tag: "Switch", component: switches },
            { tag: "Children", component: childControls }
        ]
    }

    function test_controls(data) {
        let container = createTemporaryObject(data.component, testCase)
        verify(container)

        verify(!container.group.checkedButton)

        container.control1.checked = true
        compare(container.group.checkedButton, container.control1)
        compare(container.control1.checked, true)
        compare(container.control2.checked, false)
        compare(container.control3.checked, false)

        container.control2.checked = true
        compare(container.group.checkedButton, container.control2)
        compare(container.control1.checked, false)
        compare(container.control2.checked, true)
        compare(container.control3.checked, false)

        container.control3.checked = true
        compare(container.group.checkedButton, container.control3)
        compare(container.control1.checked, false)
        compare(container.control2.checked, false)
        compare(container.control3.checked, true)
    }

    function test_buttonDestroyed() {
        let group = createTemporaryObject(buttonGroup, testCase)
        verify(group)

        let buttonsSpy = createTemporaryObject(signalSpy, testCase, {target: group, signalName: "buttonsChanged"})
        verify(buttonsSpy.valid)

        let button1 = createTemporaryObject(button, testCase, {objectName: "button1", checked: true})

        group.addButton(button1)
        compare(group.buttons.length, 1)
        compare(group.buttons[0], button1)
        compare(group.checkedButton, button1)
        compare(buttonsSpy.count, 1)

        button1.destroy()
        wait(0)
        compare(group.buttons.length, 0)
        compare(group.checkedButton, null)
        compare(buttonsSpy.count, 2)
    }

    Component {
        id: repeater
        Column {
            id: column
            property ButtonGroup group: ButtonGroup {
                buttons: column.children.filter((child) => child !== r)
            }
            property alias repeater: r
            Repeater {
                id: r
                model: 3
                delegate: RadioDelegate {
                    checked: index == 0
                    objectName: index
                }
            }
        }
    }

    function test_repeater() {
        let container = createTemporaryObject(repeater, testCase)
        verify(container)

        verify(container.group.checkedButton)
        compare(container.group.checkedButton.objectName, "0")
    }

    function test_nonExclusive() {
        let group = createTemporaryObject(nonExclusiveGroup, testCase)
        verify(group)

        compare(group.checkState, Qt.Unchecked)

        let button1 = createTemporaryObject(button, testCase, {checked: true})
        group.addButton(button1)
        compare(button1.checked, true)
        compare(group.checkedButton, null)
        compare(group.checkState, Qt.Checked)

        let button2 = createTemporaryObject(button, testCase, {checked: true})
        group.addButton(button2)
        compare(button1.checked, true)
        compare(button2.checked, true)
        compare(group.checkedButton, null)
        compare(group.checkState, Qt.Checked)

        let button3 = createTemporaryObject(button, testCase, {checked: false})
        group.addButton(button3)
        compare(button1.checked, true)
        compare(button2.checked, true)
        compare(button3.checked, false)
        compare(group.checkedButton, null)
        compare(group.checkState, Qt.PartiallyChecked)

        button1.checked = false
        compare(button1.checked, false)
        compare(button2.checked, true)
        compare(button3.checked, false)
        compare(group.checkedButton, null)
        compare(group.checkState, Qt.PartiallyChecked)

        button2.checked = false
        compare(button1.checked, false)
        compare(button2.checked, false)
        compare(button3.checked, false)
        compare(group.checkedButton, null)
        compare(group.checkState, Qt.Unchecked)

        button1.checked = true
        compare(button1.checked, true)
        compare(button2.checked, false)
        compare(button3.checked, false)
        compare(group.checkedButton, null)
        compare(group.checkState, Qt.PartiallyChecked)

        button2.checked = true
        compare(button1.checked, true)
        compare(button2.checked, true)
        compare(button3.checked, false)
        compare(group.checkedButton, null)
        compare(group.checkState, Qt.PartiallyChecked)

        button3.checked = true
        compare(button1.checked, true)
        compare(button2.checked, true)
        compare(button3.checked, true)
        compare(group.checkedButton, null)
        compare(group.checkState, Qt.Checked)
    }

    Component {
        id: checkedButtonColumn
        Column {
            id: column
            ButtonGroup { buttons: column.children.filter((child) => child !== repeater) }
            Repeater {
                id: repeater
                delegate: Button {
                    checkable: true
                    text: modelData
                    onClicked: listModel.remove(index)
                }
                model: ListModel {
                    id: listModel
                    Component.onCompleted: {
                        for (let i = 0; i < 10; ++i)
                            append({text: i})
                    }
                }
            }
        }
    }

    function test_checkedButtonDestroyed() {
        let column = createTemporaryObject(checkedButtonColumn, testCase)
        verify(column)

        waitForRendering(column)
        mouseClick(column.children[0])
        wait(0) // don't crash (QTBUG-62946, QTBUG-63470)
    }

    Component {
        id: buttonGroupComp

        Item {

            property ButtonGroup buttonGroup: ButtonGroup { }
            property int buttonGroupCount: buttonGroup.buttons.length
            property int buttonGrpSigCnt: 0

            function clearButtonGroup() {
                buttonGroup.buttons = []
            }
            function assignButtonGroup() {
                radioButton1.ButtonGroup.group = buttonGroup
            }

            Column {
                RadioButton {
                    id: radioButton1
                    visible: false
                    ButtonGroup.group: buttonGroup
                    ButtonGroup.onGroupChanged: { (ButtonGroup.group === null) ? --buttonGrpSigCnt : ++buttonGrpSigCnt }
                }
                RadioButton {
                    id: radioButton2
                    visible: false
                    ButtonGroup.group: buttonGroup
                    ButtonGroup.onGroupChanged: { (ButtonGroup.group === null) ? --buttonGrpSigCnt : ++buttonGrpSigCnt }
                }
            }
        }
    }

    function test_resetButtonGroup() {
        let container = createTemporaryObject(buttonGroupComp, testCase)
        verify(container)

        // Check for initial buttons assigned to button group
        compare(container.buttonGroupCount, 2)
        compare(container.buttonGrpSigCnt, 2)
        // Clear buttons in the button group
        container.clearButtonGroup()
        // Check for buttons in the group and group changed event for buttons
        compare(container.buttonGroupCount, 0)
        compare(container.buttonGrpSigCnt, 0)
        // Assign same group to the button
        container.assignButtonGroup()
        // Check for group change event from the button
        compare(container.buttonGroupCount, 1)
        compare(container.buttonGrpSigCnt, 1)
    }
}
