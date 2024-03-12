// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2

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
    Text {
        text: "Hello World\n\nrotation=" + Math.round(r.rotation)
        anchors.centerIn: parent
    }
}
