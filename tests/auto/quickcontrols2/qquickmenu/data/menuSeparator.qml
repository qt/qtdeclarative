// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 200
    height: 200

    property alias menu: menu

    MenuItem {
        id: newMenuItem
        text: qsTr("New")
    }

    MenuSeparator {
        id: menuSeparator
    }

    MenuItem {
        id: saveMenuItem
        text: qsTr("Save")
    }

    Menu {
        id: menu
        cascade: true

        Component.onCompleted: {
            addItem(newMenuItem)
            addItem(menuSeparator)
            addItem(saveMenuItem)
        }
    }
}
