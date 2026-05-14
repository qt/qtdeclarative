// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias menu: menu
    property alias menuItem1: menuItem1
    property alias menuItem2: menuItem2
    property alias menuItem3: menuItem3
    property alias menuItem4: menuItem4
    property alias menuItem5: menuItem5
    property alias separator1: separator1
    property alias separator2: separator2
    property alias separator3: separator3
    property alias boundSeparator: boundSeparator
    property bool showBoundSeparator: true

    Menu {
        id: menu

        MenuItem { id: menuItem1; text: "Item 1" }
        MenuItem { id: menuItem2; text: "Item 2" }

        MenuSeparator { id: separator1 }

        MenuItem { id: menuItem3; text: "Item 3" }
        MenuItem { id: menuItem4; text: "Item 4" }

        MenuSeparator { id: separator2 }

        MenuItem { id: menuItem5; text: "Item 5" }

        MenuSeparator { id: boundSeparator; visible: showBoundSeparator }

        MenuSeparator { id: separator3 }
    }
}
