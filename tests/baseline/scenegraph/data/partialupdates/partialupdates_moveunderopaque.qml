// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-132192
Item {
    id: root
    width: 320; height: 240
    property bool suspendGrabbing: stepper.running
    Stepper { id: stepper; scene: root; frames: 50 }

    Rectangle { anchors.fill: parent; color: "white" }
    Rectangle { id: mover; y: 100; width: 40; height: 40 }
    Rectangle { x: 120; y: 60; width: 80; height: 120; color: "#303030" }

    function step(f) {
        mover.x = 10 + f * 5.3
        mover.color = f >= 18 && f < 30 ? "#e08020" : "#2080e0"
    }
}
