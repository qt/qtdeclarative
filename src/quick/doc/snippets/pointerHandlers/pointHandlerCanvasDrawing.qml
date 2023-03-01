// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Canvas {
    id: canvas
    width: 800
    height: 600
    antialiasing: true
    renderTarget: Canvas.FramebufferObject
    property var points: []
    onPaint: {
        if (points.length < 2)
            return
        var ctx = canvas.getContext('2d');
        ctx.save()
        ctx.strokeStyle = stylusHandler.active ? "blue" : "white"
        ctx.lineCap = "round"
        ctx.beginPath()
        ctx.moveTo(points[0].x, points[0].y)
        for (var i = 1; i < points.length; i++)
            ctx.lineTo(points[i].x, points[i].y)
        ctx.lineWidth = 3
        ctx.stroke()
        points = points.slice(points.length - 2, 1)
        ctx.restore()
    }

    PointHandler {
        id: stylusHandler
        acceptedPointerTypes: PointerDevice.Pen
        onPointChanged: {
            canvas.points.push(point.position)
            canvas.requestPaint()
        }
    }

    PointHandler {
        id: eraserHandler
        acceptedPointerTypes: PointerDevice.Eraser
        onPointChanged: {
            canvas.points.push(point.position)
            canvas.requestPaint()
        }
    }

    Rectangle {
        width: 10; height: 10
        color: stylusHandler.active ? "green" : eraserHandler.active ? "red" : "beige"
    }
}
//![0]
