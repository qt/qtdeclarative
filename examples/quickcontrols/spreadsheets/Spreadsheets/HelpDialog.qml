// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    modal: true
    standardButtons: Dialog.Ok

    component SectionHelpInfo: Item {
        property alias sectionText: section.text
        property alias sectionDescription: sectionDesc.text

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20

            Item {
                implicitWidth: 40
                Layout.fillHeight: true

                Label {
                    id: section
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Rectangle {
                implicitWidth: 90
                color: palette.base
                border.width: 1
                border.color: Qt.styleHints.colorScheme === Qt.Light ? palette.dark : palette.light
                Layout.fillHeight: true

                Label {
                    id: sectionDesc
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 10

        Item {
            Layout.topMargin: 20
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            implicitWidth: infoLabel.width
            implicitHeight: infoLabel.height

            Label {
                id: infoLabel
                text: qsTr("A formula starts with `=` follows with the operator and arguments.\n" +
                           "Formula could be")
            }
        }

        SectionHelpInfo {
            implicitHeight: 30
            Layout.fillWidth: true
            Layout.leftMargin: 40
            sectionText: "Cell assignment"
            sectionDescription: "=A1"
        }

        SectionHelpInfo {
            implicitHeight: 30
            Layout.fillWidth: true
            Layout.leftMargin: 40
            sectionText: "Addition"
            sectionDescription: "=A1+A2"
        }

        SectionHelpInfo {
            implicitHeight: 30
            Layout.fillWidth: true
            Layout.leftMargin: 40
            sectionText: "Subtraction"
            sectionDescription: "=A1-A2"
        }

        SectionHelpInfo {
            implicitHeight: 30
            Layout.fillWidth: true
            Layout.leftMargin: 40
            sectionText: "Division"
            sectionDescription: "=A1/A2"
        }

        SectionHelpInfo {
            implicitHeight: 30
            Layout.fillWidth: true
            Layout.leftMargin: 40
            sectionText: "Multiplication"
            sectionDescription: "=A1*A2"
        }

        SectionHelpInfo {
            implicitHeight: 30
            Layout.fillWidth: true
            Layout.leftMargin: 40
            sectionText: "Summation"
            sectionDescription: "=SUM A1:A2"
        }
    }
}
