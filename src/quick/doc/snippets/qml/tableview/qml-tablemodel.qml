// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick
import Qt.labs.qmlmodels

TableView {
    anchors.fill: parent
    columnSpacing: 1
    rowSpacing: 1
    clip: true

    model: TableModel {
        TableModelColumn { display: "name" }
        TableModelColumn { display: "color" }

        rows: [
            {
                "name": "cat",
                "color": "black"
            },
            {
                "name": "dog",
                "color": "brown"
            },
            {
                "name": "bird",
                "color": "white"
            }
        ]
    }

    delegate: Rectangle {
        implicitWidth: 100
        implicitHeight: 50
        border.width: 1

        Text {
            text: display
            anchors.centerIn: parent
        }
    }
}
//![0]
