// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 600
    height: 400
    visible: true
    when: windowShown
    name: "DialogButtonBox"

    Component {
        id: buttonBox
        DialogButtonBox { }
    }

    Component {
        id: button
        Button { }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(buttonBox, testCase)
        verify(control)
        compare(control.count, 0)
        verify(control.delegate)
        compare(control.standardButtons, 0)
    }

    function test_standardButtons() {
        let control = createTemporaryObject(buttonBox, testCase)
        verify(control)
        compare(control.count, 0)

        control.standardButtons = DialogButtonBox.Ok
        compare(control.count, 1)
        let okButton = control.itemAt(0)
        verify(okButton)
        compare(okButton.text.toUpperCase(), "OK")

        control.standardButtons = DialogButtonBox.Cancel
        compare(control.count, 1)
        let cancelButton = control.itemAt(0)
        verify(cancelButton)
        compare(cancelButton.text.toUpperCase(), "CANCEL")

        control.standardButtons = DialogButtonBox.Ok | DialogButtonBox.Cancel
        compare(control.count, 2)
        if (control.itemAt(0).text.toUpperCase() === "OK") {
            okButton = control.itemAt(0)
            cancelButton = control.itemAt(1)
        } else {
            okButton = control.itemAt(1)
            cancelButton = control.itemAt(0)
        }
        verify(okButton)
        verify(cancelButton)
        compare(okButton.text.toUpperCase(), "OK")
        compare(cancelButton.text.toUpperCase(), "CANCEL")
        compare(control.standardButton(DialogButtonBox.Ok), okButton)
        compare(control.standardButton(DialogButtonBox.Cancel), cancelButton)

        control.standardButtons = 0
        compare(control.count, 0)

        compare(control.standardButton(DialogButtonBox.Ok), null)
        compare(control.standardButton(DialogButtonBox.Cancel), null)
    }

    function test_attached() {
        let control = createTemporaryObject(buttonBox, testCase)
        verify(control)

        control.standardButtons = DialogButtonBox.Ok
        let okButton = control.itemAt(0)
        compare(okButton.DialogButtonBox.buttonBox, control)
        compare(okButton.DialogButtonBox.buttonRole, DialogButtonBox.AcceptRole)

        let saveButton = button.createObject(control, {text: "Save"})
        compare(saveButton.DialogButtonBox.buttonBox, control)
        compare(saveButton.DialogButtonBox.buttonRole, DialogButtonBox.InvalidRole)
        saveButton.DialogButtonBox.buttonRole = DialogButtonBox.AcceptRole
        compare(saveButton.DialogButtonBox.buttonRole, DialogButtonBox.AcceptRole)

        let closeButton = createTemporaryObject(button, null, {text: "Save"})
        compare(closeButton.DialogButtonBox.buttonBox, null)
        compare(closeButton.DialogButtonBox.buttonRole, DialogButtonBox.InvalidRole)
        closeButton.DialogButtonBox.buttonRole = DialogButtonBox.DestructiveRole
        compare(closeButton.DialogButtonBox.buttonRole, DialogButtonBox.DestructiveRole)
        control.addItem(closeButton)
        compare(closeButton.DialogButtonBox.buttonBox, control)

        control.contentModel.clear()
        compare(okButton.DialogButtonBox.buttonBox, null)
        compare(saveButton.DialogButtonBox.buttonBox, null)
        compare(closeButton.DialogButtonBox.buttonBox, null)
    }

    function test_signals_data() {
        return [
            { tag: "Ok", standardButton: DialogButtonBox.Ok, buttonRole: DialogButtonBox.AcceptRole, signalName: "accepted" },
            { tag: "Open", standardButton: DialogButtonBox.Open, buttonRole: DialogButtonBox.AcceptRole, signalName: "accepted" },
            { tag: "Save", standardButton: DialogButtonBox.Save, buttonRole: DialogButtonBox.AcceptRole, signalName: "accepted" },
            { tag: "Cancel", standardButton: DialogButtonBox.Cancel, buttonRole: DialogButtonBox.RejectRole, signalName: "rejected" },
            { tag: "Close", standardButton: DialogButtonBox.Close, buttonRole: DialogButtonBox.RejectRole, signalName: "rejected" },
            { tag: "Discard", standardButton: DialogButtonBox.Discard, buttonRole: DialogButtonBox.DestructiveRole, signalName: "discarded" },
            { tag: "Apply", standardButton: DialogButtonBox.Apply, buttonRole: DialogButtonBox.ApplyRole, signalName: "applied" },
            { tag: "Reset", standardButton: DialogButtonBox.Reset, buttonRole: DialogButtonBox.ResetRole, signalName: "reset" },
            { tag: "RestoreDefaults", standardButton: DialogButtonBox.RestoreDefaults, buttonRole: DialogButtonBox.ResetRole, signalName: "reset" },
            { tag: "Help", standardButton: DialogButtonBox.Help, buttonRole: DialogButtonBox.HelpRole, signalName: "helpRequested" },
            { tag: "SaveAll", standardButton: DialogButtonBox.SaveAll, buttonRole: DialogButtonBox.AcceptRole, signalName: "accepted" },
            { tag: "Yes", standardButton: DialogButtonBox.Yes, buttonRole: DialogButtonBox.YesRole, signalName: "accepted" },
            { tag: "YesToAll", standardButton: DialogButtonBox.YesToAll, buttonRole: DialogButtonBox.YesRole, signalName: "accepted" },
            { tag: "No", standardButton: DialogButtonBox.No, buttonRole: DialogButtonBox.NoRole, signalName: "rejected" },
            { tag: "NoToAll", standardButton: DialogButtonBox.NoToAll, buttonRole: DialogButtonBox.NoRole, signalName: "rejected" },
            { tag: "Abort", standardButton: DialogButtonBox.Abort, buttonRole: DialogButtonBox.RejectRole, signalName: "rejected" },
            { tag: "Retry", standardButton: DialogButtonBox.Retry, buttonRole: DialogButtonBox.AcceptRole, signalName: "accepted" },
            { tag: "Ignore", standardButton: DialogButtonBox.Ignore, buttonRole: DialogButtonBox.AcceptRole, signalName: "accepted" }
        ]
    }

    function test_signals(data) {
        let control = createTemporaryObject(buttonBox, testCase)
        verify(control)

        control.standardButtons = data.standardButton
        compare(control.count, 1)
        let button = control.itemAt(0)
        verify(button)
        compare(button.DialogButtonBox.buttonRole, data.buttonRole)

        let clickedSpy = signalSpy.createObject(control, {target: control, signalName: "clicked"})
        verify(clickedSpy.valid)
        let roleSpy = signalSpy.createObject(control, {target: control, signalName: data.signalName})
        verify(roleSpy.valid)

        button.clicked()
        compare(clickedSpy.count, 1)
        compare(clickedSpy.signalArguments[0][0], button)
        compare(roleSpy.count, 1)
    }

    function test_buttonLayout_data() {
        return [
            { tag: "WinLayout", buttonLayout: DialogButtonBox.WinLayout, button1Role: DialogButtonBox.AcceptRole, button2Role: DialogButtonBox.RejectRole },
            { tag: "MacLayout", buttonLayout: DialogButtonBox.MacLayout, button1Role: DialogButtonBox.RejectRole, button2Role: DialogButtonBox.AcceptRole },
            { tag: "KdeLayout", buttonLayout: DialogButtonBox.KdeLayout, button1Role: DialogButtonBox.AcceptRole, button2Role: DialogButtonBox.RejectRole },
            { tag: "GnomeLayout", buttonLayout: DialogButtonBox.GnomeLayout, button1Role: DialogButtonBox.RejectRole, button2Role: DialogButtonBox.AcceptRole },
            { tag: "AndroidLayout", buttonLayout: DialogButtonBox.AndroidLayout, button1Role: DialogButtonBox.RejectRole, button2Role: DialogButtonBox.AcceptRole }
        ]
    }

    function test_buttonLayout(data) {
        let control = createTemporaryObject(buttonBox, testCase, {buttonLayout: data.buttonLayout, standardButtons: DialogButtonBox.Ok|DialogButtonBox.Cancel})
        verify(control)

        compare(control.count, 2)

        let button1 = control.itemAt(0)
        verify(button1)
        compare(button1.DialogButtonBox.buttonRole, data.button1Role)

        let button2 = control.itemAt(1)
        verify(button2)
        compare(button2.DialogButtonBox.buttonRole, data.button2Role)
    }

    function test_implicitSize_data() {
        return [
            { tag: "Ok", standardButtons: DialogButtonBox.Ok },
            { tag: "Yes|No", standardButtons: DialogButtonBox.Yes | DialogButtonBox.No }
        ]
    }

    // QTBUG-59719
    function test_implicitSize(data) {
        let control = createTemporaryObject(buttonBox, testCase, {standardButtons: data.standardButtons})
        verify(control)

        let listView = control.contentItem
        verify(listView && listView.hasOwnProperty("contentWidth"))
        waitForRendering(listView)

        let implicitContentWidth = control.leftPadding + control.rightPadding
        for (let i = 0; i < listView.contentItem.children.length; ++i) {
            let button = listView.contentItem.children[i]
            if (!button.hasOwnProperty("text"))
                continue
            implicitContentWidth += button.implicitWidth
        }

        verify(implicitContentWidth > control.leftPadding + control.rightPadding)
        verify(control.implicitWidth >= implicitContentWidth, qsTr("implicit width (%1) is less than content width (%2)").arg(control.implicitWidth).arg(implicitContentWidth))
    }

    Component {
        id: okCancelBox
        DialogButtonBox {
            Button {
                text: qsTr("OK")
            }
            Button {
                text: qsTr("Cancel")
            }
        }
    }

    function test_buttonSize() {
        let control = createTemporaryObject(okCancelBox, testCase)
        verify(control)

        let okButton = control.itemAt(0)
        verify(okButton)
        verify(okButton.width > 0)

        let cancelButton = control.itemAt(1)
        verify(cancelButton)
        verify(cancelButton.width > 0)

        compare(okButton.width + cancelButton.width, control.availableWidth - control.spacing)
    }

    function test_oneButtonInFixedWidthBox() {
        let control = createTemporaryObject(buttonBox, testCase,
            { width: 400, standardButtons: Dialog.Close })
        verify(control)

        let listView = control.contentItem
        waitForRendering(listView)

        let button = control.itemAt(0)
        verify(button)

        // The button should never go outside of the box.
        tryVerify(function() { return button.mapToItem(control, 0, 0).x >= 0 },
            1000, "Expected left edge of button to be within left edge of DialogButtonBox (i.e. greater than or equal to 0)" +
                ", but it's " + button.mapToItem(control, 0, 0).x)
        tryVerify(function() { return button.mapToItem(control, 0, 0).x + button.width <= control.width },
            1000, "Expected right edge of button to be within right edge of DialogButtonBox (i.e. less than or equal to " +
                control.width + "), but it's " + (button.mapToItem(control, 0, 0).x + button.width))
    }

    Component {
        id: dialogComponent
        // Based on the Basic style, where a single button fills
        // half the dialog's width and is aligned to the right.
        Dialog {
            id: control
            standardButtons: Dialog.Ok
            visible: true

            footer: DialogButtonBox {
                id: box
                visible: count > 0
                alignment: count === 1 ? Qt.AlignRight : undefined

                implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                    (count === 1 ? implicitContentWidth * 2 : implicitContentWidth) + leftPadding + rightPadding)
                implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                    implicitContentHeight + topPadding + bottomPadding)
                contentWidth: contentItem.contentWidth

                delegate: Button {
                    width: box.count === 1 ? box.availableWidth / 2 : undefined
                }
            }
        }
    }

    // QTBUG-73860
    function test_oneButtonAlignedRightInImplicitWidthBox() {
        let dialog = createTemporaryObject(dialogComponent, testCase)
        verify(dialog)

        let box = dialog.footer
        let listView = box.contentItem
        waitForRendering(listView)

        let button = box.itemAt(0)
        verify(button)

        // The button should never go outside of the box.
        tryVerify(function() { return button.mapToItem(box, 0, 0).x >= 0 },
            1000, "Expected left edge of button to be within left edge of DialogButtonBox (i.e. greater than or equal to 0)" +
                ", but it's " + button.mapToItem(box, 0, 0).x)
        tryVerify(function() { return button.mapToItem(box, 0, 0).x + button.width <= box.width },
            1000, "Expected right edge of button to be within right edge of DialogButtonBox (i.e. less than or equal to " +
                box.width + "), but it's " + (button.mapToItem(box, 0, 0).x + button.width))
        compare(box.width, dialog.width)
        // There's a single button and we align it to the right.
        compare(box.contentItem.width, button.width)
        compare(box.contentItem.x, box.width - box.rightPadding - box.contentItem.width)
    }

    Component {
        id: customButtonBox

        DialogButtonBox {
            objectName: "customButtonBox"
            alignment: Qt.AlignRight

            property alias okButton: okButton

            Button {
                id: okButton
                text: "OK"

                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }
    }

    Component {
        id: customButtonBoxTwoButtons

        DialogButtonBox {
            objectName: "customButtonBoxTwoButtons"
            alignment: Qt.AlignRight

            property alias okButton: okButton

            Button {
                id: okButton
                text: "OK"

                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
            Button {
                text: "Cancel"

                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
        }
    }

    function test_changeCustomButtonText_data() {
        return [
            { tag: "oneButton", component: customButtonBox },
            { tag: "twoButtons", component: customButtonBoxTwoButtons },
        ]
    }

    // QTBUG-72886
    function test_changeCustomButtonText(data) {
        let control = createTemporaryObject(data.component, testCase, {})
        verify(control)

        let listView = control.contentItem
        waitForRendering(listView)

        let button = control.okButton
        verify(button)
        button.text = "some longer text";

        // The button should never go outside of the box.
        tryVerify(function() { return button.mapToItem(control, 0, 0).x >= 0 },
            1000, "Expected left edge of button to be within left edge of DialogButtonBox (i.e. greater than or equal to 0)" +
                ", but it's " + button.mapToItem(control, 0, 0).x)
        tryVerify(function() { return button.mapToItem(control, 0, 0).x + button.width <= control.width },
            1000, "Expected right edge of button to be within right edge of DialogButtonBox (i.e. less than or equal to " +
                control.width + "), but it's " + (button.mapToItem(control, 0, 0).x + button.width))
    }

    Component {
        id: customButtonBoxInDialog

        Dialog {
            width: 300
            visible: true

            footer: DialogButtonBox {
                objectName: "customButtonBoxInDialog"
                alignment: Qt.AlignRight

                property alias okButton: okButton

                Button {
                    id: okButton
                    text: "OK"

                    DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                }
            }
        }
    }

    Component {
        id: customButtonBoxTwoButtonsInDialog

        Dialog {
            width: 300
            visible: true

            footer: DialogButtonBox {
                objectName: "customButtonBoxTwoButtonsInDialog"
                alignment: Qt.AlignRight

                property alias okButton: okButton

                Button {
                    id: okButton
                    text: "OK"

                    DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                }
                Button {
                    text: "Cancel"

                    DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                }
            }
        }
    }

    function test_changeCustomButtonImplicitWidth_data() {
        return [
            { tag: "oneButton", component: customButtonBoxInDialog },
            { tag: "twoButtons", component: customButtonBoxTwoButtonsInDialog },
        ]
    }

    // QTBUG-102558
    function test_changeCustomButtonImplicitWidth(data) {
        let dialog = createTemporaryObject(data.component, testCase, {})
        verify(dialog)

        let control = dialog.footer
        verify(control)

        let listView = control.contentItem
        waitForRendering(listView)

        let button = control.okButton
        verify(button)
        button.implicitWidth *= 1.5

        // The button should never go outside of the box.
        tryVerify(function() { return button.mapToItem(control, 0, 0).x >= 0 },
            1000, "Expected left edge of button to be within left edge of DialogButtonBox (i.e. greater than or equal to 0)" +
                ", but it's " + button.mapToItem(control, 0, 0).x)
        tryVerify(function() { return button.mapToItem(control, 0, 0).x + button.width <= control.width },
            1000, "Expected right edge of button to be within right edge of DialogButtonBox (i.e. less than or equal to " +
                control.width + "), but it's " + (button.mapToItem(control, 0, 0).x + button.width))
    }

    Component {
        id: noRolesDialog

        Dialog {
            footer: DialogButtonBox {
                Button { text: "A" }
                Button { text: "B" }
                Button { text: "C" }
            }
        }
    }

    function test_orderWithNoRoles() {
        for (let i = 0; i < 10; ++i) {
            let control = createTemporaryObject(noRolesDialog, testCase)
            verify(control)

            control.open()
            tryCompare(control, "opened", true)
            let footer = control.footer
            verify(footer)
            waitForItemPolished(footer.contentItem)
            compare(footer.itemAt(0).text, "A")
            compare(footer.itemAt(1).text, "B")
            compare(footer.itemAt(2).text, "C")

            control.destroy()
        }
    }

    Component {
        id: contentItemDeletionOrder1

        Item {
            objectName: "parentItem"

            Item {
                id: item
                objectName: "contentItem"
            }
            DialogButtonBox {
                objectName: "control"
                contentItem: item
            }
        }
    }

    Component {
        id: contentItemDeletionOrder2

        Item {
            objectName: "parentItem"

            DialogButtonBox {
                objectName: "control"
                contentItem: item
            }
            Item {
                id: item
                objectName: "contentItem"
            }
        }
    }

    function test_contentItemDeletionOrder() {
        let control1 = createTemporaryObject(contentItemDeletionOrder1, testCase)
        verify(control1)
        let control2 = createTemporaryObject(contentItemDeletionOrder2, testCase)
        verify(control2)
    }

    Component {
        id: backgroundDeletionOrder1

        Item {
            objectName: "parentItem"

            Item {
                id: item
                objectName: "backgroundItem"
            }
            DialogButtonBox {
                objectName: "control"
                background: item
            }
        }
    }

    Component {
        id: backgroundDeletionOrder2

        Item {
            objectName: "parentItem"

            DialogButtonBox {
                objectName: "control"
                background: item
            }
            Item {
                id: item
                objectName: "backgroundItem"
            }
        }
    }

    function test_backgroundDeletionOrder() {
        let control1 = createTemporaryObject(backgroundDeletionOrder1, testCase)
        verify(control1)
        let control2 = createTemporaryObject(backgroundDeletionOrder2, testCase)
        verify(control2)
    }
}
