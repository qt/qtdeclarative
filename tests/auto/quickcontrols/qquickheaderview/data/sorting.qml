// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import TestTableModelWithHeader

Window {
    objectName: "sorting"
    width: 400
    height: 400
    visible: true

    TestTableModelWithHeader {
        id: tableHeaderModel
        objectName: "tableHeaderModel"
        rowCount: 4
        columnCount: 2
    }

    HorizontalHeaderView {
        id: hh
        objectName: "horizontalHeader"
        anchors.top: parent.top
        x: vh.width
        syncView: tv
        showSortIndicator: true
        sortIndicatorClearable: true
    }

    VerticalHeaderView {
        id: vh
        objectName: "verticalHeader"
        anchors.left: parent.left
        y: hh.height
        syncView: tv
    }

    TableView {
        id: tv
        objectName: "tableView"
        anchors.top: hh.bottom
        anchors.left: vh.right
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        model: sortModel
        sortingEnabled: true
        delegate: Rectangle {
            implicitHeight: 25
            implicitWidth: 50
            color: "red"
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
