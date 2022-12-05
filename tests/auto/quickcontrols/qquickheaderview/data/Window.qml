// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import TestTableModel
import TestTableModelWithHeader
import HeaderDataProxyModel

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

    HeaderDataProxyModel {
        id: pm
        objectName: "proxyModel"
    }

    TestTableModel {
        id: tm
        objectName: "tableModel"
        rowCount: 5
        columnCount: 10
    }

    TestTableModelWithHeader {
        id: thm
        objectName: "tableHeaderModel"
        rowCount: 5
        columnCount: 10
    }

    HorizontalHeaderView {
        id: hhv
        objectName: "horizontalHeader"
        width: 200
        height: 200
        model: thm
        delegate: cellDelegate
    }

    VerticalHeaderView {
        id: vhv
        objectName: "verticalHeader"
        width: 200
        height: 200
        model: thm
        delegate: cellDelegate
    }

    TableView {
        id: tv
        objectName: "tableView"
        width: 400
        height: 400
        model: thm
        delegate:cellDelegate
    }
}
