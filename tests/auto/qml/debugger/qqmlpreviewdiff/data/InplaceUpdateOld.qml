// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15

Rectangle {
    width: 100
    height: 100
    color: "blue"
    property int count: 10

    Timer {
        interval: 300
        repeat: true
        onTriggered: console.log("count=" + parent.count)
    }
}
