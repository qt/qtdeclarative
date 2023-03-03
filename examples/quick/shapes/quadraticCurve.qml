// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256

    Item {
        width: 200
        height: 100
        anchors.centerIn: parent

        Shape {
            id: shape
            anchors.fill: parent

            ShapePath {
                strokeWidth: 4
                strokeColor: "black"
                fillColor: "transparent"

                startX: 50
                startY: 50
                PathQuad {
                    x: 150
                    y: 50
                    controlX: cp.x
                    controlY: cp.y
                }
            }
        }

        Rectangle {
            id: cp
            color: "red"
            width: 10
            height: 10
            SequentialAnimation on x {
                loops: Animation.Infinite
                NumberAnimation {
                    from: 0
                    to: shape.width - cp.width
                    duration: 5000
                }
                NumberAnimation {
                    from: shape.width - cp.width
                    to: 0
                    duration: 5000
                }
            }
        }
    }
}
