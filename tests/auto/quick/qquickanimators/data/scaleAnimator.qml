// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: root
    property alias animator: scaleAnimator
    property alias rectangle: rectangle
    ScaleAnimator {
        id: scaleAnimator
        target: rectangle
        from: 1.0
        to: 0.5
        loops: Animation.Infinite
    }
    Rectangle {
        id: rectangle
        color:"red"
        opacity: 0.75
        width: 200
        height: 200
    }
}
