// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

// TODO
QtObject {
    property string customControlName: qsTr("MenuBar & MenuBarItem")

    property var supportedStates: [
        []
    ]

    property Component component: MenuBar {
        MenuBarItem {
            text: qsTr("Normal")
            menu: Menu { Action { text: "action" } }
        }
        MenuBarItem {
            text: qsTr("Pressed")
            down: true
            menu: Menu { Action { text: "action" } }
        }
        MenuBarItem {
            text: qsTr("Highlighted")
            highlighted: true
            menu: Menu { Action { text: "action" } }
        }
        MenuBarItem {
            text: qsTr("Disabled")
            enabled: false
            menu: Menu { Action { text: "action" } }
        }
    }
}
