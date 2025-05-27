// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick
import "myscript.js" as Logic

Item {
    width: 320
    height: 480

    Rectangle {
        color: "#272822"
        width: 320
        height: 480
    }

    TapHandler {
        // This line uses the JS function from the separate JS file
        onTapped: rectangle.rotation = Logic.getRandom(rectangle.rotation);
    }

    Rectangle {
        id: rectangle
        anchors.centerIn: parent
        width: 160
        height: 160
        color: "green"
        Behavior on rotation { RotationAnimation { direction: RotationAnimation.Clockwise } }
    }

}
//![0]
