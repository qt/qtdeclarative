// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    width: 100
    height: 100
    color: "green"
    property int base: 5
    property int computed: base + 1

    Timer {
        repeat: true
        interval: 300
        running: true
        onTriggered: console.log("binding computed=" + parent.computed)
    }
}
