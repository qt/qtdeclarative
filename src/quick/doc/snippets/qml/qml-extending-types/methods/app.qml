// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
//![0]
Rectangle {
    id: rect
    width: 100; height: 100

    function say(text) {
        console.log("You said: " + text);
    }

    MouseArea {
        anchors.fill: parent
        onClicked: rect.say("Mouse clicked")
    }
}
//![0]
