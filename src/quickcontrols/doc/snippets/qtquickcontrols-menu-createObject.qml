// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
                menu.addItem(menuItem)
            }
        }
    }
}
//! [createObject]
