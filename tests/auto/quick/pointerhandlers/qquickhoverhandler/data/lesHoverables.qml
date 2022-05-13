// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15

Rectangle {
    id: root
    width: 640
    height: 480
    color: "#444"

    Component {
        id: buttonsAndStuff
        Column {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

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
                    cursorShape: tapHandler.pressed ? Qt.BusyCursor : Qt.PointingHandCursor
                }

                TapHandler {
                    id: tapHandler
                }

                Text {
                    anchors.centerIn: parent
                    text: "HoverHandler"
                }
            }
        }
    }

    Rectangle {
        id: paddle
        objectName: "paddle"
        width: 100
        height: 100
        color: paddleHH.hovered ? "indianred" : "#888"
        x: (parent.width - width) / 2
        y: parent.height - 100
        radius: 10

        HoverHandler {
            id: paddleHH
            objectName: "paddleHH"
        }
    }

    Rectangle {
        objectName: "topSidebar"
        radius: 5
        antialiasing: true
        x: -radius
        y: -radius
        width: 120
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

        Loader {
            objectName: "topSidebarLoader"
            sourceComponent: buttonsAndStuff
            anchors.fill: parent
        }
    }

    Rectangle {
        objectName: "bottomSidebar"
        radius: 5
        antialiasing: true
        x: -radius
        anchors.bottom: parent.bottom
        anchors.bottomMargin: -radius
        width: 120
        height: 200
        border.color: bottomSidebarMA.containsMouse ? "cyan" : "black"
        color: "#777"

        MouseArea {
            id: bottomSidebarMA
            objectName: "bottomSidebarMA"
            hoverEnabled: true
            cursorShape: Qt.ClosedHandCursor
            anchors.fill: parent
        }

        Loader {
            objectName: "bottomSidebarLoader"
            sourceComponent: buttonsAndStuff
            anchors.fill: parent
        }
    }
}
