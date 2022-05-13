// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

Item {
    width: 200
    height: 200

//! [1]
Rectangle {
    id: frame
    clip: true
    width: 160
    height: 160
    border.color: "black"
    anchors.centerIn: parent

    Text {
        id: content
        text: "ABC"
        font.pixelSize: 169

        MouseArea {
            id: mouseArea
            drag.target: content
            drag.minimumX: frame.width - width
            drag.minimumY: frame.height - height
            drag.maximumX: 0
            drag.maximumY: 0
            anchors.fill: content
        }
    }

    ScrollIndicator {
        id: verticalIndicator
        active: mouseArea.pressed
        orientation: Qt.Vertical
        size: frame.height / content.height
        position: -content.y / content.height
        anchors { top: parent.top; right: parent.right; bottom: parent.bottom }
    }

    ScrollIndicator {
        id: horizontalIndicator
        active: mouseArea.pressed
        orientation: Qt.Horizontal
        size: frame.width / content.width
        position: -content.x / content.width
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
    }
}
//! [1]

Component.onCompleted: {
    horizontalIndicator.active = true;
    verticalIndicator.active = true;
}
}
