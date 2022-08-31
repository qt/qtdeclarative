// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    color: "goldenrod"
    width: 400
    height: 400
    ParticleSystem {
        width: 300
        height: 300
        anchors.centerIn: parent

        ImageParticle {
            source: "qrc:///particleresources/glowdot.png"
            z: 2
            anchors.fill: parent
            color: "#336666CC"
            colorVariation: 0.0
        }

        Emitter {
            anchors.fill: parent
            emitRate: 6000
            lifeSpan: 720
            size: 10
            //! [0]
            shape: MaskShape {
                source: "images/starfish_mask.png"
            }
            //! [0]
        }

    }
}
