// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.7

Item {
    id: root
    height: 1000
    width: 1000

    GridView {
        id: grid
        objectName: "grid"
        height: 500
        width: parent.width
        topMargin: 20
        leftMargin: 200
        rightMargin: 200
        bottomMargin: 30
        cellWidth: (grid.width - grid.leftMargin - grid.rightMargin) / 3
        model: 9
        delegate: Rectangle {
            border.width: 1
            objectName: "child"
            width: grid.cellWidth
            height: 100
            Text {
                anchors.centerIn: parent
                text: modelData
            }
        }
    }
}
