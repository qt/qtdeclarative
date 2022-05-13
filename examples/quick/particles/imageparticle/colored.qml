// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    width: 360
    height: 540
    color: "black"
    ParticleSystem {
        anchors.fill: parent
        ImageParticle {
            groups: ["stars"]
            anchors.fill: parent
            source: "qrc:///particleresources/star.png"
        }
        Emitter {
            group: "stars"
            emitRate: 800
            lifeSpan: 2400
            size: 24
            sizeVariation: 8
            anchors.fill: parent
        }

        // ![0]
        ImageParticle {
            anchors.fill: parent
            source: "qrc:///particleresources/star.png"
            alpha: 0
            alphaVariation: 0.2
            colorVariation: 1.0
        }
        // ![0]

        Emitter {
            anchors.centerIn: parent
            emitRate: 400
            lifeSpan: 2400
            size: 48
            sizeVariation: 8
            velocity: AngleDirection {angleVariation: 180; magnitude: 60}
        }

        Turbulence {
            anchors.fill: parent
            strength: 2
        }
    }
}
