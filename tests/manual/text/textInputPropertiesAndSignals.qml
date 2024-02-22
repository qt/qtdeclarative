// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    width: 600; height: 840

    GridLayout {
        columns: 3; rowSpacing: 6; columnSpacing: 6
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }

        // ====================================================
        CheckBox {
            id: enabledCB
            text: "enabled"
            checked: true
            Layout.fillWidth: false
        }

        // ====================================================
        Rule {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            text: "TextInput"
        }

        // ----------------------------------------------------
        CheckBox {
            id: textInputMouseSelCB
            text: "mouse select"
            onCheckedChanged: textInput.selectByMouse = checked
            Layout.fillWidth: false
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: textInput.implicitHeight + 8
            radius: 4; antialiasing: true
            border.color: "black"; color: "transparent"

            TextInput {
                id: textInput
                anchors.fill: parent; anchors.margins: 4
                enabled: enabledCB.checked
                Component.onCompleted: textInputMouseSelCB.checked = selectByMouse

                onTextEdited: textInputTextEditedInd.blip()
                onTextChanged: textInputTextChangedInd.blip()
                onPreeditTextChanged: textInputPreeditInd.blip()
                onDisplayTextChanged: textInputDisplayTextInd.blip()
                onSelectedTextChanged: textInputSelectedTextChangedInd.blip()
            }
        }

        SignalIndicator {
            id: textInputTextEditedInd
            label: "textEdited"
        }

        // ----------------------------------------------------
        Text { text: "text" }

        Text { text: textInput.text; Layout.fillWidth: true }

        SignalIndicator {
            id: textInputTextChangedInd
            label: "textChanged"
        }

        // ----------------------------------------------------
        Text { text: "preeditText" }

        Text { text: textInput.preeditText; Layout.fillWidth: true }

        SignalIndicator {
            id: textInputPreeditInd
            label: "preeditTextChanged"
        }

        // ----------------------------------------------------
        Text { text: "displayText" }

        Text { text: textInput.displayText; Layout.fillWidth: true }

        SignalIndicator {
            id: textInputDisplayTextInd
            label: "displayTextChanged"
        }

        // ----------------------------------------------------
        Text { text: "selectedText" }

        Text { text: textInput.selectedText; Layout.fillWidth: true }

        SignalIndicator {
            id: textInputSelectedTextChangedInd
            label: "selectedTextChanged"
        }

        // ====================================================
        Rule {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            text: "TextField"
        }

        // ----------------------------------------------------
        CheckBox {
            id: textFieldMouseSelCB
            text: "mouse select"
            onCheckedChanged: textField.selectByMouse = checked
            Layout.fillWidth: false
        }

        TextField {
            id: textField
            enabled: enabledCB.checked
            Layout.fillWidth: true
            Component.onCompleted: textFieldMouseSelCB.checked = selectByMouse

            onTextEdited: textFieldEditedInd.blip()
            onTextChanged: textFieldChangedInd.blip()
            onPreeditTextChanged: textFieldPreeditInd.blip()
            onDisplayTextChanged: textFieldDisplayTextInd.blip()
            onSelectedTextChanged: textFieldSelectedTextChangedInd.blip()
        }

        SignalIndicator {
            id: textFieldEditedInd
            label: "textEdited"
        }

        // ----------------------------------------------------
        Text { text: "text" }

        Text { text: textField.text; Layout.fillWidth: true }

        SignalIndicator {
            id: textFieldChangedInd
            label: "textChanged"
        }

        // ----------------------------------------------------
        Text { text: "preeditText" }

        Text { text: textField.preeditText; Layout.fillWidth: true }

        SignalIndicator {
            id: textFieldPreeditInd
            label: "preeditTextChanged"
        }

        // ----------------------------------------------------
        Text { text: "displayText" }

        Text { text: textField.displayText; Layout.fillWidth: true }

        SignalIndicator {
            id: textFieldDisplayTextInd
            label: "displayTextChanged"
        }

        // ----------------------------------------------------
        Text { text: "selectedText" }

        Text { text: textField.selectedText; Layout.fillWidth: true }

        SignalIndicator {
            id: textFieldSelectedTextChangedInd
            label: "selectedTextChanged"
        }

        // ====================================================
        Rule {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            text: "tools"
        }

        // ----------------------------------------------------
        TextField {
            id: copyFrom
            Layout.column: 1
            Layout.row: 14
            Layout.fillWidth: true
            text: "copy this"
        }

        Button {
            text: "setText"
            Layout.column: 2
            Layout.row: 14
            Layout.fillWidth: false
            onClicked: {
                Qt.inputMethod.reset()
                textInput.text = copyFrom.text
                textField.text = copyFrom.text
                textEdit.text = copyFrom.text
                textArea.text = copyFrom.text
            }
        }

        // ====================================================
        Rule {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            text: "TextEdit in Flickable"
        }

        // ----------------------------------------------------
        ColumnLayout {
            CheckBox {
                id: textEditReadOnly
                text: "read-only"
                Layout.fillWidth: false
            }
            CheckBox {
                id: textEditMouseSelCB
                text: "mouse select"
                Layout.fillWidth: false
                onCheckedChanged: textEdit.selectByMouse = checked
            }
            CheckBox {
                id: textEditPressDelayCB
                Layout.fillWidth: false
                text: "press delay: " + flick.pressDelay
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 100
            radius: 4; antialiasing: true
            border.color: "black"; color: "transparent"

            Flickable {
                id: flick
                anchors.fill: parent; anchors.margins: 4
                contentHeight: textEdit.paintedHeight
                clip: true
                pressDelay: textEditPressDelayCB.checked ? 500 : 0

                function ensureVisible(r)
                {
                    if (contentX >= r.x)
                        contentX = r.x;
                    else if (contentX+width <= r.x+r.width)
                        contentX = r.x+r.width-width;
                    if (contentY >= r.y)
                        contentY = r.y;
                    else if (contentY+height <= r.y+r.height)
                        contentY = r.y+r.height-height;
                }

                ScrollBar.vertical: ScrollBar { }

                TextEdit {
                    id: textEdit
                    textFormat: TextArea.AutoText
                    width: flick.width - 10
                    wrapMode: TextEdit.Wrap
                    enabled: enabledCB.checked
                    readOnly: textEditReadOnly.checked
                    Component.onCompleted: textEditMouseSelCB.checked = selectByMouse

                    onCursorRectangleChanged: flick.ensureVisible(cursorRectangle)
                    onEditingFinished: textEditEditedInd.blip()
                    onTextChanged: textEditChangedInd.blip()
                    onPreeditTextChanged: preeditEditInd.blip()
                    onSelectedTextChanged: selectedTextChangedInd.blip()
                    onHoveredLinkChanged: hoveredLinkChangedInd.blip()
                    onLinkActivated: (text) => {
                        linkActivatedInd.blip()
                        linkActivatedText.text = text
                    }
                }
            }
        }

        SignalIndicator {
            id: textEditEditedInd
            label: "editingFinished"
        }

        // ----------------------------------------------------
        Text { text: "text" }

        Text {
            text: textEdit.text
            Layout.fillWidth: true
            Layout.maximumHeight: textEditChangedInd.implicitHeight * 1.5
            clip: true
        }

        SignalIndicator {
            id: textEditChangedInd
            label: "textChanged"
        }

        // ----------------------------------------------------
        Text { text: "preeditText" }

        Text { text: textEdit.preeditText; Layout.fillWidth: true }

        SignalIndicator {
            id: preeditEditInd
            label: "preeditTextChanged"
        }

        // ----------------------------------------------------
        Text { text: "selectedText" }

        Text {
            text: textEdit.selectedText
            Layout.fillWidth: true
            Layout.maximumHeight: selectedTextChangedInd.implicitHeight * 1.5
            clip: true
        }

        SignalIndicator {
            id: selectedTextChangedInd
            label: "selectedTextChanged"
        }

        // ----------------------------------------------------
        Text { text: "hoveredLink" }

        Text { text: textEdit.hoveredLink; Layout.fillWidth: true }

        SignalIndicator {
            id: hoveredLinkChangedInd
            label: "hoveredLinkChanged"
        }

        // ----------------------------------------------------
        Text { text: "linkActivated" }

        Text { id: linkActivatedText; Layout.fillWidth: true }

        SignalIndicator {
            id: linkActivatedInd
            label: "linkActivated"
        }

        // ====================================================
        Rule {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            text: "TextArea in ScrollView"
        }

        // ----------------------------------------------------
        ColumnLayout {
            CheckBox {
                id: textAreaReadOnly
                text: "read-only"
                Layout.fillWidth: false
            }
            CheckBox {
                id: textAreaMouseSelCB
                text: "mouse select"
                Layout.fillWidth: false
                onCheckedChanged: textArea.selectByMouse = checked
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 100
            radius: 4; antialiasing: true
            border.color: "black"; color: "transparent"

            ScrollView {
                 id: view
                 anchors.fill: parent; anchors.margins: 4
                 // pressDelay is not directly possible: QTBUG-104794

                 TextArea {
                     id: textArea
                     textFormat: TextArea.AutoText
                     enabled: enabledCB.checked
                     readOnly: textAreaReadOnly.checked
                     Component.onCompleted: textAreaMouseSelCB.checked = selectByMouse

                     onEditingFinished: textAreaEditedInd.blip()
                     onTextChanged: textAreaChangedInd.blip()
                     onPreeditTextChanged: preeditAreaInd.blip()
                     onSelectedTextChanged: selectedTextChangedAreaInd.blip()
                     onHoveredLinkChanged: hoveredLinkChangedAreaInd.blip()
                     onLinkActivated: (text) => {
                         linkActivatedAreaInd.blip()
                         linkActivatedAreaText.text = text
                     }
                 }
            }
        }

        SignalIndicator {
            id: textAreaEditedInd
            label: "editingFinished"
        }

        // ----------------------------------------------------
        Text { text: "text" }

        Text {
            text: textArea.text
            Layout.fillWidth: true
            Layout.maximumHeight: textAreaChangedInd.implicitHeight * 1.5
            clip: true
        }

        SignalIndicator {
            id: textAreaChangedInd
            label: "textChanged"
        }

        // ----------------------------------------------------
        Text { text: "preeditText" }

        Text { text: textArea.preeditText; Layout.fillWidth: true }

        SignalIndicator {
            id: preeditAreaInd
            label: "preeditTextChanged"
        }

        // ----------------------------------------------------
        Text { text: "selectedText" }

        Text {
            text: textArea.selectedText
            Layout.fillWidth: true
            Layout.maximumHeight: selectedTextChangedAreaInd.implicitHeight * 1.5
            clip: true
        }

        SignalIndicator {
            id: selectedTextChangedAreaInd
            label: "selectedTextChanged"
        }

        // ----------------------------------------------------
        Text { text: "hoveredLink" }

        Text { text: textArea.hoveredLink; Layout.fillWidth: true }

        SignalIndicator {
            id: hoveredLinkChangedAreaInd
            label: "hoveredLinkChanged"
        }

        // ----------------------------------------------------
        Text { text: "linkActivated" }

        Text { id: linkActivatedAreaText; Layout.fillWidth: true }

        SignalIndicator {
            id: linkActivatedAreaInd
            label: "linkActivated"
        }
    }
}
