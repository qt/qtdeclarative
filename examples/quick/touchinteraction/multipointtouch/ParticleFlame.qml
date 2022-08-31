// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

ParticleSystem {
    anchors.fill: parent

    property alias emitterX: emitter.x
    property alias emitterY: emitter.y

    property alias color: img.color
    property alias emitting: emitter.enabled
    ImageParticle {
        id: img
        source: "blur-circle.png"
        colorVariation: 0.1
        color: "#ff521d"
        alpha: 0
    }
    Emitter {
        id: emitter
        velocityFromMovement: 10
        emitRate: 80
        lifeSpan: 1500
        velocity: PointDirection{ y: -90; yVariation: 50; }
        acceleration: PointDirection{ xVariation: 100; yVariation: 90; }
        size: 51
        sizeVariation: 53
        endSize: 64
    }
}
