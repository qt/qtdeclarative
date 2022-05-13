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
            text: "Squircle with Clip"
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#777"
        }
        Canvas {
            id: canvas
            width: 320
            height: 280
            property color strokeStyle: Qt.darker(fillStyle, 1.2)
            property color fillStyle:"#14aaff" // green
            property int lineWidth:2
            property int nSize:nCtrl.value
            property real radius:rCtrl.value
            property bool fill:true
            property bool stroke:false
            property real px: width/2
            property real py: height/2 + 20
            property real alpha: 1.0
            property string imagefile:"../contents/qt-logo.png"
            antialiasing: true
            Component.onCompleted:loadImage(canvas.imagefile);

            onRadiusChanged:requestPaint();
            onLineWidthChanged:requestPaint();
            onNSizeChanged:requestPaint();
            onFillChanged:requestPaint();
            onStrokeChanged:requestPaint();
            onImageLoaded : requestPaint();
            onPaint: squcirle();

            function squcirle() {
                var ctx = canvas.getContext("2d");
                var N = canvas.nSize;
                var R = canvas.radius;

                N=Math.abs(N);
                var M=N;
                if (N>100) M=100;
                if (N<0.00000000001) M=0.00000000001;

                ctx.save();
                ctx.globalAlpha = canvas.alpha;
                ctx.fillStyle = "white";
                ctx.fillRect(0, 0, canvas.width, canvas.height);

                ctx.strokeStyle = canvas.strokeStyle;
                ctx.fillStyle = canvas.fillStyle;
                ctx.lineWidth = canvas.lineWidth;

                ctx.beginPath();
                var i = 0, x, y;
                for (i=0; i<(2*R+1); i++){
                    x = Math.round(i-R) + canvas.px;
                    y = Math.round(Math.pow(Math.abs(Math.pow(R,M)-Math.pow(Math.abs(i-R),M)),1/M)) + canvas.py;

                    if (i == 0)
                        ctx.moveTo(x, y);
                    else
                        ctx.lineTo(x, y);
                }

                for (i=(2*R); i<(4*R+1); i++){
                    x =Math.round(3*R-i)+canvas.px;
                    y = Math.round(-Math.pow(Math.abs(Math.pow(R,M)-Math.pow(Math.abs(3*R-i),M)),1/M)) + canvas.py;
                    ctx.lineTo(x, y);
                }
                ctx.closePath();
                if (canvas.stroke) {
                    ctx.stroke();
                }

                if (canvas.fill) {
                    ctx.fill();
                }

                //! [0]
                ctx.clip();
                ctx.drawImage(canvas.imagefile, 0, 0);
                //! [0]

                ctx.restore();
            }
        }
    }
    Column {
        id: controls
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12

        LabeledSlider {id: nCtrl; min: 1; max: 10; init: 4; name: "N"; width: container.width}
        LabeledSlider {id: rCtrl; min: 30; max: 180; init: 80; name: "Radius"; width: container.width}
    }
}
