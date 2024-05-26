// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick
import QtQuick.Controls.Basic

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

    MenuSeparator {
        padding: 0
        topPadding: 12
        bottomPadding: 12
        contentItem: Rectangle {
            implicitWidth: 200
            implicitHeight: 1
            color: "#1E000000"
        }
    }

    MenuItem {
        text: qsTr("Exit")
    }
}
}
//! [file]
