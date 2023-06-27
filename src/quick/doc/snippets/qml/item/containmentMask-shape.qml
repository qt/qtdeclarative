// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

//![0]
Rectangle {
    width: 90; height: 100
    color: hoverHandler.hovered ? "wheat" : "lightgray"
    containmentMask: shape

    HoverHandler { id: hoverHandler }

    Shape {
        id: shape
        containsMode: Shape.FillContains

        ShapePath {
            fillColor: "lightsteelblue"
            startX: 10; startY: 20
            PathArc {
                x: 10; y: 80
                radiusX: 40; radiusY: 40
                useLargeArc: true
            }
            PathLine {
                x: 10; y: 20
            }
        }
    }
}
//![0]
