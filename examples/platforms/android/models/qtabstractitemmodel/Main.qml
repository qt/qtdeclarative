// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls

//! [0]
Rectangle {
    id: mainRectangle

    property AbstractItemModel dataModel
//! [0]
    color: "#00414A"
    border.color: "#00414A"
    border.width: 2

//! [1]
    TableView {
        id: tableView

        model: mainRectangle.dataModel

        anchors {fill: parent; margins: 20}
        columnSpacing: 4
        rowSpacing: 6
        boundsBehavior: TableView.OvershootBounds
        clip: true

        ScrollBar.vertical: ScrollBar {
           policy: ScrollBar.AsNeeded
        }
        ScrollBar.horizontal: ScrollBar{
           policy: ScrollBar.AsNeeded
        }
//! [1]

//! [2]
        delegate: Rectangle {
            width: tableView.width
            color: "#2CDE85"
            border.color: "#00414A"
            border.width: 2
            Text {
                // Calls MyDataModel::data to get data based on the roles.
                // Called in Qt qtMainLoopThread thread context.
                text: model.row + model.column
                font.pixelSize: 26
                font.bold: true
            }
        }
//! [2]
    }
}
