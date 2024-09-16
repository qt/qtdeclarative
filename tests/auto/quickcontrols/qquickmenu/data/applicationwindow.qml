// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    title: "Test Application Window"
    width: 400
    height: 400

    property alias emptyMenu: emptyMenu
    property alias menu: menu
    property alias menuButton: menuButton
    property Overlay overlay: menu.Overlay.overlay

    Menu {
        id: emptyMenu
    }

    Menu {
        id: menu
        cascade: true

        MenuItem {
            objectName: "firstMenuItem"
            text: "A"
        }
        MenuItem {
            objectName: "secondMenuItem"
            text: "B"
        }
        MenuItem {
            objectName: "thirdMenuItem"
            text: "C"
        }
    }

    Button {
        id: menuButton
        x: 250
        visible: false
        text: "Open Menu"
        onClicked: menu.open()
    }
}
