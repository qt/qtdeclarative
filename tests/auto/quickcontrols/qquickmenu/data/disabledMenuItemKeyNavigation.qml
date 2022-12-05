// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 200
    height: 200

    property alias menu: menu

    Menu {
        id: menu

        MenuItem {
            text: qsTr("Enabled 1")
        }
        MenuItem {
            text: qsTr("Disabled 1")
            enabled: false
        }
        MenuItem {
            text: qsTr("Enabled 2")
        }
    }
}
