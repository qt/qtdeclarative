// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12
import Qt.labs.qmlmodels 1.0

Item {
    id: root
    width: 200
    height: 200

    property alias testModel: testModel
    property alias tableView: tableView

    function setRows() {
        testModel.rows = [
            {
                name: "John",
                age: 22
            },
            {
                name: "Oliver",
                age: 33
            }
        ]
    }

    function appendJohn() {
        testModel.appendRow({
            name: "John",
            age: 22
        })
    }

    function appendOliver() {
        testModel.appendRow({
            name: "Oliver",
            age: 33
        })
    }

    TableModel {
        id: testModel
        objectName: "testModel"

        TableModelColumn { display: "name" }
        TableModelColumn { display: "age" }
    }
    TableView {
        id: tableView
        anchors.fill: parent
        model: testModel
        delegate: Text {
            text: model.display
        }
    }
}
