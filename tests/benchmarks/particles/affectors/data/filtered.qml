// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        running: false //Benchmark will manage it

        ImageParticle {
            groups: ["A"]
            source: "../../shared/star.png"
        }

        Emitter{
            //central position
            id: emitter
            x: 160
            y: 160
            group: "A"
            size: 32
            emitRate: 1000
            lifeSpan: Emitter.InfiniteLife
            maximumEmitted: 1000
            enabled: false
            Component.onCompleted: emitter.burst(1000);
        }

        Affector{
            groups: ["A"]
            anchors.fill: parent
            //whenCollidingWith: ["A"] //Test separately?
        }
    }
}
