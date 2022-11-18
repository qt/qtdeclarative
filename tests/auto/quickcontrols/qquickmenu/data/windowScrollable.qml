// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Window

Window {
    title: "Test Window"
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
