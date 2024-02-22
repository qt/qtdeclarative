// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml

QtObject {
    id: root

    property bool cycleEnabled: false
    property bool cycleFirst: false
    property bool cycleSecond: false

    property Timer enableTimer: Timer {
        running: root.cycleEnabled
        interval: 1
        onTriggered: {
            conn.enabled = !conn.enabled;
            root.cycleEnabled = false;
        }
    }

    property Timer firstTimer: Timer {
        id: firstTimer
        objectName: "first"
        running: root.cycleFirst
        interval: 1
        onTriggered: root.cycleFirst = false
    }

    property Timer secondTimer: Timer {
        objectName: "second"
        running: root.cycleSecond
        interval: 1
        onTriggered: conn.target = this;
        repeat: true
    }

    property Connections conn: Connections {
        id: conn
        target: firstTimer
        function onTriggered(m) {
            root.objectName = target.objectName
            root.cycleSecond = false;
        }
    }
}
