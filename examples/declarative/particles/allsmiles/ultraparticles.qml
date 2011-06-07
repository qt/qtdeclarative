/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

Rectangle{
    color: "white"
    width: 640
    height: 480
    ParticleSystem{ 
        id: sys 
    }
    ImageParticle{
        sprites: [
            Sprite{
                name: "licking"
                source: "content/squarefacewhite.png"
                frames: 6
                duration: 120
                to: {"dying":1, "licking":5}
            },
            Sprite{
                name: "dying"
                source: "content/squarefacewhiteX.png"
                frames: 4
                duration: 120
                to: {"dead":1}
            },
            Sprite{
                name: "dead"
                source: "content/squarefacewhiteXX.png"
                frames: 1
                duration: 120
            }
        ]
        colorVariation: 0.5
        rotationSpeedVariation: 360
        system: sys
        colorTable: "../trails/content/colortable.png"
    }
    Friction{
        factor: 0.1
        system: sys
    }
    Emitter{
        system: sys
        anchors.centerIn: parent
        id: particles
        particlesPerSecond: 200
        particleDuration: 6000
        emitting: true
        speed: AngledDirection{angleVariation: 360; magnitude: 80; magnitudeVariation: 40}
        particleSize: 40
        particleEndSize: 80
    }
    Text{
        x: 16
        y: 16
        text: "QML..."
        style: Text.Outline; styleColor: "#AAAAAA"
        font.pixelSize: 32
    }
    Text{
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 16
        text: "... can you be trusted with the power?"
        style: Text.Outline; styleColor: "#AAAAAA"
        font.pixelSize: 32
    }
}
