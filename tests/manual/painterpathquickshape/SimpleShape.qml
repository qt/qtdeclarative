// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

ControlledShape {
    delegate: [
        // A triangle
        PathPolyline {
            id: ppl
            path: [ Qt.point(150.0, 100.0),
                Qt.point(1250.0, 150.0),
                Qt.point(100.0, 1000.0),
                Qt.point(150, 100)
            ]
        },
        // A very narrow shape with one convex and one concave curve
        PathMove { x: 600; y: 1200},
        PathQuad { x: 800; y: 1200; controlX: 700; controlY: 600 },
        PathQuad { x: 600; y: 1200; controlX: 700; controlY: 700 },

        // A more complex path with editable points
        PathMove { x: p1.cx; y: p1.cy },
        PathQuad { x: p2.cx; y: p2.cy; controlX: c1.cx; controlY: c1.cy },
        PathQuad { x: p3.cx; y: p3.cy; controlX: c2.cx; controlY: c2.cy },
        PathQuad { x: p4.cx; y: p4.cy; controlX: c3.cx; controlY: c3.cy },
        PathLine { x: p5.cx; y: p5.cy },
        PathQuad { x: p6.cx; y: p6.cy; controlX: c5.cx; controlY: c5.cy },
        PathQuad { x: p7.cx; y: p7.cy; controlX: c6.cx; controlY: c6.cy },
        PathQuad { x: p8.cx; y: p8.cy; controlX: c7.cx; controlY: c7.cy }
    ]

    // Control points for the editable part:
    // Curve p1-c1-p2, Curve p2-c2-p3, Curve p3-c3-p4
    // Line p4-p5, Curve p5-c5-p6, Curve p6-c6-p7, Curve p7-c7-p8

    ControlPoint {
        id: p1
        cx: 100
        cy: 1000
    }
    ControlPoint {
        id: c1
        color: "blue"
        cx: 200
        cy: 1500
    }
    ControlPoint {
        id: p2
        cx: 700
        cy: 1500
    }
    ControlPoint {
        id: c2
        color: "blue"
        cx: 1200
        cy: 1500
    }
    ControlPoint {
        id: p3
        cx: 1200
        cy: 1000
    }
    ControlPoint {
        id: c3
        color: "blue"
        cx: 1100
        cy: 700
    }
    ControlPoint {
        id: p4
        cx: 800
        cy: 600
    }
    ControlPoint {
        id: p5
        cx: 800
        cy: 800
    }
    ControlPoint {
        id: c5
        color: "blue"
        cx: 1000
        cy: 600
    }
    ControlPoint {
        id: p6
        cx: 1000
        cy: 1000
    }
    ControlPoint {
        id: c6
        color: "blue"
        cx: 1000
        cy: 1300
    }
    ControlPoint {
        id: p7
        cx: 700
        cy: 1300
    }
    ControlPoint {
        id: c7
        color: "blue"
        cx: 400
        cy: 1300
    }
    ControlPoint {
        id: p8
        cx: 400
        cy: 1000
    }

}
