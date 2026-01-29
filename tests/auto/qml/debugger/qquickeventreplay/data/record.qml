// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest

Window {
    id: window
    visible: true
    width: 100
    height: 100

    Component.onCompleted: window.requestActivate()

    Timer {
        id: timer
        interval: 100
        running: window.active
        onTriggered: {
            event.keyClick(Qt.Key_Q, Qt.NoModifier, -1)
            event.mouseMove(area, 12, 13, -1, Qt.NoButton, Qt.NoModifier)
            event.mouseClick(area, 12, 13, Qt.LeftButton, Qt.NoModifier, -1)
        }
    }

    MouseArea {
        id: area
        focus: true
        anchors.fill: parent
        onClicked: console.log("clicked")
    }

    TestEvent {
        id: event
    }
}

