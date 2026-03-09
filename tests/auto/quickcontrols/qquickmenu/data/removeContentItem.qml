// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 600
    height: 400

    property alias menu: contextMenu
    property Item oldContentItem : null

    function removeContentItem()
    {
        oldContentItem = contextMenu.contentItem
        contextMenu.contentItem = null
    }

    function restoreContentItem()
    {
        contextMenu.contentItem = oldContentItem
    }

    Item {
        width: 100
        height: 100
        anchors.centerIn: parent
        Menu {
            id: contextMenu
            title: "Some Menu"
            visible: true
            MenuItem {
                text: "Action 1"
            }
        }
    }
}

