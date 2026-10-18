// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// Items changing stacking order, parent, visibility and count
Item {
    id: root
    width: 181; height: 131
    property int frames: 24
    property int tolerance: 0

    Rectangle { anchors.fill: parent; color: "white" }
    Item { id: left; x: 5; y: 5; width: 80; height: 120 }
    Item { id: right; x: 95; y: 5; width: 80; height: 120 }
    Rectangle { id: a; parent: left; x: 10; y: 10; width: 50; height: 40; color: "#c03030" }
    Rectangle { id: b; parent: left; x: 25; y: 30; width: 50; height: 40; color: "#3030c0" }
    Rectangle { id: c; parent: right; x: 5; y: 60; width: 60; height: 30; color: "#30a030" }
    Column {
        x: 100; y: 10
        spacing: 2
        Repeater {
            id: rows
            model: 3
            Rectangle { width: 70; height: 8; color: "#806020" }
        }
    }

    function step(f) {
        a.z = f % 3 === 0 ? 1 : 0
        b.visible = f % 5 !== 2
        c.parent = Math.floor((f + 5) / 6) % 2 ? right : left
        rows.model = 1 + f % 4
    }
}
