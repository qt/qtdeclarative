// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![file]
import QtQuick
import QtQuick.Controls
import Qt.labs.qmlmodels

ApplicationWindow {
    visible: true
    width: 640
    height:  480

    //! [horizontal]
    HorizontalHeaderView {
        id: horizontalHeader
        syncView: tableView
        anchors.left: tableView.left
    }
    //! [horizontal]

    //! [vertical]
    VerticalHeaderView {
        id: verticalHeader
        syncView: tableView
        anchors.top: tableView.top
    }
    //! [vertical]

    TableView {
        id: tableView
        anchors.fill: parent
        anchors.topMargin: horizontalHeader.height
        anchors.leftMargin: verticalHeader.width
        columnSpacing: 1
        rowSpacing: 1
        clip: true

        model: TableModel {
            TableModelColumn { display: "name" }
            TableModelColumn { display: "color" }

            rows: [
                {
                    "name": "cat",
                    "color": "black"
                },
                {
                    "name": "dog",
                    "color": "brown"
                },
                {
                    "name": "bird",
                    "color": "white"
                }
            ]
        }

        delegate: Rectangle {
            implicitWidth: 100
            implicitHeight: 50
            border.width: 1

            Text {
                text: display
                anchors.centerIn: parent
            }
        }
    }
}

//![file]
