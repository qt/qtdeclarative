/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import "cache.js" as TwitterUserCache
Rectangle {
    width:360
    height:600
    color:"black"
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
        width:parent.width-4
        height:40
        anchors.top:parent.top
        anchors.topMargin:4
        anchors.horizontalCenter:parent.horizontalCenter
        radius:8
        border.color:"steelblue"
        Text {
            text:inputName.text == "" ? "Enter your twitter name..." : ""
            id:inputLabel
            anchors.centerIn:parent
            font.pointSize:12
            opacity:.5
            color:"steelblue"
        }
        TextInput {
            id:inputName
            anchors.centerIn:parent
            font.pointSize : 20
            opacity:1
            color:"steelblue"
            width:parent.width-6
            height:parent.height-6
            text:""
            autoScroll:true
            focus:true
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
      renderStrategy: Canvas.Immediate

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
      ctx.save();
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
      ctx.restore();
    }
  }
}
