// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3

Item {
    id: root
    width: 640
    height: 450

    property alias tableView: tableView

    TableView {
        id: tableView
        width: 600
        height: 400
        anchors.margins: 1
        clip: true
        delegate: tableViewDelegate
    }

    Item {
        Repeater {
            model: 100
            Item { property string someCustomProperty: index }
        }
        Component.onCompleted: tableView.model = children
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            objectName: "tableViewDelegate"
            implicitWidth: 100
            implicitHeight: 50
            color: "lightgray"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: column
            }
        }
    }

}
