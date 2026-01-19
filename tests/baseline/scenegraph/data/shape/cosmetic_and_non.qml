// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

Item {
    width: 320; height: 400

    component Shapes: Shape {
        ShapePath {
            strokeColor: "black"
            strokeWidth: 10
            startX: 40
            PathLine { relativeX: 20; relativeY: 10 }
        }
        ShapePath {
            strokeColor: "darkred"
            strokeWidth: 10
            cosmeticStroke: true
            startX: 40
            startY: 30
            PathLine { relativeX: 20; relativeY: 10 }
        }
    }

    Repeater {
        model: [Shape.GeometryRenderer, Shape.CurveRenderer]
        Item {
            x: index * 150
            Shapes {
                y: 30
                preferredRendererType: modelData
                Text {
                    y: -30
                    text: "renderer " + parent.rendererType
                }
            }
            Shapes {
                y: 100
                scale: 0.5
                preferredRendererType: modelData
            }
            Shapes {
                y: 260
                scale: 4
                preferredRendererType: modelData
            }
        }
    }
}
