// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
            syncView: tableView
            textRole: "customRole"
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
