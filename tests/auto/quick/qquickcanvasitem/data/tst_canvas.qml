import QtQuick 2.0
import QtTest 1.0

Rectangle {
    id:container
    width:100
    height:100
    Component {
        id:canvas
        Canvas {
            id:c
             width:100;height:100
             onPaint: {
                 var context = getContext("2d");
                 context.fillStyle = "red";
                 context.fillRect(0, 0, 100, 100);
             }
             property int paintCount:spyPaint.count
             property int paintedCount:spyPainted.count
             property int canvasSizeChangedCount:spyCanvasSizeChanged.count
             property int tileSizeChangedCount:spyTileSizeChanged.count
             property int renderStrategyChangedCount:spyRenderStrategyChanged.count
             property int canvasWindowChangedCount:spyCanvasWindowChanged.count
             property int renderTargetChangedCount:spyRenderTargetChanged.count
             property int imageLoadedCount:spyImageLoaded.count
             property int availableChangedCount:spyAvailableChanged.count

             SignalSpy {id: spyPaint;target:c;signalName: "paint"}
             SignalSpy {id: spyPainted;target:c;signalName: "painted"}
             SignalSpy {id: spyCanvasSizeChanged;target:c;signalName: "canvasSizeChanged"}
             SignalSpy {id: spyTileSizeChanged;target:c;signalName: "tileSizeChanged"}
             SignalSpy {id: spyRenderStrategyChanged;target:c;signalName: "renderStrategyChanged"}
             SignalSpy {id: spyCanvasWindowChanged;target:c;signalName: "canvasWindowChanged"}
             SignalSpy {id: spyRenderTargetChanged;target:c;signalName: "renderTargetChanged"}
             SignalSpy {id: spyImageLoaded;target:c;signalName: "imageLoaded"}
             SignalSpy {id: spyAvailableChanged;target:c;signalName: "availableChanged"}
        }
    }

   TestCase {
       name: "Canvas"; when: windowShown
       function test_canvasSize() {
           var c = canvas.createObject(container);
           verify(c);
           wait(100);
           verify(c.availableChangedCount, 1);

           //by default canvasSize is same with canvas' actual size
           // when canvas size changes, canvasSize should be changed as well.
           compare(c.canvasSize.width, c.width);
           compare(c.canvasSize.height, c.height);
           c.width = 20;
           compare(c.canvasSize.width, 20);
           compare(c.canvasSizeChangedCount, 1);
           c.height = 5;
           compare(c.canvasSizeChangedCount, 2);
           compare(c.canvasSize.height, 5);

           //change canvasSize manually, then canvasSize detaches from canvas
           //actual size.
           c.canvasSize.width = 100;
           compare(c.canvasSizeChangedCount, 3);
           compare(c.canvasSize.width, 100);
           compare(c.width, 20);
           c.canvasSize.height = 50;
           compare(c.canvasSizeChangedCount, 4);
           compare(c.canvasSize.height, 50);
           compare(c.height, 5);

           c.width = 10;
           compare(c.canvasSizeChangedCount, 4);
           compare(c.canvasSize.width, 100);
           compare(c.canvasSize.height, 50);

           c.height = 10;
           compare(c.canvasSizeChangedCount, 4);
           compare(c.canvasSize.width, 100);
           compare(c.canvasSize.height, 50);
           c.destroy();
      }
       function test_tileSize() {
           var c = canvas.createObject(container);
           verify(c);
           wait(100);
           verify(c.availableChangedCount, 1);

           compare(c.tileSize.width, c.width);
           compare(c.tileSize.height, c.height);
           c.width = 20;
           compare(c.tileSize.width, 20);
           compare(c.tileSizeChangedCount, 1);
           c.height = 5;
           compare(c.tileSizeChangedCount, 2);
           compare(c.tileSize.height, 5);

           c.tileSize.width = 100;
           compare(c.tileSizeChangedCount, 3);
           compare(c.tileSize.width, 100);
           compare(c.width, 20);
           c.tileSize.height = 50;
           compare(c.tileSizeChangedCount, 4);
           compare(c.tileSize.height, 50);
           compare(c.height, 5);

           c.width = 10;
           compare(c.tileSizeChangedCount, 4);
           compare(c.tileSize.width, 100);
           compare(c.tileSize.height, 50);

           c.height = 10;
           compare(c.tileSizeChangedCount, 4);
           compare(c.tileSize.width, 100);
           compare(c.tileSize.height, 50);
           c.destroy();

       }

       function test_canvasWindow() {
           var c = canvas.createObject(container);
           verify(c);
           wait(100);
           verify(c.availableChangedCount, 1);
           compare(c.canvasWindow.x, 0);
           compare(c.canvasWindow.y, 0);
           compare(c.canvasWindow.width, c.width);
           compare(c.canvasWindow.height, c.height);

           c.width = 20;
           compare(c.canvasWindow.width, 20);
           compare(c.canvasWindowChangedCount, 1);
           c.height = 5;
           compare(c.canvasWindowChangedCount, 2);
           compare(c.canvasWindow.height, 5);

           c.canvasWindow.x = 5;
           c.canvasWindow.y = 6;
           c.canvasWindow.width = 10;
           c.canvasWindow.height =20;
           compare(c.canvasWindowChangedCount, 6);
           compare(c.canvasWindow.width, 10);
           compare(c.canvasWindow.height, 20);
           compare(c.canvasWindow.x, 5);
           compare(c.canvasWindow.y, 6);
           c.destroy();

      }
       function test_renderTargetAndStrategy() {
           var c = canvas.createObject(container);
           verify(c);
           wait(100);
           verify(c.availableChangedCount, 1);

           compare(c.renderTarget, Canvas.Image);
           compare(c.renderStrategy, Canvas.Threaded);
           c.destroy();

      }
       function test_save() {
           var c = canvas.createObject(container);
           verify(c);
           wait(100);
           verify(c.availableChangedCount, 1);

           c.renderTarget = Canvas.Image;
           c.requestPaint();
           wait(100);
           verify(c.save("c.png"));
           c.loadImage("c.png");
           wait(200);
           compare(c.imageLoadedCount, 1);
           verify(c.isImageLoaded("c.png"));
           verify(!c.isImageLoading("c.png"));
           verify(!c.isImageError("c.png"));
           c.destroy();

      }
       function test_toDataURL_data() {
           return [{mimeType:"image/png"},
                   {mimeType:"image/bmp"},
                   {mimeType:"image/jpeg"},
                   {mimeType:"image/x-portable-pixmap"},
                   //{mimeType:"image/tiff"}, QTBUG-23980
                   {mimeType:"image/xpm"},
                   ];
       }

       function test_toDataURL(data) {
           var c = canvas.createObject(container);
           verify(c);
           wait(100);
           verify(c.availableChangedCount, 1);

           var ctx = c.getContext("2d");
           ctx.fillStyle = "red";
           ctx.fillRect(0, 0, c.width, c.height);
           wait(100);

           var dataUrl = c.toDataURL();
           verify(dataUrl != "data:,");
           dataUrl = c.toDataURL("image/invalid");
           verify(dataUrl == "data:,");

           dataUrl = c.toDataURL(data.mimeType);
           verify(dataUrl != "data:,");

           ctx.save();
           ctx.fillStyle = "blue";
           ctx.fillRect(0, 0, c.width, c.height);
           ctx.restore();
           wait(100);

           var dataUrl2 = c.toDataURL(data.mimeType);
           verify (dataUrl2 != "data:,");
           verify (dataUrl2 != dataUrl);
           c.destroy();

      }
       function test_paint() {
           var c = canvas.createObject(container);
           verify(c);
           wait(100);
           verify(c.availableChangedCount, 1);

           c.renderTarget = Canvas.Image;
           c.renderStrategy = Canvas.Immediate;
           var ctx = c.getContext("2d");
           ctx.fillRect(0, 0, c.width, c.height);
           c.toDataURL();
           wait(100);

           compare(c.paintedCount, 1);
           compare(c.paintCount, 1);
           c.destroy();

      }
       function test_loadImage() {
           var c = canvas.createObject(container);
           verify(c);
           wait(100);
           verify(c.availableChangedCount, 1);

           c.loadImage("red.png");
           wait(200);
           compare(c.imageLoadedCount, 1);
           verify(c.isImageLoaded("red.png"));
           verify(!c.isImageLoading("red.png"));
           verify(!c.isImageError("red.png"));

           c.unloadImage("red.png");
           verify(!c.isImageLoaded("red.png"));
           verify(!c.isImageLoading("red.png"));
           verify(!c.isImageError("red.png"));
           c.destroy();

      }

       function test_getContext() {
           var c = canvas.createObject(container);
           verify(c);
           wait(100);
           verify(c.availableChangedCount, 1);

           var ctx = c.getContext("2d");
           verify(ctx);
           compare(ctx.canvas, c);
           ctx = c.getContext('2d');
           verify(ctx);
           compare(ctx.canvas, c);
           ctx = c.getContext('2D');
           verify(ctx);
           compare(ctx.canvas, c);
           ctx = c.getContext('invalid');
           verify(!ctx);
           c.destroy();

      }
   }
}
