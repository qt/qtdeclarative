// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

MenuBar {
    Accessible.role: Accessible.MenuBar

    signal showTime
    signal showDate
    signal showAboutDialog
    Menu {
        title: "&Chrono"
        MenuItem {
            text: "&Date"
            Accessible.role: Accessible.MenuItem
            Accessible.name: text
            Accessible.description: text
            onTriggered: {
                showDate()
            }
        }
        MenuItem {
            text: "&Time"
            Accessible.role: Accessible.MenuItem
            Accessible.name: text
            Accessible.description: text
            onTriggered: {
                showTime()
            }
        }
    }
    Menu {
        title: "&Help"

        MenuItem {
            text: "&about"
            Accessible.role: Accessible.MenuItem
            Accessible.name: text
            Accessible.description: text
            onTriggered: {
                showAboutDialog()
            }
        }
    }
}
