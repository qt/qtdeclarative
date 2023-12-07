// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQml.Models
import Qt.labs.qmlmodels

Window {
    width: 480
    height: 640
    visible: true
    visibility: Window.AutomaticVisibility

//![0]
    TableView {
        id: tableView
        anchors.fill: parent
        clip: true

        model: TableModel {
            TableModelColumn { display: "name" }
            rows: [ { "name": "Harry" }, { "name": "Hedwig" } ]
        }

        selectionModel: ItemSelectionModel {
            model: tableView.model
        }

        delegate: Rectangle {
            implicitWidth: 100
            implicitHeight: 30
            color: selected ? "blue" : "lightgray"

            required property bool selected

            Text { text: display }
        }
    }

    SelectionRectangle {
        target: tableView
    }
//![0]
}
