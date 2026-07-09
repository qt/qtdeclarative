// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: root
    width: 200
    height: 100

    App {
        id: app
        anchors.fill: parent
    }

    // Continuously toggle the singleton, mirroring the coffee example's theme
    // toggle. Every toggle fires all bindings that read Colors.currentTheme,
    // including any stale one left over on a freed button after a reload.
    Timer {
        repeat: true
        interval: 100
        running: true
        onTriggered: {
            Colors.currentTheme = (Colors.currentTheme == Colors.dark)
                    ? Colors.light : Colors.dark
            console.log("valuetype_singleton icon=" + app.themeButton.icon.color)
        }
    }
}
