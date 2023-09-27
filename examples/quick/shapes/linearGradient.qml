// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256
    Shape {
        width: 200
        height: 150
        anchors.centerIn: parent
        ShapePath {
            strokeWidth: 4
            strokeColor: "red"
            fillGradient: LinearGradient {
                x1: 20
                y1: 20
                x2: 180
                y2: 130
                GradientStop {
                    position: 0
                    color: "blue"
                }
                GradientStop {
                    position: 0.2
                    color: "green"
                }
                GradientStop {
                    position: 0.4
                    color: "red"
                }
                GradientStop {
                    position: 0.6
                    color: "yellow"
                }
                GradientStop {
                    position: 1
                    color: "cyan"
                }
            }
            fillColor: "blue" // ignored with the gradient set
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 4 ]
            startX: 20
            startY: 20
            PathLine {
                x: 180
                y: 130
            }
            PathLine {
                x: 20
                y: 130
            }
            PathLine {
                x: 20
                y: 20
            }
        }
        transform: Rotation {
            origin.x: 100
            origin.y: 50
            axis {
                x: 0
                y: 1
                z: 0
            }
            SequentialAnimation on angle {
                NumberAnimation {
                    from: 0
                    to: 75
                    duration: 2000
                }
                NumberAnimation {
                    from: 75
                    to: -75
                    duration: 4000
                }
                NumberAnimation {
                    from: -75
                    to: 0
                    duration: 2000
                }
                loops: Animation.Infinite
            }
        }
    }
}
