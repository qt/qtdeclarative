// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    width: 200
    height: 200

    ChildItem {
        id: child
        anchors.fill: parent
    }

    Timer {
        repeat: true
        interval: 300
        running: true
        onTriggered: console.log("typeref_test label=" + child.label)
    }
}
