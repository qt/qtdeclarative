// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-136738, QTBUG-133314
Item {
    width: 161; height: 121
    property int frames: 20
    property int tolerance: 2

    Rectangle { anchors.fill: parent; color: "#e0e0e0" }
    Rectangle { x: 30; y: 20; width: 100; height: 80; color: "#2050a0" }
    Rectangle { id: a; width: 40; height: 30; radius: 8; color: "#e04040" }
    Rectangle {
        id: b
        width: 44; height: 34; topLeftRadius: 12
        color: "#40e040"; border.width: 2; border.color: "black"
    }
    Rectangle {
        id: c
        width: 30; height: 30; radius: 15
        color: "#f0c000"; border.width: 3; border.color: "#404040"
    }

    function step(f) {
        a.x = 5 + f * 3.3; a.y = 10 + f * 1.7
        b.x = 110 - f * 2.9; b.y = 15 + f * 2.3
        c.x = 60 + f * 0.7; c.y = 80 - f * 2.1
        a.color = Math.floor(f / 4) % 2 ? "#e04040" : "#a020a0"
    }
}
