// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Item {
    id: window
    width: 320; height: 480
    Rectangle {
        id: sky
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "DeepSkyBlue"
            }
            GradientStop {
                position: 1.0
                color: "SkyBlue"
            }
        }
    }

    Rectangle {
        id: ground
        width: parent.height * 2
        height: parent.height
        y: parent.height/2
        x: parent.width/2 - parent.height
        transformOrigin: Item.Top
        rotation: 0
        gradient: Gradient {
            GradientStop { position: 0.0; color: "ForestGreen"; }
            GradientStop { position: 1.0; color: "DarkGreen"; }
        }
    }

    MouseArea {
        anchors.fill: parent
        onPositionChanged: {
            var rot = Math.atan2(mouseY - window.height/2,mouseX - window.width/2) * 180/Math.PI;
            ground.rotation = rot;
        }
    }

    ParticleSystem { id: sys }
    //! [0]
    Gravity {
        system: sys
        magnitude: 32
        angle: ground.rotation + 90
    }
    //! [0]
    Emitter {
        system: sys
        anchors.centerIn: parent
        emitRate: 1
        lifeSpan: 10000
        size: 64
    }
    ImageParticle {
        anchors.fill: parent
        system: sys
        source: "images/realLeaf1.png"
    }

}
