// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Particles 2.0

Item {
    id: block
    property bool dying: false
    property bool spawned: false
    property int type: 0
    property ParticleSystem particleSystem

    Behavior on x {
        enabled: spawned;
        SpringAnimation{ spring: 2; damping: 0.2 }
    }
    Behavior on y {
        SpringAnimation{ spring: 2; damping: 0.2 }
    }

    Image {
        id: img
        source: {
            if(type == 0){
                "pics/redStone.png";
            } else if(type == 1) {
                "pics/blueStone.png";
            } else {
                "pics/greenStone.png";
            }
        }
        opacity: 0
        Behavior on opacity { NumberAnimation { duration: 200 } }
        anchors.fill: parent
    }
    Emitter {
        id: particles
        system: particleSystem
        group: {
            if(type == 0){
                "red";
            } else if (type == 1) {
                "blue";
            } else {
                "green";
            }
        }
        anchors.fill: parent

        velocity: TargetDirection{targetX: block.width/2; targetY: block.height/2; magnitude: -60; magnitudeVariation: 60}
        shape: EllipseShape{fill:true}
        enabled: false;
        lifeSpan: 700; lifeSpanVariation: 100
        emitRate: 1000
        maximumEmitted: 100 //only fires 0.1s bursts (still 2x old number)
        size: 28
        endSize: 14
    }

    states: [
        State {
            name: "AliveState"; when: spawned == true && dying == false
            PropertyChanges { target: img; opacity: 1 }
        },

        State {
            name: "DeathState"; when: dying == true
            StateChangeScript { script: particles.pulse(0.1); }
            PropertyChanges { target: img; opacity: 0 }
            StateChangeScript { script: block.destroy(1000); }
        }
    ]
}
