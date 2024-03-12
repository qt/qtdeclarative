// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3

Item {
    width: 640
    height: 450

    property alias tableView: tableView
    property real columnWidths: 80

    TableView {
        id: tableView
        width: 600
        height: 400
        anchors.margins: 1
        delegate: tableViewDelegate
        columnSpacing: 1
        rowSpacing: 1

        columnWidthProvider: function(c) { return columnWidths; }
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            color: "lightgray"
            border.width: 1
            implicitHeight: 100

            Text {
                anchors.centerIn: parent
                text: modelData
            }
        }
    }

}
