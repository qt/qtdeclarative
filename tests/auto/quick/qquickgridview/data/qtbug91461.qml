// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

Item {
    width: 640
    height: 800

    property real cellSize: 100

    GridView {
        id: grid
        anchors.fill: parent
        model: 1000
        cellWidth: cellSize
        cellHeight: cellSize
        cacheBuffer: 0
        currentIndex: 0

        delegate: Rectangle {
            implicitWidth: grid.cellWidth - 10
            implicitHeight: grid.cellHeight - 10
            color: GridView.isCurrentItem ? "green" : "lightgreen"
            radius: 5
            border.width: GridView.isCurrentItem ? 3 : 1
            border.color: "black"

            Text {
                anchors.centerIn: parent
                text: model.index
            }
        }
    }
}
