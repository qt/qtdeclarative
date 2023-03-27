/****************************************************************************
**
** Copyright (C) 2023 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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
import QtQuick 2.0

Rectangle {
    id: page
    color: "white"
    width: 182; height: 164

//! [tiled border image]
BorderImage {
    anchors { fill: parent; margins: 6 }
    border { left: 30; top: 30; right: 30; bottom: 30 }
    horizontalTileMode: BorderImage.Round
    verticalTileMode: BorderImage.Round
    source: "pics/borderframe.png"
}
//! [tiled border image]

    Rectangle {
        anchors.fill: parent
        anchors.margins: 5
        color: "transparent"
        border.color: "gray"

        Rectangle {
            x: 30; y: 0
            width: 1; height: parent.height
            color: "gray"

            Text {
                text: "1"
                font.pixelSize: 9
                color: "red"
                anchors.right: parent.right
                anchors.rightMargin: 1
                y: 20
            }

            Text {
                text: "4"
                font.pixelSize: 9
                color: "red"
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 1
            }

            Text {
                text: "7"
                font.pixelSize: 9
                color: "red"
                y: parent.height - 30
                anchors.right: parent.right
                anchors.rightMargin: 1
            }
        }

        Rectangle {
            x: parent.width - 30; y: 0
            width: 1; height: parent.height
            color: "gray"

            Text {
                text: "3"
                font.pixelSize: 9
                color: "red"
                x: 1
                y: 20
            }

            Text {
                text: "6"
                font.pixelSize: 9
                color: "red"
                x: 1
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "9"
                font.pixelSize: 9
                color: "red"
                x: 1
                y: parent.height - 30
            }
        }

        Text {
            text: "5"
            font.pixelSize: 9
            color: "red"
            anchors.centerIn: parent
        }

        Rectangle {
            x: 0; y: 30
            width: parent.width; height: 1
            color: "gray"

            Text {
                text: "2"
                font.pixelSize: 9
                color: "red"
                anchors.horizontalCenter: parent.horizontalCenter
                y: -10
            }
        }

        Rectangle {
            x: 0; y: parent.height - 30
            width: parent.width; height: 1
            color: "gray"

            Text {
                text: "8"
                font.pixelSize: 9
                color: "red"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
