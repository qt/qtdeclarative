// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Item {

    width: 320
    height: 480

    Rectangle {
        color: "#272822"
        width: 320
        height: 480
    }

    Rectangle {
        rotation: 45 // This rotates the Rectangle by 45 degrees
        x: 20
        y: 160
        width: 100
        height: 100
        color: "blue"
    }
    Rectangle {
        scale: 0.8 // This scales the Rectangle down to 80% size
        x: 160
        y: 160
        width: 100
        height: 100
        color: "green"
    }
}
//![0]
