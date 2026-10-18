// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-132192
Item {
    width: 161; height: 121
    property int frames: 30
    property int tolerance: 0

    Rectangle { anchors.fill: parent; color: "white" }
    Rectangle { id: mover; y: 50; width: 20; height: 20 }
    Rectangle { x: 60; y: 30; width: 40; height: 60; color: "#303030" }

    function step(f) {
        mover.x = 10 + f * 4.3
        mover.color = f >= 12 && f < 18 ? "#e08020" : "#2080e0"
    }
}
