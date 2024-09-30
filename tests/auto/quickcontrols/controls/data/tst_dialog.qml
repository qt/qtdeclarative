// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtTest
import QtQuick.Controls
import QtQuick.Templates as T

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "Dialog"

    Component {
        id: dialog
        Dialog { }
    }

    Component {
        id: qtbug71444
        Dialog {
            header: null
            footer: null
        }
    }

    Component {
        id: buttonBox
        DialogButtonBox { }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    function init() {
        failOnWarning(/.?/)
        tryCompare(testCase.Window.window, "active", true)
    }

    function test_defaults() {
        let control = createTemporaryObject(dialog, testCase)
        verify(control)
        verify(control.header)
        verify(control.footer)
        compare(control.standardButtons, 0)
        verify(control.focus)
    }

    function test_accept() {
        let control = createTemporaryObject(dialog, testCase)

        let openedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "opened"})
        verify(openedSpy.valid)

        control.open()
        openedSpy.wait()
        compare(openedSpy.count, 1)
        verify(control.visible)

        let acceptedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "accepted"})
        verify(acceptedSpy.valid)

        let closedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "closed"})
        verify(closedSpy.valid)

        control.accept()
        compare(acceptedSpy.count, 1)
        compare(control.result, Dialog.Accepted)

        tryCompare(control, "visible", false)
        compare(acceptedSpy.count, 1)
        compare(closedSpy.count, 1)
    }

    function test_reject() {
        let control = createTemporaryObject(dialog, testCase)
        let openedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "opened"})
        verify(openedSpy.valid)

        control.popupType = Popup.Item
        control.open()
        openedSpy.wait()
        compare(openedSpy.count, 1)
        verify(control.visible)

        let rejectedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "rejected"})
        verify(rejectedSpy.valid)

        let closedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "closed"})
        verify(closedSpy.valid)

        control.reject()
        compare(rejectedSpy.count, 1)
        compare(control.result, Dialog.Rejected)

        tryCompare(control, "visible", false)
        compare(rejectedSpy.count, 1)
        compare(closedSpy.count, 1)

        // Check that rejected() is emitted when CloseOnEscape is triggered.
        control.x = 10
        control.y = 10
        control.width = 100
        control.height = 100
        control.closePolicy = Popup.CloseOnEscape
        control.open()
        verify(control.visible)

        keyPress(Qt.Key_Escape)
        compare(rejectedSpy.count, 2)
        tryCompare(control, "visible", false)
        compare(rejectedSpy.count, 2)
        compare(closedSpy.count, 2)

        keyRelease(Qt.Key_Escape)
        compare(rejectedSpy.count, 2)
        compare(closedSpy.count, 2)

        // Check that rejected() is emitted when CloseOnPressOutside is triggered.
        control.closePolicy = Popup.CloseOnPressOutside
        control.open()
        verify(control.visible)
        // wait for enter transitions to finish
        openedSpy.wait()

        mousePress(testCase, 1, 1)
        compare(rejectedSpy.count, 3)
        tryCompare(control, "visible", false)
        compare(rejectedSpy.count, 3)
        compare(closedSpy.count, 3)

        mouseRelease(testCase, 1, 1)
        compare(rejectedSpy.count, 3)
        compare(closedSpy.count, 3)

        // Check that rejected() is emitted when CloseOnReleaseOutside is triggered.
        // For this, we need to make the dialog modal, because the overlay won't accept
        // the press event because it doesn't want to block the press.
        control.modal = true
        control.closePolicy = Popup.CloseOnReleaseOutside
        control.open()
        verify(control.visible)
        openedSpy.wait()

        mousePress(testCase, 1, 1)
        compare(rejectedSpy.count, 3)
        verify(control.visible)

        mouseRelease(testCase, 1, 1)
        compare(rejectedSpy.count, 4)
        tryCompare(control, "visible", false)
        compare(rejectedSpy.count, 4)
        compare(closedSpy.count, 4)
    }

    function test_destructiveRoleDialogClose() {
        let control = createTemporaryObject(dialog, testCase)
        verify(control)

        // Set up the dialog with a DestructiveRole button
        control.standardButtons = Dialog.Discard

        let discardedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "discarded"})
        verify(discardedSpy.valid)

        let closedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "closed"})
        verify(closedSpy.valid)

        control.open()
        verify(control.visible)

        let discardButton = control.standardButton(Dialog.Discard)
        verify(discardButton)
        discardButton.clicked()

        // Check that the discarded() signal is emitted
        compare(discardedSpy.count, 1)

        if (control.visible)
            control.close()

       // Check that the dialog is closed
       tryCompare(control, "visible", false)
       compare(closedSpy.count, 1)
    }

    function test_buttonBox_data() {
        return [
            { tag: "default" },
            { tag: "custom", custom: true }
        ]
    }

    function test_buttonBox(data) {
        let control = createTemporaryObject(dialog, testCase)

        if (data.custom)
            control.footer = buttonBox.createObject(testCase)
        control.standardButtons = Dialog.Ok | Dialog.Cancel
        let box = control.footer
        verify(box)
        compare(box.standardButtons, Dialog.Ok | Dialog.Cancel)

        let acceptedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "accepted"})
        verify(acceptedSpy.valid)
        box.accepted()
        compare(acceptedSpy.count, 1)

        let rejectedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "rejected"})
        verify(rejectedSpy.valid)
        box.rejected()
        compare(rejectedSpy.count, 1)
    }

    function test_qtbug71444() {
        let control = createTemporaryObject(qtbug71444, testCase)
        verify(control)
    }

    function test_standardButtons() {
        let control = createTemporaryObject(dialog, testCase)

        control.standardButtons = Dialog.Ok

        let box = control.footer ? control.footer : control.header
        verify(box)
        compare(box.count, 1)
        let okButton = box.itemAt(0)
        verify(okButton)
        compare(okButton.text.toUpperCase(), "OK")

        control.standardButtons = Dialog.Cancel
        compare(box.count, 1)
        let cancelButton = control.footer.itemAt(0)
        verify(cancelButton)
        compare(cancelButton.text.toUpperCase(), "CANCEL")

        control.standardButtons = Dialog.Ok | Dialog.Cancel
        compare(box.count, 2)
        if (box.itemAt(0).text.toUpperCase() === "OK") {
            okButton = box.itemAt(0)
            cancelButton = box.itemAt(1)
        } else {
            okButton = box.itemAt(1)
            cancelButton = box.itemAt(0)
        }
        verify(okButton)
        verify(cancelButton)
        compare(okButton.text.toUpperCase(), "OK")
        compare(cancelButton.text.toUpperCase(), "CANCEL")

        control.standardButtons = 0
        compare(box.count, 0)
    }

    function test_layout() {
        let control = createTemporaryObject(dialog, testCase, {width: 100, height: 100})
        verify(control)

        let openedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "opened"})
        verify(openedSpy.valid)

        control.open()
        openedSpy.wait()
        compare(openedSpy.count, 1)
        verify(control.visible)

        compare(control.width, 100)
        compare(control.height, 100)
        compare(control.contentItem.width, control.availableWidth)
        compare(control.contentItem.height, control.availableHeight)

        control.header = buttonBox.createObject(control.contentItem)
        compare(control.header.width, control.width)
        verify(control.header.height > 0)
        compare(control.contentItem.width, control.availableWidth)
        compare(control.contentItem.height, control.availableHeight - control.header.height)

        control.footer = buttonBox.createObject(control.contentItem)
        compare(control.footer.width, control.width)
        verify(control.footer.height > 0)
        compare(control.contentItem.width, control.availableWidth)
        compare(control.contentItem.height, control.availableHeight - control.header.height - control.footer.height)

        control.topPadding = 9
        control.leftPadding = 2
        control.rightPadding = 6
        control.bottomPadding = 7

        compare(control.header.x, 0)
        compare(control.header.y, 0)
        compare(control.header.width, control.width)
        verify(control.header.height > 0)

        compare(control.footer.x, 0)
        compare(control.footer.y, control.height - control.footer.height)
        compare(control.footer.width, control.width)
        verify(control.footer.height > 0)

        compare(control.contentItem.x, control.leftPadding)
        compare(control.contentItem.y, control.topPadding + control.header.height)
        compare(control.contentItem.width, control.availableWidth)
        compare(control.contentItem.height, control.availableHeight - control.header.height - control.footer.height)

        control.header.visible = false
        compare(control.contentItem.x, control.leftPadding)
        compare(control.contentItem.y, control.topPadding)
        compare(control.contentItem.width, control.availableWidth)
        compare(control.contentItem.height, control.availableHeight - control.footer.height)

        control.footer.visible = false
        compare(control.contentItem.x, control.leftPadding)
        compare(control.contentItem.y, control.topPadding)
        compare(control.contentItem.width, control.availableWidth)
        compare(control.contentItem.height, control.availableHeight)

        control.contentItem.implicitWidth = 50
        control.contentItem.implicitHeight = 60
        compare(control.implicitWidth, control.contentItem.implicitWidth + control.leftPadding + control.rightPadding)
        compare(control.implicitHeight, control.contentItem.implicitHeight + control.topPadding + control.bottomPadding)

        control.header.visible = true
        compare(control.implicitHeight, control.contentItem.implicitHeight + control.topPadding + control.bottomPadding
                                      + control.header.implicitHeight)

        control.footer.visible = true
        compare(control.implicitHeight, control.contentItem.implicitHeight + control.topPadding + control.bottomPadding
                                      + control.header.implicitHeight + control.footer.implicitHeight)

        control.footer.implicitWidth = 0
        control.header.implicitWidth = 150
        compare(control.implicitWidth, control.header.implicitWidth)

        control.footer.implicitWidth = 160
        compare(control.implicitWidth, control.footer.implicitWidth)
    }

    function test_spacing_data() {
        return [
            { tag: "content", header: false, content: true, footer: false },
            { tag: "header,content", header: true, content: true, footer: false },
            { tag: "content,footer", header: false, content: true, footer: true },
            { tag: "header,content,footer", header: true, content: true, footer: true },
            { tag: "header,footer", header: true, content: false, footer: true },
            { tag: "header", header: true, content: false, footer: false },
            { tag: "footer", header: false, content: false, footer: true },
        ]
    }

    function test_spacing(data) {
        let control = createTemporaryObject(dialog, testCase, {spacing: 20, width: 100, height: 100})
        verify(control)

        let openedSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "opened"})
        verify(openedSpy.valid)

        control.open()
        openedSpy.wait()
        compare(openedSpy.count, 1)
        verify(control.visible)

        control.contentItem.visible = data.content
        control.header = buttonBox.createObject(control.contentItem, {visible: data.header})
        control.footer = buttonBox.createObject(control.contentItem, {visible: data.footer})

        compare(control.header.x, 0)
        compare(control.header.y, 0)
        compare(control.header.width, control.width)
        verify(control.header.height > 0)

        compare(control.footer.x, 0)
        compare(control.footer.y, control.height - control.footer.height)
        compare(control.footer.width, control.width)
        verify(control.footer.height > 0)

        compare(control.contentItem.x, control.leftPadding)
        compare(control.contentItem.y, control.topPadding + (data.header ? control.header.height + control.spacing : 0))
        compare(control.contentItem.width, control.availableWidth)
        compare(control.contentItem.height, control.availableHeight
                                            - (data.header ? control.header.height + control.spacing : 0)
                                            - (data.footer ? control.footer.height + control.spacing : 0))
    }

    function test_signals_data() {
        return [
            { tag: "Ok", standardButton: Dialog.Ok, signalName: "accepted" },
            { tag: "Open", standardButton: Dialog.Open, signalName: "accepted" },
            { tag: "Save", standardButton: Dialog.Save, signalName: "accepted" },
            { tag: "Cancel", standardButton: Dialog.Cancel, signalName: "rejected" },
            { tag: "Close", standardButton: Dialog.Close, signalName: "rejected" },
            { tag: "Discard", standardButton: Dialog.Discard, signalName: "discarded" },
            { tag: "Apply", standardButton: Dialog.Apply, signalName: "applied" },
            { tag: "Reset", standardButton: Dialog.Reset, signalName: "reset" },
            { tag: "RestoreDefaults", standardButton: Dialog.RestoreDefaults, signalName: "reset" },
            { tag: "Help", standardButton: Dialog.Help, signalName: "helpRequested" },
            { tag: "SaveAll", standardButton: Dialog.SaveAll, signalName: "accepted" },
            { tag: "Yes", standardButton: Dialog.Yes, signalName: "accepted" },
            { tag: "YesToAll", standardButton: Dialog.YesToAll, signalName: "accepted" },
            { tag: "No", standardButton: Dialog.No, signalName: "rejected" },
            { tag: "NoToAll", standardButton: Dialog.NoToAll, signalName: "rejected" },
            { tag: "Abort", standardButton: Dialog.Abort, signalName: "rejected" },
            { tag: "Retry", standardButton: Dialog.Retry, signalName: "accepted" },
            { tag: "Ignore", standardButton: Dialog.Ignore, signalName: "accepted" }
        ]
    }

    function test_signals(data) {
        let control = createTemporaryObject(dialog, testCase)
        verify(control)

        control.standardButtons = data.standardButton
        let button = control.standardButton(data.standardButton)
        verify(button)

        let buttonSpy = signalSpy.createObject(control.contentItem, {target: control, signalName: data.signalName})
        verify(buttonSpy.valid)

        button.clicked()
        compare(buttonSpy.count, 1)
    }

    Component {
        id: qtbug85884
        ApplicationWindow {
            property alias focusItemActiveFocus: item.activeFocus
            property alias focusDialogVisible: dialog.visible
            function closeAndOpen() {
                dialog.close()
                dialog.open()
                dialog.close()
            }
            visible: true
            Item {
                id: item
                focus: true
            }
            Dialog {
                id: dialog
                focus: true
                visible: false
                onActiveFocusChanged: {
                    if (!activeFocus)
                        visible = false
                }
                enter: Transition {
                    NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 10 }
                }
                exit: Transition {
                    NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 10 }
                }
            }
        }
    }

    function test_focusLeavingDialog(data) {
        if (Qt.platform.pluginName === "offscreen")
            skip("QTBUG-89909")

        let window = createTemporaryObject(qtbug85884, testCase)
        verify(window)
        tryCompare(window, "focusItemActiveFocus", true)

        window.focusDialogVisible = true
        tryCompare(window, "focusDialogVisible", true)
        tryCompare(window, "focusItemActiveFocus", false)

        window.focusDialogVisible = false
        tryCompare(window, "focusDialogVisible", false)
        tryCompare(window, "focusItemActiveFocus", true)

        window.focusDialogVisible = true
        tryCompare(window, "focusDialogVisible", true)
        tryCompare(window, "focusItemActiveFocus", false)
        window.closeAndOpen()
        tryCompare(window, "focusDialogVisible", false)
        tryCompare(window, "focusItemActiveFocus", true)
    }
}
