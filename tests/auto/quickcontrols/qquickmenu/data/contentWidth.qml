// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 600
    height: 600

    property alias menu: menu

    Menu {
        id: menu

        MenuItem {
            objectName: "shortItem"
            text: "Short"
        }
        MenuItem {
            objectName: "longItem"
            text: "A much longer menu item text that should expand the menu"
        }
        MenuItem {
            objectName: "mediumItem"
            text: "Medium length text"
        }
    }

    Component {
        id: menuItemComponent
        MenuItem {}
    }

    function addLongMenuItem() {
        var item = menuItemComponent.createObject(menu, {
            objectName: "dynamicLongItem",
            text: "This is an extremely long menu item text that is definitely wider than anything else"
        });
        menu.addItem(item);
        return item;
    }
}
