import QtQuick 2.0
import QtTest 1.0
import "testhelper.js" as Helper
Canvas {
   id:canvas; width:100;height:50; renderTarget: Canvas.Image; renderStrategy:Canvas.Threaded
   TestCase {
       //TODO

       name: "shadow"; when: windowShown
       function test_basic() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_blur() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
       }

       function test_clip() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
       }

       function test_composite() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
       }

       function test_enable() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
       }

       function test_gradient() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
       }
       function test_image() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
       }
       function test_offset() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
       }
       function test_pattern() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
       }
       function test_stroke() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
       }
       function test_tranform() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
       }
   }
}
