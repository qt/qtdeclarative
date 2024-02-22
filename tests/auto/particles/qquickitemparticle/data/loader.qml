// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle {
    color: "black"
    width: 320
    height: 320

    Component {
        id: component

        ParticleSystem {
            id: sys
            objectName: "system"
            anchors.fill: parent
            running: visible

            ItemParticle {
                delegate: Image { source: "../../shared/star.png" }
            }

            Emitter {
                //0,0 position
                size: 32
                emitRate: 10
                lifeSpan: 150000
            }
        }
    }

    Loader {
        id: loader
        objectName: "loader"
        sourceComponent: component
        anchors.fill: parent
    }
}
