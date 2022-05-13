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

    function setRowsValid() {
        testModel.rows = [
            {
                name: "Max",
                age: 20
            },
            {
                name: "Imum",
                age: 41
            },
            {
                name: "Power",
                age: 89
            }
        ]
    }

    function setRowsInvalid() {
        testModel.rows = [
            {
                nope: "Nope",
                age: 20
            },
            {
                nope: "Nah",
                age: 41
            },
            {
                nope: "No",
                age: 89
            }
        ]
    }

    TableView {
        id: tableView
        anchors.fill: parent
        model: TestModel {
            id: testModel
        }
        delegate: Text {
            text: model.display
        }
    }
}
