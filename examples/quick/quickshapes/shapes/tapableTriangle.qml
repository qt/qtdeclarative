// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    width: 256
    height: 256
    color: th.pressed ? "steelBlue" : "lightGray"
    containmentMask: ctr

    TapHandler {
        id: th
    }

    Shape {
        id: ctr
        anchors.fill: parent
        containsMode: Shape.FillContains

        ShapePath {
            strokeColor: "red"
            fillColor: "blue"

            SequentialAnimation on strokeWidth {
                loops: Animation.Infinite
                NumberAnimation {
                    from: 1
                    to: 30
                    duration: 5000
                }
                NumberAnimation {
                    from: 30
                    to: 1
                    duration: 5000
                }
                PauseAnimation {
                    duration: 2000
                }
            }

            startX: 30
            startY: 30
            PathLine {
                x: ctr.width - 30
                y: ctr.height - 30
            }
            PathLine {
                x: 30
                y: ctr.height - 30
            }
            PathLine {
                x: 30
                y: 30
            }
        }

        // Besides ShapePath, Shape supports visual and non-visual objects too, allowing
        // free mixing without going through extra hoops:
        Rectangle {
            id: testRect
            color: "green"
            opacity: 0.3
            width: 20
            height: 20
            anchors.right: parent.right
        }
        Timer {
            interval: 100
            repeat: true
            onTriggered: testRect.width = testRect.width > 1 ? testRect.width - 1 : 20
            running: true
        }
    }
}
