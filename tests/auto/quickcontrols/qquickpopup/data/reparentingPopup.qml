// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Window {
    width: 400
    height: 400

    property alias popup: simplepopup
    property alias rectangle1: item1
    property alias rectangle2: item2
    property alias rectangle3: item3

    Popup {
        id: simplepopup
        popupType: Popup.Window
        x: 10
        y: 10
        width: 200
        height: 200
    }

    Rectangle {
        id: item1
        color: "red"
        width: 200
        height: 200
    }

    Rectangle {
        id: item2
        color: "green"
        x: 0
        y: 200
        width: parent.width
        height: 200
        Rectangle {
            id: item3
            color: "blue"
            x: 200
            y: 0
            width: 200
            height: item2.height
        }
    }
}
