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
        font.pixelSize: 160
        x: -hbar.position * width
        y: -vbar.position * height
    }

    ScrollBar {
        id: vbar
        hoverEnabled: true
        active: hovered || pressed
        orientation: Qt.Vertical
        size: frame.height / content.height
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

    ScrollBar {
        id: hbar
        hoverEnabled: true
        active: hovered || pressed
        orientation: Qt.Horizontal
        size: frame.width / content.width
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
//! [1]

Component.onCompleted: {
    hbar.active = true
    vbar.active = true
}
}
