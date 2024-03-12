// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 500
    height: 600

    property alias menu: menu
    property alias menuItem1: menuItem1
    property alias menuItem2: menuItem2
    property alias menuItem3: menuItem3
    property alias button: button

    function popupAtCursor() {
        menu.popup()
    }

    function popupAtPos(pos) {
        menu.popup(pos)
    }

    function popupAtCoord(x, y) {
        menu.popup(x, y)
    }

    function popupItemAtCursor(item) {
        menu.popup(item)
    }

    function popupItemAtPos(pos, item) {
        menu.popup(pos, item)
    }

    function popupItemAtCoord(x, y, item) {
        menu.popup(x, y, item)
    }

    function popupAtParentCursor(parent) {
        menu.popup(parent)
    }

    function popupAtParentPos(parent, pos) {
        menu.popup(parent, pos)
    }

    function popupAtParentCoord(parent, x, y) {
        menu.popup(parent, x, y)
    }

    function popupItemAtParentCursor(parent, item) {
        menu.popup(parent, item)
    }

    function popupItemAtParentPos(parent, pos, item) {
        menu.popup(parent, pos, item)
    }

    function popupItemAtParentCoord(parent, x, y, item) {
        menu.popup(parent, x, y, item)
    }

    Menu {
        id: menu
        MenuItem { id: menuItem1; text: "Foo" }
        MenuItem { id: menuItem2; text: "Bar" }
        MenuItem { id: menuItem3; text: "Baz" }
    }

    Button {
        id: button
        text: "Button"
        anchors.centerIn: parent
    }
}
