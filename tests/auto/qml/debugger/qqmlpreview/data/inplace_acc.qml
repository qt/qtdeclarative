// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    property int acc: 0

    Timer {
        repeat: true
        interval: 300
        running: true
        onTriggered: console.log("acc=" + parent.acc)
    }
}
