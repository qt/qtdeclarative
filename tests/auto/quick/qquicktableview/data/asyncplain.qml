// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3
import TestModel 0.1

Item {
    id: root
    width: 640
    height: 450

    property alias tableView: tableView
    property Loader loader: parent

    property int statusWhenDelegate0_0Created: Loader.Null
    property int statusWhenDelegate5_5Created: Loader.Null

    property real tableViewWidthWhileBuilding: -1
    property real tableViewHeightWhileBuilding: -1

    TableView {
        id: tableView
        anchors.fill: parent
        clip: true
        delegate: tableViewDelegate
        columnSpacing: 1
        rowSpacing: 1
        model: TestModel {
            rowCount: 100
            columnCount: 100
        }
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            implicitWidth: 100
            implicitHeight: 50
            color: "lightgray"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: modelData
            }

            Component.onCompleted: {
                if (row === 0 && column === 0) {
                    statusWhenDelegate0_0Created = loader.status
                    tableViewWidthWhileBuilding = tableView.width
                    tableViewHeightWhileBuilding = tableView.height
                }
                else if (row === 5 && column === 5)
                    statusWhenDelegate5_5Created = loader.status
            }
        }
    }

}
