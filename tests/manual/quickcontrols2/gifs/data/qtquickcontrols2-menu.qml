// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

// TODO: restore and finish https://codereview.qt-project.org/#/c/123948/
ApplicationWindow {
    width: menu.contentItem.width + 20
    height: menu.contentItem.height + fileButton.height + 20

    property alias fileButton: fileButton
    property alias menu: menu

    Button {
        id: fileButton
        text: "File"
        onClicked: menu.open()
        x: 10
        y: 10
    }
    Menu {
        id: menu
        // TODO
        contentItem.x: fileButton.x
        contentItem.y: fileButton.y + fileButton.height

        MenuItem {
            text: "New..."
        }
        MenuItem {
            text: "Open..."
        }
        MenuItem {
            text: "Save"
        }
    }
}
