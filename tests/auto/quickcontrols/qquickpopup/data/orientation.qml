// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: 200
    height: 100

    property alias popup: popup

    Rectangle {
        width: 100
        height: 80
        anchors.centerIn: parent
        border.width: 1

        Popup {
            id: popup
            width: 50
            height: 40
            anchors.centerIn: parent
            visible: true
            modal: false
            dim: true

            Overlay.modeless: Rectangle {
                opacity: 0.5
                color: "blue"
            }
        }
    }
}
