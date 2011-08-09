import QtQuick 2.0
import "../contents"
Item {
  id:container
  width:360
  height:600

  Column {
    spacing:5
    anchors.fill:parent
    Text { font.pointSize:25; text:"Processing pixels"; anchors.horizontalCenter:parent.horizontalCenter}

    Canvas {
      id:canvas
      width:360
      height:360
      smooth:true
      renderTarget:Canvas.Image
      threadRendering:false
      property string image :"../contents/qt-logo.png"
      Component.onCompleted:loadImage(image);
      onImageLoaded:requestPaint();
    onPaint: {
      
      var ctx = canvas.getContext('2d');
      if (canvas.isImageLoaded(image)) {
          var pixels = ctx.createImageData(image);
          //pixels.mirror();
          pixels.filter(Canvas.GrayScale);
          //pixels.filter(Canvas.Threshold, 100); //default 127
          //pixels.filter(Canvas.Blur, 20); //default 10
          //pixels.filter(Canvas.Opaque);
          //pixels.filter(Canvas.Invert);
          //pixels.filter(Canvas.Convolute, [0,-1,0,
          //                                 -1,5,-1,
          //                                 0,-1,0]);
          //ctx.putImageData(pixels, 0, 0, canvas.width, canvas.height);
          ctx.putImageData(pixels, 0, 0);
      }
    }
  }
 }
}