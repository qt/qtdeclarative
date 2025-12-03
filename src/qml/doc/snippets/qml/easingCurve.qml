// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
//! [children]
    Rectangle {
        id: rect
        width: 100
        height: 100
        anchors.centerIn: parent
        color: "red"
        opacity: 0
    }

    FrameAnimation {
        id: frameAnimation
        running: true

        property real elapsed // In seconds.
        readonly property real duration: 2 // Two seconds.
//! [easingCurve declaration]
        readonly property easingCurve inQuartCurve: Easing.InQuart
//! [easingCurve declaration]

        onTriggered: {
            elapsed += frameTime
            // Loop once we reach the duration.
            if (elapsed > duration)
                elapsed = 0

            // Increase the opacity from 0 slowly at first, then quickly.
            rect.opacity = inQuartCurve.valueForProgress(elapsed / duration)
        }
    }
//! [children]

//! [easingCurve structed value type declaration]
    readonly property easingCurve inElasticCurve: ({
        type: Easing.InElastic,
        amplitude: 4,
        period: 3
    })
//! [easingCurve structed value type declaration]
}
