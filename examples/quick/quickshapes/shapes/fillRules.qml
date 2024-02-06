// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256
    Shape {
        width: 100
        height: 100
        anchors.centerIn: parent
        ShapePath {
            id: star
            strokeColor: "blue"
            fillColor: "magenta"
            strokeWidth: 2
            PathMove {
                x: 90
                y: 50
            }
            PathLine {
                x: 50 + 40 * Math.cos(0.8 * 1 * Math.PI)
                y: 50 + 40 * Math.sin(0.8 * 1 * Math.PI)
            }
            PathLine {
                x: 50 + 40 * Math.cos(0.8 * 2 * Math.PI)
                y: 50 + 40 * Math.sin(0.8 * 2 * Math.PI)
            }
            PathLine {
                x: 50 + 40 * Math.cos(0.8 * 3 * Math.PI)
                y: 50 + 40 * Math.sin(0.8 * 3 * Math.PI)
            }
            PathLine {
                x: 50 + 40 * Math.cos(0.8 * 4 * Math.PI)
                y: 50 + 40 * Math.sin(0.8 * 4 * Math.PI)
            }
            PathLine {
                x: 90
                y: 50
            }
        }
        NumberAnimation on rotation {
            from: 0
            to: 360
            duration: 5000
            loops: Animation.Infinite
        }
    }
    Timer {
        interval: 2000
        onTriggered: star.fillRule = (star.fillRule === ShapePath.OddEvenFill ? ShapePath.WindingFill : ShapePath.OddEvenFill)
        repeat: true
        running: true
    }
    Text {
        anchors.right: parent.right
        text: star.fillRule === ShapePath.OddEvenFill ? qsTr("OddEvenFill") : qsTr("WindingFill")
    }
}
