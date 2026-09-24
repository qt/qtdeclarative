// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick

Item {
    property alias animation: animation

    Rectangle {
        width: 20
        height: 20

        NumberAnimation on x {
            id: animation
            from: 0
            to: 180
            duration: Math.abs(100)
            loops: Animation.Infinite
        }
    }
}
