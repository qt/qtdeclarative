// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.11

Rectangle {
    id: root
    width: 480
    height: 480
    color: "white"
    Text {
        id: pressedLabel
        x: 0
        y: 0
        width: 480
        height: 50
        font.pointSize: 18
    }

    MouseArea {
        anchors.fill: parent
        onPressed: {
            pressedLabel.text = "Pressed - " + (mouse.flags === Qt.MouseEventCreatedDoubleClick ?
                                "Press from double click" : "No flags")
        }
    }
}
