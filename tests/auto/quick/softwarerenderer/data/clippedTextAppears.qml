// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-151166
Item {
    width: 161; height: 121
    property int frames: 10
    property int tolerance: 2

    Rectangle { anchors.fill: parent; color: "white" }
    Text { id: t; x: -300; y: 40; width: 120; height: 30; clip: true; font.pixelSize: 20 }

    function step(f) {
        t.text = f >= 3 ? "Hello clip" : ""
        t.x = f >= 3 ? 20 : -300
    }
}
