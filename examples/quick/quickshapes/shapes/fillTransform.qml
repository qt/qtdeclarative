// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256

    Shape {
        anchors.centerIn: parent
        width: 200
        height: 100

        ShapePath {
            id: path1
            fillGradient: RadialGradient {
                id: grad
                centerX: path1.startX + 100
                centerY: path1.startY + 50
                centerRadius: 50
                focalX: centerX
                focalY: centerY
                GradientStop { position: 0.0; color: "blue" }
                GradientStop { position: 0.5; color: "cyan" }
                GradientStop { position: 1.0; color: "blue" }
            }

            property real fillScale
            NumberAnimation on fillScale {
                running: true
                loops: Animation.Infinite
                from: 1
                to: 5
                duration: 3000
                easing.type: Easing.InQuad
            }

            fillTransform: PlanarTransform.fromScale(fillScale, 1, grad.centerX, grad.centerY)

            PathRectangle { width: 200; height: 100 }
        }
    }
}
