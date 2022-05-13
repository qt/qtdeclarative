// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.animation

Item {
    id: root
    objectName: "viewport"
    default property alias data: __contentItem.data
    property alias velocity: anim.velocity
    property alias contentX: __contentItem.x // sign is reversed compared to Flickable.contentX
    property alias contentY: __contentItem.y // sign is reversed compared to Flickable.contentY
    property alias contentWidth: __contentItem.width
    property alias contentHeight: __contentItem.height
    signal flickStarted
    signal flickEnded

    Item {
        id: __contentItem
        objectName: "__contentItem"
        width: childrenRect.width
        height: childrenRect.height

        BoundaryRule on x {
            id: xbr
            minimum: root.width - __contentItem.width
            maximum: 0
            minimumOvershoot: 100
            maximumOvershoot: 100
            overshootFilter: BoundaryRule.Peak
        }

        BoundaryRule on y {
            id: ybr
            minimum: root.height - __contentItem.height
            maximum: 0
            minimumOvershoot: 100
            maximumOvershoot: 100
            overshootFilter: BoundaryRule.Peak
        }

        DragHandler {
            id: dragHandler
            onActiveChanged:
                if (active) {
                    anim.stop()
                    root.flickStarted()
                } else {
                    var vel = centroid.velocity
                    if (xbr.returnToBounds())
                        vel.x = 0
                    if (ybr.returnToBounds())
                        vel.y = 0
                    if (vel.x !== 0 || vel.y !== 0)
                        anim.restart(vel)
                    else
                        root.flickEnded()
                }
        }
        WheelHandler {
            rotationScale: 15
            property: "x"
            orientation: Qt.Horizontal
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onActiveChanged:
                // emitting signals in both instances is redundant but hard to avoid
                // when the touchpad is flicking along both axes
                if (active) {
                    anim.stop()
                    root.flickStarted()
                } else {
                    xbr.returnToBounds()
                    root.flickEnded()
                }
        }
        WheelHandler {
            rotationScale: 15
            property: "y"
            orientation: Qt.Vertical
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onActiveChanged:
                if (active) {
                    anim.stop()
                    root.flickStarted()
                } else {
                    ybr.returnToBounds()
                    root.flickEnded()
                }
        }
        MomentumAnimation {
            id: anim
            target: __contentItem
            onStarted: root.flickStarted()
            onStopped: {
                xbr.returnToBounds()
                ybr.returnToBounds()
                root.flickEnded()
            }
        }
    }
}
