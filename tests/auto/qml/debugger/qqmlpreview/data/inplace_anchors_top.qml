// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    width: 200
    height: 200

    Rectangle {
        id: target
        anchors.top: parent.top
        width: 50
        height: 50
        color: "red"
    }

    Timer {
        repeat: true
        interval: 300
        running: true
        onTriggered: console.log("anchors_top target.y=" + target.y)
    }
}
