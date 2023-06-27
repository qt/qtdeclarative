// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick
import Qt.labs.animation

Item {
    width: 320; height: 480
    Flow {
        id: content
        width: parent.width
        spacing: 2; padding: 2

        WheelHandler {
            orientation: Qt.Vertical
            property: "y"
            rotationScale: 15
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onActiveChanged: if (!active) ybr.returnToBounds()
        }

        DragHandler {
            xAxis.enabled: false
            onActiveChanged: if (!active) ybr.returnToBounds()
        }

        BoundaryRule on y {
            id: ybr
            minimum: content.parent.height - content.height
            maximum: 0
            minimumOvershoot: 400; maximumOvershoot: 400
            overshootFilter: BoundaryRule.Peak
        }

        Repeater {
            model: 1000
            Rectangle { color: "gray"; width: 10 + Math.random() * 100; height: 15 }
        }
    }
}
//![0]
