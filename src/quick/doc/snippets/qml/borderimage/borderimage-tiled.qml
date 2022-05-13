// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

Rectangle {
    id: page
    color: "white"
    width: 180; height: 180

//! [tiled border image]
BorderImage {
    width: 180; height: 180
    border { left: 30; top: 30; right: 30; bottom: 30 }
    horizontalTileMode: BorderImage.Repeat
    verticalTileMode: BorderImage.Repeat
    source: "pics/borderframe.png"
}
//! [tiled border image]

    Rectangle {
        x: 30; y: 0
        width: 1; height: 180
        color: "gray"
    }

    Rectangle {
        x: 150; y: 0
        width: 1; height: 180
        color: "gray"
    }

    Rectangle {
        x: 0; y: 30
        width: 180; height: 1
        color: "gray"
    }

    Rectangle {
        x: 0; y: 150
        width: 180; height: 1
        color: "gray"
    }
}
