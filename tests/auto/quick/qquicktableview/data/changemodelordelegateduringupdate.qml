// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3

Item {
    width: 640
    height: 450

    property alias tableView: tableView
    property bool changeDelegate: false
    property bool changeModel: false

    TableView {
        id: tableView
        width: 600
        height: 400
        clip: true
        delegate: tableViewDelegate
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            implicitWidth: 100
            implicitHeight: 100
            color: "lightgray"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: modelData
            }

            Component.onCompleted: {
                if (changeDelegate)
                    TableView.view.delegate = null
                if (changeModel)
                    TableView.view.model = null
            }
        }
    }

}
