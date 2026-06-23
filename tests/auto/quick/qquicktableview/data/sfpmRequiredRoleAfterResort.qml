// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import TestModel 0.1

Item {
    width: 640
    height: 450

    property alias tableView: tableView
    property alias sourceModel: sourceModel

    TestModel {
        id: sourceModel
        rowCount: 4
        columnCount: 1
    }

    SortFilterProxyModel {
        id: sortModel
        sourceModel: sourceModel
        sorters: [
            RoleSorter {
                roleName: "display"
                column: 0
                sortOrder: Qt.AscendingOrder
                enabled: true
            }
        ]
    }

    TableView {
        id: tableView
        width: parent.width
        height: parent.height
        model: sortModel
        delegate: Rectangle {
            required property string display
            implicitWidth: 200
            implicitHeight: 50
            color: "white"
        }
    }
}
