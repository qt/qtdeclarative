// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import TestTableModelWithHeader

ApplicationWindow {
    width: 400
    height: 400

    property alias headerView: headerView

    Column {
        HorizontalHeaderView {
            id: headerView
            objectName: "horizontalHeader"
            syncView: tableView
            textRole: "customRole"
            movableColumns: true
        }
        TableView {
            id: tableView
            width: 100
            height: 100
            model: TestTableModelWithHeader {
                rowCount: 2
                columnCount: 2
            }
            delegate: Label {
                text: customRole
                leftPadding: 10
                rightPadding: 10

                required property string customRole
            }
        }
    }
}
