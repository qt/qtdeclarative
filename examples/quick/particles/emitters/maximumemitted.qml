// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    color: "black"
    width: 360
    height: 540
    ParticleSystem {
        id: sys
        anchors.fill: parent
        onEmptyChanged: if (empty) sys.pause();

        ImageParticle {
            system: sys
            id: cp
            source: "qrc:///particleresources/glowdot.png"
            colorVariation: 0.4
            color: "#000000FF"
        }

        Emitter {
            //burst on click
            id: bursty
            system: sys
            enabled: ma.pressed
            x: ma.mouseX
            y: ma.mouseY
            emitRate: 16000
            maximumEmitted: 4000
            acceleration: AngleDirection {angleVariation: 360; magnitude: 360; }
            size: 8
            endSize: 16
            sizeVariation: 4
        }

        MouseArea {
            anchors.fill: parent
            onPressed: sys.resume()
            id: ma
        }
    }
}
