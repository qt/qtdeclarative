// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    width: 200
    height: 200
    color: "white"

    Rectangle {
        id: target
        objectName: "target"
        x: 50
        y: 50
        width: 100
        height: 100
        color: "red"
    }

    ShaderEffectSource {
        objectName: "effectSource"
        width: 0
        height: 0
    }

    OpacityAnimator {
        objectName: "animator"
        target: target
        from: 0.5
        to: 1
        duration: 1000000
    }
}
