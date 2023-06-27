// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
TableView {
    id: tableView

    property var columnWidths: [100, 50, 80, 150]
    columnWidthProvider: function (column) { return columnWidths[column] }

    Timer {
        running: true
        interval: 2000
        onTriggered: {
            tableView.columnWidths[2] = 150
            tableView.forceLayout();
        }
    }
}
//![0]
