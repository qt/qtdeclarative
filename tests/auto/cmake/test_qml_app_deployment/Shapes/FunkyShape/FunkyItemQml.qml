// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Shapes

Shape {
    width: 200
    height: 150
    anchors.centerIn: parent
    ShapePath {
        strokeWidth: 4
        strokeColor: "red"
        fillGradient: LinearGradient {
            x1: 20; y1: 20
            x2: 180; y2: 130
            GradientStop { position: 0; color: "blue" }
            GradientStop { position: 0.2; color: "green" }
            GradientStop { position: 0.4; color: "red" }
            GradientStop { position: 0.6; color: "yellow" }
            GradientStop { position: 1; color: "cyan" }
        }
        strokeStyle: ShapePath.DashLine
        dashPattern: [ 1, 4 ]
        startX: 20; startY: 20
        PathLine { x: 180; y: 130 }
        PathLine { x: 20; y: 130 }
        PathLine { x: 20; y: 20 }
    }
}
