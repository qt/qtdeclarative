// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias rootItem: rootItem
    property alias focusItem: focusItem
    property alias popup: popup

    Item {
        id: rootItem
    }

    Item {
        id: focusItem
        focus: true
        Component.onCompleted: forceActiveFocus()
    }

    Popup {
        id: popup
        focus: true
        modal: false
        popupType: Popup.Item
        contentItem: Rectangle {
            width: 100
            height: 100
            color: "lightgray"
            focus: true
            Component.onCompleted: forceActiveFocus()
        }
    }
}
