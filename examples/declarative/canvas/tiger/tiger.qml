/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import "../contents"
import "tiger.js" as Tiger
Item {
  id:container
  width:360
  height:600

  Column {
    spacing:5
    anchors.fill:parent
    Text { font.pointSize:25; text:"Tiger with SVG path"; anchors.horizontalCenter:parent.horizontalCenter}

    Canvas {
        id:canvas
        width:360
        height:360
        smooth:true
        renderTarget:Canvas.FramebufferObject
        renderInThread:false
        property string strokeStyle:"steelblue"
        property string fillStyle:"yellow"
        property bool fill:true
        property bool stroke:true
        property real alpha:alphaCtrl.value
        property real scaleX : scaleXCtrl.value
        property real scaleY : scaleYCtrl.value
        property real rotate : rotateCtrl.value
        property int frame:0

        onFillChanged: requestPaint();
        onStrokeChanged: requestPaint();
        onAlphaChanged: requestPaint();
        onScaleXChanged: requestPaint();
        onScaleYChanged: requestPaint();
        onRotateChanged: requestPaint();

        onPainted : {
            canvas.frame++;
            if (canvas.frame < Tiger.tiger.length)
                requestPaint();
        }
        onPaint: {
            var ctx = canvas.getContext('2d');
            ctx.reset();
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.globalAlpha = canvas.alpha;
            ctx.scale(canvas.scaleX, canvas.scaleY);
            ctx.rotate(canvas.rotate);
            ctx.globalCompositeOperation = "source-over";
            ctx.translate(canvas.width/2, canvas.height/2);
            ctx.strokeStyle = Qt.rgba(.3, .3, .3,1);
            ctx.lineWidth = 1;


            for (var i = 0; i < canvas.frame && i < Tiger.tiger.length; i++) {
            if (Tiger.tiger[i].width != undefined)
            ctx.lineWidth = Tiger.tiger[i].width;

            if (Tiger.tiger[i].path != undefined)
            ctx.path = Tiger.tiger[i].path;

            if (Tiger.tiger[i].fill != undefined) {
            ctx.fillStyle = Tiger.tiger[i].fill;
            ctx.fill();
            }

            if (Tiger.tiger[i].stroke != undefined) {
            ctx.strokeStyle = Tiger.tiger[i].stroke;
            ctx.stroke();
            }
        }
    }
    }
    Rectangle {
        id:controls
        width:360
        height:160
        Column {
          spacing:3
          Slider {id:scaleXCtrl; width:300; height:30; min:0.1; max:10; init:0.5; name:"ScaleX"}
          Slider {id:scaleYCtrl; width:300; height:30; min:0.1; max:10; init:0.5; name:"ScaleY"}
          Slider {id:rotateCtrl; width:300; height:30; min:0; max:Math.PI*2; init:0; name:"Rotate"}
          Slider {id:alphaCtrl; width:300; height:30; min:0; max:1; init:1; name:"Alpha"}
        }
    }
  }
}