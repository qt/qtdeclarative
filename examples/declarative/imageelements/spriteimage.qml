/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/
import QtQuick 2.0

Item {
    width: 480
    height: 1280
    MouseArea {
        onClicked: anim.start();
        anchors.fill: parent
    }
    SequentialAnimation {
        id: anim
        ScriptAction { script: image.goalSprite = "falling"; }
        NumberAnimation { target: image; property: "y"; to: 1480; duration: 12000; }
        ScriptAction { script: {image.goalSprite = ""; image.jumpTo("still");} }
        PropertyAction { target: image; property: "y"; value: 0 }
    }
    SpriteImage {
        id: image
        width: 256
        height: 256
        anchors.horizontalCenter: parent.horizontalCenter
        interpolate: false
        goalSprite: ""
        Sprite{
            name: "still"
            source: "content/Bear0.png"
            frames: 1
            frameWidth: 256
            frameHeight: 256
            duration: 100
            to: {"still":1, "blink":0.1, "floating":0}
        }
        Sprite{
            name: "blink"
            source: "content/BearB.png"
            frames: 3
            frameWidth: 256
            frameHeight: 256
            duration: 100
            to: {"still":1}
        }
        Sprite{
            name: "floating"
            source: "content/Bear1.png"
            frames: 9
            frameWidth: 256
            frameHeight: 256
            duration: 160
            to: {"still":0, "flailing":1}
        }
        Sprite{
            name: "flailing"
            source: "content/Bear2.png"
            frames: 8
            frameWidth: 256
            frameHeight: 256
            duration: 160
            to: {"falling":1}
        }
        Sprite{
            name: "falling"
            source: "content/Bear3.png"
            frames: 5
            frameWidth: 256
            frameHeight: 256
            duration: 160
            to: {"falling":1}
        }
    }
}
