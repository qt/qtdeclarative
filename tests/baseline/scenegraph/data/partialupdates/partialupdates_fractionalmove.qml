// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-133368, QTBUG-113745
Item {
    id: root
    width: 320; height: 240
    property bool suspendGrabbing: stepper.running
    Stepper { id: stepper; scene: root; frames: 40 }

    Rectangle { anchors.fill: parent; color: "white" }
    Rectangle { x: 80; y: 60; width: 160; height: 120; color: "#40a040" }
    Repeater {
        id: rects
        model: 8
        Rectangle { width: 31; height: 23 }
    }
    Image { id: img; source: "../shared/col320x480.jpg"; width: 40; height: 40; smooth: false }

    function step(f) {
        for (let i = 0; i < rects.count; ++i) {
            const r = rects.itemAt(i) as Rectangle
            r.x = 8 + i * 38 + f * 0.37
            r.y = 15 + (i % 4) * 52 + f * 0.61
            r.color = (Math.floor(f / 3) + i) % 2 ? "#202080" : "#c03030"
        }
        img.x = 250 - f * 1.3
        img.y = 170 - f * 0.45
    }
}
