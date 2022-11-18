// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Window

Window {
    title: "Test Window"
    width: 300
    height: dummyItem.height * 10

    property alias menu: menu
    MenuItem {
        id: dummyItem
        objectName: "Dummy"
        text: objectName
    }

    Menu {
        id: menu
        topPadding: 10
        Repeater {
            model: 10

            delegate: MenuItem {
                objectName: text
                text: (index + 1)
            }
        }
    }
}
