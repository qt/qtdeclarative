// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Window 2.2

Rectangle {
    id: root
    width: 200
    height: 200
    visible: true

    property alias rect: rect
    property alias numberAnimation: numberAnimation
    property alias loops: numberAnimation.loops

    Rectangle {
        id: rect
        width: 100
        height: 10
        color: "tomato"

        NumberAnimation on x {
            id: numberAnimation
            from: -rect.width
            to: root.width
            duration: 60
            easing.type: Easing.Linear
        }
    }
}
