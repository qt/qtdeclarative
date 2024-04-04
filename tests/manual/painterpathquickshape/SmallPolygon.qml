// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

ControlledShape {
    delegate: [
        PathPolyline {
            path: [ point1.pt,
                    point2.pt,
                    point3.pt,
                    point4.pt,
                    point5.pt,
                    point1.pt
                  ]

        }
    ]

    ControlPoint {
        id: point1
        cx: 400
        cy: 900
    }
    ControlPoint {
        id: point2
        cx: 1500
        cy: 600
    }
    ControlPoint {
        id: point3
        cx: 2500
        cy: 1100
    }
    ControlPoint {
        id: point4
        cx: 3000
        cy: 1100
    }
    ControlPoint {
        id: point5
        cx: 2000
        cy: 1900
    }

}
