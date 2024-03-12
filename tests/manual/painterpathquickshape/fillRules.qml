// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

ControlledShape {
    id: star
    strokeColor: "blue"
    fillColor: "magenta"
    strokeWidth: 2
    delegate: [
        PathMove {
            x: 90
            y: 50
        },
        PathLine {
            x: 50 + 40 * Math.cos(0.8 * 1 * Math.PI)
            y: 50 + 40 * Math.sin(0.8 * 1 * Math.PI)
        },
        PathLine {
            x: 50 + 40 * Math.cos(0.8 * 2 * Math.PI)
            y: 50 + 40 * Math.sin(0.8 * 2 * Math.PI)
        },
        PathLine {
            x: 50 + 40 * Math.cos(0.8 * 3 * Math.PI)
            y: 50 + 40 * Math.sin(0.8 * 3 * Math.PI)
        },
        PathLine {
            x: 50 + 40 * Math.cos(0.8 * 4 * Math.PI)
            y: 50 + 40 * Math.sin(0.8 * 4 * Math.PI)
        },
        PathLine {
            x: 90
            y: 50
        }
    ]
    Timer {
        interval: 2000
        onTriggered: star.fillRule = (star.fillRule === ShapePath.OddEvenFill ? ShapePath.WindingFill : ShapePath.OddEvenFill)
        repeat: true
        running: true
    }
}
