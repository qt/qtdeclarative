// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Window {
    width: 400
    height: 400

    property alias popup: popup
    property alias dialog: dialog
    property alias toolTip: toolTip

    Popup {
        id: popup
        popupType: Popup.Window
        width: 100
        height: 100
    }

    Dialog {
        id: dialog
        popupType: Popup.Window
        width: 100
        height: 100
    }

    ToolTip {
        id: toolTip
        popupType: Popup.Window
        text: "tip"
    }
}
