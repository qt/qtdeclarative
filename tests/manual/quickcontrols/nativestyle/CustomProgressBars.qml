// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Basic.impl

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
            id: c3
            width: 100
            from: 0
            to: 10
            value: time
            indeterminate: false
            padding: 5
            background: Rectangle {
                implicitWidth: 100
                implicitHeight: 6
                color: "darkgray"
            }
            contentItem: ProgressBarImpl {
                implicitHeight: 6
                implicitWidth: 100
                progress: c3.position
                indeterminate: false
                color: "lightgreen"
            }
        }
    }
}
