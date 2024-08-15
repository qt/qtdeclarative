// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    width: 400
    height: 400

    property bool enabled: true
    property bool checkable: false
    property alias menu: menu
    property alias action: action
    property alias menuItem: menuItem
    property alias subMenu: subMenu
    property alias subMenuItem: subMenuItem
    property alias subMenuAction: subMenuAction

    Menu {
        id: menu

        Action {
            id: action
            text: "&Action"
            checkable: root.checkable
            enabled: root.enabled
        }

        MenuItem {
            id: menuItem
            text: "Menu &Item"
            checkable: root.checkable
            enabled: root.enabled
        }

        Menu {
            id: subMenu
            title: "Sub &Menu"

            Action {
                id: subMenuAction
                text: "S&ub Menu Action"
                checkable: root.checkable
                enabled: root.enabled
            }

            MenuItem {
                id: subMenuItem
                text: "&Sub Menu Item"
                checkable: root.checkable
                enabled: root.enabled
            }
        }
    }
}
