// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3

Item {
    width: 640
    height: 450

    property alias tableView: tableView
    property Component delegate: tableViewDelegate
    property bool returnUndefinedColumnWidth: false
    property bool returnUndefinedRowHeight: false
    property bool returnNegativeColumnWidth: false
    property bool returnNegativeRowHeight: false

    TableView {
        id: tableView
        width: 600
        height: 400
        anchors.margins: 1
        clip: true
        delegate: tableViewDelegate
        columnSpacing: 1
        rowSpacing: 1
        columnWidthProvider: function(column) {
            if (returnNegativeColumnWidth)
                return -1
            if (!returnUndefinedColumnWidth)
                return column + 10
        }
        rowHeightProvider: function(row) {
            if (returnNegativeRowHeight)
                return -1
            if (!returnUndefinedRowHeight)
                return row + 10
        }
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            objectName: "tableViewDelegate"
            implicitWidth: 20
            implicitHeight: 20
            color: "lightgray"
            border.width: 1
            Text {
                anchors.centerIn: parent
                text: modelData
            }
        }
    }

}
