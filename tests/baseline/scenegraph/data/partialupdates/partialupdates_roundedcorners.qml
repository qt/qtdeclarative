// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-136738, QTBUG-133314
Item {
    id: root
    width: 320; height: 240
    property bool suspendGrabbing: stepper.running
    Stepper { id: stepper; scene: root; frames: 40 }

    Rectangle { anchors.fill: parent; color: "#e0e0e0" }
    Rectangle { x: 60; y: 40; width: 200; height: 160; color: "#2050a0" }
    Rectangle { id: a; width: 60; height: 45; radius: 10; color: "#e04040" }
    Rectangle {
        id: b
        width: 66; height: 50; topLeftRadius: 16
        color: "#40e040"; border.width: 2; border.color: "black"
    }
    Rectangle {
        id: c
        width: 44; height: 44; radius: 22
        color: "#f0c000"; border.width: 3; border.color: "#404040"
    }

    function step(f) {
        a.x = 5 + f * 3.3; a.y = 10 + f * 1.7
        b.x = 240 - f * 2.9; b.y = 15 + f * 2.3
        c.x = 120 + f * 0.7; c.y = 180 - f * 2.1
        a.color = Math.floor(f / 4) % 2 ? "#e04040" : "#a020a0"
    }
}
