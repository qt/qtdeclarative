// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12

Item {
    width: 200
    height: 200
    Rectangle {
        id: rect
        objectName: "pointTracker"
        width: 20
        height: 20
        radius: 10
        color: "green"
        x: handler.point.position.x
        y: handler.point.position.y
    }

    Text {
        id: text
        anchors.top: rect.bottom
        anchors.left: rect.right
        text: "<" + handler.point.position.x + ", " + handler.point.position.y + "> " + handler.point.pressedButtons + " [" + handler.point.pressure + "]"
    }

    PointHandler {
        id: handler
        objectName: "pointHandler"
        acceptedButtons: Qt.NoButton // don't care: we want everything, moves too
    }
}
