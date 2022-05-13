// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

PointHandler {
    id: handler
    objectName: "point " + handler.point.id.toString(16)
    acceptedDevices: PointerDevice.TouchScreen | PointerDevice.TouchPad
    target: AnimatedSprite {
        objectName: "sprite " + handler.point.id.toString(16)
        source: "images/fingersprite.png"
        visible: handler.active
        running: visible // QTBUG-64544: running defaults to true, but we don't see it animating if we don't toggle it like this
        x: handler.point.position.x - 20
        y: handler.point.position.y - 13
        width: frameWidth
        height: frameHeight
        frameWidth: 43
        frameHeight: 64
        frameCount: 3
        frameRate: 5
        parent: handler.parent
    }
}
