// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-95632, the reported scene
Item {
    width: 161; height: 121
    property int frames: 12
    property int tolerance: 0

    Rectangle { anchors.fill: parent; color: "white" }
    Rectangle {
        id: main
        x: 5; y: 5; width: 150; height: 110
        color: "green"
        border.width: 3
        border.color: "cyan"

        Rectangle {
            id: line
            anchors.left: parent.left
            anchors.right: parent.right
            y: 10.4159
            height: 1
            color: "red"
        }
        Rectangle {
            id: square
            x: main.width / 2 + 0.4
            y: 20.4159
            width: 20; height: 20
            color: "yellow"
            border.width: 1
            border.color: "red"
        }
    }

    function step(f) {
        main.border.color = f % 4 === 1 || f % 4 === 2 ? "yellow" : "cyan"
        line.y = 10.4159 + Math.floor(f / 4)
        square.y = 20.4159 + Math.floor(f / 4)
    }
}
