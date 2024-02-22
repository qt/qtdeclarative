// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

ControlledShape {
    delegate: [
        PathMove { x: p1.cx; y: p1.cy },
        PathQuad { x: p2.cx; y: p2.cy; controlX: c1.cx; controlY: c1.cy },
        PathQuad { x: p3.cx; y: p3.cy; controlX: c2.cx; controlY: c2.cy },
        PathQuad { x: p4.cx; y: p4.cy; controlX: c3.cx; controlY: c3.cy },
        //PathQuad { x: 600; y: 100; controlX: 100; controlY: 100 },
        PathLine { x: p5.cx; y: p5.cy }
    ]

    ControlPoint {
        id: p1
        cx: 600
        cy: 100
    }
    ControlPoint {
        id: c1
        color: "blue"
        cx: 1100
        cy: 100
    }
    ControlPoint {
        id: p2
        cx: 1100
        cy: 600
    }
    ControlPoint {
        id: c2
        color: "blue"
        cx: 1100
        cy: 1100
    }
    ControlPoint {
        id: p3
        cx: 600
        cy: 1100
    }
    ControlPoint {
        id: c3
        color: "blue"
        cx: 100
        cy: 1100
    }
    ControlPoint {
        id: p4
        cx: 100
        cy: 600
    }
    ControlPoint {
        id: p5
        cx: 600
        cy: 330
    }
}
