import QtQuick 2.0
import QtTest 1.0

Canvas {
   id:canvas; width:1;height:1
   onPaint: {
     context.fillStyle = "red";
     context.fillRect(0, 0, canvas.width, canvas.height);
   }
   TestCase {
       name: "FillRect"; when: windowShown
       function test_fillRect() {
           var ctx = canvas.getContext('2d');
           var imageData = ctx.getImageData(0, 0, 1, 1);
           var d = imageData.data;
           verify(d.length == 4);
           verify(d[0] == 255);
           verify(d[1] == 0);
           verify(d[2] == 0);
           verify(d[3] == 255);
      }
   }
}
