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
            text: qsTr("Bezier Curve")
            color: Qt.lighter(palette.text)
            font {
                pointSize: 24
                bold: true
            }
        }

        Canvas {
            id: canvas

            readonly property color strokeStyle:  Qt.darker(fillStyle, 1.4)
            readonly property color fillStyle: "#b40000" // red
            readonly property alias lineWidth: lineWidthCtrl.value
            readonly property alias fill: toggleFillCheckBox.checked
            readonly property alias stroke: toggleStrokeCheckBox.checked
            readonly property real alpha: 1.0
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
                const originX = 85
                const originY = 75
                ctx.save()
                ctx.clearRect(0, 0, canvas.width, canvas.height)
                ctx.translate(originX, originX)
                ctx.globalAlpha = canvas.alpha
                ctx.strokeStyle = canvas.strokeStyle
                ctx.fillStyle = canvas.fillStyle
                ctx.lineWidth = canvas.lineWidth

                ctx.translate(originX, originY)
                ctx.scale(canvas.scale, canvas.scale)
                ctx.rotate(canvas.rotate)
                ctx.translate(-originX, -originY)

                //! [0]
                ctx.beginPath()
                ctx.moveTo(75,40)
                ctx.bezierCurveTo(75,37,70,25,50,25)
                ctx.bezierCurveTo(20,25,20,62.5,20,62.5)
                ctx.bezierCurveTo(20,80,40,102,75,120)
                ctx.bezierCurveTo(110,102,130,80,130,62.5)
                ctx.bezierCurveTo(130,62.5,130,25,100,25)
                ctx.bezierCurveTo(85,25,75,37,75,40)
                ctx.closePath()
                //! [0]
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
            min: 1
            max: 10
            value: 2
            width: root.width
        }

        LabeledSlider {
            id: scaleCtrl
            name: qsTr("Scale")
            min: 0.1
            max: 10
            value: 1
            width: root.width
        }

        LabeledSlider {
            id: rotateCtrl
            name: qsTr("Rotate")
            min: 0
            max: Math.PI * 2
            value: 0
            width: root.width
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
