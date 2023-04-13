// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import "../"
import "tiger.js" as Tiger

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
            text: qsTr("Tiger with SVG path")
            color: Qt.lighter(palette.text)
            font {
                pointSize: 24
                bold: true
            }
        }

        Canvas {
            id: canvas

            readonly property real alpha: alphaCtrl.value
            readonly property real scale: scaleCtrl.value
            readonly property real rotate: rotateCtrl.value

            width: root.width
            height: parent.height - controls.height
            antialiasing: true

            onAlphaChanged: requestPaint()
            onScaleChanged: requestPaint()
            onRotateChanged: requestPaint()

            onPaint: function() {
                let ctx = canvas.getContext('2d')
                const originX = canvas.width / 2 + 30
                const originY = canvas.height / 2 + 60

                ctx.save()
                ctx.clearRect(0, 0, canvas.width, canvas.height)
                ctx.globalAlpha = canvas.alpha
                ctx.globalCompositeOperation = "source-over"

                ctx.translate(originX, originY)
                ctx.scale(canvas.scale, canvas.scale)
                ctx.rotate(canvas.rotate)
                ctx.translate(-originX, -originY)

                ctx.strokeStyle = Qt.rgba(.3, .3, .3,1)
                ctx.lineWidth = 1

                //! [0]
                for (let i = 0; i < Tiger.tiger.length; i++) {
                    if (Tiger.tiger[i].width !== undefined)
                        ctx.lineWidth = Tiger.tiger[i].width

                    if (Tiger.tiger[i].path !== undefined)
                        ctx.path = Tiger.tiger[i].path

                    if (Tiger.tiger[i].fill !== undefined) {
                        ctx.fillStyle = Tiger.tiger[i].fill
                        ctx.fill()
                    }

                    if (Tiger.tiger[i].stroke !== undefined) {
                        ctx.strokeStyle = Tiger.tiger[i].stroke
                        ctx.stroke()
                    }
                }
                //! [0]
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
            id: scaleCtrl
            name: qsTr("Scale")
            width: root.width
            min: 0.1
            max: 1
            value: 0.3
        }
        LabeledSlider {
            id: rotateCtrl
            name: qsTr("Rotate")
            width: root.width
            min: 0
            max: Math.PI * 2
            value: 0
        }
        LabeledSlider {
            id: alphaCtrl
            name: qsTr("Alpha")
            width: root.width
            min: 0
            max: 1
            value: 1
        }
    }
}
