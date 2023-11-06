// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick

Item {
    width: 300
    height: 300

    property alias tableView: tableView
    property bool positionOnRowsChanged: false
    property bool positionOnColumnsChanged: false
    property bool positionOnContentHeightChanged: false
    property bool positionOnContentWidthChanged: false

    TableView {
        id: tableView
        anchors.fill: parent
        clip: true

        delegate: Rectangle {
            implicitWidth: 100
            implicitHeight: 100
            Text {
                anchors.centerIn: parent
                text: "row:" + row + "\ncol:" + column
                font.pixelSize: 10
            }
        }

        onRowsChanged: {
            if (positionOnRowsChanged)
                positionViewAtRow(rows - 1, TableView.AlignBottom)
        }

        onColumnsChanged: {
            if (positionOnColumnsChanged)
                positionViewAtColumn(columns - 1, TableView.AlignRight)
        }

        onContentHeightChanged: {
            if (positionOnContentHeightChanged)
                positionViewAtRow(rows - 1, TableView.AlignBottom)
        }

        onContentWidthChanged: {
            if (positionOnContentWidthChanged)
                positionViewAtColumn(columns - 1, TableView.AlignRight)
        }
    }

}
