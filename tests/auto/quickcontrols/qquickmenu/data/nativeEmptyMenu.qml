// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias contextMenu: contextMenu

    function addAction(menu: T.Menu, text: string) {
        menu.addAction(actionComponent.createObject(null, { text: text }))
    }

    function insertAction(menu: T.Menu, index: int, text: string) {
        menu.insertAction(index, actionComponent.createObject(null, { text: text }))
    }

    function removeAction(menu: T.Menu, index: int) {
        menu.removeAction(menu.actionAt(index))
    }

    function addMenu(menu: T.Menu, title: string) {
        menu.addMenu(menuComponent.createObject(null, { title: title }))
    }

    Component {
        id: actionComponent

        Action {
            objectName: text
        }
    }

    Component {
        id: menuComponent

        Menu {
            objectName: title
        }
    }

    Menu {
        id: contextMenu
        objectName: "menu"
        popupType: Popup.Native
    }
}
