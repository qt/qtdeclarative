// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQml.Models
import Qt.labs.qmlmodels

Window {
    width: 480
    height: 320
    visible: true
    visibility: Window.AutomaticVisibility

//![0]
    TableView {
        id: tableView
        anchors.fill: parent
        clip: true
        acceptedButtons: Qt.NoButton // mouse-drag changes selection

        model: TableModel {
            TableModelColumn { display: "name" }
            TableModelColumn { display: "age" }
            rows: [ { "name": "Harry", "age": "11" },
                    { "name": "Hedwig", "age": "8" } ]
        }

        selectionModel: ItemSelectionModel {
            model: tableView.model
        }

        delegate: TableViewDelegate { padding: 2 }
    }

    SelectionRectangle {
        target: tableView
    }
//![0]
}
