// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256

    Shape {
        width: 120
        height: 120
        anchors.centerIn: parent

        ShapePath {
            id: joinTest

            strokeColor: "black"
            strokeWidth: 16
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            property int joinStyleIdx: 0
            readonly property variant styles: [ ShapePath.BevelJoin, ShapePath.MiterJoin, ShapePath.RoundJoin ]
            readonly property variant styleTexts: [ qsTr("BevelJoin"), qsTr("MiterJoin"), qsTr("RoundJoin") ]

            joinStyle: styles[joinStyleIdx]

            startX: 30
            startY: 30
            PathLine {
                x: 100
                y: 100
            }
            PathLine {
                x: 30
                y: 100
            }
        }
    }

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: joinTest.joinStyleIdx = (joinTest.joinStyleIdx + 1) % joinTest.styles.length
    }

    Text {
        id: txt
        anchors.right: parent.right
        text: joinTest.styleTexts[joinTest.joinStyleIdx]
    }
}
