// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: root
    width: 100
    height: 100
    property int rows: 1
    Repeater {
        model: root.rows
        RepeaterDelegate { objectName: "cell" }
    }
}
