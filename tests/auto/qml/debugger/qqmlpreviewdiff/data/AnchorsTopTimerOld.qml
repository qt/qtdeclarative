// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: root
    width: 200; height: 200
    property string log: ""

    Rectangle {
        id: target
        anchors.top: parent.top
        width: 50; height: 50
    }

    Timer {
        repeat: true
        interval: 50
        running: true
        onTriggered: root.log += "y=" + target.y
    }
}
