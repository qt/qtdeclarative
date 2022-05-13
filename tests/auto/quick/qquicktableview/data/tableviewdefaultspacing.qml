// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12
import QtQuick.Window 2.3

Item {
    width: 640
    height: 450

    property alias tableView: tableView

    TableView {
        id: tableView
        width: 600
        height: 400
        anchors.margins: 1
        delegate: tableViewDelegate
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            objectName: "tableViewDelegate"
            color: "lightgray"
            border.width: 1
            implicitWidth: 15
            implicitHeight: 10
            Text {
                anchors.centerIn: parent
                text: modelData
            }
        }
    }

}
