// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    Action {
        id: actionOutsideMenu
        text: "Action declared outside menu"
        onTriggered: enabled = false
    }

    menuBar: MenuBar {
        Menu {
            title: "Menu"
            objectName: title

            Action {
                text: "Action"
                objectName: text
                onTriggered: enabled = false
            }
            MenuItem {
                objectName: "MenuItem with Action"
                action: Action {
                    text: "Action declared inside MenuItem"
                    objectName: text
                    onTriggered: enabled = false
                }
            }
            MenuItem {
                objectName: "MenuItem with Action declared outside menu"
                action: actionOutsideMenu
            }
            MenuItem {
                text: "MenuItem with no Action"
                objectName: text
                onTriggered: enabled = false
            }

            Menu {
                title: "Submenu"
                objectName: title

                Action {
                    text: "Sub-Action"
                    objectName: text
                    onTriggered: enabled = false
                }
                MenuItem {
                    objectName: "Sub-MenuItem with Action declared inside"
                    action: Action {
                        text: "Action declared inside Sub-MenuItem"
                        objectName: text
                        onTriggered: enabled = false
                    }
                }
                MenuItem {
                    objectName: "Sub-MenuItem with Action declared outside menu"
                    action: actionOutsideMenu
                }
                MenuItem {
                    text: "Sub-MenuItem with no Action"
                    objectName: text
                    onTriggered: enabled = false
                }
            }
        }
    }
}
