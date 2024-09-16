// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias contextMenu: contextMenu

    function insertRectangle(menu: T.Menu, index: int, color: color) {
        menu.insertItem(index, rectangleComponent.createObject(null, { color: color }))
    }

    Component {
        id: rectangleComponent

        Rectangle {
            objectName: "rectangle"
            width: 32
            height: 32
        }
    }

    Component {
        id: menuComponent

        Menu {
            objectName: title
            popupType: contextMenu.popupType
        }
    }

    Menu {
        id: contextMenu
        objectName: "menu"
        popupType: contextMenu.popupType

        Action {
            objectName: text
            text: "action"
        }

        MenuItem {
            text: "menuItem"
            objectName: text
        }

        Menu {
            id: subMenu
            title: "subMenu"
            objectName: title
            popupType: contextMenu.popupType

            Action {
                objectName: text
                text: "subAction1"
            }

            Action {
                objectName: text
                text: "subAction2"
            }
        }
    }
}
