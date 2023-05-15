// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    width: 360
    height: 540
    color: "black"
    Text {
        text: qsTr("Left click to start/stop\nRight click to pause/unpause")
        color: "white"
        font.pixelSize: 24
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onTapped: function (event, mouseButton)
        {
            if (mouseButton === Qt.LeftButton)
                particles.running = !particles.running
            else
                particles.paused = !particles.paused;
        }
    }

    ParticleSystem {
        id: particles
        running: false
    }

    ImageParticle {
        anchors.fill: parent
        system: particles
        source: "qrc:///particleresources/star.png"
        sizeTable: "images/sparkleSize.png"
        alpha: 0
        colorVariation: 0.6
    }

    Emitter {
        anchors.fill: parent
        system: particles
        emitRate: 2000
        lifeSpan: 2000
        size: 30
        sizeVariation: 10
    }
}
