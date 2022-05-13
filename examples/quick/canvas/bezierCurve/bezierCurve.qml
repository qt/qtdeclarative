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
        spacing:5
        anchors.fill: parent
        anchors.topMargin: 12

        Label {
            font.pointSize: 24
            font.bold: true
            text: "Bezier Curve"
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#777"
        }

        Canvas {
            id:canvas
            width:320
            height:280
            property color strokeStyle:  Qt.darker(fillStyle, 1.4)
            property color fillStyle: "#b40000" // red
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
                var originX = 85
                var originY = 75
                ctx.save();
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                ctx.translate(originX, originX);
                ctx.globalAlpha = canvas.alpha;
                ctx.strokeStyle = canvas.strokeStyle;
                ctx.fillStyle = canvas.fillStyle;
                ctx.lineWidth = canvas.lineWidth;

                ctx.translate(originX, originY)
                ctx.scale(canvas.scale, canvas.scale);
                ctx.rotate(canvas.rotate);
                ctx.translate(-originX, -originY)

                //! [0]
                ctx.beginPath();
                ctx.moveTo(75,40);
                ctx.bezierCurveTo(75,37,70,25,50,25);
                ctx.bezierCurveTo(20,25,20,62.5,20,62.5);
                ctx.bezierCurveTo(20,80,40,102,75,120);
                ctx.bezierCurveTo(110,102,130,80,130,62.5);
                ctx.bezierCurveTo(130,62.5,130,25,100,25);
                ctx.bezierCurveTo(85,25,75,37,75,40);
                ctx.closePath();
                //! [0]
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

        LabeledSlider {
            id: lineWidthCtrl; name: "Outline"; min: 1; max: 10; init: 2; width: container.width
        }

        LabeledSlider {
            id: scaleCtrl; name: "Scale"; min: 0.1; max: 10; init: 1; width: container.width
        }

        LabeledSlider {
            id: rotateCtrl; name: "Rotate"; min: 0; max: Math.PI*2; init: 0; width: container.width
        }
    }
}
