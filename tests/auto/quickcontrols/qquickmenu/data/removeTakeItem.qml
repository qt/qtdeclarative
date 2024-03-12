// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 200
    height: 200

    property alias menu: menu
    property alias menuItem1: menuItem1
    property alias menuItem2: menuItem2
    property alias menuItem3: menuItem3

    function takeSecondItem() {
        return menu.takeItem(1)
    }

    function removeFirstItem() {
        menu.removeItem(menuItem1)
    }

    function removeNullItem() {
        menu.removeItem(null)
    }

    Menu {
        id: menu
        MenuItem {
            id: menuItem1
        }
        MenuItem {
            id: menuItem2
        }
        MenuItem {
            id: menuItem3
        }
    }
}
