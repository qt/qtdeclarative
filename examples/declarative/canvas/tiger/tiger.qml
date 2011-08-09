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
        smooth:false
        renderTarget:Canvas.Image
        threadRendering:false
        property string strokeStyle:"steelblue"
        property string fillStyle:"yellow"
        property int lineWidth:lineWidthCtrl.value
        property bool fill:true
        property bool stroke:true
        property real alpha:alphaCtrl.value
        property real scaleX : scaleXCtrl.value
        property real scaleY : scaleYCtrl.value
        property real rotate : rotateCtrl.value
        property int frame:0

        onLineWidthChanged: requestPaint();
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
            ctx.fillStyle = "rgba(0,0,0,0)";
            ctx.fillRect(0, 0, canvas.width, canvas.height);
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
          Slider {id:lineWidthCtrl; width:300; height:30; min:1; max:10; init:2; name:"Line width"}
          Slider {id:scaleXCtrl; width:300; height:30; min:0.1; max:10; init:0.5; name:"ScaleX"}
          Slider {id:scaleYCtrl; width:300; height:30; min:0.1; max:10; init:0.5; name:"ScaleY"}
          Slider {id:rotateCtrl; width:300; height:30; min:0; max:Math.PI*2; init:0; name:"Rotate"}
          Slider {id:alphaCtrl; width:300; height:30; min:0; max:1; init:1; name:"Alpha"}
        }
    }
  }
}
