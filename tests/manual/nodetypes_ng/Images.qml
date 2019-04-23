/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

import QtQuick 2.3

Item {
    Rectangle {
        width: 100
        height: 100
        anchors.centerIn: parent
        color: "red"
        NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
    }

    Image {
        id: im
        source: "qrc:/qt.png"
        mipmap: true

        // Changing the mipmap property results in...nothing but a warning, but
        // regardless, enable the following to test.
//        Timer {
//            interval: 5000
//            onTriggered: {
//                if (im.mipmap) {
//                    console.log("disabling mipmap");
//                    im.mipmap = false;
//                } else {
//                    console.log("enabling mipmap");
//                    im.mipmap = true;
//                }
//            }
//            running: true
//            repeat: true
//        }

        SequentialAnimation on scale {
            loops: Animation.Infinite
            NumberAnimation {
                from: 1.0
                to: 4.0
                duration: 2000
            }
            NumberAnimation {
                from: 4.0
                to: 0.1
                duration: 3000
            }
            NumberAnimation {
                from: 0.1
                to: 1.0
                duration: 1000
            }
        }

        Image {
            anchors.centerIn: parent
            source: "qrc:/face-smile.png"
        }
    }

    Image {
        source: "qrc:/face-smile.png"
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        antialiasing: true // trigger smooth texture material
        NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
    }

    Item {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        scale: 20
        width: 20
        Image { x: 0; source: "blacknwhite.png"; smooth: false } // solid black
        Image { x: 2; source: "blacknwhite.png"; smooth: true } // fade to white on right
        Image { x: 4; source: "blacknwhite.png"; smooth: false } // solid black
        Image { x: 6; source: "blacknwhite.png"; smooth: true } // fade to white on right
    }
}
