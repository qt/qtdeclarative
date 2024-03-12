// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3
import TestModel 0.1

Item {
    width: 640
    height: 450

    property alias tableView: tableView
    property bool addRowFromDelegate: false

    onAddRowFromDelegateChanged: {
        if (!addRowFromDelegate)
            return;
        tableModel.addRow(0);
        tableView.forceLayout();
     }

    TestModel {
        id: tableModel
        rowCount: 1
        columnCount: 4
    }

    TableView {
        id: tableView
        width: 600
        height: 400
        clip: true
        model: tableModel
        delegate: tableViewDelegate
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            objectName: "tableViewDelegate"
            implicitWidth: 100
            implicitHeight: 100
            color: "lightgray"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: modelData
            }

            Component.onCompleted: {
                if (!addRowFromDelegate)
                    return;
                addRowFromDelegate = false;
                tableModel.addRow(0);
            }
        }
    }

}

