// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256

    Shape {
        id: circ1
        anchors.fill: parent

        ShapePath {
            id: p1
            fillColor: "transparent" // stroke only
            strokeWidth: 4

            SequentialAnimation on strokeColor {
                loops: Animation.Infinite
                ColorAnimation {
                    from: "black"
                    to: "yellow"
                    duration: 5000
                }
                ColorAnimation {
                    from: "yellow"
                    to: "green"
                    duration: 5000
                }
                ColorAnimation {
                    from: "green"
                    to: "black"
                    duration: 5000
                }
            }

            readonly property real r: 85
            PathAngleArc {
                centerX: circ1.width / 2
                centerY: circ1.height / 2
                radiusX: p1.r
                radiusY: p1.r
                sweepAngle: 360
            }
        }
    }

    Shape {
        id: circ2
        anchors.fill: parent

        SequentialAnimation on opacity {
            loops: Animation.Infinite
            NumberAnimation {
                from: 1.0
                to: 0.0
                duration: 5000
            }
            NumberAnimation {
                from: 0.0
                to: 1.0
                duration: 5000
            }
        }

        ShapePath {
            id: p2
            strokeWidth: -1 // or strokeColor: "transparent"

            SequentialAnimation on fillColor {
                loops: Animation.Infinite
                ColorAnimation {
                    from: "gray"
                    to: "purple"
                    duration: 3000
                }
                ColorAnimation {
                    from: "purple"
                    to: "red"
                    duration: 3000
                }
                ColorAnimation {
                    from: "red"
                    to: "gray"
                    duration: 3000
                }
            }

            readonly property real r: 55
            PathAngleArc {
                centerX: circ2.width / 2
                centerY: circ2.height / 2
                radiusX: p2.r
                radiusY: p2.r
                sweepAngle: 360
            }
        }
    }
}
