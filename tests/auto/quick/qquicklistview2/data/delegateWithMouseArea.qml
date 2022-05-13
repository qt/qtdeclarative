// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {

    width: 240
    height: 320
    color: "#ffffff"

    Component {
        id: myDelegate
        Rectangle {
            id: wrapper
            width: list.orientation == ListView.Vertical ? 240 : 20
            height: list.orientation == ListView.Vertical ? 20 : 240
            border.width: 1
            border.color: "black"
            MouseArea {
                anchors.fill: parent
            }
            Text {
                text: index + ":" + (list.orientation == ListView.Vertical ? parent.y : parent.x).toFixed(0)
            }
            color: ListView.isCurrentItem ? "lightsteelblue" : "white"
        }
    }

    ListView {
        id: list
        objectName: "list"
        focus: true
        width: 240
        height: 200
        clip: true
        model: 30
        headerPositioning: ListView.OverlayHeader
        delegate: myDelegate

        header: Rectangle {
            width: list.orientation == Qt.Vertical ? 240 : 30
            height: list.orientation == Qt.Vertical ? 30 : 240
            color: "green"
            z: 11
            Text {
                anchors.centerIn: parent
                text: "header " + (list.orientation == ListView.Vertical ? parent.y : parent.x).toFixed(1)
            }
        }
    }

    // debug
    Rectangle {
        color: "#40ff0000"
        border.width: txt.x
        border.color: "black"
        radius: 5
        width: txt.implicitWidth + 50
        height: txt.implicitHeight + 2 * txt.x
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        Text {
            id: txt
            x: 3
            y: x
            text:  "header position: " + (list.orientation == ListView.Vertical ? list.headerItem.y : list.headerItem.x).toFixed(1)
              + "\ncontent position: " + (list.orientation == ListView.Vertical ? list.contentY : list.contentX).toFixed(1)
        }
    }
}
