import QtQuick 2.0
import "../contents"
import "cache.js" as TwitterUserCache
Item {
    width:360
    height:600
    QtObject {
        id:twitterManager
        function getById(id) {
            return TwitterUserCache.cache.getById(id);
        }

        function getByName(name) {
            return TwitterUserCache.cache.getByName(name);
        }

        function createTwitterUser(canvas) {
            return TwitterUserCache.createTwitterUser(canvas);
        }
    }
    Rectangle {
        id:inputContainer
        width:parent.width
        height:40
        anchors.top : parent.top

        TextInput {
            id:inputName
            anchors.fill:parent
            font.pointSize : 20
            opacity:1
            color:"steelblue"
            width:200
            height:40
            text: "Input your twitter name..."
            selectByMouse:true
            onAccepted : {canvas.twitterName = text; canvas.requestPaint();}
        }
    }
    Canvas {
      id:canvas
      width:parent.width
      anchors.top :inputContainer.bottom
      anchors.bottom : parent.bottom
      smooth:true
      renderTarget:Canvas.Image
      threadRendering:false

      property bool layoutChanged:true
      property string twitterName:""
      property string twitterId:""
      property bool loading:false

      onLoadingChanged: requestPaint();
      onWidthChanged: { layoutChanged = true; requestPaint();}
      onHeightChanged:  { layoutChanged = true; requestPaint();}
      onTwitterNameChanged: inputName.text = twitterName;
      onImageLoaded:requestPaint();
      onPaint: {
      var ctx = canvas.getContext('2d');
      ctx.reset();
      ctx.fillStyle="black";
      ctx.fillRect(0, 0, canvas.width, canvas.height);

      if (canvas.twitterName != "" || canvas.twitterId != "") {
          var user = canvas.twitterId ? TwitterUserCache.getById(canvas.twitterId) : TwitterUserCache.getByName(canvas.twitterName);
          if (!user) {
              user = TwitterUserCache.createTwitterUser(canvas);
              user.hasFocus = true;
              user.manager = twitterManager;
              user.createByName(canvas.twitterName);
              canvas.loading = true;
          }

          if (canvas.loading) {
              ctx.font = "40px";
              ctx.fillStyle = "steelblue";
              ctx.fillText("Loading...", canvas.width/2 - 80, canvas.height/2);
          } else {
              user.show(ctx, layoutChanged);
          }
          layoutChanged = false;
      }
    }
  }
}

