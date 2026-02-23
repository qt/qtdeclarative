// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as Old but marker changed.

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

    property int marker: 2
}
