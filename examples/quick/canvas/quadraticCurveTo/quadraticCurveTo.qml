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
        spacing: 5
        anchors {
            fill: parent
            topMargin: 12
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Quadratic Curve")
            color: Qt.lighter(palette.text)
            font {
                pointSize: 24
                bold: true
            }
        }

        Canvas {
            id: canvas

            readonly property color strokeStyle: Qt.darker(fillStyle, 1.3)
            readonly property color fillStyle: "#14aaff" // blue
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

            onPaint: function() {
                let ctx = canvas.getContext('2d')
                const originX = 75
                const originY = 75

                ctx.save()
                ctx.clearRect(0, 0, canvas.width, canvas.height)
                ctx.translate(originX, originX)
                ctx.strokeStyle = canvas.strokeStyle
                ctx.fillStyle = canvas.fillStyle
                ctx.lineWidth = canvas.lineWidth

                ctx.translate(originX, originY)
                ctx.rotate(canvas.rotate)
                ctx.scale(canvas.scale, canvas.scale)
                ctx.translate(-originX, -originY)

                // ![0]
                ctx.beginPath()
                ctx.moveTo(75, 25)
                ctx.quadraticCurveTo(25, 25, 25, 62.5)
                ctx.quadraticCurveTo(25, 100, 50, 100)
                ctx.quadraticCurveTo(50, 120, 30, 125)
                ctx.quadraticCurveTo(60, 120, 65, 100)
                ctx.quadraticCurveTo(125, 100, 125, 62.5)
                ctx.quadraticCurveTo(125, 25, 75, 25)
                ctx.closePath()
                // ![0]

                if (canvas.fill)
                    ctx.fill()
                if (canvas.stroke)
                    ctx.stroke()

                // ![1]
                ctx.fillStyle = "white"
                ctx.font = "bold 17px sans-serif"
                ctx.fillText("Qt Quick", 40, 70)
                // ![1]
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
                checked: true
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
