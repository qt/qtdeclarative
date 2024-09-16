// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes
import QtQuick.Effects
import WearableStyle

Item {
    Rectangle {
        id: listitem
        width: parent.width
        height: parent.height

        radius: 8
        color: UIStyle.listItemBackground

        Shape {
            id: header
            property int ra: parent.radius
            property int h: 28
            property int w: parent.width

            preferredRendererType: Shape.CurveRenderer

            ShapePath { //Shape because Rectangle does not support diagonal gradient
                strokeWidth: 0

                startX: 0
                startY: header.ra

                PathArc {
                    x: header.ra
                    y: 0
                    radiusX: header.ra
                    radiusY: header.ra
                }
                PathLine {
                    x: header.w - header.ra
                    y: 0
                }
                PathArc {
                    x: header.w
                    y: header.ra
                    radiusX: header.ra
                    radiusY: header.ra
                }
                PathLine {
                    x: header.w
                    y: header.h
                }
                PathLine {
                    x: 0
                    y: header.h
                }
                fillGradient: LinearGradient {
                    x1: 0
                    y1: 0
                    x2: 2
                    y2: 1.3 * header.h
                    GradientStop {
                        position: 0.0
                        color: UIStyle.listHeader1
                    }
                    GradientStop {
                        position: 1.0
                        color: UIStyle.listHeader2
                    }
                }
            }
        }
    }

    MultiEffect {
        source: listitem
        anchors.fill: parent
        shadowEnabled: true
        shadowBlur: 0.3
        shadowHorizontalOffset: 2
        shadowVerticalOffset: 2
        opacity: 0.5
    }
}
