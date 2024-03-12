// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: 300
    height: tabBar.height
    visible: true

    TabBar {
        id: tabBar
        width: parent.width

        TabButton { text: qsTr("Home") }
        TabButton { text: qsTr("Discover") }
        TabButton { text: qsTr("Activity") }
    }
}
