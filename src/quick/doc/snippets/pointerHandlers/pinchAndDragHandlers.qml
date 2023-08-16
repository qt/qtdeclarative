// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![entire]
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
}
//![entire]
