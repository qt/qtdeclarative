// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    color: "goldenrod"
    width: 2000
    height: 2000
    ParticleSystem {id: sys}
    ImageParticle {
        id: up
        system: sys
        source: "images/starfish_2.png"
        autoRotation: true //leaving these two settings at default allows you to test going up performance levels
        rotation: -90
    }

    Emitter {
        anchors.centerIn: parent
        system: sys
        emitRate: 10
        size: 200
        lifeSpan: 10000
        velocity: AngleDirection {angleVariation: 360; magnitudeVariation: 100;}
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            up.autoRotation = !up.autoRotation
            up.rotation = up.autoRotation ? -90 : 0
        }
    }
}
