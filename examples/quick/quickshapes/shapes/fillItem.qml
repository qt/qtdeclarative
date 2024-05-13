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
        anchors.centerIn: parent
        width: 200
        height: 100

        ShapePath {
            strokeColor: "black"
            strokeWidth: 1
            fillItem: image
            fillTransform: PlanarTransform.fromScale(0.5, 0.5)
            startX: shape.width / 2 - 40
            startY: shape.height / 2 - 40

            PathArc {
                x: shape.width / 2 + 40
                y: shape.height / 2 + 40
                radiusX: 40
                radiusY: 40
                useLargeArc: true
            }

            PathArc {
                x: shape.width / 2 - 40
                y: shape.height / 2 - 40
                radiusX: 40
                radiusY: 40
                useLargeArc: true
            }
        }
    }
}
