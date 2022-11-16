// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.15

Item {
    width: 320; height: 320
    property alias pinchScale: pinch.scale

    Rectangle {
        objectName: "blackrect"
        width: 200; height: 200
        color: "black"
        antialiasing: true
        scale: pinch.scale
        rotation: pinch.rotation
        x: pinch.translation.x
        y: pinch.translation.y

        PinchHandler {
            id: pinch
            target: null
            minimumScale: 0.5
            maximumScale: 4
        }

        Text {
            color: "cyan"
            anchors.centerIn: parent
            text: "scale " + pinch.scale.toFixed(2) + " activeScale " + pinch.activeScale.toFixed(2)
        }
    }
}
