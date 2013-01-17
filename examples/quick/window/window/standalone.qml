/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

import QtQuick 2.0
import QtQuick.Window 2.0

Item {
    width: 320
    height: 240
    // It's not possible to set an Item's windowTitle.  If you want to modify
    // window properties, you need to explicitly create a Window.
    Text {
        id: text1
        anchors.centerIn: parent
        text: "First Window\n" + (Qt.application.supportsMultipleWindows ?
            "click the button to open a second window" : "only one window is allowed")
    }
    Rectangle {
        border.color: "black"
        radius: 4
        anchors.top: text1.bottom
        anchors.horizontalCenter: text1.horizontalCenter
        width: 100
        height: 30
        TextInput {
            id: ti1
            focus: true // but the modal popup will prevent input while it is open
            anchors.centerIn: parent
        }
    }
    Rectangle {
        border.color: "black"
        color: childWindow.visible ? "goldenrod" : "beige"
        radius: height / 4
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 10
        width: text.implicitWidth + 20
        height: text.implicitHeight + 20
        visible: Qt.application.supportsMultipleWindows
        Text {
            id: text
            text: "Pop up window"
            anchors.centerIn: parent
        }
        MouseArea {
            anchors.fill: parent
            onClicked: childWindow.visible = !childWindow.visible
        }
    }

    Window {
        id: childWindow
        width: 320
        height: 240
        x: 220
        y: 120
        color: "beige"
        title: "Second Window"
        modality: Qt.ApplicationModal
        flags: Qt.WindowStaysOnTopHint | Qt.FramelessWindowHint
        Text {
            id: text2
            anchors.centerIn: parent
            text: "Modal Frameless Stay-on-Top Window"
        }
        Text {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 10
            text: "X"
            MouseArea{
                anchors.fill: parent
                onClicked: childWindow.visible = false
            }
        }
        Rectangle {
            border.color: "black"
            radius: 4
            anchors.top: text2.bottom
            anchors.horizontalCenter: text2.horizontalCenter
            width: 100
            height: 30
            TextInput {
                id: ti2
                focus: true
                anchors.centerIn: parent
            }
        }
    }
}
