// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    property bool requestNative: false

    width: 400
    height: 400
    visible: true

    header: MenuBar {
        requestNative: root.requestNative
        Menu {
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
        Menu {
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

        Menu {
            title: "&Help"
            MenuItem { text: "&About" }
        }
    }
}
