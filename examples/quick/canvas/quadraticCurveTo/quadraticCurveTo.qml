// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import "../"

Item {
    id:container
    width:320
    height:480

    Column {
        spacing:5
        anchors.fill: parent
        anchors.topMargin: 12

        Label {
            font.pointSize: 24
            font.bold: true
            text: "Quadratic Curve"
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#777"
        }

        Canvas {
            id: canvas
            width: 320
            height: 280

            property color strokeStyle:  Qt.darker(fillStyle, 1.3)
            property color fillStyle: "#14aaff" // blue
            property int lineWidth: lineWidthCtrl.value
            property bool fill: true
            property bool stroke: true
            property real alpha: 1.0
            property real scale : scaleCtrl.value
            property real rotate : rotateCtrl.value

            antialiasing: true

            onLineWidthChanged:requestPaint();
            onFillChanged:requestPaint();
            onStrokeChanged:requestPaint();
            onScaleChanged:requestPaint();
            onRotateChanged:requestPaint();

            onPaint: {
                var ctx = canvas.getContext('2d');
                var originX = 75
                var originY = 75

                ctx.save();
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                ctx.translate(originX, originX);
                ctx.strokeStyle = canvas.strokeStyle;
                ctx.fillStyle = canvas.fillStyle;
                ctx.lineWidth = canvas.lineWidth;

                ctx.translate(originX, originY)
                ctx.rotate(canvas.rotate);
                ctx.scale(canvas.scale, canvas.scale);
                ctx.translate(-originX, -originY)

                // ![0]
                ctx.beginPath();
                ctx.moveTo(75,25);
                ctx.quadraticCurveTo(25,25,25,62.5);
                ctx.quadraticCurveTo(25,100,50,100);
                ctx.quadraticCurveTo(50,120,30,125);
                ctx.quadraticCurveTo(60,120,65,100);
                ctx.quadraticCurveTo(125,100,125,62.5);
                ctx.quadraticCurveTo(125,25,75,25);
                ctx.closePath();
                // ![0]

                if (canvas.fill)
                    ctx.fill();
                if (canvas.stroke)
                    ctx.stroke();

                // ![1]
                ctx.fillStyle = "white";
                ctx.font = "bold 17px sans-serif";
                ctx.fillText("Qt Quick", 40, 70);
                // ![1]
                ctx.restore();
            }
        }
    }
    Column {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12

        LabeledSlider {id:lineWidthCtrl; min:1; max:10; init:2; name: "Outline"; width: container.width}
        LabeledSlider {id:scaleCtrl; min:0.1; max:10; init:1; name: "Scale"; width: container.width}
        LabeledSlider {id:rotateCtrl; min:0; max:Math.PI*2; init:0; name: "Rotate"; width: container.width}
    }
}
