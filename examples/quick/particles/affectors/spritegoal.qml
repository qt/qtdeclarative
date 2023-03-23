// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Item {
    id: root
    width: 360
    height: 540
    MouseArea {
        id: ma
        anchors.fill: parent
    }

    ParticleSystem { id: sys }
    Image {
        source: "images/finalfrontier.png"
        transformOrigin: Item.Center
        anchors.centerIn: parent
        NumberAnimation on rotation {
            from: 0
            to: 360
            duration: 200000
            loops: Animation.Infinite
        }

    }
    ImageParticle {
        system: sys
        groups: ["starfield"]
        source: "qrc:///particleresources/star.png"
        colorVariation: 0.3
        color: "white"
    }
    Emitter {
        id: starField
        system: sys
        group: "starfield"

        emitRate: 80
        lifeSpan: 2500

        anchors.centerIn: parent

        //acceleration: AngledDirection {angleVariation: 360; magnitude: 200}//Is this a better effect, more consistent velocity?
        acceleration: PointDirection { xVariation: 200; yVariation: 200; }

        size: 0
        endSize: 80
        sizeVariation: 10
    }
    Emitter {
        system: sys
        group: "meteor"
        emitRate: 12
        lifeSpan: 5000
        acceleration: PointDirection { xVariation: 80; yVariation: 80; }
        size: 15
        endSize: 300
        anchors.centerIn: parent
     }
    ImageParticle {
        system: sys
        groups: ["meteor"]
        sprites:[Sprite {
                id: spinState
                name: "spinning"
                source: "images/meteor.png"
                frameCount: 35
                frameDuration: 40
                randomStart: true
                to: {"explode":0, "spinning":1}
            },Sprite {
                name: "explode"
                source: "images/_explo.png"
                frameCount: 22
                frameDuration: 40
                to: {"nullFrame":1}
            },Sprite {//Not sure if this is needed, but seemed easiest
                name: "nullFrame"
                source: "images/nullRock.png"
                frameCount: 1
                frameDuration: 1000
            }
        ]
    }
    //! [0]
    SpriteGoal {
        groups: ["meteor"]
        system: sys
        goalState: "explode"
        jump: true
        anchors.fill: rocketShip
        width: 60
        height: 60
    }
    //! [0]
    Image {
        id: rocketShip
        source: "images/rocket.png"
        anchors.centerIn: holder
        rotation: (circle.percent+0.25) * 360
        z: 2
    }
    Item {
        id: holder
        x: circle.x - Math.sin(circle.percent * 6.28316530714)*200
        y: circle.y + Math.cos(circle.percent * 6.28316530714)*200
        z: 1
    }

    Item {
        id: circle
        x: root.width / 1.2
        y: root.height / 1.7
        property real percent

        SequentialAnimation on percent {
            id: circleAnim1
            loops: Animation.Infinite
            running: true
            NumberAnimation {
            duration: 4000
            from: 1
            to: 0
            }

        }
    }

    ImageParticle {
        z:0
        system: sys
        groups: ["exhaust"]
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

    Emitter {
        id: trailsNormal2
        system: sys
        group: "exhaust"

        emitRate: 300
        lifeSpan: 500

        y: holder.y
        x: holder.x

        velocity: PointDirection { xVariation: 40; yVariation: 40; }
        velocityFromMovement: 16

        acceleration: PointDirection { xVariation: 10; yVariation: 10; }

        size: 4
        sizeVariation: 4
    }
}
