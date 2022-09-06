// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

ControlledShape {
    strokeColor: "black"
    strokeWidth: 16
    fillColor: "transparent"
    capStyle: ShapePath.RoundCap

    //readonly property variant styles: [ ShapePath.BevelJoin, ShapePath.MiterJoin, ShapePath.RoundJoin ]
    //joinStyle: styles[joinStyleIdx]

    property int joinStyleIdx: 0
    readonly property variant styles: [ ShapePath.BevelJoin, ShapePath.MiterJoin, ShapePath.RoundJoin ]
    readonly property variant styleTexts: [ qsTr("BevelJoin"), qsTr("MiterJoin"), qsTr("RoundJoin") ]

    startX: 30
    startY: 30

    delegate: [
        PathLine {
            x: 100
            y: 100
        },
        PathLine {
            x: 30
            y: 100
        }
    ]

//    Timer {
//        interval: 1000
//        repeat: true
//        running: true
//        onTriggered: joinTest.joinStyleIdx = (joinTest.joinStyleIdx + 1) % joinTest.styles.length
//    }
}
