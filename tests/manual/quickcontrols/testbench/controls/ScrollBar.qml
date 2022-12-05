// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        ["vertical"],
        ["vertical", "disabled"],
        ["vertical", "interactive"],
        ["vertical", "interactive", "disabled"],
        ["horizontal"],
        ["horizontal", "disabled"],
        ["horizontal", "interactive"],
        ["horizontal", "interactive", "disabled"]
    ]

    property Component component: Frame {
        width: 100
        height: 100
        clip: true

        Label {
            text: "ABCDEFG\nHIJKLMN"
            font.pixelSize: 40
            x: -horizontalScrollBar.position * width
            y: -verticalScrollBar.position * height
        }

        ScrollBar {
            id: verticalScrollBar
            enabled: !is("disabled")
            orientation: Qt.Vertical
            interactive: is("interactive")
            visible: is("vertical")
            size: 0.3
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            Binding {
                target: verticalScrollBar
                property: "active"
                value: verticalScrollBar.visible
            }
        }

        ScrollBar {
            id: horizontalScrollBar
            enabled: !is("disabled")
            orientation: Qt.Horizontal
            interactive: is("interactive")
            visible: is("horizontal")
            size: 0.3
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            Binding {
                target: horizontalScrollBar
                property: "active"
                value: horizontalScrollBar.visible
            }
        }
    }
}
