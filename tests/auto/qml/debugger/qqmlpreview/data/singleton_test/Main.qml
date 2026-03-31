// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: root
    color: "red"
    property int w: Settings.screenWidth
    property int h: Settings.screenHeight

    Timer {
        repeat: true
        interval: 300
        running: true
        onTriggered: console.log("singleton_width root.w=" + root.w)
    }
}
