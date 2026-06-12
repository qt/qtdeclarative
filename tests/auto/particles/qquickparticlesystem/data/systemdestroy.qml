// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Particles

Rectangle {
    id: root
    width: 320
    height: 320

    property var sys: systemA

    ParticleSystem {
        id: systemA
        objectName: "systemA"
        anchors.fill: parent
    }

    ImageParticle {
        objectName: "painter"
        source: "../../shared/star.png"
        system: root.sys
    }

    Emitter {
        objectName: "emitter"
        system: root.sys
        size: 32
        emitRate: 1000
        lifeSpan: 500
    }

    Age {
        objectName: "affector"
        system: root.sys
        lifeLeft: 100
    }
}
