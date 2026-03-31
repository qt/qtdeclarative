// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: root
    width: Colors.derivedSize
    height: Colors.baseSize
    color: Colors.currentColor

    Timer {
        repeat: true
        interval: 300
        running: true
        onTriggered: console.log("selfbind_color color=" + root.color)
    }
}
