// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "Frames"

    Row {
        spacing: container.rowSpacing

        Frame {
            width: 200
            height: 50
            background: Rectangle {
                border.width: 1
                border.color: "green"
                Text {
                    anchors.centerIn: parent
                    color: "green"
                    text: "Custom background"
                }
            }
        }
    }
}
