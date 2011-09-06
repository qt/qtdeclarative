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
    id: root
    width: 360
    height: 540
    color: "black"
    property bool spacePressed: false
    focus: true
    Image{
        source: "content/finalfrontier.png"
        anchors.centerIn:parent
    }
    Keys.onPressed: {
        if (event.key == Qt.Key_Space) {
            spacePressed = true;
            event.accepted = true;
        }
    }
    Keys.onReleased: {
        if (event.key == Qt.Key_Space) {
            spacePressed = false;
            event.accepted = true;
        }
    }

    Emitter{
        particle: "stars"
        system: particles
        emitRate: 40
        lifeSpan: 4000
        enabled: true
        size: 30
        sizeVariation: 10
        speed: PointDirection{ x: 220; xVariation: 40 }
        height: parent.height
    }
    Emitter{
        particle: "roids"
        system: particles
        emitRate: 10
        lifeSpan: 4000
        enabled: true
        size: 30
        sizeVariation: 10
        speed: PointDirection{ x: 220; xVariation: 40 }
        height: parent.height
    }
    ParticleSystem{
        id: particles
        anchors.fill: parent
    }
    ImageParticle{
        id: stars
        particles: ["stars"]
        system: particles
        source: "content/star.png"
        color: "white"
        colorVariation: 0.1
        alpha: 0
    }
    ImageParticle{
        id: roids
        particles: ["roids"]
        system: particles
        sprites: Sprite{
            id: spinState
            name: "spinning"
            source: "content/meteor.png"
            frames: 35
            duration: 60
            speedModifiesDuration: -0.1
        }
    }
    ImageParticle{
        id: shot
        particles: ["shot"]
        system: particles
        source: "content/star.png"

        color: "#0FF06600"
        colorVariation: 0.3
    }
    ImageParticle{
        id: engine
        particles: ["engine"]
        system: particles
        source: "content/particle4.png"

        color: "orange"
        SequentialAnimation on color {
            loops: Animation.Infinite
            ColorAnimation {
                from: "red"
                to: "cyan"
                duration: 1000
            }
            ColorAnimation {
                from: "cyan"
                to: "red"
                duration: 1000
            }
        }

        colorVariation: 0.2
    }
    Attractor{
        id: gs; pointX: root.width/2; pointY: root.height/2; strength: 4000000;
        system: particles
        affectedParameter: Attractor.Acceleration
        proportionalToDistance: Attractor.InverseQuadratic
    }
    Age{
        system: particles
        x: gs.pointX - 8;
        y: gs.pointY - 8;
        width: 16
        height: 16
    }
    Image{
        source:"content/rocket2.png"
        id: ship
        width: 45
        height: 22
        MouseArea{
            id: ma
            anchors.fill: parent;
            drag.axis: Drag.XandYAxis
            drag.target: ship
        }
        Emitter{
            particle: "engine"
            system: particles
            emitRate: 200
            lifeSpan: 1000
            size: 10
            endSize: 4
            sizeVariation: 4
            speed: PointDirection{ x: -128; xVariation: 32 }
            height: parent.height
            width: 20
        }
        Emitter{
            particle: "shot"
            system: particles
            emitRate: 32
            lifeSpan: 2000
            enabled: spacePressed
            size: 40
            speed: PointDirection{ x: 256; }
            x: parent.width
            y: parent.height/2
        }
    }
    Text{
        color: "white"
        anchors.bottom: parent.bottom
        text:"Drag the ship, Spacebar to fire."
    }
}

