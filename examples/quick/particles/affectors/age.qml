// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    id: root
    width: 360
    height: 600
    color: "white"

    ParticleSystem { id: particles }

    ImageParticle {
        system: particles
        sprites: Sprite {
            name: "snow"
            source: "images/snowflake.png"
            frameCount: 51
            frameDuration: 40
            frameDurationVariation: 8
        }
    }

    Emitter {
        system: particles
        emitRate: 20
        lifeSpan: 8000
        velocity: PointDirection { y:80; yVariation: 40; }
        acceleration: PointDirection { y: 4 }
        size: 36
        endSize: 12
        sizeVariation: 8
        width: parent.width
        height: 100
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
    }

    Rectangle {
        color: "#803333AA"
        border.color: "black"
        x: ma.mouseX - 36
        y: ma.mouseY - 36
        width: 72
        height: 72
        //! [0]
        Age {
            anchors.fill: parent
            system: particles
            once: true
            lifeLeft: 1200
            advancePosition: false
        }
        //! [0]
    }
}
