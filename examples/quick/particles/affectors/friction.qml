// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Item {
    width: 360
    height: 600

    Image {
        source: "images/backgroundLeaves.jpg"
        anchors.fill: parent
    }
    ParticleSystem {
        anchors.fill: parent
        Emitter {
            width: parent.width
            emitRate: 4
            lifeSpan: 14000
            size: 80
            velocity: PointDirection { y: 160; yVariation: 80; xVariation: 20 }
        }

        ImageParticle {
            anchors.fill: parent
            id: particles
            sprites: [Sprite {
                    source: "images/realLeaf1.png"
                    frameCount: 1
                    frameDuration: 1
                    to: {"a":1, "b":1, "c":1, "d":1}
                }, Sprite {
                    name: "a"
                    source: "images/realLeaf1.png"
                    frameCount: 1
                    frameDuration: 10000
                },
                Sprite {
                    name: "b"
                    source: "images/realLeaf2.png"
                    frameCount: 1
                    frameDuration: 10000
                },
                Sprite {
                    name: "c"
                    source: "images/realLeaf3.png"
                    frameCount: 1
                    frameDuration: 10000
                },
                Sprite {
                    name: "d"
                    source: "images/realLeaf4.png"
                    frameCount: 1
                    frameDuration: 10000
                }
            ]

            width: 100
            height: 100
            x: 20
            y: 20
            z:4
        }

        //! [0]
        Friction {
            anchors.fill: parent
            anchors.margins: -40
            factor: 0.4
        }
        //! [0]
    }
}
