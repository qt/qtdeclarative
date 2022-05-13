// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    color: "goldenrod"
    width: 400
    height: 400
    ParticleSystem {id:sys}

    //! [spin]
    ImageParticle {
        system: sys
        groups: ["goingLeft", "goingRight"]
        source: "images/starfish_4.png"
        rotation: 90
        rotationVelocity: 90
        autoRotation: true
    }
    //! [spin]
    //! [deform]
    ImageParticle {
        system: sys
        groups: ["goingDown"]
        source: "images/starfish_0.png"
        rotation: 180
        yVector: PointDirection { y: 0.5; yVariation: 0.25; xVariation: 0.25; }
    }
    //! [deform]

    Timer {
        running: true
        repeat: false
        interval: 100
        onTriggered: emitA.enabled = true;
    }
    Timer {
        running: true
        repeat: false
        interval: 4200
        onTriggered: emitB.enabled = true;
    }
    Timer {
        running: true
        repeat: false
        interval: 8400
        onTriggered: emitC.enabled = true;
    }

    Emitter {
        id: emitA
        x: 0
        y: 120
        system: sys
        enabled: false
        group: "goingRight"
        velocity: PointDirection { x: 100 }
        lifeSpan: 4000
        emitRate: 1
        size: 128
    }
    Emitter {
        id: emitB
        x: 400
        y: 240
        system: sys
        enabled: false
        group: "goingLeft"
        velocity: PointDirection { x: -100 }
        lifeSpan: 4000
        emitRate: 1
        size: 128
    }
    Emitter {
        id: emitC
        x: 0
        y: 360
        system: sys
        enabled: false
        group: "goingDown"
        velocity: PointDirection { x: 100 }
        lifeSpan: 4000
        emitRate: 1
        size: 128
    }
}
