// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias menu: menu
    property alias popup: popup
    property alias combo: combo.popup

    Menu {
        id: menu
        MenuItem {
            onTriggered: popup.open()
        }
    }

    Popup {
        id: popup
        modal: true
        width: 200
        height: 200

        ComboBox {
            id: combo
            model: 3
        }
    }
}
