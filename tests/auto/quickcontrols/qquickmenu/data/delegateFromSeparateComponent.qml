// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 800
    height: 800

    property alias menu: menu

    Component {
        id: menuItemComponent

        MenuItem {
            contentItem: Text {
                text: parent.text
                color: "blue"
            }
            background: Rectangle {
                color: "#00ff00"
            }
        }
    }

    Menu {
        id: menu
        title: "Root Menu"

        Action {
            text: "Action Item 1"
        }
        Menu {
            title: "Sub-menu"
            delegate: menuItemComponent

            Action {
                text: "Sub-menu Action Item 1"
            }
            Menu {
                title: "Sub-sub-menu"
                delegate: menuItemComponent

                Action {
                    text: "Sub-sub-menu Action Item 1"
                }
            }
            Action {
                text: "Sub-menu Action Item 2"
            }
        }
        Action {
            text: "Action Item 2"
        }

        delegate: menuItemComponent
        visible: true
    }
}
