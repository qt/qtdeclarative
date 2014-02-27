import QtQuick 2.0

CanvasTestCase {
   id:testCase
   name: "canvas"
   function init_data() { return testData("2d"); }

   function test_canvasSize(row) {
       var c = createCanvasObject(row);
       verify(c);
       var ctx = c.getContext("2d");
       verify(ctx);

       tryCompare(c, "availableChangedCount", 1);
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
   function test_tileSize(row) {
       var c = createCanvasObject(row);
       verify(c);
       var ctx = c.getContext("2d");
       verify(ctx);
       tryCompare(c, "availableChangedCount", 1);

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

   function test_canvasWindow(row) {
       var c = createCanvasObject(row);
       verify(c);
       var ctx = c.getContext("2d");
       verify(ctx);

       tryCompare(c, "availableChangedCount", 1);
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

   function test_save(row) {
       var c = createCanvasObject(row);
       verify(c);
       var ctx = c.getContext("2d");

       tryCompare(c, "availableChangedCount", 1);

       c.requestPaint();
       verify(c.save("c.png"));
       c.loadImage("c.png");
       wait(200);
       verify(c.isImageLoaded("c.png"));
       verify(!c.isImageLoading("c.png"));
       verify(!c.isImageError("c.png"));
       c.destroy();
  }

   function test_toDataURL(row) {
       var c = createCanvasObject(row);
       verify(c);
       var ctx = c.getContext("2d");
       verify(ctx);

       tryCompare(c, "availableChangedCount", 1);

       var imageTypes = [
                   {mimeType:"image/png"},
                   {mimeType:"image/bmp"},
                   {mimeType:"image/jpeg"},
                   {mimeType:"image/x-portable-pixmap"},
                   //{mimeType:"image/tiff"}, QTBUG-23980
                   {mimeType:"image/xpm"},
                  ];
       for (var i = 0; i < imageTypes.length; i++) {
           ctx.fillStyle = "red";
           ctx.fillRect(0, 0, c.width, c.height);

           var dataUrl = c.toDataURL();
           verify(dataUrl !== "data:,");
           dataUrl = c.toDataURL("image/invalid");
           verify(dataUrl === "data:,");

           dataUrl = c.toDataURL(imageTypes[i].mimeType);
           verify(dataUrl !== "data:,");

           ctx.save();
           ctx.fillStyle = "blue";
           ctx.fillRect(0, 0, c.width, c.height);
           ctx.restore();

           var dataUrl2 = c.toDataURL(imageTypes[i].mimeType);
           verify (dataUrl2 !== "data:,");
           verify (dataUrl2 !== dataUrl);
       }
       c.destroy();

  }
   function test_paint(row) {
       var c = createCanvasObject(row);
       verify(c);
       var ctx = c.getContext("2d");
       tryCompare(c, "availableChangedCount", 1);
       //scene graph could be available immediately
       //in this case, we force waiting a short while until the init paint finished
       tryCompare(c, "paintedCount", 1);
       ctx.fillRect(0, 0, c.width, c.height);
       c.toDataURL();
       tryCompare(c, "paintedCount", 2);
       tryCompare(c, "paintCount", 1);
       c.destroy();
  }
   function test_loadImage(row) {
       var c = createCanvasObject(row);
       verify(c);
       var ctx = c.getContext("2d");
       verify(ctx);

       tryCompare(c, "availableChangedCount", 1);

       verify(!c.isImageLoaded("red.png"));
       c.loadImage("red.png");
       wait(200);
       verify(c.isImageLoaded("red.png"));
       verify(!c.isImageLoading("red.png"));
       verify(!c.isImageError("red.png"));

       c.unloadImage("red.png");
       verify(!c.isImageLoaded("red.png"));
       verify(!c.isImageLoading("red.png"));
       verify(!c.isImageError("red.png"));
       c.destroy();

  }

   function test_getContext(row) {
       var c = createCanvasObject(row);
       verify(c);
       var ctx = c.getContext("2d");
       verify(ctx);
       tryCompare(c, "availableChangedCount", 1);

       compare(ctx.canvas, c);
       ctx = c.getContext('2d');
       verify(ctx);
       compare(ctx.canvas, c);
       ctx = c.getContext('2D');
       verify(ctx);
       compare(ctx.canvas, c);
       ignoreWarning(Qt.resolvedUrl("CanvasComponent.qml") + ":6:9: QML Canvas: Canvas already initialized with a different context type");
       ctx = c.getContext('invalid');
       verify(!ctx);
       c.destroy();

  }
}

