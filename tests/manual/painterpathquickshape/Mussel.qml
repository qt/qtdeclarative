// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

ControlledShape {
    delegate: [
        PathMove { x: p1.cx; y: p1.cy },
        PathQuad { x: p2.cx; y: p2.cy; controlX: c1.cx; controlY: c1.cy },
        PathQuad { x: p3.cx; y: p3.cy; controlX: c2.cx; controlY: c2.cy },
        PathLine { x: p1.cx; y: p1.cy }
    ]

    ControlPoint {
        id: p1
        cx: 200
        cy: 200
    }
    ControlPoint {
        id: c1
        color: "blue"
        cx: 600
        cy: 0
    }
    ControlPoint {
        id: p2
        cx: 1000
        cy: 200
    }
    ControlPoint {
        id: c2
        color: "blue"
        cx: 1000
        cy: 1000
    }
    ControlPoint {
        id: p3
        cx: 200
        cy: 1000
    }
}
