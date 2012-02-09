import QtQuick 2.0
import QtTest 1.0

Canvas {
   id:canvas; width:1;height:1;
   renderTarget:Canvas.Image
   renderStrategy: Canvas.Immediate

   TestCase {
       name: "FillRect"; when: canvas.available

       function test_fillRect() {
           var ctx = canvas.getContext('2d');
           ctx.fillStyle = "red";
           ctx.fillRect(0, 0, canvas.width, canvas.height);

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
