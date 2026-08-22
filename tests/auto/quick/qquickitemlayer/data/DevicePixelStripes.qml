// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: root

    property real dpr: 1
    property int pixelWidth: 1
    property int pixelHeight: 1
    property real position: 0

    width: 420
    height: 380

    Item {
        objectName: "target"
        layer.enabled: true
        x: root.position
        width: root.pixelWidth / root.dpr
        height: root.pixelHeight / root.dpr

        Rectangle {
            anchors.fill: parent
            color: "black"
        }

        Repeater {
            model: Math.floor(root.pixelWidth / 2)

            Rectangle {
                x: (index * 2 + 1) / root.dpr
                width: 1 / root.dpr
                height: root.pixelHeight / root.dpr
                color: "white"
            }
        }
    }
}
