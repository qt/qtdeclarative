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
            text: qsTr("Smile with arcs")
            font {
                pointSize: 24
                bold: true
            }
            color: Qt.lighter(palette.text)
        }

        Canvas {
            id: canvas

            readonly property color strokeStyle:  Qt.darker(fillStyle, 1.6)
            readonly property color fillStyle: "#e0c31e" // yellow
            readonly property alias lineWidth: lineWidthCtrl.value
            readonly property alias fill: toggleFillCheckBox.checked
            readonly property alias stroke: toggleStrokeCheckBox.checked
            readonly property alias scale: scaleCtrl.value
            readonly property alias rotate: rotateCtrl.value

            width: root.width
            height: parent.height - controls.height
            antialiasing: true

            onLineWidthChanged: requestPaint()
            onFillChanged: requestPaint()
            onStrokeChanged: requestPaint()
            onScaleChanged: requestPaint()
            onRotateChanged: requestPaint()

            onPaint: {
                let ctx = canvas.getContext('2d')
                const originX = 85
                const originY = 75
                ctx.save()
                ctx.clearRect(0, 0, canvas.width, canvas.height)
                ctx.translate(originX, originX)
                ctx.strokeStyle = canvas.strokeStyle
                ctx.fillStyle = canvas.fillStyle
                ctx.lineWidth = canvas.lineWidth

                ctx.translate(originX, originY)
                ctx.scale(canvas.scale, canvas.scale)
                ctx.rotate(canvas.rotate)
                ctx.translate(-originX, -originY)

                ctx.beginPath()
                ctx.moveTo(75 + 50 * Math.cos(0), 75 - 50 * Math.sin(Math.PI * 2))
                ctx.arc(75, 75, 50, 0, Math.PI * 2, true) // Outer circle
                ctx.moveTo(60, 60)
                ctx.arc(60, 60, 5, 0, Math.PI * 2, true)  // Left eye
                ctx.moveTo(90 + 5 * Math.cos(0), 65 - 5 * Math.sin(Math.PI * 2))
                ctx.moveTo(90, 60)
                ctx.arc(90, 60, 5, -Math.PI, Math.PI * 3, false)  // Right eye
                ctx.moveTo(75, 70)
                ctx.arc(75, 70, 35, 0, Math.PI, false)   // Mouth (clockwise)
                ctx.closePath()
                if (canvas.fill)
                    ctx.fill()
                if (canvas.stroke)
                    ctx.stroke()
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
            id: lineWidthCtrl
            name: qsTr("Outline")
            width: root.width
            min: 1
            max: 10
            value: 2
        }
        LabeledSlider {
            id: scaleCtrl
            name: qsTr("Scale")
            width: root.width
            min: 0.1
            max: 10
            value: 1
        }
        LabeledSlider {
            id: rotateCtrl
            name: qsTr("Rotate")
            width: root.width
            min: 0
            max: Math.PI * 2
            value: 0
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
