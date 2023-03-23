// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    width: 360
    height: 540
    ParticleSystem { id: particles }
    ImageParticle {
        system: particles
        sprites: Sprite {
            name: "snow"
            source: "images/snowflake.png"
            frameCount: 51
            frameDuration: 40
            frameDurationVariation: 8
        }
    }

    //! [0]
    Wander {
        id: wanderer
        system: particles
        anchors.fill: parent
        xVariance: 360/(wanderer.affectedParameter+1);
        pace: 100*(wanderer.affectedParameter+1);
    }
    //! [0]

    Emitter {
        system: particles
        emitRate: 20
        lifeSpan: 7000
        velocity: PointDirection { y:80; yVariation: 40; }
        acceleration: PointDirection { y: 4 }
        size: 20
        sizeVariation: 10
        width: parent.width
        height: 100
    }
    Row {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 4
        GreyButton {
            text: qsTr("dx/dt")
            onClicked: wanderer.affectedParameter = Wander.Position;
        }
        GreyButton {
            text: qsTr("dv/dt")
            onClicked: wanderer.affectedParameter = Wander.Velocity;
        }
        GreyButton {
            text: qsTr("da/dt")
            onClicked: wanderer.affectedParameter = Wander.Acceleration;
        }
    }
}
