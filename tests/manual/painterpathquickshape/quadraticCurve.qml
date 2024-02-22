// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

Item {
    ControlledShape {
        id: shape
        anchors.fill: parent
        strokeWidth: 4
        strokeColor: "black"
        fillColor: "transparent"

        startX: 50
        startY: 50

        delegate: [
            PathQuad {
                x: 150
                y: 50
                controlX: cp.x
                controlY: cp.y
            }
        ]
    }

    Rectangle {
        id: cp
        color: "red"
        width: 10
        height: 10
        SequentialAnimation on x {
            loops: Animation.Infinite
            NumberAnimation {
                from: 0
                to: shape.width - cp.width
                duration: 5000
            }
            NumberAnimation {
                from: shape.width - cp.width
                to: 0
                duration: 5000
            }
        }
    }
}
