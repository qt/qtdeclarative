// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window

Item {
    width: 640
    height: 450

    property alias tableView: tableView

    TableView {
        id: tableView
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: contentWidth
        delegate: tableViewDelegate
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            objectName: "tableViewDelegate"
            implicitWidth: tableView.width
            implicitHeight: 50
        }
    }

}
