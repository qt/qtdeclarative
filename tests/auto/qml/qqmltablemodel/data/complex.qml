// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import Qt.labs.qmlmodels

TableView {
    width: 100
    height: 100
    delegate: Item {
        implicitWidth: 50
        implicitHeight: 50

        Text {
            text: model.display
            anchors.centerIn: parent
        }
    }
    model: TableModel {
        id: testModel
        objectName: "testModel"

        TableModelColumn {
            display: function(modelIndex) { return testModel.rows[modelIndex.row][0].name }
            setDisplay: function(modelIndex, cellData) { testModel.rows[modelIndex.row][0].name = cellData }
        }
        TableModelColumn {
            display: function(modelIndex) { return testModel.rows[modelIndex.row][1].age }
            setDisplay: function(modelIndex, cellData) { testModel.rows[modelIndex.row][1].age = cellData }
        }

        rows: [
            [
                { name: "John" },
                { age: 22 }
            ],
            [
                { name: "Oliver" },
                { age: 33 }
            ]
        ]
    }
}
