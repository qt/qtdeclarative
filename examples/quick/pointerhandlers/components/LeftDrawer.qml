// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import Qt.labs.animation

Item {
    id: root
    objectName: "LeftHandlerDrawer"
    width: 100
    height: 400
    property real stickout: 4
    property real xOpen: rect.radius * -2
    property real xClosed: stickout - width
    x: xClosed
    y: 10

    function close() {
        openCloseAnimation.to = xClosed
        openCloseAnimation.start()
    }
    function open() {
        openCloseAnimation.to = xOpen
        openCloseAnimation.start()
    }

    DragHandler {
        id: dragHandler
        yAxis.enabled: false
        xAxis.minimum: -1000
        margin: 20
        onActiveChanged:
            if (!active) {
                if (xbr.returnToBounds())
                    return;
                if (activeTranslation.x > 0)
                    open()
                if (activeTranslation.x < 0)
                    close()
            }
    }

    BoundaryRule on x {
        id: xbr
        minimum: xClosed
        maximum: xOpen
        minimumOvershoot: rect.radius
        maximumOvershoot: rect.radius
    }

    NumberAnimation on x {
        id: openCloseAnimation
        duration: 300
        easing { type: Easing.OutBounce; overshoot: 5 }
    }

    // TODO this was supposed to be RectangularGlow but we lost QtGraphicalEffects in Qt 6
    Rectangle {
        id: effect
        anchors.fill: parent
        border.width: dragHandler.margin
        border.color: "cyan"
        opacity: 0.2
        radius: rect.radius + dragHandler.margin
    }

    Rectangle {
        id: rect
        anchors.fill: parent
        anchors.margins: 3
        color: "#333"
        border.color: "cyan"
        border.width: 2
        radius: 10
        antialiasing: true
    }
}
