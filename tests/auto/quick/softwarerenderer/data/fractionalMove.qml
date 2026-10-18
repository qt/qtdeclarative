// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-133368, QTBUG-113745
Item {
    width: 161; height: 121
    property int frames: 24
    property int tolerance: 0

    Rectangle { anchors.fill: parent; color: "white" }
    Rectangle { x: 40; y: 30; width: 80; height: 60; color: "#40a040" }

    Repeater {
        id: rects
        model: 6
        Rectangle { width: 17; height: 13 }
    }
    Image { id: img; source: "checker.png"; smooth: false }

    function step(f) {
        for (let i = 0; i < rects.count; ++i) {
            const r = rects.itemAt(i) as Rectangle
            r.x = 5 + i * 24 + f * 0.37
            r.y = 10 + (i % 3) * 35 + f * 0.61
            r.color = (Math.floor(f / 3) + i) % 2 ? "#202080" : "#c03030"
        }
        img.x = 120 - f * 1.3
        img.y = 80 - f * 0.45
    }
}
