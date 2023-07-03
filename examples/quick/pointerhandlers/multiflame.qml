// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    id: root
    width: 360
    height: 480
    color: "black"

    component ColoredEmitter: Emitter {
        id: emitter
        property string color
        group: color
        velocityFromMovement: 10
        emitRate: 80
        lifeSpan: 1500
        velocity: PointDirection {
            y: -90
            yVariation: 50
        }
        acceleration: PointDirection {
            xVariation: 100
            yVariation: 90
        }
        size: 51
        sizeVariation: 53
        endSize: 64
        enabled: handler.active
        x: handler.point.position.x
        y: handler.point.position.y

        PointHandler {
            id: handler
            parent: root
        }

        ImageParticle {
            id: img
            groups: [emitter.color]
            source: "images/blur-circle.png"
            colorVariation: 0.1
            color: emitter.color
            alpha: 0
            system: sys
        }
    }

    ParticleSystem {
        id: sys
        ColoredEmitter {
            color: "indianred"
        }
        ColoredEmitter {
            color: "greenyellow"
        }
        ColoredEmitter {
            color: "yellow"
        }
        ColoredEmitter {
            color: "darkorange"
        }
        ColoredEmitter {
            color: "violet"
        }
    }
}
