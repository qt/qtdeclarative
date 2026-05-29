// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Window {
    width: 400
    height: 400

    property alias popup: popup
    property alias slider: slider

    Button {
        id: openButton
        text: "Open Popup"
        onClicked: popup.open()
    }

    Popup {
        id: popup
        popupType: Popup.Window
        anchors.centerIn: parent
        width: 200
        height: 100

        Slider {
            id: slider
            anchors.centerIn: parent
            width: 160
            from: 0
            to: 100
            value: 50
        }
    }
}
