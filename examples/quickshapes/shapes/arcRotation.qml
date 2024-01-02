// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256

    Repeater {
        model: 2
        delegate: Shape {
            id: delegate1

            required property int index

            width: 200
            height: 200
            anchors.centerIn: parent

            ShapePath {
                fillColor: "transparent"
                strokeColor: delegate1.index === 0 ? "red" : "blue"
                strokeStyle: ShapePath.DashLine
                strokeWidth: 4

                startX: 50
                startY: 100
                PathArc {
                    x: 150
                    y: 100
                    radiusX: 50
                    radiusY: 20
                    xAxisRotation: delegate1.index === 0 ? 0 : 45
                }
            }
        }
    }

    Repeater {
        model: 2
        delegate: Shape {
            id: delegate2

            required property int index

            width: 200
            height: 200
            anchors.centerIn: parent

            ShapePath {
                fillColor: "transparent"
                strokeColor: delegate2.index === 0 ? "red" : "blue"

                startX: 50
                startY: 100
                PathArc {
                    x: 150
                    y: 100
                    radiusX: 50
                    radiusY: 20
                    xAxisRotation: delegate2.index === 0 ? 0 : 45
                    direction: PathArc.Counterclockwise
                }
            }
        }
    }

    Column {
        anchors.right: parent.right
        Text {
            text: qsTr("0 degrees")
            color: "red"
        }
        Text {
            text: qsTr("45 degrees")
            color: "blue"
        }
    }
}
