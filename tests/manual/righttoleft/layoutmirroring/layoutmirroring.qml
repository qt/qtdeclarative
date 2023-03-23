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

    Flickable {
        anchors {
            fill: parent
            margins: 10
        }
        contentWidth: parent.width
        contentHeight: content.height
        flickableDirection: Flickable.VerticalFlick

        Column {
            id: content
            spacing: 10
            anchors {
                left: parent.left
                right: parent.right
            }

            Text {
                text: qsTr("Text alignment")
                anchors.left: parent.left
            }

            Rectangle {
                width: textStrings.width + 10
                height: textStrings.height + 10
                color: "white"
                anchors.left: parent.left
                Column {
                    anchors.centerIn: parent
                    id: textStrings
                    spacing: 5
                    width: 148
                    Text {
                        id: englishText
                        width: parent.width
                        text: qsTr("English text")
                    }
                    Text {
                        id: arabicText
                        width: parent.width
                        text: qsTr("النص العربي")
                    }
                    Text {
                        id: leftAlignedText
                        width: parent.width
                        text: qsTr("Text aligned to left")
                        horizontalAlignment: Text.AlignLeft
                    }
                    Text {
                        id: rightAlignedText
                        width: parent.width
                        text: qsTr("Text aligned to right")
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }

            Text {
                text: qsTr("Item x")
                anchors.left: parent.left
            }
            Rectangle {
                id: items
                color: Qt.rgba(0.2, 0.2, 0.2, 0.6)
                width: 275
                height: 95
                anchors.left: parent.left
                Rectangle {
                    y: 5
                    x: 5
                    width: 130
                    height: 40
                    Text {
                        text: qsTr("Item with x: 5\n(not mirrored)")
                        anchors.centerIn: parent
                    }
                }
                Rectangle {
                    color:  Qt.rgba(0.7, 0.7, 0.7)
                    x: mirror(5)
                    y: 50
                    width: 130
                    height: 40
                    function mirror(value) {
                        return LayoutMirroring.enabled ? (parent.width - width - value) : value;
                    }
                    Text {
                        text: qsTr("Item with x: %1 \n(manually mirrored)").arg(parent.x)
                        anchors.centerIn: parent
                    }
                }
            }
            Text {
                text: qsTr("Item anchors")
                anchors.left: parent.left
            }

            Rectangle {
                id: anchoredItems
                color: Qt.rgba(0.2, 0.2, 0.2, 0.6)
                width: 270
                height: 170
                anchors.left: parent.left
                Rectangle {
                    id: blackRectangle
                    color: "black"
                    width: 180
                    height: 90
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        horizontalCenterOffset: 30
                    }
                    Text {
                        text: qsTr("Horizontal center anchored\nwith offset 30\nto the horizontal center\nof the parent.")
                        color: "white"
                        anchors.centerIn: parent
                    }
                }
                Rectangle {
                    id: whiteRectangle
                    color: "white"
                    width: 120
                    height: 70
                    anchors {
                        left: parent.left
                        bottom: parent.bottom
                    }
                    Text {
                        text: qsTr("Left side anchored\nto the left side\nof the parent.")
                        color: "black"
                        anchors.centerIn: parent
                    }
                }
                Rectangle {
                    id: grayRectangle
                    color: Qt.rgba(0.7, 0.7, 0.7)
                    width: 140
                    height: 90
                    anchors {
                        right: parent.right
                        bottom: parent.bottom
                    }
                    Text {
                        text: qsTr("Right side anchored\nto the right side\nof the parent.")
                        anchors.centerIn: parent
                    }
                }
            }
        }
    }

    Rectangle {
        id: mirrorButton
        color: mouseArea2.pressed ? "black" : "gray"
        height: 50
        width: 160
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
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
            onClicked: root.mirror = !root.mirror;
        }
    }
}

