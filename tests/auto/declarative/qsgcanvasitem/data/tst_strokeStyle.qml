import QtQuick 2.0
import QtTest 1.0
import "testhelper.js" as Helper

Canvas {
   id:canvas; width:1;height:1
   TestCase {
       name: "strokeStyle"; when: windowShown
       function test_default() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
           compare(ctx.strokeStyle, "#000000")
           ctx.clearRect(0, 0, 1, 1);
           compare(ctx.strokeStyle, "#000000")
       }
       function test_saverestore() {
           var ctx = canvas.getContext('2d');
           var old = ctx.strokeStyle;
           ctx.save();
           ctx.strokeStyle = "#ffaaff";
           ctx.restore();
           compare(ctx.strokeStyle, old);

           ctx.strokeStyle = "#ffcc88";
           old = ctx.strokeStyle;
           ctx.save();
           compare(ctx.strokeStyle, old);
           ctx.restore();
       }
       function test_namedColor() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
           ctx.strokeStyle = "red";
           ctx.strokeRect(0,0,1,1);
           verify(Helper.comparePixel(ctx,0,0,255,0,0,255));

           ctx.strokeStyle = "black";
           ctx.strokeRect(0,0,1,1);
           verify(Helper.comparePixel(ctx,0,0,0,0,0,255));

           ctx.strokeStyle = "white";
           ctx.strokeRect(0,0,1,1);
           verify(Helper.comparePixel(ctx,0,0,255,255,255,255));
       }


   }
}
