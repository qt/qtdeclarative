// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import TestTableModel

Window {
    objectName: "window"
    width: 400
    height: 400
    visible: true

    Component {
        id: cellDelegate
        Rectangle {
            implicitHeight: 25
            implicitWidth: 50
            color: "red"
            Text {
                text: row + "," + column
            }
        }
    }

    HorizontalHeaderView {
        id: hhv
        objectName: "horizontalHeader"
        model: ["AAA", "BBB", "CCC", "DDD", "EEE"]
        syncView: tv
        anchors.top: parent.top
        x: vhv.width
    }

    VerticalHeaderView {
        id: vhv
        objectName: "verticalHeader"
        model: ["111", "222", "333", "444", "555"]
        syncView: tv
        anchors.left: parent.left
        y: hhv.height
    }

    TableView {
        id: tv
        objectName: "tableView"
        model: TestTableModel {
            id: tm
            objectName: "tableModel"
            rowCount: 5
            columnCount: 5
        }
        delegate: cellDelegate
        anchors.top: hhv.bottom
        anchors.left: vhv.right
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
