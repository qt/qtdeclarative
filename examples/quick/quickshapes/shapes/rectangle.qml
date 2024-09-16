// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256

    Shape {
        id: myShape
        anchors.fill: parent

        ShapePath {
            id: myPath
            strokeColor: "green"
            strokeWidth: myShape.width / 25
            joinStyle: ShapePath.MiterJoin
            fillGradient: LinearGradient {
                x2: myShape.width
                y2: x2
                GradientStop { position: 0.1; color: "yellow" }
                GradientStop { position: 0.45; color: "lightyellow" }
                GradientStop { position: 0.7; color: "yellow" }
            }

            property real animRadius
            SequentialAnimation on animRadius {
                loops: Animation.Infinite
                NumberAnimation {
                    from: 0
                    to: 100
                    duration: 3000
                }
                NumberAnimation {
                    from: 100
                    to: 0
                    duration: 3000
                }
                PauseAnimation {
                    duration: 1000
                }
            }

            PathRectangle {
                x: myShape.width / 5
                y: x
                width: myShape.width - 2 * x
                height: width
                topLeftRadius: myPath.animRadius
                bottomRightRadius: myPath.animRadius
            }
        }
    }
}
