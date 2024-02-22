// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick
import QtQuick.Controls

Item {
    id: window
    width: menu.contentItem.width
    height: menu.contentItem.height
    visible: true

// Indent it like this so that the indenting in the generated doc is normal.
Menu {
    id: menu
    contentItem.parent: window

    MenuItem {
        text: qsTr("New...")
    }
    MenuItem {
        text: qsTr("Open...")
    }
    MenuItem {
        text: qsTr("Save")
    }

    MenuSeparator {}

    MenuItem {
        text: qsTr("Exit")
    }
}
}
//! [file]
