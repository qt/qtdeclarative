// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle {
    color: "black"
    width: 320
    height: 320

    ParticleSystem {
        id: sys
        objectName: "system"
        anchors.fill: parent

        ImageParticle {
            source: "../../shared/star.png"
        }

        Emitter{
            //0,0 position
            size: 32
            emitRate: 1000
            lifeSpan: 500
            velocity: CumulativeDirection {
                PointDirection { x: 100; y: -100 }
                PointDirection { x: -100; y: 100 }
            }
            acceleration: CumulativeDirection {
                AngleDirection { angle: 0; magnitude: 100 }
                AngleDirection { angle: 180; magnitude: 100 }
            }
        }
    }
}
