// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

HoverHandler {
    id: handler
    objectName: "mouse point"
    acceptedDevices: PointerDevice.Mouse
    target: Image {
        objectName: "mouse sprite"
        source: "images/mouse.png"
        opacity: (phandler.point.pressedButtons || wheelAnimationTimer.running) ? 1 : 0
        x: handler.point.position.x - width / 2
        y: handler.point.position.y - height / 2
        parent: handler.parent
        Image {
            source: "images/mouse_left.png"
            visible: phandler.point.pressedButtons & Qt.LeftButton
        }
        Image {
            source: "images/mouse_middle.png"
            visible: phandler.point.pressedButtons & Qt.MiddleButton
        }
        Image {
            source: "images/mouse_right.png"
            visible: phandler.point.pressedButtons & Qt.RightButton
        }
        PointHandler  {
            id: phandler
            acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
        }

        WheelHandler {
            blocking: false
            onWheel: (event)=> {
                wheelSprite.reverse = (event.angleDelta.y < 0)
                wheelAnimationTimer.start()
            }
        }
        AnimatedSprite {
            id: wheelSprite
            x: 19
            y: 7
            source: "images/mouse_wheel_ridges.png"
            frameWidth: 5
            frameHeight: 15
            frameCount: 3
            frameDuration: 50
            running: wheelAnimationTimer.running
            visible: running
            Timer {
                id: wheelAnimationTimer
                interval: 500
            }
        }
    }
}
