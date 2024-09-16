// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias popup: popup

    function disableOverlay() {
        popup.Overlay.overlay.enabled = false
    }

    Popup {
        id: popup
        width: 200
        height: 200
        background: Rectangle {
            color: popup.palette.base
        }
        popupType: Popup.Item
    }
}
