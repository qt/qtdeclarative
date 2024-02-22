// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 600
    height: 400

    property alias mainMenu: mainMenu
    property alias subMenu: subMenu

    Menu {
        id: mainMenu
        title: "Menu"

        Menu {
            id: subMenu
            title: "Sub Menu"
            MenuItem {
                id: subMenuItem1
                text: "Sub Menu Item 1"
                enabled: false
            }
            MenuItem {
                id: subMenuItem2
                text: "Sub Menu Item 2"
            }
        }
    }
}
