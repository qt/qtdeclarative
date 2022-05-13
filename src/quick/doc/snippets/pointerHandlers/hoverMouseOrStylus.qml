// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    width: 150; height: 50; radius: 3
    color: mouse.hovered ? "goldenrod" : stylus.hovered ? "tomato" : "wheat"

    HoverHandler {
        id: stylus
        acceptedDevices: PointerDevice.Stylus
        cursorShape: Qt.CrossCursor
    }

    HoverHandler {
        id: mouse
        acceptedDevices: PointerDevice.Mouse
        cursorShape: Qt.PointingHandCursor
    }
}
//![0]
