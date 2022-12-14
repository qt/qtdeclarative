// Copyright (C) 2014 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.4

Item {
    width: 320
    height: 480

//! [grab-to-file]
Rectangle {
    id: sourceRectangle
    width: 100
    height: 100
    gradient: Gradient {
        GradientStop { position: 0; color: "steelblue" }
        GradientStop { position: 1; color: "black" }
    }

    Component.onCompleted: {
        sourceRectangle.grabToImage(function(result) {
           result.saveToFile("something.png")
        })
    }
}
//! [grab-to-file]

//! [grab-to-image]
Image {
    id: image
}

Component.onCompleted: {
    sourceRectangle.grabToImage(function(result) {
        image.source = result.url
    }, Qt.size(50, 50))
}
//! [grab-to-image]
}
