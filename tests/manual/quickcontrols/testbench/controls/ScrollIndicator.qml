// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        ["vertical"],
        ["vertical", "disabled"],
        ["horizontal"],
        ["horizontal", "disabled"],
    ]

    property Component component: Frame {
        width: 100
        height: 100
        clip: true

        Label {
            text: "ABCDEFG\nHIJKLMN"
            font.pixelSize: 40
            x: horizontalScrollIndicator.position * width
            y: verticalScrollIndicator.position * height
        }

        ScrollIndicator {
            id: verticalScrollIndicator
            enabled: !is("disabled")
            orientation: Qt.Vertical
            active: true
            visible: is("vertical")
            size: 0.3
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
        }

        ScrollIndicator {
            id: horizontalScrollIndicator
            enabled: !is("disabled")
            orientation: Qt.Horizontal
            active: true
            visible: is("horizontal")
            size: 0.3
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            Binding {
                target: horizontalScrollIndicator
                property: "active"
                value: horizontalScrollIndicator.visible
            }
        }
    }
}
