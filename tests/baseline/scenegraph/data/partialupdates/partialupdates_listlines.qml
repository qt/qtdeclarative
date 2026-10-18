// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-136943
Item {
    id: root
    width: 320; height: 240
    property bool suspendGrabbing: stepper.running
    Stepper { id: stepper; scene: root; frames: 60 }

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
