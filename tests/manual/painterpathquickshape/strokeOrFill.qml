// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

Item {
    ControlledShape {
        id: circ1
        anchors.fill: parent
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

        readonly property real r: 60
        startX: circ1.width / 2 - r
        startY: circ1.height / 2 - r

        delegate: [
            PathArc {
                x: circ1.width / 2 + circ1.r
                y: circ1.height / 2 + circ1.r
                radiusX: circ1.r
                radiusY: circ1.r
                useLargeArc: true
            },
            PathArc {
                x: circ1.width / 2 - circ1.r
                y: circ1.height / 2 - circ1.r
                radiusX: circ1.r
                radiusY: circ1.r
                useLargeArc: true
            }
        ]
    }

    ControlledShape {
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

        readonly property real r: 40
        startX: circ2.width / 2 - r
        startY: circ2.height / 2 - r

        delegate: [
            PathArc {
                x: circ2.width / 2 + circ2.r
                y: circ2.height / 2 + circ2.r
                radiusX: circ2.r
                radiusY: circ2.r
                useLargeArc: true
            },
            PathArc {
                x: circ2.width / 2 - circ2.r
                y: circ2.height / 2 - circ2.r
                radiusX: circ2.r
                radiusY: circ2.r
                useLargeArc: true
            }
        ]
    }
}
