// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

ControlledShape {
    fillColor: "transparent"
    strokeStyle: ShapePath.DashLine
    dashOffset: 6.5
    dashPattern: [ 4, 2, 1, 2, 1, 2 ]
    delegate: [
        PathMove { x: 200; y: 200 },
        PathLine { x: 1000; y: 200 },
        PathMove { x: 200; y: 300 },
        PathLine { x: 1000; y: 300 },
        PathMove { x: 200; y: 400 },
        PathQuad { x: 1000; y: 400; controlX: 450; controlY: 600 },
        PathLine { x: 1600; y: 500 },
        PathMove { x: 200; y: 500 },
        PathQuad { x: 1000; y: 500; controlX: 450; controlY: 700 },
        PathLine { x: 1600; y: 600 }
    ]
}
