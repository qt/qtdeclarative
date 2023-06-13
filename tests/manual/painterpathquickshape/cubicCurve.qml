// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

ControlledShape {
    id: shape

    strokeWidth: 4
    strokeColor: "black"

    startX: 50
    startY: 100
    delegate: [
        PathCubic {
            x: 150
            y: 100
            control1X: cp1.x
            control1Y: cp1.y
            control2X: cp2.x
            control2Y: cp2.y
        }
    ]
    Rectangle {
        id: cp1
        color: "red"
        width: 10
        height: 10
        SequentialAnimation {
            loops: Animation.Infinite
            running: true
            NumberAnimation {
                target: cp1
                property: "x"
                from: 0
                to: flickable.width - cp1.width
                duration: 5000
            }
            NumberAnimation {
                target: cp1
                property: "x"
                from: flickable.width - cp1.width
                to: 0
                duration: 5000
            }
            NumberAnimation {
                target: cp1
                property: "y"
                from: 0
                to: flickable.height - cp1.height
                duration: 5000
            }
            NumberAnimation {
                target: cp1
                property: "y"
                from: flickable.height - cp1.height
                to: 0
                duration: 5000
            }
        }
    }

    Rectangle {
        id: cp2
        color: "blue"
        width: 10
        height: 10
        x: flickable.width - width
        SequentialAnimation {
            loops: Animation.Infinite
            running: true
            NumberAnimation {
                target: cp2
                property: "y"
                from: 0
                to: flickable.height - cp2.height
                duration: 5000
            }
            NumberAnimation {
                target: cp2
                property: "y"
                from: flickable.height - cp2.height
                to: 0
                duration: 5000
            }
            NumberAnimation {
                target: cp2
                property: "x"
                from: flickable.width - cp2.width
                to: 0
                duration: 5000
            }
            NumberAnimation {
                target: cp2
                property: "x"
                from: 0
                to: flickable.width - cp2.width
                duration: 5000
            }
        }
    }
}
