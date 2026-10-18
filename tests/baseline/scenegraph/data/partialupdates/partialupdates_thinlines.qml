// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-95632
Item {
    id: root
    width: 320; height: 240
    property bool suspendGrabbing: stepper.running
    Stepper { id: stepper; scene: root; frames: 31 }

    Rectangle { id: bg; anchors.fill: parent }
    Repeater {
        model: 16
        Rectangle {
            required property int index
            x: 8.5 + index * 19.3; y: 5.25; width: 1; height: 230; color: "black"
        }
    }
    Repeater {
        model: 5
        Rectangle {
            required property int index
            x: 15.4 + index * 60.7; y: 150.6; width: 40.3; height: 40.3
            color: "transparent"; border.width: 1; border.color: "black"
        }
    }
    Rectangle { id: sweep; y: 80; width: 35; height: 28; color: "#40c040" }

    function step(f) {
        bg.color = Math.floor(f / 2) % 2 ? "#f0f0d0" : "#d0e0f0"
        sweep.x = f * 9.7
    }
}
