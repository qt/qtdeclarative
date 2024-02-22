// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400
    id: root

    property alias drawer: drawer
    property alias backgroundButton: backgroundButton
    property alias drawerButton: drawerButton

    Button {
        id: backgroundButton
        text: "Background"
        anchors.fill: parent
    }

    Drawer {
        id: drawer
        width: 100
        height: root.height
        topPadding: 2
        leftPadding: 2
        rightPadding: 2
        bottomPadding: 2

        contentItem: Button {
            id: drawerButton
            text: "Drawer"
        }
    }
}
