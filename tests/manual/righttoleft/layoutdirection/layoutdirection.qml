// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

pragma ComponentBehavior: Bound

Rectangle {
    id: root
    property bool mirror
    property int direction: (Qt.application as Application).layoutDirection
    LayoutMirroring.enabled: mirror
    LayoutMirroring.childrenInherit: true
    width: 320
    height: 480
    Column {
        id: columnA
        spacing: 10
        x: 10
        y: 10
        width: 140

        Item {
            id: rowCell
        }
        Text {
            text: qsTr("Row")
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Row {
            layoutDirection: root.direction
            spacing: 10
            move: Transition {
                NumberAnimation {
                    properties: "x"
                }
            }
            Repeater {
                model: 3
                delegate: PositionerDelegate {
                }
            }
        }

        Text {
            text: qsTr("Grid")
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Grid {
            layoutDirection: root.direction
            spacing: 10
            columns: 3
            move: Transition {
                NumberAnimation {
                    properties: "x"
                }
            }
            Repeater {
                model: 8
                delegate: PositionerDelegate {
                }
            }
        }

        Text {
            text: qsTr("Flow")
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Flow {
            layoutDirection: root.direction
            spacing: 10; width: parent.width
            move: Transition {
                NumberAnimation {
                    properties: "x"
                }
            }
            Repeater {
                model: 8
                delegate: PositionerDelegate {

                }
            }
        }
    }
    Column {
        id: columnB
        spacing: 10
        x: 160
        y: 10

        Text {
            text: qsTr("ListView")
            anchors.horizontalCenter: parent.horizontalCenter
        }

        ListView {
            id: listView
            clip: true
            width: parent.width; height: 40
            layoutDirection: root.direction
            orientation: Qt.Horizontal
            model: 48
            delegate: ViewDelegate {
            }
        }

        Text {
            text: "GridView"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        GridView {
            clip: true
            width: 150; height: 160
            cellWidth: 50; cellHeight: 50
            layoutDirection: root.direction
            model: 48
            delegate: ViewDelegate {
            }
        }

        Rectangle {
            height: 50; width: parent.width
            color: mouseArea.pressed ? "black" : "gray"
            Column {
                anchors.centerIn: parent
                Text {
                    text: root.direction ? qsTr("Right to left") : qsTr("Left to right")
                    color: "white"
                    font.pixelSize: 16
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: qsTr("(click here to toggle)")
                    color: "white"
                    font.pixelSize: 10
                    font.italic: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                onClicked: {
                    if (root.direction === Qt.LeftToRight) {
                        root.direction = Qt.RightToLeft;
                    } else {
                        root.direction = Qt.LeftToRight;
                    }
                }
            }
        }

        Rectangle {
            height: 50; width: parent.width
            color: mouseArea2.pressed ? "black" : "gray"
            Column {
                anchors.centerIn: parent
                Text {
                    text: root.mirror ? qsTr("Mirrored") : qsTr("Not mirrored")
                    color: "white"
                    font.pixelSize: 16
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: qsTr("(click here to toggle)")
                    color: "white"
                    font.pixelSize: 10
                    font.italic: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
            MouseArea {
                id: mouseArea2
                anchors.fill: parent
                onClicked: {
                    root.mirror = !root.mirror;
                }
            }
        }
    }

    component PositionerDelegate : Rectangle {
        width: 40
        height: 40

        required property int index
        property int lightness: index + 1
        color: Qt.rgba(0.8 / lightness, 0.8 / lightness, 0.8 / lightness, 1.0)
        Text {
            text: parent.lightness
            color: "white"
            font.pixelSize: 18
            anchors.centerIn: parent
        }
    }

    component ViewDelegate : Item {
        id: delegateItem
        required property int index
        width: (listView.effectiveLayoutDirection === Qt.LeftToRight ? (index === 48 - 1) : (index === 0)) ? 40 : 50
        Rectangle {
            width: 40; height: 40
            color: Qt.rgba(0.5 + (48 - delegateItem.index) * Math.random() / 48,
                           0.3 + delegateItem.index * Math.random() / 48,
                           0.3 * Math.random(),
                           1.0)
            Text {
                text: delegateItem.index + 1
                color: "white"
                font.pixelSize: 18
                anchors.centerIn: parent
            }
        }
    }
}

