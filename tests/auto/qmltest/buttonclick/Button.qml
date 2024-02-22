// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    id: container

    property string text: "Button"

    signal clicked(int x, int y)

    width: buttonLabel.width + 20; height: buttonLabel.height + 5
    border { width: 1; color: "black" }
    smooth: true
    radius: 8

    // color the button with a gradient
    gradient: Gradient {
        GradientStop { position: 0.0; color: "blue" }
        GradientStop { position: 1.0; color: "lightblue" }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: container.clicked(mouse.x, mouse.y);
    }

    Text {
        id: buttonLabel
        anchors.centerIn: container
        color: "white"
        text: container.text
    }
}
