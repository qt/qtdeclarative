// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12
import Qt.labs.qmlmodels 1.0

Item {
    id: root
    width: 200
    height: 200

    property alias testModel: testModel
    property alias tableView: tableView

    function appendBanana() {
        testModel.appendRow({
            fruit: "Banana",
            price: 3.5
        })
    }

    function appendStrawberry() {
        testModel.appendRow({
            fruit: "Strawberry",
            price: "5"
        })
    }

    function appendInvalid() {
        testModel.appendRow({
            fruit: "Pear",
            price: "Invalid"
        })
    }

    TableModel {
        id: testModel
        objectName: "testModel"

        TableModelColumn { display: "fruit" }
        TableModelColumn { display: "price" }
        rows: [
            {
                fruit: "Apple",
                price: 1
            },
            {
                fruit: "Orange",
                price: 2
            }
        ]
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
