// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

ListModel {
    id: flickrImages
    property string tags : ""
    readonly property string queryUrl : "http://api.flickr.com/services/feeds/photos_public.gne?"

    function encodeParams(x) {
        return encodeURIComponent(x.replace(" ",","));
    }
    function fetchImages(format) {
        var requestURL = queryUrl + (tags ? "tags="+encodeParams(tags)+"&" : "") + "format=" + format + "&nojsoncallback=1";
        var xhr = new XMLHttpRequest;
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {

                if (xhr.status !== 200) {
                    console.log("Failed to get images from flickr. status code: " + xhr.status);
                    return;
                }

                var jsonText = xhr.responseText;
                var objArray = JSON.parse(jsonText.replace(/\'/g,"'"))
                if (objArray.errors !== undefined)
                    console.log("Error fetching tweets: " + objArray.errors[0].message)
                else {
                    for (var key in objArray.items) {
                        var rssItem = objArray.items[key];
                        var jsonObject = "{ \"title\": \"" + rssItem.title +"\",\"media\": \"" + rssItem.media.m + "\", \"thumbnail\": \"" + rssItem.media.m.replace(/\_m\.jpg/,"_s.jpg") +"\"}"
                        flickrImages.append(JSON.parse(jsonObject));
                    }
                }
            }
        }
        xhr.open("GET", requestURL, true);
        xhr.send();
    }
    Component.onCompleted: {
        fetchImages("json");
    }
}

