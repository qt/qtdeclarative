// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import "../"

Item {
    id: container
    width: 320
    height: 480

    Column {
        spacing: 6
        anchors.fill: parent
        anchors.topMargin: 12

        Label {
            font.pointSize: 24
            font.bold: true
            text: "Smile with arcs"
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#777"
        }

        Canvas {
            id: canvas
            width: 320
            height: parent.height - controls.height
            antialiasing: true

            property color strokeStyle:  Qt.darker(fillStyle, 1.6)
            property color fillStyle: "#e0c31e" // yellow
            property int lineWidth: lineWidthCtrl.value
            property bool fill: true
            property bool stroke: true
            property real scale : scaleCtrl.value
            property real rotate : rotateCtrl.value

            onLineWidthChanged:requestPaint();
            onFillChanged:requestPaint();
            onStrokeChanged:requestPaint();
            onScaleChanged:requestPaint();
            onRotateChanged:requestPaint();

            onPaint: {
                var ctx = canvas.getContext('2d');
                var originX = 85
                var originY = 75
                ctx.save();
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                ctx.translate(originX, originX);
                ctx.strokeStyle = canvas.strokeStyle;
                ctx.fillStyle = canvas.fillStyle;
                ctx.lineWidth = canvas.lineWidth;

                ctx.translate(originX, originY)
                ctx.scale(canvas.scale, canvas.scale);
                ctx.rotate(canvas.rotate);
                ctx.translate(-originX, -originY)

                ctx.beginPath();
                ctx.moveTo(75 + 50  * Math.cos(0),
                           75 - 50  * Math.sin(Math.PI*2));
                ctx.arc(75,75,50,0,Math.PI*2,true); // Outer circle
                ctx.moveTo(60,60);
                ctx.arc(60,60,5,0,Math.PI*2,true);  // Left eye
                ctx.moveTo(90 + 5  * Math.cos(0),
                           65 - 5  * Math.sin(Math.PI*2));
                ctx.moveTo(90,60);
                ctx.arc(90,60,5,-Math.PI,Math.PI*3,false);  // Right eye
                ctx.moveTo(75,70);
                ctx.arc(75,70,35,0,Math.PI,false);   // Mouth (clockwise)
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
        id: controls
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12

        LabeledSlider {id: lineWidthCtrl ; min: 1 ; max: 10 ; init: 2 ; name: "Outline"; width: container.width}
        LabeledSlider {id: scaleCtrl ; min: 0.1 ; max: 10 ; init: 1 ; name: "Scale"; width: container.width}
        LabeledSlider {id: rotateCtrl ; min: 0 ; max: Math.PI*2 ; init: 0 ; name: "Rotate"; width: container.width}
    }
}
