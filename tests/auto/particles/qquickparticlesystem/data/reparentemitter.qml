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

        ImageParticle {
            source: "../../shared/star.png"
        }
    }

    ParticleSystem {
        id: systemB
        objectName: "systemB"
        anchors.fill: parent

        ImageParticle {
            source: "../../shared/star.png"
        }
    }

    Item {
        id: containerA
        objectName: "containerA"
        parent: systemA

        Emitter {
            id: emitter
            objectName: "emitter"
            size: 32
            emitRate: 1000
            lifeSpan: 500
        }
    }

    Item {
        id: containerB
        objectName: "containerB"
        parent: systemB
    }
}
