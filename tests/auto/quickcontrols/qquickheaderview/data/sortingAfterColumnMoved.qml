// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import TestTableModelWithHeader

ApplicationWindow {
    width: 400
    height: 400
    visible: true

    TestTableModelWithHeader {
        id: tableHeaderModel
        objectName: "tableHeaderModel"
        rowCount: 2
        columnCount: 2
    }

    HorizontalHeaderView {
        id: hh
        objectName: "horizontalHeader"
        width: 200
        height: 25
        syncView: tv
        showSortIndicator: true
        movableColumns: true
    }

    TableView {
        id: tv
        objectName: "tableView"
        y: hh.height
        width: 200
        height: 100
        model: sortModel
        sortingEnabled: true
        delegate: Rectangle {
            implicitHeight: 25
            implicitWidth: 50
            Text {
                text: model.display
            }
        }
    }

    SortFilterProxyModel {
        id: sortModel
        sourceModel: tableHeaderModel
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
