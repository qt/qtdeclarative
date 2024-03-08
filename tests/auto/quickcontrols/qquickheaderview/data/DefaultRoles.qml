// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Controls
import TestTableModel

ApplicationWindow {
    width: 400
    height: 400

    property alias headerView: headerView

    Column {
        HorizontalHeaderView {
            id: headerView

            width: 200
            height: 200

            model: TestTableModel {
                id: tm
                objectName: "tableModel"
                rowCount: 1
                columnCount: 1
            }
            textRole: "toolTip"
            delegate: Rectangle {
                required property string toolTip
                implicitWidth: 40
                implicitHeight: 40
            }
        }
    }
}
