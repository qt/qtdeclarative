// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

ControlledShape {
    id: shape
    anchors.fill: parent
    strokeWidth: 5
    strokeColor: "blue"
    strokeStyle: ShapePath.DashLine
    //dashPattern: [ 1, 4, 4, 4 ]
    fillColor: "lightBlue"
    property real xr: 70
    property real yr: 30
    startX: shape.width / 2 - xr
    startY: shape.height / 2 - yr

    delegate: [
        PathArc {
            x: shape.width / 2 + shape.xr
            y: shape.height / 2 + shape.yr
            radiusX: shape.xr
            radiusY: shape.yr
            useLargeArc: true
        },
        PathArc {
            x: shape.width / 2 - shape.xr
            y: shape.height / 2 - shape.yr
            radiusX: shape.xr
            radiusY: shape.yr
            useLargeArc: true
        }
    ]
}
