// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    width: 720
    height: 720

    property int popupType: Popup.Item
    property int closePolicy: Popup.NoAutoClose
    property bool modal: false
    property alias popup1: p1
    property alias popup2: p2
    property alias popup3: p3

    Popup {
        id: p1
        modal: root.modal
        x: 50
        y: 50
        width: 500
        height: 500
        popupType: root.popupType
        closePolicy: root.closePolicy
        Button {
            text: "p1"
            onClicked: p2.open()
        }
        Popup {
            id: p2
            modal: root.modal
            parent: p1.contentItem.parent
            x: 50
            y: 50
            width: 500
            height: 500
            popupType: root.popupType
            closePolicy: root.closePolicy
            Button {
                text: "p2"
                onClicked: p3.open()
            }
            Popup {
                id: p3
                modal: root.modal
                parent: p2.contentItem.parent
                x: 50
                y: 50
                width: 500
                height: 500
                popupType: root.popupType
                closePolicy: root.closePolicy
                Text {
                    text: "p3"
                }
            }
        }
    }
    // Anchored away from the top-left corner: the close-cascade tests click at (4, 4)
    // relative to various ancestors (including the root overlay itself), and a click
    // that isn't consumed (e.g. a non-modal popup) falls through to whatever is
    // underneath. This button must never be there, or it'll reopen p1 right after
    // the cascade closes it.
    Button {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        text: "Open Menu"
        onClicked: p1.open()
    }
}

