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
            sprites: Sprite {
                name: "happy"
                source: "../../shared/squarefacesprite.png"
                frames: 6
                frameDuration: 120
            }
        }

        Emitter{
            //0,0 position
            size: 32
            emitRate: 1000
            lifeSpan: 500
        }
    }
}
