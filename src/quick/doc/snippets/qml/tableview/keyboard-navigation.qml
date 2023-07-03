// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import Qt.labs.qmlmodels

//![0]
ApplicationWindow {
    width: 800
    height: 600
    visible: true
    ScrollView {
        anchors.fill: parent
        TableView {
            id: tableView
            clip: true
            interactive: true
            rowSpacing: 1
            columnSpacing: 1
            model: TableModel {
                TableModelColumn { display: "checked" }
                TableModelColumn { display: "amount" }
                TableModelColumn { display: "fruitType" }
                TableModelColumn { display: "fruitName" }
                TableModelColumn { display: "fruitPrice" }

                rows: [
                    {
                        checked: false,
                        amount: 1,
                        fruitType: "Apple",
                        fruitName: "Granny Smith",
                        fruitPrice: 1.50
                    },
                    {
                        checked: true,
                        amount: 4,
                        fruitType: "Orange",
                        fruitName: "Navel",
                        fruitPrice: 2.50
                    },
                    {
                        checked: false,
                        amount: 1,
                        fruitType: "Banana",
                        fruitName: "Cavendish",
                        fruitPrice: 3.50
                    }
                ]
            }
            selectionModel: ItemSelectionModel {}
            delegate: Rectangle {
                implicitWidth: 100
                implicitHeight: 50
                required property bool selected
                required property bool current
                border.width: current ? 2 : 0
                color: selected ? "lightblue" : palette.base
                Text{
                    text: model.display
                    padding: 12
                }
            }
        }
    }
    SelectionRectangle {
        target: tableView
    }
}
//![0]
