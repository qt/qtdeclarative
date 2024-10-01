// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 600
    height: 600

    property alias button: buttonInPopup
    property alias lowerMouseArea: lowerMA
    property alias upperMouseArea: upperMA

    MouseArea {
        id: lowerMA
        anchors.fill: parent
    }

    Popup {
        anchors.centerIn: Overlay.overlay
        width: 400
        height: 400
        modal: true
        dim: true

        Button {
            id: buttonInPopup
            anchors.centerIn: parent
            text: "click me"
        }
    }

    MouseArea {
        id: upperMA
        parent: Overlay.overlay
        anchors.fill: parent
        z: 1
    }
}

