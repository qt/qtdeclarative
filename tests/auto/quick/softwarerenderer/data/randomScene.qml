// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// Deterministic for a given seed, item index and frame
Item {
    id: root
    width: 161; height: 121
    property int frames: 30
    property int tolerance: 2
    property int seed: 1

    Rectangle { anchors.fill: parent; color: "white" }

    Repeater {
        id: rep
        model: 24
        Item {
            id: cell
            required property int index
            property int kind: index % 8
            width: 20; height: 16
            Rectangle {
                anchors.fill: parent
                visible: cell.kind <= 3
                radius: cell.kind === 2 ? 5 : 0
                border.width: cell.kind === 3 ? 1 : 0
                color: cell.kind === 1 ? Qt.rgba(0.2, 0.6, 0.2, 0.5) : "#3060c0"
                rotation: cell.kind === 0 && cell.index % 3 === 0 ? 17 : 0
            }
            Image { visible: cell.kind === 4; source: "checker.png"; smooth: cell.index % 2 }
            Text { visible: cell.kind === 5; text: "Ab"; font.pixelSize: 13 }
            Item {
                visible: cell.kind === 6
                anchors.fill: parent; clip: true
                Rectangle { x: -6; y: -4; width: 26; height: 20; color: "#c08020" }
            }
            Rectangle {
                visible: cell.kind === 7
                anchors.fill: parent; color: "#a03070"; opacity: 0.7
            }
        }
    }

    function rnd(i, k) {
        let h = (seed * 73856093) ^ (i * 19349663) ^ (k * 83492791)
        h = Math.imul(h ^ (h >>> 13), 0x5bd1e995)
        return ((h ^ (h >>> 15)) >>> 0) / 4294967296
    }

    function step(f) {
        for (let i = 0; i < rep.count; ++i) {
            const it = rep.itemAt(i)
            const vx = (rnd(i, 1) - 0.5) * 6, vy = (rnd(i, 2) - 0.5) * 6
            it.x = rnd(i, 3) * 141 + vx * f
            it.y = rnd(i, 4) * 105 + vy * f
            it.z = Math.floor(rnd(i, 5 + Math.floor(f / 7)) * 4)
            it.visible = rnd(i, 100 + f) > 0.1
            it.width = 12 + rnd(i, 6) * 20 + (rnd(i, 7) > 0.7 ? f * 0.3 : 0)
            it.height = 10 + rnd(i, 8) * 16
        }
    }
}
