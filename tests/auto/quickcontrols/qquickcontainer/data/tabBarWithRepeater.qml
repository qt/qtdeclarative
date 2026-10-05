// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 640
    height: 480

    property alias tabBar: tabBar
    property alias staticTabButton: staticTabButton

    function addTabButton() {
        tabBar.addItem(tabButtonComponent.createObject(tabBar, { text: "Dynamic" }))
    }

    Component {
        id: tabButtonComponent
        TabButton {}
    }

    TabBar {
        id: tabBar

        contentItem: Row {
            spacing: tabBar.spacing

            Repeater {
                model: tabBar.contentModel
            }
        }

        TabButton {
            id: staticTabButton
            text: "Static"
        }
    }
}
