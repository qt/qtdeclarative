// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Effects

// Custom Component for displaying Icons
Item {
    id: root

    required property url path
    property real padding: 5
    property real size: 30
    property alias iconColor: overlay.colorizationColor
    property alias hovered: mouse.hovered

    width: size
    height: size

    Image {
        id: icon
        anchors.fill: root
        anchors.margins: padding
        source: path
        sourceSize: Qt.size(size, size)
        fillMode: Image.PreserveAspectFit
        smooth: true
        antialiasing: true
        asynchronous: true
    }

    MultiEffect {
        id: overlay
        anchors.fill: icon
        source: icon
        colorization: 1.0
        brightness: 1.0
    }

    HoverHandler {
        id: mouse
        acceptedDevices: PointerDevice.Mouse
    }
}
