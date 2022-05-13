// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    width: 360
    height: 600
    color: "black"
    ParticleSystem {
        anchors.fill: parent
        id: syssy
        //! [0]
        ParticleGroup {
            name: "fire"
            duration: 2000
            durationVariation: 2000
            to: {"splode":1}
        }
        //! [0]
        //! [1]
        ParticleGroup {
            name: "splode"
            duration: 400
            to: {"dead":1}
            TrailEmitter {
                group: "works"
                emitRatePerParticle: 100
                lifeSpan: 1000
                maximumEmitted: 1200
                size: 8
                velocity: AngleDirection {angle: 270; angleVariation: 45; magnitude: 20; magnitudeVariation: 20;}
                acceleration: PointDirection {y:100; yVariation: 20}
            }
        }
        //! [1]
        //! [2]
        ParticleGroup {
            name: "dead"
            duration: 1000
            Affector {
                once: true
                onAffected: (x, y)=> worksEmitter.burst(400,x,y)
            }
        }
        //! [2]

        Timer {
            interval: 6000
            running: true
            triggeredOnStart: true
            repeat: true
            onTriggered:startingEmitter.pulse(100);
        }
        Emitter {
            id: startingEmitter
            group: "fire"
            width: parent.width
            y: parent.height
            enabled: false
            emitRate: 80
            lifeSpan: 6000
            velocity: PointDirection {y:-100;}
            size: 32
        }

        Emitter {
            id: worksEmitter
            group: "works"
            enabled: false
            emitRate: 100
            lifeSpan: 1600
            maximumEmitted: 6400
            size: 8
            velocity: CumulativeDirection {
                PointDirection {y:-100}
                AngleDirection {angleVariation: 360; magnitudeVariation: 80;}
            }
            acceleration: PointDirection {y:100; yVariation: 20}
        }

        ImageParticle {
            groups: ["works", "fire", "splode"]
            source: "qrc:///particleresources/glowdot.png"
            entryEffect: ImageParticle.Scale
        }
    }
}

