// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15


Rectangle {
    id: brownRect
    objectName: "brownRect"

    width: 400
    height: 400

    HoverHandler {
        id: hh
        cursorShape: parent.colorIndex == 0 ?
                         Qt.CrossCursor :
                         Qt.OpenHandCursor
    }

    property list<color> colors: ["beige", "brown"]
    property int colorIndex: 0

    color: colors[colorIndex]

    Timer {
        id: colorTimer
        interval: 200
        running: true
        repeat: true

        onTriggered: {
            parent.colorIndex = (parent.colorIndex + 1) % parent.colors.length;
            parent.color = parent.colors[parent.colorIndex];
        }
    }
}
