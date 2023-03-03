// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256
    Shape {
        id: shape
        width: 220
        height: 200
        anchors.centerIn: parent

        ShapePath {
            fillGradient: LinearGradient {
                y2: shape.height
                GradientStop {
                    position: 0
                    color: "yellow"
                }
                GradientStop {
                    position: 1
                    color: "green"
                }
            }

            startX: 10
            startY: 100
            PathArc {
                relativeX: 50
                y: 100
                radiusX: 25
                radiusY: 25
            }
            PathArc {
                relativeX: 50
                y: 100
                radiusX: 25
                radiusY: 35
            }
            PathArc {
                relativeX: 50
                y: 100
                radiusX: 25
                radiusY: 60
            }
            PathArc {
                relativeX: 50
                y: 100
                radiusX: 50
                radiusY: 120
            }
        }
    }

    Shape {
        width: 120
        height: 130
        anchors.bottom: parent.bottom
        anchors.right: parent.right

        scale: 0.5

        ShapePath {
            fillColor: "transparent"
            strokeColor: "darkBlue"
            strokeWidth: 20
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: 65
                centerY: 95
                radiusX: 45
                radiusY: 45
                startAngle: -180
                SequentialAnimation on sweepAngle {
                    loops: Animation.Infinite
                    NumberAnimation {
                        to: 360
                        duration: 2000
                    }
                    NumberAnimation {
                        to: 0
                        duration: 2000
                    }
                }
            }
        }
    }
}
