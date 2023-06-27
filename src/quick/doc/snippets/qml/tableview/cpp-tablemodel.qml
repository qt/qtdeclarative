// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick
import TableModel 0.1

TableView {
    anchors.fill: parent
    columnSpacing: 1
    rowSpacing: 1
    clip: true

    model: TableModel {}

    delegate: Rectangle {
        implicitWidth: 100
        implicitHeight: 50
        Text {
            text: display
        }
    }
}
//![0]
