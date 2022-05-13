// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Particles

Rectangle {
    id: root
    width: 360
    height: 600
    color: "darkblue"
    property bool cloneMode: false
    ParticleSystem {
        id: sys
    }
    MouseArea {
        anchors.fill: parent
        onClicked: cloneMode = !cloneMode;
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Click to Toggle"
        color: "white"
        font.pixelSize: 24
    }
    Emitter {
        system: sys
        y:root.height + 20
        width: root.width
        emitRate: 200
        lifeSpan: 4000
        startTime: 4000
        velocity: PointDirection { y: -120; }
    }

    ImageParticle {
        system: sys
        visible: !cloneMode
        source: "images/particle2.png"
    }

    ImageParticle {
        system: sys
        visible: cloneMode
        z: 0
        source: "images/particle3.png"
    }

    ImageParticle {
        system: sys
        clip: true
        visible: cloneMode
        y: 120
        height: 240
        width: root.width
        z: 1
        source: "qrc:///particleresources/glowdot.png"
    }
}
