// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    id: root
    width: 360
    height: 540
    color: "black"
    Image {
        source: "images/finalfrontier.png"
        anchors.centerIn:parent
    }
    ParticleSystem {
        id: particles
        anchors.fill: parent

        Emitter {
            group: "stars"
            emitRate: 40
            lifeSpan: 4000
            enabled: true
            size: 30
            sizeVariation: 10
            velocity: PointDirection { x: 220; xVariation: 40 }
            height: parent.height
        }
        Emitter {
            group: "roids"
            emitRate: 10
            lifeSpan: 4000
            enabled: true
            size: 30
            sizeVariation: 10
            velocity: PointDirection { x: 220; xVariation: 40 }
            height: parent.height
        }
        ImageParticle {
            id: stars
            groups: ["stars"]
            source: "qrc:///particleresources/star.png"
            color: "white"
            colorVariation: 0.1
            alpha: 0
        }
        ImageParticle {
            id: roids
            groups: ["roids"]
            sprites: Sprite {
                id: spinState
                name: "spinning"
                source: "images/meteor.png"
                frameCount: 35
                frameDuration: 60
            }
        }
        ImageParticle {
            id: shot
            groups: ["shot"]
            source: "qrc:///particleresources/star.png"

            color: "#0FF06600"
            colorVariation: 0.3
        }
        ImageParticle {
            id: engine
            groups: ["engine"]
            source: "qrc:///particleresources/fuzzydot.png"

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
        //! [0]
        Attractor {
            id: gs; pointX: root.width/2; pointY: root.height/2; strength: 4000000;
            affectedParameter: Attractor.Acceleration
            proportionalToDistance: Attractor.InverseQuadratic
        }
        //! [0]
        Age {
            x: gs.pointX - 8;
            y: gs.pointY - 8;
            width: 16
            height: 16
        }
        Rectangle {
            color: "black"
            width: 8
            height: 8
            radius: 4
            x: gs.pointX - 4
            y: gs.pointY - 4
        }

        Image {
            source:"images/rocket2.png"
            id: ship
            width: 45
            height: 22
            //Automatic movement
            SequentialAnimation on x {
                loops: -1
                NumberAnimation{to: root.width-45; easing.type: Easing.InOutQuad; duration: 2000}
                NumberAnimation{to: 0; easing.type: Easing.OutInQuad; duration: 6000}
            }
            SequentialAnimation on y {
                loops: -1
                NumberAnimation{to: root.height-22; easing.type: Easing.OutInQuad; duration: 6000}
                NumberAnimation{to: 0; easing.type: Easing.InOutQuad; duration: 2000}
            }
        }
        Emitter {
            group: "engine"
            emitRate: 200
            lifeSpan: 1000
            size: 10
            endSize: 4
            sizeVariation: 4
            velocity: PointDirection { x: -128; xVariation: 32 }
            height: ship.height
            y: ship.y
            x: ship.x
            width: 20
        }
        Emitter {
            group: "shot"
            emitRate: 32
            lifeSpan: 1000
            enabled: true
            size: 40
            velocity: PointDirection { x: 256; }
            x: ship.x + ship.width
            y: ship.y + ship.height/2
        }
    }
}

