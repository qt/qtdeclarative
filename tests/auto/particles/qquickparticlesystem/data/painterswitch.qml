// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Particles

Rectangle {
    id: root
    width: 320
    height: 320

    ParticleSystem {
        id: systemA
        objectName: "systemA"
        anchors.fill: parent

        Emitter {
            size: 32
            emitRate: 1000
            lifeSpan: 500
        }

        ImageParticle {
            id: painter
            objectName: "painter"
            source: "../../shared/star.png"
        }
    }

    ParticleSystem {
        id: systemB
        objectName: "systemB"
        anchors.fill: parent

        Emitter {
            size: 32
            emitRate: 1000
            lifeSpan: 500
        }
    }
}
