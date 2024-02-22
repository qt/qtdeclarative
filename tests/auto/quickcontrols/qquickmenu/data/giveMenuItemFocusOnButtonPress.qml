// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 640
    height: 480

    property alias menuButton: menuButton
    property alias menu: menu

    signal menuButtonClicked

    Button {
        id: menuButton
        text: "Open menu"

        // Buttons do not emit clicked() for enter/return, hence the Keys usage.
        // The signal is just for the test to ensure that the return was actually handled.
        Keys.onReturnPressed: {
            menuButtonClicked()
            menu.open()
        }
    }

    Menu {
        id: menu
        parent: menuButton

        onOpened: command1.forceActiveFocus()

        MenuItem {
            id: command1
            objectName: text
            text: "Command 1"
        }

        MenuItem {
            objectName: text
            text: "Command 2"
        }

        MenuItem {
            objectName: text
            text: "Command 3"
        }
    }
}
