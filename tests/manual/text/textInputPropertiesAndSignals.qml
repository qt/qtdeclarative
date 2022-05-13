// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.9
import QtQuick.Layouts 1.1
import "qrc:/quick/shared/" as Shared

Item {
    width: 600; height: 600
    GridLayout {
        columns: 3; rowSpacing: 6; columnSpacing: 6
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }

        // ----------------------------------------------------
        Text {
            text: "try typing and input methods in the TextInput below:"
            Layout.columnSpan: 3
        }

        // ----------------------------------------------------
        Text {
            text: "TextInput"
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: textInput.implicitHeight + 8
            radius: 4; antialiasing: true
            border.color: "black"; color: "transparent"

            TextInput {
                id: textInput
                anchors.fill: parent; anchors.margins: 4

                onTextEdited: textEditedInd.blip()
                onTextChanged: textChangedInd.blip()
                onPreeditTextChanged: preeditInd.blip()
                onDisplayTextChanged: displayTextInd.blip()
            }

        }

        SignalIndicator {
            id: textEditedInd
            label: "textEdited"
        }

        // ----------------------------------------------------
        Text { text: "text" }

        Text { text: textInput.text; Layout.fillWidth: true }

        SignalIndicator {
            id: textChangedInd
            label: "textChanged"
        }

        // ----------------------------------------------------
        Text { text: "preeditText" }

        Text { text: textInput.preeditText; Layout.fillWidth: true }

        SignalIndicator {
            id: preeditInd
            label: "preeditTextChanged"
        }

        // ----------------------------------------------------
        Text { text: "displayText" }

        Text { text: textInput.displayText; Layout.fillWidth: true }

        SignalIndicator {
            id: displayTextInd
            label: "displayTextChanged"
        }

        // ----------------------------------------------------
        Shared.TextField {
            id: copyFrom
            Layout.column: 1
            Layout.row: 5
            Layout.fillWidth: true
            text: "copy this"
        }

        Shared.Button {
            Layout.column: 2
            Layout.row: 5
            text: "setText"
            onClicked: {
                Qt.inputMethod.reset()
                textInput.text = copyFrom.text
            }
        }
    }
}
