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
            groups: ["", "notdefault"]
            source: "../../shared/star.png"
        }

        Emitter{
            width: 100
            height: 100
            size: 32
            shape: RectangleShape{}
            emitRate: 1000
            lifeSpan: 500
        }
        Emitter{
            width: 100
            height: 100
            group: "notdefault"
            size: 32
            shape: RectangleShape{fill: false}
            emitRate: 1000
            lifeSpan: 500
        }
    }
}
