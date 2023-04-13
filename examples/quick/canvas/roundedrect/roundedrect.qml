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
            text: qsTr("Rounded rectangle")
            color: Qt.lighter(palette.text)
            font {
                pointSize: 24
                bold: true
            }
        }
        Canvas {
            id: canvas

            readonly property alias radius: rCtrl.value
            readonly property int rectx: 60
            readonly property int recty: 60
            readonly property int rectWidth: width - 2 * rectx
            readonly property int rectHeight: height - 2 * recty
            readonly property color strokeStyle:  Qt.darker(fillStyle, 1.4)
            readonly property color fillStyle: "#ae32a0" // purple
            readonly property alias lineWidth: lineWidthCtrl.value
            readonly property alias fill: toggleFillCheckBox.checked
            readonly property alias stroke: toggleStrokeCheckBox.checked
            readonly property real alpha: 1.0

            width: root.width
            height: parent.height - controls.height
            antialiasing: true

            onLineWidthChanged: requestPaint()
            onFillChanged: requestPaint()
            onStrokeChanged: requestPaint()
            onRadiusChanged: requestPaint()

            onPaint: function() {
                var ctx = getContext("2d")
                ctx.save()
                ctx.clearRect(0, 0, canvas.width, canvas.height)
                ctx.strokeStyle = canvas.strokeStyle
                ctx.lineWidth = canvas.lineWidth
                ctx.fillStyle = canvas.fillStyle
                ctx.globalAlpha = canvas.alpha
                ctx.beginPath()
                ctx.moveTo(rectx + radius, recty)                 // top side
                ctx.lineTo(rectx + rectWidth - radius, recty)
                // draw top right corner
                ctx.arcTo(rectx + rectWidth, recty, rectx + rectWidth, recty + radius, radius)
                ctx.lineTo(rectx + rectWidth, recty + rectHeight - radius)    // right side
                // draw bottom right corner
                ctx.arcTo(rectx + rectWidth, recty + rectHeight, rectx + rectWidth - radius, recty + rectHeight, radius)
                ctx.lineTo(rectx + radius, recty + rectHeight)              // bottom side
                // draw bottom left corner
                ctx.arcTo(rectx, recty + rectHeight, rectx, recty + rectHeight - radius, radius)
                ctx.lineTo(rectx, recty + radius)                 // left side
                // draw top left corner
                ctx.arcTo(rectx, recty, rectx + radius, recty, radius)
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
            id: rCtrl
            name: qsTr("Radius")
            width: root.width
            min: 10
            max: 80
            value: 40
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
