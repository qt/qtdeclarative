import QtQuick 2.0
import "../contents"
Item {
  id:container
  width:360
  height:600

  Column {
    spacing:5
    anchors.fill:parent
    Text { font.pointSize:25; text:"Rounded rectangle"; anchors.horizontalCenter:parent.horizontalCenter}
    Canvas {
        id:canvas
        width:360
        height:360
        smooth:true
        renderTarget:Canvas.Image
        threadRendering:false

        property int radius: rCtrl.value
        property int rectx: rxCtrl.value
        property int recty: ryCtrl.value
        property int rectWidth: width - 2*rectx
        property int rectHeight: height - 2*recty
        property string strokeStyle:"blue"
        property string fillStyle:"steelblue"
        property int lineWidth:lineWidthCtrl.value
        property bool fill:true
        property bool stroke:true
        property real alpha:alphaCtrl.value

        onLineWidthChanged:requestPaint();
        onFillChanged:requestPaint();
        onStrokeChanged:requestPaint();
        onRadiusChanged:requestPaint();
        onRectxChanged:requestPaint();
        onRectyChanged:requestPaint();
        onAlphaChanged:requestPaint();

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
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
        }
    }

    Rectangle {
        id:controls
        width:360
        height:160
        Column {
          spacing:3
          Slider {id:lineWidthCtrl; width:300; height:30; min:1; max:10; init:2; name:"Line width"}
          Slider {id:rxCtrl; width:300; height:30; min:5; max:30; init:10; name:"rectx"}
          Slider {id:ryCtrl; width:300; height:30; min:5; max:30; init:10; name:"recty"}
          Slider {id:rCtrl; width:300; height:30; min:10; max:100; init:40; name:"Radius"}
          Slider {id:alphaCtrl; width:300; height:30; min:0; max:1; init:1; name:"Alpha"}
        }
    }
  }
}
