/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import "components"

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

    HoverHandler {
        id: rootHover
    }

    Text {
        anchors.right: parent.right
        color: "cyan"
        text: "scene " + rootHover.point.scenePosition.x.toFixed(1) + ", " + rootHover.point.scenePosition.y.toFixed(1)
    }
}
