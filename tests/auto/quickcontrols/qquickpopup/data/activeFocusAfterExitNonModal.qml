// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 600
    height: 400
    title: "activeFocusItem: " + activeFocusItem

    property alias rootItem: rootItem
    property alias focusItem: focusItem
    property alias popup: popup

    Rectangle {
        id: rootItem
        objectName: "rootItem"
        width: 32
        height: 32
        color: activeFocus ? "salmon" : "lightgray"
    }

    Rectangle {
        id: focusItem
        objectName: "focusItem"
        y: 32
        width: 32
        height: 32
        color: activeFocus ? "salmon" : "lightgray"
        focus: true
        Component.onCompleted: forceActiveFocus()
    }

    Popup {
        id: popup
        focus: true
        modal: false
        popupType: Popup.Item
        width: 100
        height: 100
        contentItem: Rectangle {
            objectName: "popupContentItem"
            color: activeFocus ? "salmon" : "lightgray"
            focus: true
            Component.onCompleted: forceActiveFocus()
        }
    }
}
