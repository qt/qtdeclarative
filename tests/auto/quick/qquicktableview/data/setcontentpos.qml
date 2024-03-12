// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3

Item {
    width: 640
    height: 450

    property alias tableView: tableView

    TableView {
        id: tableView
        width: 400
        height: 400
        anchors.margins: 1
        clip: true
        delegate: tableViewDelegate
        contentX: (contentWidth - width) / 2;
        contentY: (contentHeight - height) / 2;
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            implicitWidth: 100
            implicitHeight: 100
            color: "lightgray"
        }
    }

}
