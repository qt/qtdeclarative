// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    width: 200
    height: 100

    Rectangle {
        width: 20
        height: 20

        NumberAnimation on x {
            id: animation
            from: 0
            to: 180
            duration: Math.abs(1000)
            loops: Animation.Infinite
        }
    }

    Timer {
        repeat: true
        interval: 100
        running: true
        onTriggered: console.log("animation duration=" + animation.duration
                                 + " running=" + animation.running)
    }
}
