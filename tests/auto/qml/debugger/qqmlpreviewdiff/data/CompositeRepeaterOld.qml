// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Composite with Repeater — rebuild must not crash when dynamic children exist.

import QtQuick

Item {
    width: 200
    height: 200
    property string tag: "old"

    CompositeWithRepeater {
        id: rep
        width: parent.width
        height: parent.height
        count: 5
    }
}
