// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A component with a child Timer exposed via alias. The outer wrapper sets
// an external binding on "button.interval". When this CU is rebuilt, the
// Timer child gets replaced and the binding must be migrated to the new object.

import QtQml
import QtQuick

Item {
    id: root
    property alias button: button

    Timer {
        id: button
        interval: 1000
    }

    Rectangle {
        id: indicator
        width: 50
        height: 50
        color: "red"
    }
}
