// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Item {
    id: container
    width: 320
    height: 480

    function randomNumber() {
        return Math.random() * 360;
    }

    function getNumber() {
        return container.randomNumber();
    }

    TapHandler {
        // This line uses the JS function from the item
        onTapped: rectangle.rotation = container.getNumber();
    }

    Rectangle {
        color: "#272822"
        width: 320
        height: 480
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
