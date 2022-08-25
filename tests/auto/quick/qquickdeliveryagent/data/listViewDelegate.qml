// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Rectangle {
    id: root
    width: 320
    height: 240

    ListView {
        anchors.fill: parent
        id: listView
        objectName: "listView"
        width: 320
        height: 240
        model: ListModel {}
        delegate: Rectangle {
            objectName: "delegate"
            width: ListView.view.width
            height: ListView.view.height / ListView.view.count
            color: model.color
        }
    }

    Timer {
        interval: 100
        running: true
        repeat: false
        onTriggered: {
            listView.model.append({color: "red"});
        }
    }
}