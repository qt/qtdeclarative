// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

Rectangle {

    width: column.width + 100
    height: column.height + 100
    property int direction: Qt.application.layoutDirection

    Column {
        id: column
        spacing: 10
        anchors.centerIn: parent
        width: 230

        Text {
            text: "Row"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Row {
            layoutDirection: direction
            spacing: 10
            move: Transition {
                NumberAnimation {
                    properties: "x"
                }
            }
            Repeater {
                model: 4
                Loader {
                    property int value: index
                    sourceComponent: delegate
                }
            }
        }
        Text {
            text: "Grid"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Grid {
           layoutDirection: direction
           spacing: 10; columns: 4
           move: Transition {
               NumberAnimation {
                   properties: "x"
               }
           }
           Repeater {
               model: 11
               Loader {
                   property int value: index
                   sourceComponent: delegate
               }
            }
        }
        Text {
            text: "Flow"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Flow {
           layoutDirection: direction
           spacing: 10; width: parent.width
           move: Transition {
               NumberAnimation {
                   properties: "x"
               }
           }
           Repeater {
               model: 10
               Loader {
                   property int value: index
                   sourceComponent: delegate
               }
            }
        }
        Rectangle {
           height: 50; width: parent.width
           color: mouseArea.pressed ? "black" : "gray"
           Text {
                text: direction ? "Right to left" : "Left to right"
                color: "white"
                font.pixelSize: 16
                anchors.centerIn: parent
            }
            MouseArea {
                id: mouseArea
                onClicked: {
                    if (direction == Qt.LeftToRight) {
                        direction = Qt.RightToLeft;
                    } else {
                        direction = Qt.LeftToRight;
                    }
                }
                anchors.fill: parent
            }
        }
    }

    Component {
        id: delegate
        Rectangle {
            width: 50; height: 50
            color: Qt.rgba(0.8/(parent.value+1),0.8/(parent.value+1),0.8/(parent.value+1),1.0)
            Text {
                text: parent.parent.value+1
                color: "white"
                font.pixelSize: 20
                anchors.centerIn: parent
            }
        }
    }
}
