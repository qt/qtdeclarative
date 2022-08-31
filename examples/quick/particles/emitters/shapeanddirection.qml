// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    id: root
    width: 360
    height: 540
    color: "black"
    Image {
        anchors.fill: parent
        source: "images/portal_bg.png"
    }

    ParticleSystem {
        id: particles
        anchors.fill: parent

        ImageParticle {
            groups: ["center","edge"]
            anchors.fill: parent
            source: "qrc:///particleresources/glowdot.png"
            colorVariation: 0.1
            color: "#009999FF"
        }

        Emitter {
            anchors.fill: parent
            group: "center"
            emitRate: 400
            lifeSpan: 2000
            size: 20
            sizeVariation: 2
            endSize: 0
            //! [0]
            shape: EllipseShape {fill: false}
            velocity: TargetDirection {
                targetX: root.width/2
                targetY: root.height/2
                proportionalMagnitude: true
                magnitude: 0.5
            }
            //! [0]
        }

        Emitter {
            anchors.fill: parent
            group: "edge"
            startTime: 2000
            emitRate: 2000
            lifeSpan: 2000
            size: 28
            sizeVariation: 2
            endSize: 16
            shape: EllipseShape {fill: false}
            velocity: TargetDirection {
                targetX: root.width/2
                targetY: root.height/2
                proportionalMagnitude: true
                magnitude: 0.1
                magnitudeVariation: 0.1
            }
            acceleration: TargetDirection {
                targetX: root.width/2
                targetY: root.height/2
                targetVariation: 200
                proportionalMagnitude: true
                magnitude: 0.1
                magnitudeVariation: 0.1
            }
        }
    }
}
