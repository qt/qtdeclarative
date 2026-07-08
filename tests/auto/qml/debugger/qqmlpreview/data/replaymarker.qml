// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Window {
    id: window
    visible: true
    width: 100
    height: 100
    title: "aaa"
    color: "blue"

    // Logged exactly once per instantiation, so the number of "scene loaded"
    // lines counts how often the scene was (re)loaded.
    Component.onCompleted: console.log("scene loaded")

    Timer {
        repeat: true
        interval: 200
        running: true
        onTriggered: console.log(window.title, window.color)
    }

    MouseArea {
        anchors.fill: parent
        onClicked: color = "red"
    }
}
