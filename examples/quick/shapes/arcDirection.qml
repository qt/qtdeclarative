// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256

    Rectangle {
        width: 100
        height: 100
        anchors.centerIn: parent
        border.color: "gray"

        Repeater {
            model: 2
            Shape {
                anchors.fill: parent

                ShapePath {
                    fillColor: "transparent"
                    strokeColor: model.index === 0 ? "red" : "blue"
                    strokeStyle: ShapePath.DashLine
                    strokeWidth: 4

                    startX: 4; startY: 4
                    PathArc {
                        id: arc
                        x: 96; y: 96
                        radiusX: 100; radiusY: 100
                        direction: model.index === 0 ? PathArc.Clockwise : PathArc.Counterclockwise
                    }
                }
            }
        }
    }

    Column {
        anchors.right: parent.right
        Text {
            text: "Clockwise (sweep 1)"
            color: "red"
        }
        Text {
            text: "Counter clockwise (sweep 0)"
            color: "blue"
        }
    }
}
