// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: root
    width: 400
    height: 400
    objectName: "root"

    // Two HoverHandlers with different cursorShapes on the same item
    // (typically, to have different cursors for mouse and stylus)
    HoverHandler {
        id: handler1
        objectName: "handler1"
        cursorShape: Qt.CrossCursor
    }

    HoverHandler {
        id: handler2
        objectName: "handler2"
        cursorShape: Qt.PointingHandCursor
    }
}
