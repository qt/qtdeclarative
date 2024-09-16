// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias popup: popup
    property alias button: button

    Button {
        id: button
        text: "button"
        focus: true
    }

    Popup {
        id: popup
        focus: true
        width: 100
        height: 100
        anchors.centerIn: Overlay.overlay
        popupType: Popup.Item
    }
}
