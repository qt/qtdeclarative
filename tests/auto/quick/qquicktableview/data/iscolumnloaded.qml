// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Window

Item {
    width: 640
    height: 450

    property alias tableView: tableView
    property int itemsInColumnAfterLoaded: 0
    property int itemsInRowAfterLoaded: 0

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
            if (isColumnLoaded(column)) {
                var count = 0;
                for (var row = topRow; row <= bottomRow; ++row) {
                    var item = itemAtCell(Qt.point(column, row))
                    if (item)
                        count++;
                }
                itemsInColumnAfterLoaded = count;
            }

            return -1
        }
        rowHeightProvider: function(row) {
            if (isRowLoaded(row)) {
                var count = 0;
                for (var column = leftColumn; column <= rightColumn; ++column) {
                    var item = itemAtCell(Qt.point(column, row))
                    if (item)
                        count++;
                }
                itemsInRowAfterLoaded = count;
            }

            return -1
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
