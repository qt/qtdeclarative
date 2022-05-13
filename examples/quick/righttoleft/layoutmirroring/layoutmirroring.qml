// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root
    property bool mirror: (Qt.application as Application).layoutDirection == Qt.RightToLeft
    LayoutMirroring.enabled: mirror
    LayoutMirroring.childrenInherit: true
    width: 320
    height: 480
    color: "lightsteelblue"

    Column {
        spacing: 10
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 10 }

        Text {
            text: "Text alignment"
            anchors.left: parent.left
        }

        Rectangle {
            id: textStrings
            width: 148
            height: 85
            color: "white"
            anchors.left: parent.left
            Column {
                spacing: 5
                width: parent.width
                anchors { fill: parent; margins: 5 }
                Text {
                    id: englishText
                    width: parent.width
                    text: "English text"
                }
                Text {
                    id: arabicText
                    width: parent.width
                    text: "النص العربي"
                }
                Text {
                    id: leftAlignedText
                    width: parent.width
                    text: "Text aligned to left"
                    horizontalAlignment: Text.AlignLeft
                }
                Text {
                    id: rightAlignedText
                    width: parent.width
                    text: "Text aligned to right"
                    horizontalAlignment: Text.AlignRight
                }
            }
        }

        Text {
            text: "Item x"
            anchors.left: parent.left
        }
        Rectangle {
            id: items
            color: Qt.rgba(0.2, 0.2, 0.2, 0.6)
            width: 275; height: 95
            anchors.left: parent.left
            Rectangle {
                y: 5; x: 5
                width: 130; height: 40
                Text {
                    text: "Item with x: 5\n(not mirrored)"
                    anchors.centerIn: parent
                }
            }
            Rectangle {
                color:  Qt.rgba(0.7, 0.7, 0.7)
                y: 50; x: mirror(5)
                width: 130; height: 40
                function mirror(value) {
                    return LayoutMirroring.enabled ? (parent.width - width - value) : value;
                }
                Text {
                    text: "Item with x: " + parent.x + "\n(manually mirrored)"
                    anchors.centerIn: parent
                }
            }
        }
        Text {
            text: "Item anchors"
            anchors.left: parent.left
        }

        Rectangle {
            id: anchoredItems
            color: Qt.rgba(0.2, 0.2, 0.2, 0.6)
            width: 270; height: 170
            anchors.left: parent.left
            Rectangle {
                id: blackRectangle
                color: "black"
                width: 180; height: 90
                anchors { horizontalCenter: parent.horizontalCenter; horizontalCenterOffset: 30 }
                Text {
                    text: "Horizontal center anchored\nwith offset 30\nto the horizontal center\nof the parent."
                    color: "white"
                    anchors.centerIn: parent
                }
            }
            Rectangle {
                id: whiteRectangle
                color: "white"
                width: 120; height: 70
                anchors { left: parent.left; bottom: parent.bottom }
                Text {
                    text: "Left side anchored\nto the left side\nof the parent."
                    color: "black"
                    anchors.centerIn: parent
                }
            }
            Rectangle {
                id: grayRectangle
                color: Qt.rgba(0.7, 0.7, 0.7)
                width: 140; height: 90
                anchors { right: parent.right; bottom: parent.bottom }
                Text {
                    text: "Right side anchored\nto the right side\nof the parent."
                    anchors.centerIn: parent
                }
            }
        }
    }

    Rectangle {
        id: mirrorButton
        color: mouseArea2.pressed ? "black" : "gray"
        height: 50; width: 160
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        Column {
            anchors.centerIn: parent
            Text {
                text: root.mirror ? "Mirrored" : "Not mirrored"
                color: "white"
                font.pixelSize: 16
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Text {
                text: "(click here to toggle)"
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

