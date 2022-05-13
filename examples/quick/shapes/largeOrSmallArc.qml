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
        Shape {
            width: 200
            height: 200
            anchors.centerIn: parent

            ShapePath {
                fillColor: "transparent"
                strokeColor: model.index === 0 ? "red" : "blue"
                strokeStyle: ShapePath.DashLine
                strokeWidth: 4

                startX: 50; startY: 100
                PathArc {
                    x: 100; y: 150
                    radiusX: 50; radiusY: 50
                    useLargeArc: model.index === 1
                }
            }
        }
    }

    Column {
        anchors.right: parent.right
        Text {
            text: "Small"
            color: "red"
        }
        Text {
            text: "Large"
            color: "blue"
        }
    }
}
