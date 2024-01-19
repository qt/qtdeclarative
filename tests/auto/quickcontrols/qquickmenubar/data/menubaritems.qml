// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    readonly property Button oopsButton: oopsButton
    property bool requestNative: false
    property alias fileMenu: fileMenu

    width: 400
    height: 400
    visible: true

    header: MenuBar {
        requestNative: root.requestNative
        MenuBarItem {
            menu: Menu {
                id: fileMenu
                title: "&File"
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
                    Menu {
                        title: "&Horizontal"
                        MenuItem { text: "&Left" }
                        MenuItem { text: "&Center" }
                        MenuItem { text: "&Right" }
                    }
                    Menu {
                        title: "&Vertical"
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
                MenuItem { text: "&About" }
            }
        }
    }

    Button {
        id: oopsButton
        text: "&Oops"
    }
}
