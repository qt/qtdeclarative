// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias menu: menu
    property alias action: action
    property alias menuItem: menuItem
    property alias subMenu: subMenu
    property alias subMenuItem: subMenuItem

    Menu {
        id: menu

        Action {
            id: action
            text: "&Action"
        }

        MenuItem {
            id: menuItem
            text: "Menu &Item"
        }

        Menu {
            id: subMenu
            title: "Sub &Menu"

            MenuItem {
                id: subMenuItem
                text: "&Sub Menu Item"
            }
        }
    }
}
