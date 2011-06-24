import QtQuick 2.0
import "tiger.js" as Tiger

Canvas {
  id:canvas
  width:1900
  height:1150
  property int frame:0    

  Timer {
    repeat:true
    interval:100
    running:true
    onTriggered: {
      canvas.frame++;
      if (canvas.frame > Tiger.tiger.length) {
        canvas.frame = 0;
      } else {
        Tiger.draw(canvas.getContext(), canvas.frame);
      }
    }
  }
/*
  onDrawRegion:{
    Tiger.draw(context, canvas.frame);
  }
Text {
  anchors.top : parent.top
  font.pixelSize : 30
  text: "drawing path:" + canvas.frame + "/" + Tiger.tiger.length;
}
*/
}
