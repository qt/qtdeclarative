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
        delegate: Item { /* unsupported since it's not a MenuBarItem */ }
        Menu {
            id: fileMenu
            title: "&File"
            MenuItem { text: "&Open..." }
            MenuItem { text: "&Save" }
            MenuItem { text: "Save &As..." }
            MenuSeparator { }
            MenuItem { text: "&Quit" }
        }

        Menu {
            title: "&Edit"
            MenuItem { text: "&Cut" }
            MenuItem { text: "&Copy" }
            MenuItem { text: "&Paste" }
        }

        MenuBarItem {
            menu: Menu {
                title: "&Help"
                MenuItem { text: "&About" }
            }
        }
    }
}
