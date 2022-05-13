// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQml.Models

Item {
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
        columnSpacing: 1
        rowSpacing: 1
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            objectName: "tableViewDelegate"
            implicitWidth: 20
            implicitHeight: 20
            required property bool selected
            required property bool current
            color: selected ? "lightgray" : "green"
            border.color: current ? "red" : "transparent"
        }
    }

}
