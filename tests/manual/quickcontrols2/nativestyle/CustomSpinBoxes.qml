// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "SpinBoxes"

    Row {
        spacing: container.rowSpacing

        SpinBox {
            id: custombg
            value: 1000
            to: 2000
            background: Rectangle {
                border.color: "green"
                implicitWidth: 50
            }
        }

        SpinBox {
            id: customIndicator
            value: 500
            to: 2000

            rightPadding: 17
            spacing: 0
            implicitWidth: 60
            implicitHeight: 25

            up.indicator: Rectangle {
                x: customIndicator.width - width - 4
                y: 4
                implicitWidth: customIndicator.rightPadding - 4
                implicitHeight: 8
                border.width: 1
                border.color: "green"
                color: customIndicator.up.pressed ? "gray" : "transparent"
                Text {
                    text: "+"
                    font.pixelSize: 8
                    font.bold: true
                    anchors.centerIn: parent
                }
            }

            down.indicator: Rectangle {
                x: customIndicator.width - width - 4
                y: height + 6
                implicitWidth: customIndicator.rightPadding - 4
                implicitHeight: 8
                border.width: 1
                border.color: "green"
                color: customIndicator.down.pressed ? "gray" : "transparent"
                Text {
                    text: "-"
                    font.pixelSize: 10
                    font.bold: true
                    anchors.centerIn: parent
                }
            }
        }

        SpinBox {
            id: allCustom
            value: 500
            to: 2000

            rightPadding: 17
            spacing: 0
            implicitWidth: 60
            implicitHeight: 25

            background: Rectangle {
                border.color: "green"
                implicitWidth: 50
            }

            up.indicator: Rectangle {
                x: allCustom.width - width - 4
                y: 4
                implicitWidth: allCustom.rightPadding - 4
                implicitHeight: 8
                border.width: 1
                border.color: "green"
                color: allCustom.up.pressed ? "gray" : "transparent"
                Text {
                    text: "+"
                    font.pixelSize: 8
                    font.bold: true
                    anchors.centerIn: parent
                }
            }

            down.indicator: Rectangle {
                x: allCustom.width - width - 4
                y: height + 6
                implicitWidth: allCustom.rightPadding - 4
                implicitHeight: 8
                border.width: 1
                border.color: "green"
                color: allCustom.down.pressed ? "gray" : "transparent"
                Text {
                    text: "-"
                    font.pixelSize: 10
                    font.bold: true
                    anchors.centerIn: parent
                }
            }

            contentItem: TextInput {
                text: allCustom.displayText
                font: allCustom.font
                color: "green"
                selectionColor: allCustom.palette.highlight
                selectedTextColor: allCustom.palette.highlightedText
                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter

                topPadding: 2
                bottomPadding: 2
                leftPadding: 10
                rightPadding: 10

                readOnly: !allCustom.editable
                validator: allCustom.validator
                inputMethodHints: allCustom.inputMethodHints
            }

        }

    }

}
