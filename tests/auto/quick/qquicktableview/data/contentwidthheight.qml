// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3

Item {
    width: 640
    height: 450

    property alias tableView: tableView
    property Component delegate: tableViewDelegate

    TableView {
        id: tableView
        width: 500
        height: 500
        anchors.margins: 1
        clip: true
        delegate: tableViewDelegate
        columnSpacing: 1
        rowSpacing: 1
        leftMargin: 10
        rightMargin: 10
        topMargin: 10
        bottomMargin: 10
        columnWidthProvider: function(column) { return column < 20 ? 100 : 200 }
        rowHeightProvider: function(row) { return row < 20 ? 100 : 200 }
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            objectName: "tableViewDelegate"
            color: "lightgray"
            border.width: 1
            Text {
                anchors.centerIn: parent
                text: column
            }
        }
    }

}
