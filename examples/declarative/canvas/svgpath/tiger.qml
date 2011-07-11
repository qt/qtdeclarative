import QtQuick 2.0
import "tiger.js" as Tiger

Canvas {
  id:canvas
  width:1900
  height:1100
  fillColor:"#000000"
  focus:true
  renderTarget:PaintedItem.FramebufferObject
  property int frame:0    
  property date paintingTime
  Component.onCompleted: {
    canvas.frame++;
    canvas.requestPaint();
  }
  onPainted: {
      var endPaintingTime = new Date;
      console.log("painting time:" + (endPaintingTime.valueOf() - canvas.paintingTime.valueOf()));
      canvas.frame++;
      if (canvas.frame > Tiger.tiger.length) {
//        canvas.frame = 0;
          canvas.frame = Tiger.tiger.length;
      } else {
          canvas.requestPaint();
      }
  }
  onPaint:{
    canvas.paintingTime = new Date();
    Tiger.draw(context, canvas.frame);
  }
  Keys.onPressed : {
    if (event.key == Qt.Key_Plus) {
       canvas.contentsScale *= 1.5;
    }
    if (event.key == Qt.Key_Minus) {
       canvas.contentsScale *= 0.7;
    }
    if (event.key == Qt.Key_Left) {
       canvas.canvasX +=60;
    }
    if (event.key == Qt.Key_Right) {
       canvas.canvasX -= 60;
    }
    if (event.key == Qt.Key_Up) {
       canvas.canvasY += 40;
    }
    if (event.key == Qt.Key_Down) {
       canvas.canvasY -= 40;
    }

    canvas.requestPaint();
  }
  Rectangle {
      anchors.bottom : parent.bottom
      color: "white"
      opacity:0.7
      height:50
      width:canvas.width
      radius:4
      Text {
         anchors.bottom : parent.bottom
         font.pixelSize : 30
         text: "drawing path:" + canvas.frame + "/" + Tiger.tiger.length
              + " moving to:(" + canvas.canvasX + "," + canvas.canvasY + ")"
              + " scale:" + canvas.contentsScale;
      }
  }

  MouseArea {
    id:mouseArea
    anchors.fill:parent
    property real pressedX;
    property real pressedY;
    onPressed: {
      pressedX = mouseX;
      pressedY = mouseY;
    }
    onPositionChanged : {
      canvas.canvasX = mouseX - pressedX;
      canvas.canvasY = mouseY - pressedY;
      canvas.requestPaint();
    }
  }
}
