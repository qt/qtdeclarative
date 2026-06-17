// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    id: root
    width: 200
    height: 120
    color: "white"
    property int marker: 1

    Image {
        objectName: "pic"
        source: "inplace_pixel.png"
        anchors.fill: parent
    }

    Timer {
        repeat: true
        interval: 200
        running: true
        onTriggered: console.log("image_test marker=" + root.marker)
    }
}
