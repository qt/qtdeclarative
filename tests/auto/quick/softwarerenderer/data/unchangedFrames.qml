// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// Frames in which nothing changes, or properties are set to their
// current values, between frames with changes
Item {
    width: 161; height: 121
    property int frames: 20
    property int tolerance: 2

    Rectangle { anchors.fill: parent; color: "#f8f8f0" }
    Rectangle { id: mover; y: 20; width: 30; height: 24; color: "#2070c0" }
    Rectangle { id: blinker; x: 100; y: 70; width: 40; height: 30; radius: 6 }
    Text { id: label; x: 10; y: 80; text: "0" }

    function step(f) {
        mover.x = 10 + Math.min(Math.max(f - 3, 0), 4) * 9.5
        blinker.color = f >= 9 && f < 12 ? "#c03030" : "#30a030"
        label.text = f >= 15 ? "15" : "0"
    }
}
