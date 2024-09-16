// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick

Rectangle {
    id: root
    width: 400
    height: 400
    color: ph.active ? "aquamarine" : "beige"

    PinchHandler {
        id: ph
        grabPermissions: PointerHandler.TakeOverForbidden
    }

    Rectangle {
        objectName: "rect1"
        x: 50
        width: 100
        height: 100
        color: dh1.active ? "tomato" : "wheat"
        DragHandler {
            id: dh1
            objectName: "dh1"
        }
    }

    Rectangle {
        objectName: "rect2"
        x: 250
        width: 100
        height: 100
        color: dh2.active ? "tomato" : "lightsteelblue"
        DragHandler {
            id: dh2
            objectName: "dh2"
        }
    }

    Rectangle {
        objectName: "rect3"
        x: 150
        y: 150
        width: 100
        height: 100
        color: dh3.active ? "tomato" : "darksalmon"
        DragHandler {
            id: dh3
            objectName: "dh3"
        }
    }

    PointHandler {
        id: ph1
        acceptedDevices: PointerDevice.TouchScreen
        target: Text {
            parent: root
            color: "orange"
            x: ph1.point.position.x - 3
            y: ph1.point.position.y - 3
            font.bold: true
            text: ph1.point.id
        }
    }

    PointHandler {
        id: ph2
        acceptedDevices: PointerDevice.TouchScreen
        target: Text {
            parent: root
            color: "green"
            x: ph2.point.position.x - 3
            y: ph2.point.position.y - 3
            font.bold: true
            text: ph2.point.id
        }
    }
}
