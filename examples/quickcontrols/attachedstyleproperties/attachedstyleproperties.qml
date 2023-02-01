// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts

import MyStyle

ApplicationWindow {
    width: 800
    height: 600
    title: qsTr("Attached Objects")
    visible: true

    MyStyle.theme: darkModeSwitch.checked ? MyStyle.Dark : MyStyle.Light

    header: ToolBar {
        MyStyle.theme: MyStyle.Dark

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12

            Label {
                text: qsTr("This is a Label in a ToolBar")
            }

            Item {
                Layout.fillWidth: true
            }

            Switch {
                id: darkModeSwitch
                text: qsTr("Dark mode")
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        Label {
            text: qsTr("This is a Label in an ApplicationWindow")
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            Button {
                text: qsTr("Open Popup")
                onClicked: popup.open()
            }

            Button {
                text: qsTr("Open Window")
                onClicked: {
                    if (!childWindow.active)
                        childWindow.show()
                    else
                        childWindow.raise()
                }
            }
        }
    }

    Popup {
        id: popup
        anchors.centerIn: parent
        closePolicy: Popup.NoAutoClose

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 20

            Label {
                text: qsTr("This is a Label in a Popup")
                Layout.alignment: Qt.AlignHCenter
            }

            Button {
                text: qsTr("Close Popup")
                Layout.alignment: Qt.AlignHCenter
                onClicked: popup.close()
            }
        }
    }

    ApplicationWindow {
        id: childWindow
        width: 600
        height: 400
        title: qsTr("Attached Objects - Child Window")

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 20

            Label {
                text: qsTr("This is a Label in a child ApplicationWindow")
                Layout.alignment: Qt.AlignHCenter
            }

            Button {
                text: qsTr("Close Window")
                Layout.alignment: Qt.AlignHCenter
                onClicked: childWindow.close()
            }
        }
    }
}
