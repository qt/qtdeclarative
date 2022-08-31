// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Item {
    id: block

    property int type: 0
    property bool dying: false

    //![1]
    property bool spawned: false

    Behavior on x {
        enabled: block.spawned;
        SpringAnimation{ spring: 2; damping: 0.2 }
    }
    Behavior on y {
        SpringAnimation{ spring: 2; damping: 0.2 }
    }
    //![1]

    //![2]
    Image {
        id: img

        anchors.fill: parent
        source: {
            if (block.type == 0)
                return "pics/redStone.png";
            else if (block.type == 1)
                return "pics/blueStone.png";
            else
                return "pics/greenStone.png";
        }
        opacity: 0

        Behavior on opacity {
            NumberAnimation { properties:"opacity"; duration: 200 }
        }
    }
    //![2]

    //![3]
    ParticleSystem {
        id: sys
        anchors.centerIn: parent
        ImageParticle {
            // ![0]
            source: {
                if (block.type == 0)
                    return "pics/redStar.png";
                else if (block.type == 1)
                    return "pics/blueStar.png";
                else
                    return "pics/greenStar.png";
            }
            rotationVelocityVariation: 360
            // ![0]
        }

        Emitter {
            id: particles
            anchors.centerIn: parent
            emitRate: 0
            lifeSpan: 700
            velocity: AngleDirection {angleVariation: 360; magnitude: 80; magnitudeVariation: 40}
            size: 16
        }
    }
    //![3]

    //![4]
    states: [
        State {
            name: "AliveState"
            when: block.spawned == true && block.dying == false
            PropertyChanges { img.opacity: 1 }
        },

        State {
            name: "DeathState"
            when: block.dying == true
            StateChangeScript { script: particles.burst(50); }
            PropertyChanges { img.opacity: 0 }
            StateChangeScript { script: block.destroy(1000); }
        }
    ]
    //![4]
}
