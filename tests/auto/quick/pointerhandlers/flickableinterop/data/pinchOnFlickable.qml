// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Flickable {
    id: root
    width: 800
    height: 480
    contentWidth: 1000
    contentHeight: 600

    Rectangle {
        id: pinchable
        objectName: "pinchable"
        border.color: "black"
        color: pinch.active ? "salmon" : "peachpuff"
        x: 100
        y: 100
        width: 200
        height: 200
        radius: 80
        PinchHandler {
            id: pinch
        }
        PointHandler {
            id: p1
            target: Rectangle {
                parent: pinchable
                color: "green"
                visible: p1.active
                x: p1.point.position.x - width / 2
                y: p1.point.position.y - height / 2
                width: 9; height: width; radius: width / 2
            }
        }
        PointHandler {
            id: p0
            target: Rectangle {
                parent: pinchable
                color: "red"
                visible: p0.active
                x: p0.point.position.x - width / 2
                y: p0.point.position.y - height / 2
                width: 9; height: width; radius: width / 2
            }
        }
    }
}
