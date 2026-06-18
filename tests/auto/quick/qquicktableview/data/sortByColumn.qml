// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import TestModel 0.1

ApplicationWindow {
    width: 400
    height: 400
    visible: true

    TableView {
        id: tv
        objectName: "tableView"
        width: parent.width
        height: parent.height
        model: sortModel
    }

    SortFilterProxyModel {
        id: sortModel
        objectName: "sortModel"
        sourceModel: TestModel {
            rowCount: 4
            columnCount: 2
        }

        sorters: [
            RoleSorter {
                roleName: "display"
                column: tv.sortColumn >= 0 ? tv.sortColumn : 0
                sortOrder: tv.sortOrder
                enabled: tv.sortColumn >= 0
            }
        ]
    }
}
