// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![file]
import QtQuick
import QtQuick.Window
import Qt.labs.qmlmodels

Window {
    width: 400
    height: 400
    visible: true

    TableView {
        anchors.fill: parent
        columnSpacing: 1
        rowSpacing: 1
        boundsBehavior: Flickable.StopAtBounds

        model: TableModel {
            TableModelColumn {
                display: function(modelIndex) { return rows[modelIndex.row][0].checked }
                setDisplay: function(modelIndex, cellData) { rows[modelIndex.row][0].checked = cellData }
            }
            TableModelColumn {
                display: function(modelIndex) { return rows[modelIndex.row][1].amount }
                setDisplay: function(modelIndex, cellData) { rows[modelIndex.row][1].amount = cellData }
            }
            TableModelColumn {
                display: function(modelIndex) { return rows[modelIndex.row][2].fruitType }
                setDisplay: function(modelIndex, cellData) { rows[modelIndex.row][2].fruitType = cellData }
            }
            TableModelColumn {
                display: function(modelIndex) { return rows[modelIndex.row][3].fruitName }
                setDisplay: function(modelIndex, cellData) { rows[modelIndex.row][3].fruitName = cellData }
            }
            TableModelColumn {
                display: function(modelIndex) { return rows[modelIndex.row][4].fruitPrice }
                setDisplay: function(modelIndex, cellData) { rows[modelIndex.row][4].fruitPrice = cellData }
            }

            // Each row is one type of fruit that can be ordered
//![rows]
            rows: [
                [
                    // Each object (line) is one cell/column.
                    { checked: false, checkable: true },
                    { amount: 1 },
                    { fruitType: "Apple" },
                    { fruitName: "Granny Smith" },
                    { fruitPrice: 1.50 }
                ],
                [
                    { checked: true, checkable: true },
                    { amount: 4 },
                    { fruitType: "Orange" },
                    { fruitName: "Navel" },
                    { fruitPrice: 2.50 }
                ],
                [
                    { checked: false, checkable: false },
                    { amount: 1 },
                    { fruitType: "Banana" },
                    { fruitName: "Cavendish" },
                    { fruitPrice: 3.50 }
                ]
            ]
//![rows]
        }
//![delegate]
        delegate:  TextInput {
            text: model.display
            padding: 12
            selectByMouse: true

            onAccepted: model.display = text

            Rectangle {
                anchors.fill: parent
                color: "#efefef"
                z: -1
            }
        }
//![delegate]
    }
}
//![file]
