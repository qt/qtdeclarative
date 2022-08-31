// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Particles

//! [0]
TouchPoint {
    id: container
    property ParticleSystem system
    onPressedChanged: {
        if (pressed) {
            timer.restart();
            child.enabled = true;
            system.explode(x,y);
        }
    }
    property QtObject obj: Timer {
        id: timer
        interval: 100
        running: false
        repeat: false
        onTriggered: container.child.enabled = false
    }
    property Item child: SpriteGoal {
        enabled: false
        x: container.area.x - 16
        y: container.area.y - 16
        width: container.area.width + 32
        height: container.area.height + 32 //+32 so it doesn't have to hit the exact center
        system: container.system
        parent: container.system
        goalState: "falling"
    }
}
//! [0]
