// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 400
    height: 400

    property alias drawer: drawer
    property alias popup: popup
    property alias button: button

    Drawer {
        id: drawer
        width: 200
        height: parent.height
        dragMargin: parent.width
    }

    Popup {
        id: popup
        x: 10; y: 10
        width: window.width - 10
        height: window.height - 10
        popupType: Popup.Item

        Button {
            id: button
            text: "Button"
            anchors.fill: parent
        }
    }
}
