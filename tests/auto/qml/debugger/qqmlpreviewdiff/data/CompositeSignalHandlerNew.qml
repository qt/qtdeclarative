// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as Old but trigger color changed.

import QtQuick

Item {
    width: 200
    height: 200

    CompositeWithSignalHandler {
        id: handler
        width: parent.width
        height: parent.height
    }

    Rectangle {
        id: trigger
        width: 50
        height: 50
        color: "yellow"
    }
}
