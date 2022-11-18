// Copyright (C) 2022 The Qt Company Ltd.
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

        selectionModel: ItemSelectionModel {}

        delegate: Rectangle {
            implicitWidth: 100
            implicitHeight: 50

            Text {
                anchors.centerIn: parent
                text: display
            }

            TableView.editDelegate: TextField {
                text: display
                horizontalAlignment: TextInput.AlignHCenter
                verticalAlignment: TextInput.AlignVCenter
                Component.onCompleted: selectAll()

                TableView.onCommit: {
                    let index = TableView.view.modelIndex(column, row)
                    TableView.view.model.setData(index, text, Qt.DisplayRole)
                }
            }
        }
    }
//![0]
}
