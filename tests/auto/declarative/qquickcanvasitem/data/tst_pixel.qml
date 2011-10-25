import QtQuick 2.0
import QtTest 1.0
import "testhelper.js" as Helper
Canvas {
   id:canvas; width:100;height:50; renderTarget: Canvas.Image
   TestCase {
       //TODO
       name: "pixel"; when: windowShown
       function test_createImageData() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_getImageData() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_object() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_putImageData() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_filters() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
   }
}
