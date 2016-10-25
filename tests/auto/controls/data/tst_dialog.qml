/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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

import QtQuick 2.4
import QtTest 1.0
import QtQuick.Controls 2.1
import QtQuick.Templates 2.1 as T

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
        id: buttonBox
        DialogButtonBox { }
    }

    Component {
        id: headerBox
        DialogButtonBox { position: DialogButtonBox.Header }
    }

    Component {
        id: footerBox
        DialogButtonBox { position: DialogButtonBox.Footer }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    function test_defaults() {
        var control = dialog.createObject(testCase)
        verify(control)
        verify(!control.header)
        verify(!control.footer)
        verify(control.buttonBox)
        compare(control.standardButtons, 0)
        control.destroy()
    }

    function test_accept() {
        var control = dialog.createObject(testCase)

        control.open()
        waitForRendering(control.contentItem)
        verify(control.visible)

        var acceptedSpy = signalSpy.createObject(testCase, {target: control, signalName: "accepted"})
        verify(acceptedSpy.valid)
        control.accept()
        compare(acceptedSpy.count, 1)

        tryCompare(control, "visible", false)

        control.destroy()
    }

    function test_reject() {
        var control = dialog.createObject(testCase)

        control.open()
        waitForRendering(control.contentItem)
        verify(control.visible)

        var rejectedSpy = signalSpy.createObject(testCase, {target: control, signalName: "rejected"})
        verify(rejectedSpy.valid)
        control.reject()
        compare(rejectedSpy.count, 1)

        tryCompare(control, "visible", false)

        control.destroy()
    }

    function test_buttonBox_data() {
        return [
            { tag: "default header", property: "header", buttonBox: headerBox },
            { tag: "default footer", property: "footer", buttonBox: footerBox },
            { tag: "custom header", property: "header", position: DialogButtonBox.Header },
            { tag: "custom footer", property: "footer", position: DialogButtonBox.Footer }
        ]
    }

    function test_buttonBox(data) {
        var control = dialog.createObject(testCase)

        if (data.buttonBox)
            control.buttonBox = data.buttonBox
        else
            control[data.property] = buttonBox.createObject(testCase, {position: data.position})
        control.standardButtons = Dialog.Ok | Dialog.Cancel
        var box = control[data.property]
        verify(box)
        compare(box.standardButtons, Dialog.Ok | Dialog.Cancel)

        var acceptedSpy = signalSpy.createObject(testCase, {target: control, signalName: "accepted"})
        verify(acceptedSpy.valid)
        box.accepted()
        compare(acceptedSpy.count, 1)

        var rejectedSpy = signalSpy.createObject(testCase, {target: control, signalName: "rejected"})
        verify(rejectedSpy.valid)
        box.rejected()
        compare(rejectedSpy.count, 1)

        control.destroy()
    }

    function test_standardButtons() {
        var control = dialog.createObject(testCase)

        control.standardButtons = Dialog.Ok

        var box = control.footer ? control.footer : control.header
        verify(box)
        compare(box.count, 1)
        var okButton = box.itemAt(0)
        verify(okButton)
        compare(okButton.text.toUpperCase(), "OK")

        control.standardButtons = Dialog.Cancel
        compare(box.count, 1)
        var cancelButton = control.footer.itemAt(0)
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

        control.destroy()
    }

    function test_warnings() {
        var control = dialog.createObject(testCase)
        verify(control)

        var testComponent = Qt.createComponent("TestItem.qml")
        verify(testComponent)

        control.buttonBox = headerBox
        control.header = testComponent.createObject(testCase)
        ignoreWarning(Qt.resolvedUrl("tst_dialog.qml") + ":56:9: QML Dialog: Custom header detected. Cannot assign buttonBox as a header. No standard buttons will appear in the header.")
        control.standardButtons = Dialog.Apply

        control.buttonBox = footerBox
        control.footer = testComponent.createObject(testCase)
        ignoreWarning(Qt.resolvedUrl("tst_dialog.qml") + ":56:9: QML Dialog: Custom footer detected. Cannot assign buttonBox as a footer. No standard buttons will appear in the footer.")
        control.standardButtons = Dialog.Cancel

        control.buttonBox = testComponent
        ignoreWarning(Qt.resolvedUrl("tst_dialog.qml") + ":56:9: QML Dialog: buttonBox must be an instance of DialogButtonBox")
        control.standardButtons = Dialog.Ok

        control.destroy()
    }

    function test_layout() {
        var control = dialog.createObject(testCase, {width: 100, height: 100})
        verify(control)

        control.open()
        waitForRendering(control.contentItem)
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

        control.header.implicitWidth = 150
        compare(control.implicitWidth, control.header.implicitWidth)

        control.footer.implicitWidth = 160
        compare(control.implicitWidth, control.footer.implicitWidth)

        control.destroy()
    }
}
