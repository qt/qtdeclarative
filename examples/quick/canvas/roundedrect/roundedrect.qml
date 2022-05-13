// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import "../"

Item {
    id:container
    width: 320
    height: 480

    Column {
        spacing: 6
        anchors.fill: parent
        anchors.topMargin: 12

        Label {
            font.pointSize: 24
            font.bold: true
            text: "Rounded rectangle"
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#777"
        }
        Canvas {
            id: canvas
            width: 320
            height: 280
            antialiasing: true

            property int radius: rCtrl.value
            property int rectx: 60
            property int recty: 60
            property int rectWidth: width - 2*rectx
            property int rectHeight: height - 2*recty
            property color strokeStyle:  Qt.darker(fillStyle, 1.4)
            property color fillStyle: "#ae32a0" // purple
            property int lineWidth: lineWidthCtrl.value
            property bool fill: true
            property bool stroke: true
            property real alpha: 1.0

            onLineWidthChanged:requestPaint();
            onFillChanged:requestPaint();
            onStrokeChanged:requestPaint();
            onRadiusChanged:requestPaint();

            onPaint: {
                var ctx = getContext("2d");
                ctx.save();
                ctx.clearRect(0,0,canvas.width, canvas.height);
                ctx.strokeStyle = canvas.strokeStyle;
                ctx.lineWidth = canvas.lineWidth
                ctx.fillStyle = canvas.fillStyle
                ctx.globalAlpha = canvas.alpha
                ctx.beginPath();
                ctx.moveTo(rectx+radius,recty);                 // top side
                ctx.lineTo(rectx+rectWidth-radius,recty);
                // draw top right corner
                ctx.arcTo(rectx+rectWidth,recty,rectx+rectWidth,recty+radius,radius);
                ctx.lineTo(rectx+rectWidth,recty+rectHeight-radius);    // right side
                // draw bottom right corner
                ctx.arcTo(rectx+rectWidth,recty+rectHeight,rectx+rectWidth-radius,recty+rectHeight,radius);
                ctx.lineTo(rectx+radius,recty+rectHeight);              // bottom side
                // draw bottom left corner
                ctx.arcTo(rectx,recty+rectHeight,rectx,recty+rectHeight-radius,radius);
                ctx.lineTo(rectx,recty+radius);                 // left side
                // draw top left corner
                ctx.arcTo(rectx,recty,rectx+radius,recty,radius);
                ctx.closePath();
                if (canvas.fill)
                    ctx.fill();
                if (canvas.stroke)
                    ctx.stroke();
                ctx.restore();
            }
        }
    }
    Column {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12

        LabeledSlider {id: lineWidthCtrl ; min: 1 ; max: 10; init: 2 ; name: "Outline"; width: container.width}
        LabeledSlider {id: rCtrl ; min: 10 ; max: 80 ; init: 40 ; name: "Radius"; width: container.width}
    }
}
