import QtQuick 2.0
import QtTest 1.0
import "testhelper.js" as Helper
Canvas {
   id:canvas; width:100;height:50; renderTarget: Canvas.Image; renderStrategy:Canvas.Threaded
   TestCase {
       //TODO
       name: "text"; when: windowShown
       function test_baseLine() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_align() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_stroke() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_fill() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_font() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_measure() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
   }
}
