// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

//! [createObject]
Row {
    anchors.centerIn: parent

    Component {
        id: menuItemComponent

        MenuItem {}
    }

    Button {
        id: button
        text: "Menu"
        onClicked: menu.open()
        Menu {
            id: menu
        }
    }

    Button {
        text: "Add item"
        onClicked: {
            onClicked: {
                let menuItem = menuItemComponent.createObject(
                    menu.contentItem, { text: qsTr("New item") })
                menu.addMenu(menuItem)
            }
        }
    }
}
//! [createObject]
