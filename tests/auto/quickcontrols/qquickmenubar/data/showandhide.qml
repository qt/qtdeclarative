// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root

    width: 400
    height: 400
    visible: true

    menuBar: MenuBar {
        Menu {
            title: "Menu1"
            Action { text: qsTr("Action") }
        }

        Menu {
            title: "Menu2"
            Action { text: qsTr("Action") }
        }

        MenuBarItem {
            menu: Menu {
                title: "Menu3"
                Action { text: qsTr("Action") }
            }
        }

        MenuBarItem {
            visible: false
            menu: Menu {
                title: "Menu4"
                Action { text: qsTr("Action") }
            }
        }
    }
}
