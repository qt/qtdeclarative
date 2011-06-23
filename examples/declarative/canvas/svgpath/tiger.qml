import QtQuick 2.0
import "tiger.js" as Tiger

Canvas {
  id:canvas
  width:800
  height:800
  property int frame:0    

  Timer {
    repeat:true
    interval:10
    running:true
    onTriggered: {
      frame++;
      if (frame > Tiger.tiger.length)
        frame = 0;
      Tiger.draw(canvas.getContext(), canvas.frame);
    }
  }

  /*onDrawRegion:{
    Tiger.draw(context, canvas.frame);
  }*/
}
