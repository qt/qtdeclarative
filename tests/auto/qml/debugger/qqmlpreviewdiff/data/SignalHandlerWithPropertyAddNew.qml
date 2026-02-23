// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick

Rectangle {
    width: 100
    height: 100
    color: "blue"
    property int count: 10
    property string label: "new"

    Timer {
        repeat: true
        interval: 300
        running: true
        onTriggered: console.log("count=" + parent.count + " label=" + parent.label)
    }
}
