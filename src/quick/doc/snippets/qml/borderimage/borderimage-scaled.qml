// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: page
    color: "white"
    width: 182; height: 182
    border.color: "gray"

//! [scaled border image]
BorderImage {
    anchors { fill: parent; margins: 1 }
    border { left: 30; top: 30; right: 30; bottom: 30 }
    horizontalTileMode: BorderImage.Stretch
    verticalTileMode: BorderImage.Stretch
    source: "pics/borderframe.png"
}
//! [scaled border image]

    Rectangle {
        x: 30; y: 0
        width: 1; height: parent.height
        color: "gray"

        Text {
            text: "1"
            font.pixelSize: 9
            color: "red"
            anchors.right: parent.right
            anchors.rightMargin: 1
            y: 20
        }

        Text {
            text: "4"
            font.pixelSize: 9
            color: "red"
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 1
        }

        Text {
            text: "7"
            font.pixelSize: 9
            color: "red"
            y: parent.height - 30
            anchors.right: parent.right
            anchors.rightMargin: 1
        }
    }

    Rectangle {
        x: parent.width - 30; y: 0
        width: 1; height: parent.height
        color: "gray"

        Text {
            text: "3"
            font.pixelSize: 9
            color: "red"
            x: 1
            y: 20
        }

        Text {
            text: "6"
            font.pixelSize: 9
            color: "red"
            x: 1
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: "9"
            font.pixelSize: 9
            color: "red"
            x: 1
            y: parent.height - 30
        }
    }

    Text {
        text: "5"
        font.pixelSize: 9
        color: "red"
        anchors.centerIn: parent
    }

    Rectangle {
        x: 0; y: 30
        width: parent.width; height: 1
        color: "gray"

        Text {
            text: "2"
            font.pixelSize: 9
            color: "red"
            anchors.horizontalCenter: parent.horizontalCenter
            y: -10
        }
    }

    Rectangle {
        x: 0; y: parent.height - 30
        width: parent.width; height: 1
        color: "gray"

        Text {
            text: "8"
            font.pixelSize: 9
            color: "red"
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
