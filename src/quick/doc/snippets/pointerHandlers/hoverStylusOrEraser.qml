// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    id: rect
    width: 150; height: 150

    HoverHandler {
        id: stylus
        acceptedPointerTypes: PointerDevice.Pen
        cursorShape: Qt.CrossCursor
    }

    HoverHandler {
        id: eraser
        acceptedPointerTypes: PointerDevice.Eraser
        cursorShape: Qt.BlankCursor
        target: Image {
            parent: rect
            source: "images/cursor-eraser.png"
            visible: eraser.hovered
            x: eraser.point.position.x
            y: eraser.point.position.y - 32
        }
    }
}
//![0]
