// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


import QtQuick 2.7

Rectangle {
    id: root
    width: 320
    height: 240
    color: "black"

    property int current: list.currentIndex
    property int horizontalVelocityZeroCount: 0

    ListView {
        id: list
        objectName: "list"
        anchors.fill: parent

        focus: true

        orientation: ListView.Horizontal

        snapMode: ListView.SnapToItem
        flickableDirection: Flickable.HorizontalFlick

        model: 10
        delegate: Item {
            width: root.width / 3
            height: root.height
            Rectangle {
                anchors.centerIn: parent
                width: 50
                height: 50
                color: list.currentIndex === index ? "red" : "white"
            }
        }

        onHorizontalVelocityChanged:  {
            if (list.horizontalVelocity === 0.0)
                root.horizontalVelocityZeroCount++
        }

    }

    Rectangle {
        color: "red"
        width: 50
        height: 50
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        MouseArea {
            anchors.fill: parent
            onClicked: {
                list.currentIndex--;
            }
        }
    }

    Rectangle {
        color: "red"
        width: 50
        height: 50
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        MouseArea {
            anchors.fill: parent
            onClicked: {
                list.currentIndex++;
            }
        }
    }
}

