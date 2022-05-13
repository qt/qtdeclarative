// Copyright (C) 2021 The Qt Company Ltd.

import QtQuick
import QtQuick.Layouts

Rectangle {
    id: appWindow

    visible: false
    objectName: "Basic layouts"
    property int margin: 11

    Component.onCompleted: {
        width = mainLayout.implicitWidth + 2 * margin
        height = mainLayout.implicitHeight + 2 * margin
    }

    width: mainLayout.Layout.minimumWidth + 2 * margin
    height: mainLayout.Layout.minimumHeight + 2 * margin

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: appWindow.margin
        Rectangle {
            id: rowBox
            objectName: "Row layout"
            Layout.fillWidth: true
            Layout.minimumWidth: rowLayout.Layout.minimumWidth + 30

            RowLayout {
                id: rowLayout
                anchors.fill: parent
                Item {
                    objectName: "This wants to grow horizontally"
                    Layout.fillWidth: true
                }
                Item {
                    property string text: "Button"
                }
            }
        }

        Rectangle {
            id: gridBox
            objectName: "Grid layout"
            Layout.fillWidth: true
            Layout.minimumWidth: gridLayout.Layout.minimumWidth + 30

            GridLayout {
                id: gridLayout
                rows: 3
                flow: GridLayout.TopToBottom
                anchors.fill: parent

                Item { property string text: "Line 1" }
                Item { property string text: "Line 2" }
                Item { property string text: "Line 3" }

                Item { }
                Item { }
                Item { }

                Item {
                    property string text: "This widget spans over three rows in the GridLayout.\n"
                        + "All items in the GridLayout are implicitly positioned from top to bottom."
                    property int wrapMode: Text.WordWrap
                    Layout.rowSpan: 3
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.minimumHeight: implicitHeight
                    Layout.minimumWidth: 100     // guesstimate, should be size of largest word
                }
            }
        }
        Item {
            id: t3
            property string text: "This fills the whole cell"
            Layout.minimumHeight: 30
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
        Rectangle {
            id: stackBox
            objectName: "Stack layout"
            implicitWidth: 200
            implicitHeight: 60
            Layout.minimumHeight: 60
            Layout.fillWidth: true
            Layout.fillHeight: true
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
                        MouseArea {
                            anchors.centerIn: parent
                            onClicked: stackLayout.advance()
                        }
                    }
                }
            }
        }
    }
}
