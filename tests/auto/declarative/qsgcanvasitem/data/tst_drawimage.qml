import QtQuick 2.0
import QtTest 1.0
import "testhelper.js" as Helper
Canvas {
   id:canvas; width:100;height:50; renderTarget: Canvas.Image
   TestCase {
       //TODO
       name: "image"; when: windowShown
       function test_3args() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_5args() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_9args() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_animated() {
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
       function test_path() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
       function test_transform() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
      }
   }
}
