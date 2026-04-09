// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle {
    color: "black"
    width: 320
    height: 320

    // Parent item with enabled: false (disables user interaction,
    // but should NOT prevent particle animation)
    Item {
        enabled: false
        anchors.fill: parent

        ParticleSystem {
            id: sys
            objectName: "system"
            anchors.fill: parent

            ItemParticle {
                delegate: Image { source: "../../shared/star.png" }
            }

            Emitter {
                //0,0 position
                size: 32
                emitRate: 1000
                lifeSpan: 500
            }
        }
    }
}
