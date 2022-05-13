// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    color: "lightsteelblue"
    width: 800
    height: 800
    id: root

    SpriteSequence {
        sprites: Sprite {
                name: "bear"
                source: "images/bear_tiles.png"
                frameCount: 13
                frameDuration: 120
            }
        width: 250
        height: 250
        x: 20
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        z:4
    }

    ParticleSystem { id: sys }

    ImageParticle {
        anchors.fill: parent
        id: particles
        system: sys
        sprites: [Sprite {
            name: "happy"
            source: "images/starfish_1.png"
            frameCount: 1
            frameDuration: 260
            to: {"happy": 1, "silly": 1, "angry": 1}
        }, Sprite {
            name: "angry"
            source: "images/starfish_0.png"
            frameCount: 1
            frameDuration: 260
            to: {"happy": 1, "silly": 1, "angry": 1}
        }, Sprite {
            name: "silly"
            source: "images/starfish_2.png"
            frameCount: 1
            frameDuration: 260
            to: {"happy": 1, "silly": 1, "noticedbear": 0}
        }, Sprite {
            name: "noticedbear"
            source: "images/starfish_3.png"
            frameCount: 1
            frameDuration: 2600
        }]
    }

    Emitter {
        system: sys
        emitRate: 2
        lifeSpan: 10000
        velocity: AngleDirection {angle: 90; magnitude: 60; angleVariation: 5}
        acceleration: PointDirection { y: 10 }
        size: 160
        sizeVariation: 40
        width: parent.width
        height: 100
    }

    SpriteGoal {
        system: sys
        width: root.width;
        height: root.height/2;
        y: root.height/2;
        goalState:"noticedbear"
    }
}
