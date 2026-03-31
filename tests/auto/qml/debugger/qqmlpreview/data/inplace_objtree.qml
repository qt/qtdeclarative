// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    id: root
    width: 200
    height: 200
    color: "white"

    Image {
        source: "content/gfx/background.png"
        anchors.fill: parent
    }

    Timer {
        repeat: true
        interval: 300
        running: true
        onTriggered: console.log("objtree children=" + root.children.length)
    }
}
