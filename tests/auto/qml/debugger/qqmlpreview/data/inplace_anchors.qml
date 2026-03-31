// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    Rectangle {
        width: 200
        height: 200
        color: "red"

        Rectangle {
            id: sibling
            width: 100
            height: 100
            color: "blue"
        }

        Rectangle {
            id: target
            anchors.fill: parent
            color: "green"
        }

        Timer {
            repeat: true
            interval: 300
            running: true
            onTriggered: console.log("anchors target.w=" + target.width)
        }
    }
}
