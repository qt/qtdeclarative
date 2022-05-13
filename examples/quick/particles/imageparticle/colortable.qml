// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick.Particles
import QtQuick

Rectangle {
    id: root
    width: 360
    height: 540
    color: "black"

    ParticleSystem { id: particles }

    ImageParticle {
        system: particles
        colorVariation: 0.5
        alpha: 0

        //! [0]
        source: "qrc:///particleresources/glowdot.png"
        colorTable: "images/colortable.png"
        sizeTable: "images/colortable.png"
        //! [0]
    }

    Emitter {
        system: particles
        emitRate: 500
        lifeSpan: 2000

        y: root.height / 2 + Math.sin(t * 2) * root.height * 0.3
        x: root.width / 2 + Math.cos(t) * root.width * 0.3
        property real t;

        NumberAnimation on t {
            from: 0; to: Math.PI * 2; duration: 10000; loops: Animation.Infinite
        }

        velocityFromMovement: 20

        velocity: PointDirection { xVariation: 5; yVariation: 5;}
        acceleration: PointDirection { xVariation: 5; yVariation: 5;}

        size: 16
        //endSize: 8
        //sizeVariation: 8
    }
}
