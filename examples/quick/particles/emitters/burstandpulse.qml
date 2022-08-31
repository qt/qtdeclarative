// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    id: root

    width: 320
    height: 480
    color: "black"
    property bool lastWasPulse: false
    Timer {
        interval: 3500
        triggeredOnStart: true
        running: true
        repeat: true
        onTriggered: {
        //! [0]
            if (root.lastWasPulse) {
                burstEmitter.burst(500);
                root.lastWasPulse = false;
            } else {
                pulseEmitter.pulse(500);
                root.lastWasPulse = true;
            }
        //! [0]
        }
    }
    ParticleSystem {
        id: particles
        anchors.fill: parent
        ImageParticle {
            source: "qrc:///particleresources/star.png"
            alpha: 0
            colorVariation: 0.6
        }

        Emitter {
            id: burstEmitter
            x: parent.width/2
            y: parent.height/3
            emitRate: 1000
            lifeSpan: 2000
            enabled: false
            velocity: AngleDirection{magnitude: 64; angleVariation: 360}
            size: 24
            sizeVariation: 8
            Text {
                anchors.centerIn: parent
                color: "white"
                font.pixelSize: 18
                text: "Burst"
            }
        }
        Emitter {
            id: pulseEmitter
            x: parent.width/2
            y: 2*parent.height/3
            emitRate: 1000
            lifeSpan: 2000
            enabled: false
            velocity: AngleDirection{magnitude: 64; angleVariation: 360}
            size: 24
            sizeVariation: 8
            Text {
                anchors.centerIn: parent
                color: "white"
                font.pixelSize: 18
                text: "Pulse"
            }
        }
    }
}
