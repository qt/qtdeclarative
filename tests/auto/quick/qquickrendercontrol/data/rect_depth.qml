// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    width: 200
    height: 200
    color: "steelblue"
    Rectangle {
        id: r
        width: 150
        height: 150
        anchors.centerIn: parent
        color: "palegreen"
        NumberAnimation on rotation { from: 0; to: 360; duration: 5000; loops: -1 }
    }
    Rectangle {
        width: 100
        height: 100
        color: "red"
        z: -1 //  not visible as it is completely below, but this only works if depth testing works (and there's a depth buffer) since these are opaque items
    }
    Text {
        text: "Hello World\n\nrotation=" + Math.round(r.rotation)
        anchors.centerIn: parent
    }
}
