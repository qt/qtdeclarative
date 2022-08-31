// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

Rectangle {
    width: 360
    height: 480
    color: "black"

//! [0]
    MultiPointTouchArea {
        anchors.fill: parent
        minimumTouchPoints: 1
        maximumTouchPoints: 5
        touchPoints: [
            TouchPoint { id: touch1 },
            TouchPoint { id: touch2 },
            TouchPoint { id: touch11 },
            TouchPoint { id: touch21 },
            TouchPoint { id: touch31 }
        ]
    }
//! [0]

//! [1]
    ParticleFlame {
        color: "red"
        emitterX: touch1.x
        emitterY: touch1.y
        emitting: touch1.pressed
    }
//! [1]
    ParticleFlame {
        color: "green"
        emitterX: touch2.x
        emitterY: touch2.y
        emitting: touch2.pressed
    }
    ParticleFlame {
        color: "yellow"
        emitterX: touch11.x
        emitterY: touch11.y
        emitting: touch11.pressed
    }
    ParticleFlame {
        color: "blue"
        emitterX: touch21.x
        emitterY: touch21.y
        emitting: touch21.pressed
    }
    ParticleFlame {
        color: "violet"
        emitterX: touch31.x
        emitterY: touch31.y
        emitting: touch31.pressed
    }
}
