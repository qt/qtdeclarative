import QtQuick 2.0
import QtTest 1.0
import "testhelper.js" as Helper
Canvas {
   id:canvas; width:100;height:50; renderTarget: Canvas.Image; renderStrategy:Canvas.Threaded
   TestCase {
       //TODO
       name: "pattern"; when: windowShown
       function test_basic() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_animated() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_image() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_modified() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_paint() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_repeat() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
   }
}
