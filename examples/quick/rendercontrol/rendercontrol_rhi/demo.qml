// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    id: root

    gradient: Gradient {
        GradientStop { position: 0; color: "steelblue" }
        GradientStop { position: 1; color: "black" }
    }

    Text {
        anchors.centerIn: parent
        text: "Qt Quick in a texture"
        font.pointSize: 40
        color: "white"

        SequentialAnimation on rotation {
            PauseAnimation { duration: 2500 }
            NumberAnimation { from: 0; to: 360; duration: 5000; easing.type: Easing.InOutCubic }
            loops: Animation.Infinite
        }
    }

    ParticleSystem {
        id: particles
        anchors.fill: parent

        ImageParticle {
            id: smoke
            system: particles
            anchors.fill: parent
            groups: ["A", "B"]
            source: "qrc:///particleresources/glowdot.png"
            colorVariation: 0
            color: "#00111111"
        }
        ImageParticle {
            id: flame
            anchors.fill: parent
            system: particles
            groups: ["C", "D"]
            source: "qrc:///particleresources/glowdot.png"
            colorVariation: 0.1
            color: "#00ff400f"
        }

        Emitter {
            id: fire
            system: particles
            group: "C"

            y: parent.height
            width: parent.width

            emitRate: 350
            lifeSpan: 3500

            acceleration: PointDirection { y: -17; xVariation: 3 }
            velocity: PointDirection {xVariation: 3}

            size: 24
            sizeVariation: 8
            endSize: 4
        }

        TrailEmitter {
            id: fireSmoke
            group: "B"
            system: particles
            follow: "C"
            width: root.width
            height: root.height - 68

            emitRatePerParticle: 1
            lifeSpan: 2000

            velocity: PointDirection {y:-17*6; yVariation: -17; xVariation: 3}
            acceleration: PointDirection {xVariation: 3}

            size: 36
            sizeVariation: 8
            endSize: 16
        }

        TrailEmitter {
            id: fireballFlame
            anchors.fill: parent
            system: particles
            group: "D"
            follow: "E"

            emitRatePerParticle: 120
            lifeSpan: 180
            emitWidth: TrailEmitter.ParticleSize
            emitHeight: TrailEmitter.ParticleSize
            emitShape: EllipseShape{}

            size: 16
            sizeVariation: 4
            endSize: 4
        }

        TrailEmitter {
            id: fireballSmoke
            anchors.fill: parent
            system: particles
            group: "A"
            follow: "E"

            emitRatePerParticle: 128
            lifeSpan: 2400
            emitWidth: TrailEmitter.ParticleSize
            emitHeight: TrailEmitter.ParticleSize
            emitShape: EllipseShape{}

            velocity: PointDirection {yVariation: 16; xVariation: 16}
            acceleration: PointDirection {y: -16}

            size: 24
            sizeVariation: 8
            endSize: 8
        }

        Emitter {
            id: balls
            system: particles
            group: "E"

            y: parent.height
            width: parent.width

            emitRate: 2
            lifeSpan: 7000

            velocity: PointDirection {y:-17*4*2; xVariation: 6*6}
            acceleration: PointDirection {y: 17*2; xVariation: 6*6}

            size: 8
            sizeVariation: 4
        }

        Turbulence { //A bit of turbulence makes the smoke look better
            anchors.fill: parent
            groups: ["A","B"]
            strength: 32
            system: particles
        }
    }

    onWidthChanged: particles.reset()
    onHeightChanged: particles.reset()
}
