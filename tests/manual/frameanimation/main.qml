/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the manual tests of the Qt Toolkit.
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
import QtQuick.Controls

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("FrameAnimation Tester")

    component AnimatedComponent : Item {

        property alias text: textItem.text

        width: 60
        height: 60
        Rectangle {
            anchors.fill: parent
            color: "#b0b0b0"
            border.width: 1
            border.color: "#404040"
        }
        Text {
            id: textItem
            anchors.centerIn: parent
            font.bold: true
            font.pixelSize: 14
            color: "#202020"
        }
    }

    FrameAnimation {
        id: frameAnimation
        onTriggered: {
            var rotation = rotatingRect1.rotation;
            // How long (in seconds) a full rotation takes
            var rotationDuration = 4.0;
            rotation += (360 / rotationDuration) * frameTime;
            rotatingRect1.rotation = rotation;
            if (currentFrame % 2 == 0)
                rotatingRect2.rotation = rotation;
            if (currentFrame % 2 == 1)
                rotatingRect3.rotation = rotation;
            if (currentFrame % 3 == 0)
                rotatingRect4.rotation = rotation;
            if (currentFrame % 10 == 0)
                rotatingRect5.rotation = rotation;
            if (currentFrame % 10 == 4)
                rotatingRect6.rotation = rotation;
        }
    }

    Row {
        id: buttonsRow
        anchors.top: parent.top
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10
        Button {
            width: 100
            checkable: true
            checked: frameAnimation.running
            text: "running"
            onCheckedChanged: {
                frameAnimation.running = checked;
            }
        }
        Button {
            width: 100
            checkable: true
            checked: frameAnimation.paused
            text: "paused"
            onCheckedChanged: {
                frameAnimation.paused = checked;
            }
        }
    }

    Row {
        id: buttonsRow2
        anchors.top: buttonsRow.bottom
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10
        Button {
            width: 80
            text: "start()"
            onClicked: {
                frameAnimation.start();
            }
        }
        Button {
            width: 80
            text: "stop()"
            onClicked: {
                frameAnimation.stop();
            }
        }
        Button {
            width: 80
            text: "restart()"
            onClicked: {
                frameAnimation.restart();
            }
        }
        Button {
            width: 80
            text: "pause()"
            onClicked: {
                frameAnimation.pause();
            }
        }
        Button {
            width: 80
            text: "resume()"
            onClicked: {
                frameAnimation.resume();
            }
        }
        Button {
            width: 80
            text: "reset()"
            onClicked: {
                frameAnimation.reset();
            }
        }
    }

    Column {
        id: statusTexts
        anchors.top: buttonsRow2.bottom
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10
        Text {
            text: "FRAME: <b>" + frameAnimation.currentFrame + "</b>"
            font.pixelSize: 16
        }
        Text {
            text: "FRAME TIME: <b>" + frameAnimation.frameTime.toFixed(4) + "</b>"
            font.pixelSize: 16
        }
        Text {
            text: "SMOOTH FRAME TIME: <b>" + frameAnimation.smoothFrameTime.toFixed(4) + "</b>"
            font.pixelSize: 16
        }
        Text {
            text: "ELAPSED TIME: <b>" + frameAnimation.elapsedTime.toFixed(4) + "</b>"
            font.pixelSize: 16
        }
    }

    Text {
        id: infoText
        anchors.top: statusTexts.bottom
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Animations with different update slot / refresh times"
        font.pixelSize: 16
    }

    Row {
        id: rotatingRects
        anchors.top: infoText.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 20
        AnimatedComponent {
            id: rotatingRect1
            text: "1/1"
        }
        AnimatedComponent {
            id: rotatingRect2
            text: "1/2"
        }
        AnimatedComponent {
            id: rotatingRect3
            text: "2/2"
        }
        AnimatedComponent {
            id: rotatingRect4
            text: "1/3"
        }
        AnimatedComponent {
            id: rotatingRect5
            text: "1/10"
        }
        AnimatedComponent {
            id: rotatingRect6
            text: "5/10"
        }
    }

    Row {
        id: movingRects
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        spacing: 20
        height: 120
        AnimatedComponent {
            id: movingRect1
            text: "1/1"
            y: Math.sin(frameAnimation.elapsedTime) * 50
        }
        AnimatedComponent {
            id: movingRect2
            text: "1/2"
            y: (frameAnimation.currentFrame % 2 == 0) ? Math.sin(frameAnimation.elapsedTime) * 50 : y
        }
        AnimatedComponent {
            id: movingRect3
            text: "2/2"
            y: (frameAnimation.currentFrame % 2 == 1) ? Math.sin(frameAnimation.elapsedTime) * 50 : y
        }
        AnimatedComponent {
            id: movingRect4
            text: "1/3"
            y: (frameAnimation.currentFrame % 3 == 0) ? Math.sin(frameAnimation.elapsedTime) * 50 : y
        }
        AnimatedComponent {
            id: movingRect5
            text: "1/10"
            y: (frameAnimation.currentFrame % 10 == 0) ? Math.sin(frameAnimation.elapsedTime) * 50 : y
        }
        AnimatedComponent {
            id: movingRect6
            text: "5/10"
            y: (frameAnimation.currentFrame % 10 == 4) ? Math.sin(frameAnimation.elapsedTime) * 50 : y
        }
    }
}
