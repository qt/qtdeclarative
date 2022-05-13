// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias menu: menu
    property alias subMenu: subMenu
    property alias buttonMenu: buttonMenu

    Menu {
        id: menu
        objectName: "menu"

        Action {
            objectName: text
            text: "action1"
            shortcut: "A"
        }

        Menu {
            id: subMenu
            objectName: "subMenu"

            Action {
                objectName: text
                text: "subAction1"
                shortcut: "B"
            }
        }
    }

    Button {
        text: "Menu button"

        Menu {
            id: buttonMenu

            Action {
                objectName: text
                text: "buttonMenuAction1"
                shortcut: "C"
            }
        }
    }
}
