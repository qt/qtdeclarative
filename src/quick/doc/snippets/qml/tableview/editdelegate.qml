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
                anchors.fill: parent
                text: display
                horizontalAlignment: TextInput.AlignHCenter
                verticalAlignment: TextInput.AlignVCenter
                Component.onCompleted: selectAll()

                TableView.onCommit: {
                    display = text
                    // 'display = text' is short-hand for:
                    // let index = TableView.view.index(row, column)
                    // TableView.view.model.setData(index, text, Qt.DisplayRole)
                }
            }
        }
    }
//![0]

    TableView {
        id: tableView1
        anchors.fill: parent
        clip: true

        model: TableModel {
            TableModelColumn { display: "name" }
            rows: [ { "name": "Harry" }, { "name": "Hedwig" } ]
        }

        selectionModel: ItemSelectionModel {}

//![1]
        delegate: Rectangle {
            implicitWidth: 100
            implicitHeight: 50

            required property bool editing

            Text {
                id: textField
                anchors.fill: parent
                anchors.margins: 5
                text: display
                visible: !editing
            }

            TableView.editDelegate: TextField {
                x: textField.x
                y: textField.y
                width: textField.width
                height: textField.height
                text: display
                TableView.onCommit: display = text
            }
        }
//![1]
    }

}
