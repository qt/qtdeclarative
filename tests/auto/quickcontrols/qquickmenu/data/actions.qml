// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias menu: menu

    Menu {
        id: menu
        Action { text: "action1" }
        MenuItem { text: "menuitem2" }
        Action { text: "action3" }
        MenuItem { text: "menuitem4" }
    }
}
