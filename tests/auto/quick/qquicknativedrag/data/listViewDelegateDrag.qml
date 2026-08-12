// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    width: 200
    height: 200
    color: "black"

    // Receives the drag events that QSimpleDrag feeds back into this process.
    DropArea {
        objectName: "dropArea"
        anchors.fill: parent
    }

    ListView {
        id: list
        objectName: "list"
        anchors.fill: parent
        model: 100
        cacheBuffer: 0
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        // The view keeps currentItem alive outside visibleItems, so with the default
        // currentIndex of 0 the first delegate would never be released when it scrolls
        // out of view -- and the test needs it to be.
        currentIndex: -1

        delegate: Rectangle {
            objectName: "delegate" + index
            width: list.width
            height: 50
            color: "white"

            Drag.dragType: Drag.Automatic
            Drag.supportedActions: Qt.MoveAction
            Drag.mimeData: { "text/plain": "" + index }
        }
    }
}
