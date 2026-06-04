// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
        color: "blue"
    }
}
