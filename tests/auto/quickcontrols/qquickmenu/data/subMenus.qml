// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 600
    height: 400

    property alias mainMenu: mainMenu
    property alias subMenu1: subMenu1
    property alias subMenu2: subMenu2
    property alias subSubMenu1: subSubMenu1

    Menu {
        id: mainMenu
        MenuItem {
            id: mainMenuItem1
            objectName: "mainMenuItem1"
            text: "Main 1"
        }

        Menu {
            overlap: 0
            id: subMenu1
            objectName: "subMenu1"
            title: "Sub Menu 1"

            MenuItem {
                id: subMenuItem1
                objectName: "subMenuItem1"
                text: "Sub 1"
            }

            MenuItem {
                id: subMenuItem2
                objectName: "subMenuItem2"
                text: "Sub 2"
            }

            Menu {
                overlap: 0
                id: subSubMenu1
                objectName: "subSubMenu1"
                title: "Sub Sub Menu 1"

                MenuItem {
                    id: subSubMenuItem1
                    objectName: "subSubMenuItem1"
                    text: "Sub Sub 1"
                }
                MenuItem {
                    id: subSubMenuItem2
                    objectName: "subSubMenuItem2"
                    text: "Sub Sub 2"
                }
            }
        }

        MenuItem {
            id: mainMenuItem2
            objectName: "mainMenuItem2"
            text: "Main 2"
        }

        Menu {
            id: subMenu2
            objectName: "subMenu2"
            title: "Sub Menu 2"

            MenuItem {
                id: subMenuItem3
                objectName: "subMenuItem3"
                text: "Sub 3"
            }
            MenuItem {
                id: subMenuItem4
                objectName: "subMenuItem4"
                text: "Sub 4"
            }
        }

        MenuItem {
            id: mainMenuItem3
            objectName: "mainMenuItem3"
            text: "Main 3"
        }
    }
}
