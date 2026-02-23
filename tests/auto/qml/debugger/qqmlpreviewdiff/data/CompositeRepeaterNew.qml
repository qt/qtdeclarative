// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as Old but tag changed.

import QtQuick

Item {
    width: 200
    height: 200
    property string tag: "new"

    CompositeWithRepeater {
        id: rep
        width: parent.width
        height: parent.height
        count: 5
    }
}
