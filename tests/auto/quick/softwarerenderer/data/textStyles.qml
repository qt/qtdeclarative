// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-137404. Glyphs stay below 64 device pixels, larger ones are
// antialiased paths, which are not clip invariant (QTBUG-149304).
Item {
    width: 161; height: 121
    property int frames: 20
    property int tolerance: 2

    Rectangle { anchors.fill: parent; color: "white" }
    Item {
        x: 10; y: 10; width: 140; height: 100
        clip: true
        Text { id: t1; text: "Outline"; font.pixelSize: 18; style: Text.Outline; styleColor: "red" }
        Text {
            id: t2
            y: 50
            text: "Raised"; font.pixelSize: 18; style: Text.Raised; styleColor: "gray"
        }
    }

    function step(f) {
        t1.x = -10 + f * 2.35
        t2.x = 90 - f * 3.1
    }
}
