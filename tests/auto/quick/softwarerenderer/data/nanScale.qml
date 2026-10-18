// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-117522
Item {
    width: 161; height: 121
    property int frames: 6
    property int tolerance: 0

    Rectangle { anchors.fill: parent; color: "white" }
    Rectangle { id: r; x: 40; y: 30; width: 50; height: 40; color: "#c04040" }

    function step(f) {
        r.scale = f >= 2 && f < 4 ? NaN : 1
    }
}
