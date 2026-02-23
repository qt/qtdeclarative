// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A composite type with a signal handler that references internal IDs.

import QtQuick

Item {
    id: root

    property alias indicator: statusRect
    signal activated()

    Rectangle {
        id: statusRect
        width: 20
        height: 20
        color: "gray"
    }

    Timer {
        id: pulseTimer
        interval: 100
        onTriggered: statusRect.color = "green"
    }

    onActivated: {
        statusRect.color = "blue"
        pulseTimer.start()
    }
}
