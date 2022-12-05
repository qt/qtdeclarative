// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: 600
    height: 300

    property alias popup: popup

    Rectangle {
        width: 60
        height: 30
        anchors.centerIn: parent
        border.width: 1

        Popup {
            id: popup
            x: parent.width
            y: parent.height
            width: 30
            height: 60
            visible: true
        }
    }
}
