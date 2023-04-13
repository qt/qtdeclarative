// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import "../"

Item {
    id: root
    width: 320
    height: 480

    Column {
        spacing: 6
        anchors {
            fill: parent
            topMargin: 12
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Squircles")
            color: Qt.lighter(palette.text)
            font {
                pointSize: 24
                bold: true
            }
        }

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "squircle.png"
            width: 250
            height: 25
        }

        Canvas {
            id: canvas

            readonly property color strokeStyle: Qt.darker(fillStyle, 1.2)
            readonly property color fillStyle: "#6400aa"
            readonly property int lineWidth: 2
            readonly property alias nSize: nCtrl.value
            readonly property alias radius: rCtrl.value
            readonly property alias fill: toggleFillCheckBox.checked
            readonly property alias stroke: toggleStrokeCheckBox.checked
            readonly property real px: width / 2
            readonly property real py: height / 2 + 10
            readonly property real alpha: 1.0

            width: root.width
            height: parent.height - controls.height
            antialiasing: true

            onRadiusChanged: requestPaint()
            onLineWidthChanged: requestPaint()
            onNSizeChanged: requestPaint()
            onFillChanged: requestPaint()
            onStrokeChanged: requestPaint()

            onPaint: function () {
                let ctx = canvas.getContext("2d")
                const N = Math.abs(canvas.nSize)
                const R = canvas.radius

                const M = Math.max(0.00000000001, Math.min(N, 100))

                ctx.save()
                ctx.globalAlpha = canvas.alpha
                ctx.fillStyle = "white"
                ctx.fillRect(0, 0, canvas.width, canvas.height)

                ctx.strokeStyle = canvas.strokeStyle
                ctx.fillStyle = canvas.fillStyle
                ctx.lineWidth = canvas.lineWidth

                ctx.beginPath()
                let i, x, y;
                for (i = 0; i < (2 * R + 1); i++) {
                    x = Math.round(i - R) + canvas.px
                    y = Math.round(Math.pow(Math.abs(Math.pow(R, M) - Math.pow(Math.abs(i - R), M)), 1 / M)) + canvas.py

                    if (i === 0)
                        ctx.moveTo(x, y)
                    else
                        ctx.lineTo(x, y)
                }

                for (i = (2 * R); i < (4 * R + 1); i++) {
                    x = Math.round(3 * R - i) + canvas.px
                    y = Math.round(-Math.pow(Math.abs(Math.pow(R, M) - Math.pow(Math.abs(3 * R - i), M)), 1 / M)) + canvas.py
                    ctx.lineTo(x, y)
                }
                ctx.closePath()
                if (canvas.stroke) {
                    ctx.stroke()
                }
                if (canvas.fill) {
                    ctx.fill()
                }
                ctx.restore()
            }
        }

    }
    Column {
        id: controls
        anchors {
            bottom: parent.bottom
            bottomMargin: 12
        }

        LabeledSlider {
            id: nCtrl
            name: qsTr("N")
            width: root.width
            min: 1
            max: 10
            value: 2
        }
        LabeledSlider {
            id: rCtrl
            name: qsTr("Radius")
            width: root.width
            min: 30
            max: 180
            value: 60
        }
        Row {
            CheckBox {
                id: toggleFillCheckBox
                text: qsTr("Toggle fill")
            }
            CheckBox {
                id: toggleStrokeCheckBox
                checked: true
                text: qsTr("Toggle stroke")
            }
        }
    }
}
