// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 800
    height: 800

    Component {
        id: menuBarItemComponent

        MenuBarItem {
            contentItem: Text {
                text: parent.text
                color: "blue"
            }
            background: Rectangle {
                color: "#00ff00"
            }
        }
    }

    menuBar: MenuBar {
        delegate: menuBarItemComponent

        Menu {
            title: "Menu"
        }
    }
}
