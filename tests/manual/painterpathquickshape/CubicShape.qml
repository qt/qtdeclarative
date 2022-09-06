// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

ControlledShape {
    fillRule: ShapePath.OddEvenFill
    delegate: [
        PathMove { x: start.cx; y: start.cy},
        PathCubic { x: end.cx; y: end.cy;
                    control1X:  control1.cx; control1Y:  control1.cy
                    control2X:  control2.cx; control2Y:  control2.cy },
        PathMove { x: start2.cx; y: start2.cy },
        PathCubic { x: end2.cx; y: end2.cy;
            control1X:  control21.cx; control1Y:  control21.cy
            control2X:  control22.cx; control2Y:  control22.cy },
        PathLine { x: lineEnd.cx; y: lineEnd.cy }
    ]

    // Cubic path 1
    ControlPoint {
        id: start
        cx: 200
        cy: 400
    }
    ControlPoint {
        id: control1
        color: "blue"
        cx: 800
        cy: 0
    }
    ControlPoint {
        id: control2
        color: "blue"
        cx: 800
        cy: 1000
    }
    ControlPoint {
        id: end
        cx: 200
        cy: 600
    }

    // Cubic path 2
    ControlPoint {
        id: start2
        cx: 2200
        cy: 200
    }
    ControlPoint {
        id: control21
        color: "blue"
        cx: 1200
        cy: 600
    }
    ControlPoint {
        id: control22
        color: "blue"
        cx: 3200
        cy: 1000
    }
    ControlPoint {
        id: end2
        cx: 2200
        cy: 1400
    }
    ControlPoint {
        id: lineEnd
        cx: 1200
        cy: 200
    }
}
