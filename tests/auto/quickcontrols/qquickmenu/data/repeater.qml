// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 200
    height: 200

    property alias menu: menu
    property alias repeater: repeater

    Menu {
        id: menu
        Repeater {
            id: repeater
            model: 5
            MenuItem { property int idx: index }
        }
    }
}
