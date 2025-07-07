// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import Qt.labs.qmlmodels

TableModel {
    id: testModel
    objectName: "testModel"

    TableModelColumn { display: "name" }
    TableModelColumn { display: "age" }

    rows: [
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
