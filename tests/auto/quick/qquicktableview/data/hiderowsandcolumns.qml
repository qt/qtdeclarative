// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3

Item {
    width: 640
    height: 450

    property alias tableView: tableView
    property var rowsToHide
    property var columnsToHide

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
            if (columnsToHide.includes(column))
                return 0;
        }
        rowHeightProvider: function(row) {
            if (rowsToHide.includes(row))
                return 0;
        }
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            objectName: "tableViewDelegate"
            color: "lightgray"
            border.width: 1
            implicitWidth: 50
            implicitHeight: 50
            Text {
                anchors.centerIn: parent
                text: column + "," + row
            }
        }
    }

}
