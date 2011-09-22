import QtQuick 2.0
import QtTest 1.0
import "testhelper.js" as Helper

Canvas {
   id:canvas; width:1;height:1
   TestCase {
       name: "Colors"; when: windowShown
       function test_globalAlpha() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
           ctx.fillStyle = Qt.rgba(1, 0.7, 0.2, 0.5);
           ctx.globalAlpha = 0.5;
           ctx.fillRect(0,0,1,1);
           var d = ctx.getImageData(0,0,1,1).data;
           verify(Helper.comparePixel(ctx, 0, 0, 255, 0.7 * 255, 0.2*255, 0.25 * 255));
      }
   }
}
