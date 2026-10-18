// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// Opaque layers starting at fractional device pixels, where the target
// rect of the layer is padded by a device pixel
Item {
    width: 181; height: 141
    property int frames: 6
    property int tolerance: 0

    Rectangle { anchors.fill: parent; color: "#e0e0f0" }
    Repeater {
        model: 8
        Item {
            id: cell
            required property int index
            x: 6 + (index % 4) * 43.3
            y: 7 + Math.floor(index / 4) * 61 + (index % 4) * 0.35
            width: 36; height: 54
            layer.enabled: true
            Rectangle { anchors.fill: parent; color: Qt.rgba(cell.index / 8, 0.4, 0.7, 1) }
        }
    }
    Rectangle { id: dot; x: 2; y: 130; width: 6; height: 6; color: "red" }

    function step(f) {
        dot.x = 2 + f * 20
    }
}
