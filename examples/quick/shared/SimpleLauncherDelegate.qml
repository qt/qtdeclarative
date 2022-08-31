// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

Rectangle {
    id: container
    required property string name
    required property string description

    property Item exampleItem
    width: ListView.view.width
    height: button.implicitHeight + 22

    signal clicked()

    gradient: Gradient {
        GradientStop {
            position: 0
            Behavior on color {ColorAnimation { duration: 100 }}
            color: tapHandler.pressed ? "#e0e0e0" : "#fff"
        }
        GradientStop {
            position: 1
            Behavior on color {ColorAnimation { duration: 100 }}
            color: tapHandler.pressed ? "#e0e0e0" : button.containsMouse ? "#f5f5f5" : "#eee"
        }
    }

    Image {
        id: image
        opacity: 0.7
        Behavior on opacity {NumberAnimation {duration: 100}}
        source: "images/next.png"
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 16
    }

    Item {
        id: button
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.right:image.left
        implicitHeight: col.height
        height: implicitHeight
        width: buttonLabel.width + 20
        property alias containsMouse: hoverHandler.hovered

        TapHandler {
            id: tapHandler
            onTapped: container.clicked()
        }
        HoverHandler {
            id: hoverHandler
        }

        Column {
            spacing: 2
            id: col
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            Text {
                id: buttonLabel
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.right: parent.right
                anchors.rightMargin: 10
                text: container.name
                color: "black"
                font.pixelSize: 22
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                styleColor: "white"
                style: Text.Raised

            }
            Text {
                id: buttonLabel2
                anchors.left: parent.left
                anchors.leftMargin: 10
                text: container.description
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                color: "#666"
                font.pixelSize: 12
            }
        }
    }

    Rectangle {
        height: 1
        color: "#ccc"
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }
}
