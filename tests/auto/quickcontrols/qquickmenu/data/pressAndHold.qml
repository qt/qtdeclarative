// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias menu: menu

    MouseArea {
        anchors.fill: parent
        onPressAndHold: menu.open()
    }

    Menu {
        id: menu
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        MenuItem { text: "One" }
        MenuItem { text: "Two" }
        MenuItem { text: "Three" }
    }
}
