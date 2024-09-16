// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "ProgressBars"

    property int time: 0
    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            time++
            if (time > 10)
                time = 0
        }
    }

    Row {
        spacing: container.rowSpacing

        ProgressBar {
            width: 100
            from: 0
            to: 10
            value: time
            indeterminate: false
        }

        ProgressBar {
            width: 100
            from: 0
            to: 10
            value: time
            enabled: false
            indeterminate: false
        }

        ProgressBar {
            width: 100
            from: 0
            to: 10
            indeterminate: true
        }

        ProgressBar {
            width: 80
            from: 0
            to: 10
            value: time
            indeterminate: false
            property bool qqc2_style_small
        }

        ProgressBar {
            width: 60
            from: 0
            to: 10
            value: time
            indeterminate: false
            property bool qqc2_style_mini
        }
    }
}
