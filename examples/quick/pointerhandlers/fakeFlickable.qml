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
    color: "#444"
    width: 480
    height: 640

    FakeFlickable {
        id: ff
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: rightSB.width

        Text {
            id: text
            color: "beige"
            font.family: "mono"
            font.pointSize: slider.value
            onTextChanged: console.log("text geom " + width + "x" + height +
                                       ", parent " + parent + " geom " + parent.width + "x" + parent.height)
        }

        onFlickStarted: {
            root.border.color = "green"
            console.log("flick started with velocity " + velocity)
        }
        onFlickEnded: {
            root.border.color = "transparent"
            console.log("flick ended with velocity " + velocity)
        }

        Component.onCompleted: {
            var request = new XMLHttpRequest()
            request.open('GET', 'components/FakeFlickable.qml')
            request.onreadystatechange = function(event) {
                if (request.readyState === XMLHttpRequest.DONE)
                    text.text = request.responseText
            }
            request.send()
        }
    }

    ScrollBar {
        id: rightSB
        objectName: "rightSB"
        flick: ff
        height: parent.height - width
        anchors.right: parent.right
    }

    ScrollBar {
        id: bottomSB
        objectName: "bottomSB"
        flick: ff
        width: parent.width - height
        anchors.bottom: parent.bottom
    }

    Rectangle {
        id: cornerCover
        color: "lightgray"
        width: rightSB.width
        height: bottomSB.height
        anchors {
            right: parent.right
            bottom: parent.bottom
        }
    }

    LeftDrawer {
        width: 100
        anchors.verticalCenter: parent.verticalCenter
        Slider {
            id: slider
            label: "font\nsize"
            anchors.fill: parent
            anchors.margins: 10
            maximumValue: 36
            value: 14
        }
    }
}
