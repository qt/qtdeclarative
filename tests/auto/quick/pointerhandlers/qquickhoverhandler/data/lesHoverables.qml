// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15

Rectangle {
    id: root
    width: 640
    height: 480
    color: "#444"

    component CheckBox: Row {
        id: cbRoot
        property bool checked : true
        property string label : "CheckBox"
        spacing: 4
        Rectangle {
            width: 16; height: 16
            // comment out this color change to test whether we rely on "dirty" items to
            // trigger QQuickDeliveryAgentPrivate::flushFrameSynchronousEvents() to update hover state
            color: cbRoot.checked ? "cadetblue" : "transparent"
            border.color: "black"
            TapHandler { onTapped: cbRoot.checked = !cbRoot.checked }
        }
        Text { text: cbRoot.label }
    }

    component ButtonsAndStuff: Column {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8
        function toggleMAEnabled() { maButtonCB.checked = !maButtonCB.checked }
        function toggleMAHover() { maButtonHoverCB.checked = !maButtonHoverCB.checked }
        function toggleHHEnabled() { hhButtonHoverCB.checked = !hhButtonHoverCB.checked }

        CheckBox {
            id: maButtonCB
            label: "enabled"
        }

        CheckBox {
            id: maButtonHoverCB
            label: "hover enabled"
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
                enabled: maButtonCB.checked
                hoverEnabled: maButtonHoverCB.checked
                cursorShape: Qt.UpArrowCursor
                anchors.fill: parent
            }

            Text {
                anchors.centerIn: parent
                text: "MouseArea"
            }
        }

        CheckBox {
            id: hhButtonHoverCB
            label: "hover enabled"
        }

        Rectangle {
            id: buttonRoot
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
                enabled: hhButtonHoverCB.checked
                cursorShape: tapHandler.pressed ? Qt.BusyCursor : Qt.PointingHandCursor
            }

            TapHandler {
                id: tapHandler
                onTapped: {
                    console.log("buttonRoot tapped")
                    buttonHH.enabled = !buttonHH.enabled
                }
            }

            Text {
                anchors.centerIn: parent
                text: "HoverHandler"
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

        ButtonsAndStuff {
            id: tbs
            objectName: "topSidebarContents"
            anchors.fill: parent
            Shortcut {
                sequence: "Ctrl+E"
                onActivated: tbs.toggleMAEnabled()
            }
            Shortcut {
                sequence: "Ctrl+M"
                onActivated: tbs.toggleMAHover()
            }
            Shortcut {
                sequence: "Ctrl+H"
                onActivated: tbs.toggleHHEnabled()
            }
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

        ButtonsAndStuff {
            objectName: "bottomSidebarContents"
            anchors.fill: parent
        }
    }
}
