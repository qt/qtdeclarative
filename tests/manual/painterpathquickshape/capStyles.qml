// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

ControlledShape {
    id: capTest
    strokeColor: "green"
    strokeWidth: 20
    fillColor: "transparent"

    property int capStyleIdx: 0
    readonly property variant styles: [ ShapePath.FlatCap, ShapePath.SquareCap, ShapePath.RoundCap ]
    readonly property variant styleTexts: [ qsTr("FlatCap"), qsTr("SquareCap"), qsTr("RoundCap") ]

    capStyle: styles[capStyleIdx]

    startX: 40
    startY: 30
    delegate: [
        PathQuad {
            x: 50
            y: 80
            controlX: 0
            controlY: 80
        },
        PathLine {
            x: 150
            y: 80
        },
        PathQuad {
            x: 160
            y: 30
            controlX: 200
            controlY: 80
        }
    ]

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: capTest.capStyleIdx = (capTest.capStyleIdx + 1) % capTest.styles.length
    }
}
