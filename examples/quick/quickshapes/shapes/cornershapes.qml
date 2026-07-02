// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256

    Shape {
        id: myShape
        preferredRendererType: root.requestedBackend
        anchors.fill: parent

        ShapePath {
            id: myPath
            strokeColor: "blue"
            strokeWidth: myShape.width / 25
            joinStyle: ShapePath.MiterJoin
            fillGradient: LinearGradient {
                x2: myShape.width
                y2: x2
                GradientStop { position: 0.1; color: "red" }
                GradientStop { position: 0.45; color: "salmon" }
                GradientStop { position: 0.7; color: "red" }
            }

            property real animRadius
            SequentialAnimation on animRadius {
                loops: Animation.Infinite
                NumberAnimation {
                    from: 0
                    to: 200
                    duration: 3000
                }
                NumberAnimation {
                    from: 200
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
                topRightRadius: myPath.animRadius
                bottomRightRadius: myPath.animRadius
                topLeftCornerShape: PathRectangle.Rounded
                topRightCornerShape: PathRectangle.Squircle
                bottomRightCornerShape: PathRectangle.Bevel
            }
        }
    }
}
