// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias contextMenu: contextMenu
//    property alias buttonMenu: buttonMenu

    Menu {
        id: contextMenu
        objectName: "menu"

        Action {
            objectName: text
            text: "action1"
            shortcut: "A"
        }

        Action {
            objectName: text
            text: "action2"
            shortcut: "B"
        }

        Menu {
            id: subMenu
            objectName: "subMenu"

            Action {
                objectName: text
                text: "subAction1"
                shortcut: "1"
            }
        }
    }

//    Button {
//        text: "Menu button"

//        Menu {
//            id: buttonMenu

//            Action {
//                objectName: text
//                text: "buttonMenuAction1"
//                shortcut: "Z"
//            }
//        }
//    }
}
