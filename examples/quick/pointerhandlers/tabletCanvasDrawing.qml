// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import "components"

Rectangle {
    width: 1024
    height: 1024
    color: "#444"

    ColumnLayout {
        x: -15; width: 80
        height: parent.height
        Slider {
            id: hueSlider
            width: 80; height: 200
            Layout.fillHeight: true
            label: "hue"
            Rectangle {
                color: "beige"
                width: 30
                height: 25
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.horizontalCenter: parent.horizontalCenter
                Rectangle {
                    border.color: "white"
                    color: canvas.drawingColor
                    anchors.fill: parent
                }
            }
        }
        Slider {
            id: saturationSlider
            width: 80; height: 200
            Layout.fillHeight: true
            label: "sat"
        }
        Slider {
            id: lightnessSlider
            width: 80; height: 200
            Layout.fillHeight: true
            label: "light"
        }
    }

    ColumnLayout {
        x: parent.width - 65; width: 80
        height: parent.height
        Slider {
            id: widthSlider
            width: 80; height: 200
            Layout.fillHeight: true
            label: "width"
        }
        Slider {
            id: alphaSlider
            width: 80; height: 200
            Layout.fillHeight: true
            label: "alpha"
        }
    }

    Rectangle {
        id: rect
        width: 640
        height: 480
        color: "beige"
        anchors {
            fill: parent
            margins: 10
            leftMargin: 50
            rightMargin: 50
        }

        Canvas {
            id: canvas
            anchors.fill: parent
            antialiasing: true
            renderTarget: Canvas.FramebufferObject
            property color drawingColor: Qt.hsla(hueSlider.value / 100.0,
                                                 saturationSlider.value / 100.0,
                                                 lightnessSlider.value / 100.0,
                                                 alphaSlider.value / 100.0)
            property var points: []
            property var pressures: []
            property var pointerType: PointerDevice.Pen
            onPaint: {
                if (points.length < 2)
                    return
                var ctx = canvas.getContext('2d');
                ctx.save()
                ctx.strokeStyle = pointerType === PointerDevice.Pen ? drawingColor : "beige"
                ctx.lineCap = "round"
                if (pressures.length === points.length) {
                    for (var i = 1; i < points.length; i++) {
                        ctx.lineWidth = pressures[i] * widthSlider.value
                        ctx.beginPath()
                        ctx.moveTo(points[i - 1].x, points[i - 1].y)
                        ctx.lineTo(points[i].x, points[i].y)
                        ctx.stroke()
                    }
                    points = points.slice(points.length - 2, 1)
                    pressures = pressures.slice(pressures.length - 2, 1)
                } else {
                    ctx.beginPath()
                    ctx.moveTo(points[0].x, points[0].y)
                    for (var i = 1; i < points.length; i++)
                        ctx.lineTo(points[i].x, points[i].y)
                    ctx.lineWidth = widthSlider
                    ctx.stroke()
                    points = points.slice(points.length - 2, 1)
                    pressures = []
                }
                ctx.restore()
            }
        }

        Text {
            text: "draw with a drawing-tablet stylus"
            anchors {
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
                margins: 6
            }
        }

        PointHandler {
            acceptedPointerTypes: PointerDevice.Pen
            onActiveChanged:
                if (active) {
                    canvas.pointerType = PointerDevice.Pen
                } else {
                    canvas.points = []
                    canvas.pressures = []
                }
            onPointChanged:
                if (active) {
                    canvas.points.push(point.position)
                    canvas.pressures.push(point.pressure)
                    canvas.requestPaint()
                }
        }

        PointHandler {
            acceptedPointerTypes: PointerDevice.Eraser
            onActiveChanged:
                if (active) {
                    canvas.pointerType = PointerDevice.Eraser
                } else {
                    canvas.points = []
                    canvas.pressures = []
                }
            onPointChanged:
                if (active) {
                    canvas.points.push(point.position)
                    canvas.pressures.push(point.pressure)
                    canvas.requestPaint()
                }
        }

        HoverHandler {
            id: stylusHandler
            acceptedDevices: PointerDevice.Stylus
            acceptedPointerTypes: PointerDevice.Pen
            target: Image {
                parent: rect
                source: stylusHandler.point.rotation === 0 ?
                            "images/cursor-pencil.png" : "images/cursor-felt-marker.png"
                visible: stylusHandler.hovered
                rotation: stylusHandler.point.rotation
                x: stylusHandler.point.position.x
                y: stylusHandler.point.position.y
            }
        }

        HoverHandler {
            id: airbrushHandler
            acceptedDevices: PointerDevice.Airbrush
            acceptedPointerTypes: PointerDevice.Pen
            target: Image {
                parent: rect
                source: "images/cursor-airbrush.png"
                visible: airbrushHandler.hovered
                x: airbrushHandler.point.position.x
                y: airbrushHandler.point.position.y
            }
        }

        HoverHandler {
            id: eraserHandler
            acceptedPointerTypes: PointerDevice.Eraser
            target: Image {
                parent: rect
                source: "images/cursor-eraser.png"
                visible: eraserHandler.hovered
                x: eraserHandler.point.position.x
                y: eraserHandler.point.position.y - 32
            }
        }
    }
}
