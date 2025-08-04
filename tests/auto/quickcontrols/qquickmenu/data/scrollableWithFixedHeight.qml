// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Window

Window {
    width: 300
    height: 300

    property alias menu: menu

    Menu {
        id: menu
        anchors.centerIn: parent
        height: 100

        Repeater {
            model: 10
            delegate: MenuItem {
                objectName: text
                text: (index + 1)
            }
        }
    }
}
