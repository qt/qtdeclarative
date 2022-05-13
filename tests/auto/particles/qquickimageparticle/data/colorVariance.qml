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

        ImageParticle {
            source: "../../shared/star.png"
            alpha: 0.5
            color: "#000000"
            redVariation: 0.5
            greenVariation: 0.25
            blueVariation: 0.125
            alphaVariation: 0.25
        }

        Emitter{
            //0,0 position
            size: 32
            emitRate: 1000
            lifeSpan: 500
        }
    }
}
