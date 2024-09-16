// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias contextMenu: contextMenu

    function addSubMenu(title: string) {
        contextMenu.addMenu(subMenuComponent.createObject(null, { title: title }))
    }

    function addAction(menu: T.Menu, text: string) {
        menu.addAction(actionComponent.createObject(null, { text: text }))
    }

    function insertAction(menu: T.Menu, index: int, text: string) {
        menu.insertAction(index, actionComponent.createObject(null, { text: text }))
    }

    Component {
        id: actionComponent

        Action {
            objectName: text
        }
    }

    Component {
        id: subMenuComponent

        Menu {
            id: subMenu
            objectName: title
            popupType: Popup.Native

            Action {
                text: subMenu.objectName + "Action1"
            }
        }
    }

    Menu {
        id: contextMenu
        objectName: "menu"
        popupType: Popup.Native
    }
}
