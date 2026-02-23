// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Uses a composite type that has an explicit Component child.

import QtQuick

Item {
    width: 300
    height: 200

    CompositeWithComponent {
        id: comp
        width: parent.width
        height: parent.height
        label.text: "Hello"
    }

    property int marker: 1
}
