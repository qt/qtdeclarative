// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: root
    property int counter: 0
    signal fired()

    Rectangle {
        id: indicator
        width: 50
        height: 50
        color: "red"
    }
}
