// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    title: "Test Application Window"
    width: 300
    height: 300

    property alias menu: menu

    Menu {
        id: menu

        Repeater {
            model: 20

            delegate: MenuItem {
                objectName: text
                text: (index + 1)
            }
        }
    }
}
