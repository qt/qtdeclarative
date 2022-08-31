// Copyright (C) 2017 The Qt Company Ltd.
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
            groups: ["A"]
            anchors.fill: parent
            source: "qrc:///particleresources/star.png"
            color:"#FF1010"
            redVariation: 0.8
        }

        Emitter {
            group: "A"
            emitRate: 100
            lifeSpan: 2800
            size: 32
            sizeVariation: 8
            velocity: PointDirection{ x: 66; xVariation: 20 }
            width: 80
            height: 80
        }

        //! [A]
        Affector {
            groups: ["A"]
            x: 120
            width: 80
            height: 80
            once: true
            position: PointDirection { x: 120; }
        }
        //! [A]

        ImageParticle {
            groups: ["B"]
            anchors.fill: parent
            source: "qrc:///particleresources/star.png"
            color:"#10FF10"
            greenVariation: 0.8
        }

        Emitter {
            group: "B"
            emitRate: 100
            lifeSpan: 2800
            size: 32
            sizeVariation: 8
            velocity: PointDirection{ x: 240; xVariation: 60 }
            y: 260
            width: 10
            height: 10
        }

        //! [B]
        Affector {
            groups: ["B"]
            x: 120
            y: 240
            width: 80
            height: 80
            once: true
            velocity: AngleDirection { angleVariation:360; magnitude: 72 }
        }
        //! [B]

        ImageParticle {
            groups: ["C"]
            anchors.fill: parent
            source: "qrc:///particleresources/star.png"
            color:"#1010FF"
            blueVariation: 0.8
        }

        Emitter {
            group: "C"
            y: 400
            emitRate: 100
            lifeSpan: 2800
            size: 32
            sizeVariation: 8
            velocity: PointDirection{ x: 80; xVariation: 10 }
            acceleration: PointDirection { y: 10; x: 20; }
            width: 80
            height: 80
        }

        //! [C]
        Affector {
            groups: ["C"]
            x: 120
            y: 400
            width: 80
            height: 120
            once: true
            relative: false
            acceleration: PointDirection { y: -80; }
        }
        //! [C]

    }
}
