// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    readonly property Button oopsButton: oopsButton
    property alias fileMenu: fileMenu

    width: 400
    height: 400
    visible: true

    menuBar: MenuBar {
        MenuBarItem {
            menu: Menu {
                id: fileMenu
                title: "&File"
                objectName: title
                MenuItem { text: "&Open..." }
                MenuItem { text: "&Save" }
                MenuItem { text: "Save &As..." }
                MenuSeparator { }
                MenuItem { text: "&Quit" }
            }
        }
        MenuBarItem {
            menu: Menu {
                title: "&Edit"
                objectName: title
                MenuItem { text: "&Cut" }
                MenuItem { text: "&Copy" }
                MenuItem { text: "&Paste" }
            }
        }
        MenuBarItem {
            menu: Menu {
                title: "&View"
                Menu {
                    title: "&Alignment"
                    objectName: title
                    Menu {
                        title: "&Horizontal"
                        objectName: title
                        MenuItem { text: "&Left" }
                        MenuItem { text: "&Center" }
                        MenuItem { text: "&Right" }
                    }
                    Menu {
                        title: "&Vertical"
                        objectName: title
                        MenuItem { text: "&Top" }
                        MenuItem { text: "&Center" }
                        MenuItem { text: "&Bottom" }
                    }
                }
            }
        }

        MenuBarItem {
            menu: Menu {
                title: "&Help"
                objectName: title
                MenuItem { text: "&About" }
            }
        }
    }

    Button {
        id: oopsButton
        text: "&Oops"
    }
}
