// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: palette.base

    RowLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 20

        TextField {
            id: textField
            Layout.fillWidth: true
            focus: true

            DropArea {
                id: dropArea
                anchors.fill: parent

                onDropped: (drop) => {
                    if (drop.hasText)
                        textField.text = drop.text
                }

                Rectangle {
                    anchors.fill: parent
                    color: Application.styleHints.colorScheme === Qt.Dark ? "white" : "black"
                    opacity: 0.05
                    visible: dropArea.containsDrag
                }
            }
        }

        Button {
            id: button
            text: qsTr("Click Me!")
            Layout.fillHeight: true
            focus: true

            onClicked: textField.text = qsTr("Click Me! button is clicked")

            Shortcut {
                context: Qt.ApplicationShortcut
                sequence: "Ctrl+B"
                onActivated: button.click()
            }
        }
    }
}
