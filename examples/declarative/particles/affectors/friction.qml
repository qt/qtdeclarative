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
import QtQuick.Particles 2.0

Item {
    width: 360
    height: 600

    Image {
        source: "../images/backgroundLeaves.jpg"
        anchors.fill: parent
    }
    ParticleSystem { id: sys }
    Emitter {
        system: sys
        width: parent.width
        emitRate: 4
        lifeSpan: 14000
        size: 80
        speed: PointDirection { y: 160; yVariation: 80; xVariation: 20 }
    }

    ImageParticle {
        anchors.fill: parent
        id: particles
        system: sys
        sprites: [Sprite {
                source: "../images/realLeaf1.png"
                frames: 1
                frameDuration: 1
                to: {"a":1, "b":1, "c":1, "d":1}
            }, Sprite {
                name: "a"
                source: "../images/realLeaf1.png"
                frames: 1
                frameDuration: 10000
            },
            Sprite {
                name: "b"
                source: "../images/realLeaf2.png"
                frames: 1
                frameDuration: 10000
            },
            Sprite {
                name: "c"
                source: "../images/realLeaf3.png"
                frames: 1
                frameDuration: 10000
            },
            Sprite {
                name: "d"
                source: "../images/realLeaf4.png"
                frames: 1
                frameDuration: 10000
            }
        ]

        width: 100
        height: 100
        x: 20
        y: 20
        z:4
    }
    Friction {
        anchors.fill: parent
        anchors.margins: -40
        system: sys
        factor: 0.4
    }
}
