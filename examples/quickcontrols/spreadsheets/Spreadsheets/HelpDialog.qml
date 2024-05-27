// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    modal: true
    standardButtons: Dialog.Ok

    contentItem: GridLayout {
        columns: 3
        columnSpacing: 10

        Label {
            Layout.columnSpan: 3
            text: qsTr("A formula starts with `=` follows with the operator and arguments.\n" +
                       "Formula could be")
        }

        Label {
            Layout.leftMargin: 20
            text: qsTr("Cell assignment")
        }
        Rectangle {
            implicitWidth: 90
            implicitHeight: 30
            color: palette.base
            border.width: 1
            border.color: Qt.styleHints.colorScheme === Qt.Light ? palette.dark : palette.light
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("=A1")
            }
        }
        Item { Layout.fillWidth: true }

        Label {
            Layout.leftMargin: 20
            text: qsTr("Addition")
        }
        Rectangle {
            implicitWidth: 90
            implicitHeight: 30
            color: palette.base
            border.width: 1
            border.color: Qt.styleHints.colorScheme === Qt.Light ? palette.dark : palette.light
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("=A1+A2")
            }
        }
        Item { Layout.fillWidth: true }

        Label {
            Layout.leftMargin: 20
            text: qsTr("Subtraction")
        }
        Rectangle {
            implicitWidth: 90
            implicitHeight: 30
            color: palette.base
            border.width: 1
            border.color: Qt.styleHints.colorScheme === Qt.Light ? palette.dark : palette.light
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("=A1-A2")
            }
        }
        Item { Layout.fillWidth: true }

        Label {
            Layout.leftMargin: 20
            text: qsTr("Division")
        }
        Rectangle {
            implicitWidth: 90
            implicitHeight: 30
            color: palette.base
            border.width: 1
            border.color: Qt.styleHints.colorScheme === Qt.Light ? palette.dark : palette.light
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("=A1/A2")
            }
        }
        Item { Layout.fillWidth: true }

        Label {
            Layout.leftMargin: 20
            text: qsTr("Multiplication")
        }
        Rectangle {
            implicitWidth: 90
            implicitHeight: 30
            color: palette.base
            border.width: 1
            border.color: Qt.styleHints.colorScheme === Qt.Light ? palette.dark : palette.light
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("=A1*A2")
            }
        }
        Item { Layout.fillWidth: true }

        Label {
            Layout.leftMargin: 20
            text: qsTr("Summation")
        }
        Rectangle {
            implicitWidth: 90
            implicitHeight: 30
            color: palette.base
            border.width: 1
            border.color: Qt.styleHints.colorScheme === Qt.Light ? palette.dark : palette.light
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("=SUM A1:A2")
            }
        }
        Item { Layout.fillWidth: true }
    }
}
