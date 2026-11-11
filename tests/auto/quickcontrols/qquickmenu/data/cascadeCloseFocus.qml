// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 300

    property alias textField: textField
    property alias menu: menu
    property alias subMenu: subMenu
    property alias subSubMenu: subSubMenu
    property alias itemPopup: itemPopup

    // Every popup gets an exit transition, whatever the style, so that the popups
    // finish closing after all of them have started to close.
    component FadingMenu: Menu {
        popupType: Popup.Window
        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 100 }
        }
    }

    TextField {
        id: textField
        focus: true
    }

    FadingMenu {
        id: menu
        MenuItem { text: "Menu item" }
        FadingMenu {
            id: subMenu
            title: "Sub menu"
            MenuItem { text: "Sub menu item" }
            FadingMenu {
                id: subSubMenu
                title: "Sub-sub menu"
                MenuItem { text: "Sub-sub menu item" }
            }
        }
    }

    // An item popup that the test parents into a menu's popup window. It has no exit
    // transition, so it finishes closing while that menu is still closing.
    Popup {
        id: itemPopup
        popupType: Popup.Item
        focus: true
        width: 50
        height: 50
        exit: null
    }
}
