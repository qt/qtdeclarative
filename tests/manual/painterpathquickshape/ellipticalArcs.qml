// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Item {
    ControlledShape {
        id: shape
        anchors.top: parent.top
        anchors.left: parent.left
        width: parent.width
        height: parent.height / 2

        startX: 10
        startY: 100
        delegate: [
            PathArc {
                relativeX: 50
                y: 100
                radiusX: 25
                radiusY: 25
            },
            PathArc {
                relativeX: 50
                y: 100
                radiusX: 25
                radiusY: 35
            },
            PathArc {
                relativeX: 50
                y: 100
                radiusX: 25
                radiusY: 60
            },
            PathArc {
                relativeX: 50
                y: 100
                radiusX: 50
                radiusY: 120
            }
        ]
    }

    ControlledShape {
        width: parent.width
        height: parent.height / 2
        anchors.bottom: parent.bottom
        anchors.right: parent.right

        fillColor: "transparent"
        strokeColor: "darkBlue"
        strokeWidth: 20
        capStyle: ShapePath.RoundCap

        delegate: [
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
        ]
    }
}
