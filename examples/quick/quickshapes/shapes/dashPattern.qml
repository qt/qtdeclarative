// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256
    Shape {
        id: shape
        anchors.fill: parent

        ShapePath {
            id: p
            strokeWidth: 5
            strokeColor: "blue"
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 4, 4, 4 ]
            fillColor: "lightBlue"

            PathAngleArc {
                centerX: shape.width / 2
                centerY: shape.height / 2
                radiusX: shape.width / 2.5
                radiusY: shape.height / 6
                sweepAngle: 360
            }
        }
    }
}
