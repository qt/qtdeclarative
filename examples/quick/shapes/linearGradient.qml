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

            property real xr: 70
            property real yr: 30
            startX: shape.width / 2 - xr
            startY: shape.height / 2 - yr
            PathArc {
                x: shape.width / 2 + p.xr
                y: shape.height / 2 + p.yr
                radiusX: p.xr
                radiusY: p.yr
                useLargeArc: true
            }
            PathArc {
                x: shape.width / 2 - p.xr
                y: shape.height / 2 - p.yr
                radiusX: p.xr
                radiusY: p.yr
                useLargeArc: true
            }
        }
    }
}
