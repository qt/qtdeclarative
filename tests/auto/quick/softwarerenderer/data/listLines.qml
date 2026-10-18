// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-136943
Item {
    width: 160; height: 120
    property int frames: 40
    property int tolerance: 0
    property bool historyDependent: true

    Rectangle { anchors.fill: parent; color: "black" }
    ListView {
        id: list
        anchors.fill: parent
        clip: true
        model: 60
        interactive: false
        delegate: Item {
            width: ListView.view.width
            height: 65
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "red"
            }
        }
    }

    function step(f) {
        list.contentY = f * 2.37
    }
}
