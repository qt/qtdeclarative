// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-95632
Item {
    width: 161; height: 121
    property int frames: 16
    property int tolerance: 0

    Rectangle { id: bg; anchors.fill: parent }
    Repeater {
        model: 8
        Rectangle {
            required property int index
            x: 8.5 + index * 18.3; y: 5.25; width: 1; height: 110; color: "black"
        }
    }
    Repeater {
        model: 3
        Rectangle {
            required property int index
            x: 15.4 + index * 45.7; y: 70.6; width: 30.3; height: 30.3
            color: "transparent"; border.width: 1; border.color: "black"
        }
    }
    Rectangle { id: sweep; y: 40; width: 25; height: 18; color: "#40c040" }

    function step(f) {
        bg.color = Math.floor(f / 2) % 2 ? "#f0f0d0" : "#d0e0f0"
        sweep.x = f * 9.7
    }
}
