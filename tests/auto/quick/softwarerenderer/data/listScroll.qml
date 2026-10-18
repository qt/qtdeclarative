// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-136943
Item {
    width: 161; height: 121
    property int frames: 24
    property int tolerance: 0

    Rectangle { anchors.fill: parent; color: "white" }
    ListView {
        id: list
        anchors.fill: parent
        model: 40
        interactive: false
        delegate: Rectangle {
            required property int index
            width: ListView.view.width; height: 23
            color: index % 2 ? "#f0f0f0" : "#d8e8f8"
            border.width: 1; border.color: "#808080"
        }
    }

    function step(f) {
        list.contentY = f * 3.7
    }
}
