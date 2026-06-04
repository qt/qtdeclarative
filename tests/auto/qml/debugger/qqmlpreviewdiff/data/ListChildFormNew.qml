// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as ListChildFormOld.qml but with a changed constant to trigger rebuild.

import QtQml
import QtQuick

Item {
    id: root
    property list<Timer> timers: [t1, t2]

    Timer {
        id: t1
        interval: 100
    }

    Timer {
        id: t2
        interval: 200
    }

    Rectangle {
        id: indicator
        width: 50
        height: 50
        color: "blue"
    }
}
