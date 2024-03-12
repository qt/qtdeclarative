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
            groups: ["","notdefault"]
            source: "../../shared/star.png"
        }


        TrailEmitter {
            group: "notdefault"
            follow: ""
            size: 32
            emitRatePerParticle: 2
            lifeSpan: 500
            velocity: PointDirection{ x: 500; y: 500 }
        }
        Emitter{
            x: 4
            y: 4
            size: 32
            emitRate: 1000
            lifeSpan: 500
        }
    }
}
