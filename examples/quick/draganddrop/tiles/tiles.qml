// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root

    width: 320
    height: 480

    color: "black"

    Grid {
        id: redDestination

        anchors {
            left: redSource.right
            top: parent.top
            margins: 5
        }
        width: 64*3
        height: 64*3
        opacity: 0.5
        columns: 3

        Repeater {
            model: 9
            delegate: DropTile { colorKey: "red" }
        }
    }

    Grid {
        anchors {
            right: blueSource.left
            bottom: parent.bottom
            margins: 5
        }
        width: 64*3
        height: 64*3

        opacity: 0.5

        columns: 3

        Repeater {
            model: 9
            delegate: DropTile { colorKey: "blue" }
        }
    }

    Column {
        id: redSource

        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
            margins: 5
        }
        width: 64
        spacing: -16

        Repeater {
            model: 9
            delegate: DragTile { colorKey: "red" }
        }
    }
    Column {
        id: blueSource

        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
            margins: 5
        }
        width: 64
        spacing: -16

        Repeater {
            model: 9
            delegate: DragTile { colorKey: "blue" }
        }
    }
}
