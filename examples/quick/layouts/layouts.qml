// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

pragma ComponentBehavior: Bound

ApplicationWindow {
    id: appWindow

    visible: true
    title: qsTr("Basic layouts")
    readonly property int margin: 11

    Component.onCompleted: {
        width = mainLayout.implicitWidth + 2 * margin
        height = mainLayout.implicitHeight + 2 * margin
    }

    minimumWidth: mainLayout.Layout.minimumWidth + 2 * margin
    minimumHeight: mainLayout.Layout.minimumHeight + 2 * margin

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: appWindow.margin
        GroupBox {
            id: rowBox
            title: qsTr("Row layout")
            Layout.minimumWidth: rowLayout.Layout.minimumWidth + 30

            RowLayout {
                id: rowLayout
                anchors.fill: parent
                TextField {
                    placeholderText: qsTr("This wants to grow horizontally")
                }
                Button {
                    text: qsTr("Button")
                }
            }
        }

        GroupBox {
            id: gridBox
            title: qsTr("Grid layout")
            Layout.minimumWidth: gridLayout.Layout.minimumWidth + 30

            GridLayout {
                id: gridLayout
                rows: 3
                flow: GridLayout.TopToBottom
                anchors.fill: parent

                Label { text: qsTr("Line 1") }
                Label { text: qsTr("Line 2") }
                Label { text: qsTr("Line 3") }

                TextField { }
                TextField { }
                TextField { }

                TextArea {
                    text: qsTr("This widget spans over three rows in the GridLayout.\n")
                        + qsTr("All items in the GridLayout are implicitly positioned from top to bottom.")
                    wrapMode: TextArea.WordWrap
                    Layout.rowSpan: 3
                    Layout.minimumHeight: implicitHeight
                    Layout.minimumWidth: 100     // guesstimate, should be size of largest word
                }
            }
        }
        TextArea {
            id: t3
            text: qsTr("This fills the whole cell")
            Layout.minimumHeight: 30
        }
        GroupBox {
            id: stackBox
            title: qsTr("Stack layout")
            implicitWidth: 200
            implicitHeight: 60
            Layout.minimumHeight: 60
            StackLayout {
                id: stackLayout
                anchors.fill: parent

                function advance() { currentIndex = (currentIndex + 1) % count }

                Repeater {
                    id: stackRepeater
                    model: 5
                    Rectangle {
                        required property int index
                        color: Qt.hsla((0.5 + index) / stackRepeater.count, 0.3, 0.7, 1)
                        Button {
                            anchors.centerIn: parent
                            text: qsTr("Page ") + (parent.index + 1)
                            onClicked: stackLayout.advance()
                        }
                    }
                }
            }
        }
    }
}
