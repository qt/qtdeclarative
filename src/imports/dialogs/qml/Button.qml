/*****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick.Dialogs module of the Qt Toolkit.
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
*****************************************************************************/

import QtQuick 2.1

Item {
    height: label.implicitHeight * 2
    width: Math.max(label.implicitWidth * 1.2, height * 2.5);
    anchors.verticalCenter: parent.verticalCenter
    property alias text: label.text
    property string tooltip
    signal clicked
    SystemPalette { id: palette }
    Rectangle {
        antialiasing: true
        border.color: mouseArea.pressed ? palette.highlight : palette.light
        color: "transparent"
        anchors.fill: parent
        anchors.rightMargin: 1
        anchors.bottomMargin: 1
        radius: 3
    }
    Rectangle {
        border.color: palette.dark
        anchors.fill: parent
        anchors.leftMargin: 1
        anchors.topMargin: 1
        radius: 3
    }
    Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: mouseArea.pressed ? palette.dark : palette.light }
            GradientStop { position: 0.2; color: palette.button }
            GradientStop { position: 0.8; color: palette.button }
            GradientStop { position: 1.0; color: mouseArea.pressed ? palette.light : palette.dark }
        }
        anchors.fill: parent
        anchors.margins: 1
        radius: 3
    }
    Text {
        id: label
        anchors.centerIn: parent
        color: palette.buttonText
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}
