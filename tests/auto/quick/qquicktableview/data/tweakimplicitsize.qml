// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3

Item {
    width: 640
    height: 450

    property alias tableView: tableView
    property real delegateSize: 10
    property int hideRow: -1

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
            return -1
        }
        rowHeightProvider: function(row) {
            if (row === hideRow)
                return 0
            return -1
        }
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            objectName: "tableViewDelegate"
            implicitWidth: row === 0 ? 10 : delegateSize
            implicitHeight: column === 0 ? 10 : delegateSize
            color: "lightgray"
            border.width: 1

            Text {
                id: textItem
                anchors.centerIn: parent
                text: model.display
                renderType: Text.NativeRendering
            }
        }
    }

}

