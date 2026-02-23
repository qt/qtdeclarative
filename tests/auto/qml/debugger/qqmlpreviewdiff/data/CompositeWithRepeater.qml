// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A composite type with a Repeater creating dynamic children based on a model.
// The Repeater's delegate references the parent's ID.

import QtQuick

Item {
    id: root

    property alias items: repeater
    property int count: 3

    Column {
        id: column
        Repeater {
            id: repeater
            model: root.count
            Rectangle {
                width: root.width
                height: 20
                color: index % 2 === 0 ? "lightblue" : "lightyellow"
            }
        }
    }
}
