import QtQuick 2.0
import QtTest 1.0
import "testhelper.js" as Helper

Canvas {
   id:canvas; width:1;height:1
   TestCase {
       name: "fillStyle"; when: windowShown
       function test_default() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
           verify(ctx.fillStyle, "#000000");
           ctx.clearRect(0, 0, 1, 1);
           compare(ctx.fillStyle, "#000000");
       }
       function test_get() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
           ctx.fillStyle = '#fa0';
           compare(ctx.fillStyle, '#ffaa00');
           ctx.fillStyle = Qt.rgba(0,0,0,0);
           compare(ctx.fillStyle, 'rgba(0, 0, 0, 0.0)');
       }
       function test_hex() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
           ctx.fillStyle = '#f00';
           compare(ctx.fillStyle, '#ff0000');
           ctx.fillStyle = "#0f0";
           compare(ctx.fillStyle, '#00ff00');
           ctx.fillStyle = "#0fF";
           compare(ctx.fillStyle, '#00ffff');
           ctx.fillStyle = "#0aCCfb";
           compare(ctx.fillStyle, '#0accfb');

       }
       function test_invalid() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
           ctx.fillStyle = '#fa0';
           compare(ctx.fillStyle, '#ffaa00');
           ctx.fillStyle = "invalid";
           compare(ctx.fillStyle, '#ffaa00');
           ctx.fillStyle = "rgb (1, 2, 3)";
           compare(ctx.fillStyle, '#ffaa00');
           ctx.fillStyle = "rgba(1, 2, 3)";
           compare(ctx.fillStyle, '#ffaa00');
           ctx.fillStyle = "rgb((3,4,1)";
           compare(ctx.fillStyle, '#ffaa00');
           ctx.fillStyle = "rgb(1, 3, 4, 0.5)";
           compare(ctx.fillStyle, '#ffaa00');
           ctx.fillStyle = "hsl(2, 3, 4, 0.8)";
           compare(ctx.fillStyle, '#ffaa00');
           ctx.fillStyle = "hsl(2, 3, 4";
           compare(ctx.fillStyle, '#ffaa00');
       }
       function test_saverestore() {
           var ctx = canvas.getContext('2d');
           var old = ctx.fillStyle;
           ctx.save();
           ctx.fillStyle = "#ffaaff";
           ctx.restore();
           compare(ctx.fillStyle, old);

           ctx.fillStyle = "#ffcc88";
           old = ctx.fillStyle;
           ctx.save();
           compare(ctx.fillStyle, old);
           ctx.restore();
       }
       function test_namedColor() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
           ctx.fillStyle = "red";
           ctx.fillRect(0,0,1,1);
           verify(Helper.comparePixel(ctx,0,0,255,0,0,255));

           ctx.fillStyle = "black";
           ctx.fillRect(0,0,1,1);
           verify(Helper.comparePixel(ctx,0,0,0,0,0,255));

           ctx.fillStyle = "white";
           ctx.fillRect(0,0,1,1);
           verify(Helper.comparePixel(ctx,0,0,255,255,255,255));
       }
       function test_rgba() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
           ctx.fillStyle = "rgb(-100, 300, 255)";
           compare(ctx.fillStyle, "#00ffff");
           ctx.fillStyle = "rgba(-100, 300, 255, 0.0)";
           compare(ctx.fillStyle, "rgba(0, 255, 255, 0.0)");
           ctx.fillStyle = "rgb(-10%, 110%, 50%)";
           compare(ctx.fillStyle, "#00ff80");

           ctx.clearRect(0, 0, 1, 1);
           ctx.fillStyle = 'rgba(0%, 100%, 0%, 0.499)';
           ctx.fillRect(0, 0, 1, 1);
           //FIXME: currently we only return premultipled pixels
           verify(Helper.comparePixel(ctx, 0,0, 0,127,0,255));
           //verify(Helper.comparePixel(ctx, 0,0, 0,255,0,127));
       }

       function test_hsla() {
           var ctx = canvas.getContext('2d');
           ctx.reset();
           ctx.fillStyle = "hsla(120, 100%, 50%, 0.499)";
           ctx.fillRect(0, 0, 1, 1);
           verify(Helper.comparePixel(ctx,0,0,0,127,0,255));
       }

   }
}
