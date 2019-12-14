/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the manual tests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "content"

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
                            "resources/cursor-pencil.png" : "resources/cursor-felt-marker.png"
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
                source: "resources/cursor-airbrush.png"
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
                source: "resources/cursor-eraser.png"
                visible: eraserHandler.hovered
                x: eraserHandler.point.position.x
                y: eraserHandler.point.position.y - 32
            }
        }
    }
}
