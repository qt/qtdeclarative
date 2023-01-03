// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "components"

Rectangle {
    id: root
    width: 640
    height: 480
    color: rootHover.hovered ? "#555" : "#444"

    component ButtonsAndStuff: Column {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        CheckBox {
            id: hoverBlockingCB
            text: "Button hover is blocking"
        }

        Rectangle {
            objectName: "buttonWithMA"
            width: parent.width
            height: 30
            color: buttonMA.pressed ? "lightsteelblue" : "#999"
            border.color: buttonMA.containsMouse ? "cyan" : "transparent"

            MouseArea {
                id: buttonMA
                objectName: "buttonMA"
                hoverEnabled: true
                cursorShape: Qt.UpArrowCursor
                anchors.fill: parent
                onClicked: console.log("clicked MA")
            }

            Text {
                anchors.centerIn: parent
                text: "MouseArea"
            }
        }

        Rectangle {
            objectName: "buttonWithHH"
            width: parent.width
            height: 30
            color: flash ? "#999" : "white"
            border.color: buttonHH.hovered ? "cyan" : "transparent"
            property bool flash: true

            HoverHandler {
                id: buttonHH
                objectName: "buttonHH"
                acceptedDevices: PointerDevice.AllDevices
                blocking: hoverBlockingCB.checked
                cursorShape: tapHandler.pressed ? Qt.BusyCursor : Qt.PointingHandCursor
            }

            TapHandler {
                id: tapHandler
                onTapped: tapFlash.start()
            }

            Text {
                anchors.centerIn: parent
                text: "HoverHandler"
            }

            FlashAnimation on flash {
                id: tapFlash
            }
        }
    }

    Rectangle {
        id: paddle
        width: 100
        height: 40
        color: paddleHH.hovered ? "indianred" : "#888"
        y: parent.height - 100
        radius: 10

        HoverHandler {
            id: paddleHH
            objectName: "paddleHH"
        }

        SequentialAnimation on x {
            NumberAnimation {
                to: root.width - paddle.width
                duration: 2000
                easing { type: Easing.InOutQuad }
            }
            PauseAnimation { duration: 100 }
            NumberAnimation {
                to: 0
                duration: 2000
                easing { type: Easing.InOutQuad }
            }
            PauseAnimation { duration: 100 }
            loops: Animation.Infinite
        }
    }

    Rectangle {
        objectName: "topSidebar"
        radius: 5
        antialiasing: true
        x: -10
        y: -radius
        width: 200
        height: 200
        border.color: topSidebarHH.hovered ? "cyan" : "black"
        color: "#777"

        Rectangle {
            color: "cyan"
            width: 10
            height: width
            radius: width / 2
            visible: topSidebarHH.hovered
            x: topSidebarHH.point.position.x - width / 2
            y: topSidebarHH.point.position.y - height / 2
            z: 100
        }

        HoverHandler {
            id: topSidebarHH
            objectName: "topSidebarHH"
            cursorShape: Qt.OpenHandCursor
        }

        ButtonsAndStuff {
            anchors.fill: parent
        }

        Text {
            anchors { left: parent.left; bottom: parent.bottom; margins: 12 }
            text: "Hover Handler"
        }
    }

    Rectangle {
        objectName: "bottomSidebar"
        radius: 5
        antialiasing: true
        x: -10
        anchors.bottom: parent.bottom
        anchors.bottomMargin: -radius
        width: 200
        height: 200
        border.color: bottomSidebarMA.containsMouse ? "cyan" : "black"
        color: "#777"

        MouseArea {
            id: bottomSidebarMA
            objectName: "bottomSidebarMA"
            hoverEnabled: true
            cursorShape: Qt.ClosedHandCursor
            anchors.fill: parent

            ButtonsAndStuff {
                anchors.fill: parent
            }
        }

        Text {
            anchors { left: parent.left; bottom: parent.bottom; margins: 12 }
            text: "MouseArea"
        }
    }

    HoverHandler {
        id: rootHover
    }

    Text {
        anchors.right: parent.right
        color: "cyan"
        text: "scene " + rootHover.point.scenePosition.x.toFixed(1) + ", " + rootHover.point.scenePosition.y.toFixed(1)
    }
}
