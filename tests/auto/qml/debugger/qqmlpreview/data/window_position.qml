// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Window 2.0

Window {
    visible: true
    width: 200
    height: 150
    x: 50
    y: 50

    Timer {
        repeat: true
        interval: 300
        running: true
        onTriggered: console.log("pos x=" + parent.x + " y=" + parent.y
                                 + " w=" + parent.width + " h=" + parent.height)
    }
}
