// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Window {
    id: w
    height: 100
    width: 100
    visible: true

    Rectangle {
        width: 50
        height: 50
        color: "blue"
        anchors.centerIn: parent
    }

    Timer {
        interval: 100
        running: true
        repeat: true
        onTriggered: console.log("zoom", w.screen.devicePixelRatio)
    }
}
