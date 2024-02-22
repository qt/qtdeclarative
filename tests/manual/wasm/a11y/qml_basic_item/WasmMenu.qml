// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

MenuBar {
    Accessible.role: Accessible.MenuBar
    signal showTime
    signal showDate
    signal showAboutDialog
    Menu {
        title: "Chrono"

        MenuItem {
            text: "Date"
            Accessible.role: Accessible.ButtonMenu
            Accessible.name: text
            Accessible.description: text
            onTriggered: {
                showDate()
            }
        }
        MenuItem {
            text: "Time"
            Accessible.role: Accessible.ButtonMenu
            Accessible.name: text
            Accessible.description: text
            onTriggered: {
                showTime()
            }
        }
    }
    Menu {
        title: "Help"

        MenuItem {
            text: "About"
            Accessible.role: Accessible.ButtonMenu
            Accessible.name: text
            Accessible.description: text
            onTriggered: {
                showAboutDialog()
            }
        }
    }
    delegate: MenuBarItem {
           id: menuBarItem
           Accessible.role: Accessible.ButtonMenu
           Accessible.name: menuBarItem.text
           Accessible.description: menuBarItem.text

       }
}
