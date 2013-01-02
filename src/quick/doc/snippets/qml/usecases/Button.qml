/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
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

//![0]
import QtQuick 2.0

Rectangle {
    id: container
    // The caption property is an alias to the text of the Text element, so Button users can set the text
    property alias caption: txt.text
    // The clicked signal is emitted whenever the button is clicked, so Button users can respond
    signal clicked

    // The button is set to have rounded corners and a thin black border
    radius: 4
    border.width: 1
    // This button has a fixed size, but it could resize based on the text
    width: 160
    height: 40

    // A SystemPalette is used to get colors from the system settings for the background
    SystemPalette { id: sysPalette }

    gradient: Gradient {

        // The top gradient is darker when 'pressed', all colors come from the system palette
        GradientStop { position: 0.0; color: ma.pressed ? sysPalette.dark : sysPalette.light }

        GradientStop { position: 1.0; color: sysPalette.button }
    }

    Text {
        id: txt
        // This is the default value of the text, but most Button users will set their own with the caption property
        text: "Button"
        font.bold: true
        font.pixelSize: 16
        anchors.centerIn: parent
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        // This re-emits the clicked signal on the root item, so that Button users can respond to it
        onClicked: container.clicked()
    }
}

//![0]
