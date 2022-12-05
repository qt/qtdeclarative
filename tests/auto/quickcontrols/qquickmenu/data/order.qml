// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 200
    height: 200

    property alias menu: menu

    Component {
        id: menuItem
        MenuItem { }
    }

    Menu {
        id: menu
        property alias repeater: repeater
        MenuItem { text: "static_1" }
        Repeater {
            id: repeater
            model: 2
            MenuItem { text: "repeated_" + (index + 2) }
        }
        MenuItem { text: "static_4" }
        Component.onCompleted: {
            addItem(menuItem.createObject(menu.contentItem, {text: "dynamic_5"}))
            addItem(menuItem.createObject(menu.contentItem, {text: "dynamic_6"}))
            insertItem(0, menuItem.createObject(menu.contentItem, {text: "dynamic_0"}))
        }
    }
}
