/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Item {
    id:twitterUser
    property variant friends : [];
    property string name : "";
    property string twitterId : "";
    property string image : "";
    property string url : "";
    property string desc : "";
    width : 0;
    height : 0;
    property int posX:0;
    property int posY:0;
    property bool hasFocus : false;
    property variant canvas;
    property variant manager;
    property variant linkColor;
    property bool selected : false;

    Rectangle {
        id:twitterStatus
        x:twitterUser.width
        y:twitterUser.height
        width:250
        height:60
        opacity: 0
        border.color:"steelblue"
        border.width:3
        Column {
            spacing:2
            Text {color:"steelblue"; font.pointSize:15; width:250; height:30; text:twitterUser.name; wrapMode: Text.WrapAnywhere}
            Text {color:"steelblue"; font.pointSize:8; width:250; height:30;  text:twitterUser.url; wrapMode: Text.WrapAnywhere}
        }
    }

    function moved() {
        twitterUser.posX = twitterUser.x;
        twitterUser.posY = twitterUser.y;
        twitterUser.canvas.requestPaint();
    }

    onXChanged: moved();
    onYChanged: moved();

    MouseArea {
        anchors.fill:parent
        drag.target : twitterUser
        drag.axis : Drag.XandYAxis

        onClicked: {
            if (!twitterUser.selected) {
                twitterUser.selected = true;
                twitterStatus.opacity = 1;
                twitterStatus.visible = true;
            } else {
                twitterUser.selected = false;
                twitterStatus.opacity = 0;
            }
        }

        onDoubleClicked : {
            twitterStatus.opacity = 0;
            twitterUser.selected = false;
            twitterUser.hasFocus = true;
            twitterUser.canvas.twitterName = twitterUser.name;
            twitterUser.canvas.twitterId = twitterUser.twitterId;
            twitterUser.canvas.loading = true;
            twitterUser.createFriends();
        }
    }

    function show(ctx, layoutChanged) {
      var w = canvas.width;
      var h = canvas.height;
      if (twitterUser.hasFocus) {
          twitterUser.width = 60
          twitterUser.height = 60
          twitterUser.posX = w/2;
          twitterUser.posY = h/2;
       } else {
          twitterUser.width = 40
          twitterUser.height = 40
      }


      if (twitterUser.hasFocus) {
          if (layoutChanged)
              twitterUser.layoutFriends();
          twitterUser.linkFriends(ctx);
          twitterUser.showFriends(ctx);
          ctx.shadowOffsetX = 5;
          ctx.shadowOffsetY = 5;
          ctx.shadowBlur = 7;
          ctx.shadowColor = "blue";
          ctx.globalAlpha = 1;
      }  else {
          ctx.shadowOffsetX = 5;
          ctx.shadowOffsetY = 5;
          ctx.shadowBlur = 7;
          ctx.shadowColor = twitterUser.linkColor;
          ctx.globalAlpha = 0.6;
      }

      if (twitterUser.canvas.isImageLoaded(twitterUser.image)) {
        ctx.drawImage(twitterUser.image, twitterUser.posX, twitterUser.posY, twitterUser.width, twitterUser.height);
      }
//      ctx.font = "15px";
//      var nameSize = ctx.measureText(twitterUser.name).width;
//      ctx.fillText(twitterUser.name, twitterUser.posX + nameSize/2 - twitterUser.width/2, twitterUser.posY + twitterUser.height/2 + 10);
    }
    function dump() {
        console.log("name:" + twitterUser.name
                  + " x:" + twitterUser.posX
                  + " y:" + twitterUser.posY
                  + " width:" + twitterUser.width
                  + " height:" + twitterUser.height
                  + " id:" + twitterUser.twitterId
                  + " image:" + twitterUser.image
                  + " url:" + twitterUser.url + "\n" + twitterUser.desc);
    }

    function layoutFriends() {
        var w = canvas.width;
        var h = canvas.height;
        for (var i=0; i < twitterUser.friends.length; i++) {
            var friend = manager.getById(twitterUser.friends[i]);
            if (friend) {
                friend.x = Math.random() *w;
                friend.y = Math.random() *h;
            }
        }
    }

    function showFriends(ctx) {
        var w = canvas.width;
        var h = canvas.height;
        for (var i=0; i < twitterUser.friends.length && i < 15; i++) {
            var friend = manager.getById(twitterUser.friends[i]);
            if (friend && twitterUser.canvas.isImageLoaded(friend.image)) {
                friend.hasFocus = false;
                friend.show(ctx, false);
            }
        }
    }

    function linkFriends(ctx) {
        var w = canvas.width;
        var h = canvas.height;
        for (var i=0; i < twitterUser.friends.length && i < 15; i++) {
            var friend = manager.getById(twitterUser.friends[i]);
            if (friend && twitterUser.canvas.isImageLoaded(friend.image)) {
                if (!friend.linkColor)
                     friend.linkColor = Qt.rgba( ((Math.random() * 200) +55)/255
                                               , ((Math.random() * 200) +55)/255
                                               , ((Math.random() * 200) +55)/255, 0.8);
                ctx.strokeStyle  = friend.linkColor;
                ctx.lineWidth = 8;
                ctx.beginPath();
                ctx.moveTo(twitterUser.posX + twitterUser.width/2, twitterUser.posY + twitterUser.height/2);
                ctx.lineTo(friend.x + friend.width/2, friend.y + friend.height/2);
                ctx.stroke();
            }
        }
    }


    function create(url) {
        var x = new XMLHttpRequest;
        x.open("GET", url);

        x.onreadystatechange = function() {
            if (x.readyState == XMLHttpRequest.DONE) {
                var user = eval('(' + x.responseText +')')[0];
                twitterUser.name = user.name;
                twitterUser.twitterId = user.id;
                twitterUser.image = user.profile_image_url;
                twitterUser.canvas.loadImage(twitterUser.image);
                twitterUser.url = user.url;
                twitterUser.desc = user.description;
                twitterUser.createFriends();
           }
        }
        x.send();
    }

    function createByName(name) {
      if (twitterUser.name === "" && twitterUser.twitterId === "") {
         twitterUser.name = name;
         var userUrl = "http://api.twitter.com/1/users/lookup.json?stringify_ids=true&screen_name=" + name;
         twitterUser.create(userUrl);
      }
    }

    function createById(id) {
      if (twitterUser.name === "" && twitterUser.twitterId === "") {
         twitterUser.twitterId = id;
         var userUrl = "http://api.twitter.com/1/users/lookup.json?stringify_ids=true&user_id=" + id;
         twitterUser.create(userUrl);
      }
    }

    function createFriends() {
      if (twitterUser.friends.length === 0) {
          var x = new XMLHttpRequest;
          var friendsUrl = "https://api.twitter.com/1/friends/ids.json?cursor=-1&stringify_ids=true&user_id=" + twitterUser.twitterId;
          x.open("GET", friendsUrl);

          x.onreadystatechange = function() {
              if (x.readyState == XMLHttpRequest.DONE) {
                 twitterUser.friends = eval('(' + x.responseText +')').ids;
                 var doRequest = false;
                  var userUrl = "http://api.twitter.com/1/users/lookup.json?stringify_ids=true&user_id=";

                 for (var i=0; i<twitterUser.friends.length && i < 100; i++) {
                    var friend = manager.getById(twitterUser.friends[i]);
                    if (!friend) {
                       userUrl += "," + twitterUser.friends[i];
                       doRequest = true;
                    }
                 }

                 if (!doRequest) return;

                 var xx = new XMLHttpRequest;
                 xx.open("GET", userUrl);
                 xx.onreadystatechange = function() {
                    if (xx.readyState == XMLHttpRequest.DONE) {
                       var friendUsers = eval('(' + xx.responseText +')');
                       for(var i=0; i<friendUsers.length; i++) {
                           var friend = manager.createTwitterUser(twitterUser.canvas);
                          friend.name = friendUsers[i].name;
                          friend.twitterId = friendUsers[i].id;
                          friend.image = friendUsers[i].profile_image_url ? friendUsers[i].profile_image_url : "";
                          friend.url = friendUsers[i].url ? friendUsers[i].url : "";
                          friend.desc = friendUsers[i].description ? friendUsers[i].description : "";
                          friend.manager = twitterUser.manager;
                          twitterUser.canvas.loadImage(friend.image);
                       }

                       if (twitterUser.hasFocus && twitterUser.canvas) {
                           twitterUser.canvas.layoutChanged = true;
                           twitterUser.canvas.loading = false;
                           twitterUser.canvas.requestPaint();
                       }
                    }
                 }
                 xx.send();
              }
          }
          x.send();
      }
    }
}