// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// TODO
QtObject {
    property string customControlName: qsTr("Menu, MenuItem & MenuSeparator")

    property var supportedStates: [
        []
    ]

    property Component component: Button {
        id: menuButton
        text: qsTr("Menu")
        checked: menu.visible
        checkable: true

        Menu {
            id: menu
            x: 1
            y: 1 + parent.height
            visible: menuButton.checked
            closePolicy: Popup.CloseOnPressOutsideParent

            MenuItem {
                text: "Normal"
            }
            MenuItem {
                text: "Pressed"
                down: true
            }
            MenuItem {
                text: "Disabled"
                enabled: false
            }

            MenuSeparator {}

            MenuItem {
                text: "Checked"
                checked: true
            }
            MenuItem {
                text: "Checked + Pressed"
                checked: true
                down: true
            }
            MenuItem {
                text: "Checked + Disabled"
                checked: true
                enabled: false
            }

            MenuSeparator {}

            Menu {
                title: "Submenu"

                MenuItem {
                    text: "Submenu item"
                }
            }

            Menu {
                title: "Submenu with icon"
                icon {
                    width: 14
                    height: 14
                    source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/double-arrow.png"
                }

                MenuItem {
                    text: "Submenu item"
                }
            }

            Menu {
                title: "Disabled Submenu"
                enabled: false
            }
        }
    }
}
