// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes
import shared

Rectangle {
    color: "lightGray"
    width: 256
    height: 256

    Image {
        id: image
        source: Images.qtLogo
        visible: false
    }

    Shape {
        id: shape
        anchors.fill: parent

        ShapePath {
            strokeColor: "black"
            strokeWidth: 1
            fillItem: image
            fillTransform: PlanarTransform.fromRotate(45, shape.width / 2, shape.height / 2)

            PathAngleArc {
                centerX: shape.width / 2
                centerY: shape.height / 2
                radiusX: 100
                radiusY: radiusX
                sweepAngle: 360
            }
        }
    }
}
