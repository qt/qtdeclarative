// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.14

Rectangle {
    width: 320; height: 240
    color: "green"; antialiasing: true

    Rectangle {
        width: 100; height: 2; anchors.centerIn: parent
        Rectangle {
            width: 2; height: 100; anchors.centerIn: parent
        }
    }

    Rectangle {
        color: "red"
        width: 6; height: 6; radius: 3
        x: wheelHandler.point.position.x - radius
        y: wheelHandler.point.position.y - radius
    }

    Text {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 20
        color: "white"
        font.pixelSize: 18
        text: parent.x.toFixed(2) + ", " + parent.y.toFixed(2)
    }

    WheelHandler {
        id: wheelHandler
        activeTimeout: 0.5
    }
}
