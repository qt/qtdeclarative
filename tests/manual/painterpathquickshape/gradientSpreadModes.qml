// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

Item {
    ControlledShape {
        anchors.fill: parent
        strokeColor: "transparent"
        startX: 10
        startY: 10

        delegate: [
            PathLine {
                relativeX: 180
                relativeY: 0
            },
            PathLine {
                relativeX: 0
                relativeY: 180
            },
            PathLine {
                relativeX: -180
                relativeY: 0
            },
            PathLine {
                relativeX: 0
                relativeY: -180
            }
        ]
    }

    Timer {
        id: spreadTimer
        interval: 3000
        running: true
        repeat: true
        readonly property variant spreads: [ ShapeGradient.PadSpread, ShapeGradient.RepeatSpread, ShapeGradient.ReflectSpread ]
        readonly property variant spreadTexts: [ qsTr("PadSpread"), qsTr("RepeatSpread"), qsTr("ReflectSpread") ]
        property int spreadIdx: 0
        onTriggered: function() {
            spreadIdx = (spreadIdx + 1) % spreads.length
            grad.spread = spreads[spreadIdx]
        }
    }

    ControlledShape {
        anchors.fill: parent
        strokeColor: "gray"
        strokeWidth: 2
        fillColor: "transparent"
        delegate: [
            PathMove {
                x: 0
                y: 50
            },
            PathLine {
                relativeX: 200
                relativeY: 0
            },
            PathMove {
                x: 0
                y: 150
            },
            PathLine {
                relativeX: 200
                relativeY: 0
            }
        ]
    }
}
