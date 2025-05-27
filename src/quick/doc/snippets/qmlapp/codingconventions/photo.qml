// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//! [0]
Rectangle {
    id: photo                                               // id on the first line makes it easy to find an object

    property bool thumbnail: false                          // property declarations
    property alias image: photoImage.source

    signal clicked                                          // signal declarations

    function doSomething(x)                                 // javascript functions
    {
        return x + photoImage.width;
    }

    color: "gray"                                           // object properties
    x: 20                                                   // try to group related properties together
    y: 20
    height: 150
    width: {                                                // large bindings
        if (photoImage.width > 200) {
            photoImage.width;
        } else {
            200;
        }
    }

    states: [
        State {
            name: "selected"
            PropertyChanges { target: border; color: "red" }
        }
    ]

    transitions: [
        Transition {
            from: ""
            to: "selected"
            ColorAnimation { target: border; duration: 200 }
        }
    ]

    Rectangle {                                             // child objects
        id: border
        anchors.centerIn: parent
        color: "white"

        Image {
            id: photoImage
            anchors.centerIn: parent
        }
    }
}
//! [0]
