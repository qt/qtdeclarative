// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-114984
Item {
    width: 161; height: 121
    property int frames: 15
    property int tolerance: 0

    Rectangle { anchors.fill: parent; color: "white" }
    Item {
        id: layered
        x: 30; y: 20; width: 100; height: 80
        layer.enabled: true
        Rectangle { id: inner; anchors.fill: parent; anchors.margins: 10 }
    }

    function step(f) {
        layered.visible = f < 5 || f >= 10
        inner.color = f < 7 ? "#c02020" : "#2020c0"
    }
}
