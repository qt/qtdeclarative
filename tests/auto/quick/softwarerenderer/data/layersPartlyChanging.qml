// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// Layers whose content changes every frame, once, or never, and frames
// where nothing changes at all
Item {
    width: 201; height: 141
    property int frames: 24
    property int tolerance: 2

    Rectangle { anchors.fill: parent; color: "#e8e8f0" }
    Grid {
        x: 4.5; y: 6
        columns: 3
        spacing: 7
        Repeater {
            id: cells
            model: 6
            Item {
                required property int index
                property alias inner: inner
                width: 56; height: 54
                layer.enabled: true
                Rectangle { anchors.fill: parent; color: Qt.rgba(index / 6, 0.5, 0.8, 1) }
                Rectangle {
                    id: inner
                    x: 8; y: 8; width: 24; height: 18
                    color: "white"
                }
            }
        }
    }

    function step(f) {
        // Every fourth frame repeats the previous state
        const g = f % 4 === 3 ? f - 1 : f
        cells.itemAt(0).inner.x = 8 + (g % 10) * 2.5
        cells.itemAt(1).inner.rotation = g * 13
        cells.itemAt(2).inner.color = g >= 5 ? "black" : "white"
    }
}
