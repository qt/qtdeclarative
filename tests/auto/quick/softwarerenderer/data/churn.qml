// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// Complex damage behind translucent and opaque items
Item {
    width: 241; height: 181
    property int frames: 24
    property int tolerance: 2

    Rectangle { anchors.fill: parent; color: "#f0f0e0" }
    Grid {
        x: 3.3; y: 4.6
        columns: 12
        spacing: 3
        Repeater {
            id: cells
            model: 96
            Rectangle {
                required property int index
                width: 12; height: 9
                color: Qt.rgba((index % 5) / 5, 0.4, 1 - (index % 7) / 7, index % 3 ? 1 : 0.6)
            }
        }
    }
    Rectangle { id: veil; x: 20.5; y: 30.25; width: 90; height: 50; color: "#6000a000" }
    Rectangle { id: block; x: 120.4; y: 60.7; width: 40; height: 30; color: "#303030" }
    Rectangle { id: dot; x: 220; y: 160; width: 6; height: 6; color: "red" }

    function step(f) {
        for (let i = 0; i < cells.count; ++i)
            cells.itemAt(i).rotation = (i * 37 + f * (5 + i % 4)) % 360
        veil.x = 20.5 + f * 1.7
        block.y = 60.7 - f * 1.3
        dot.visible = f % 2 === 0
    }
}
