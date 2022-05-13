// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias drawer: drawer
    property alias popup: popup

    Drawer {
        id: drawer
        width: 200
        height: parent.height
        dragMargin: parent.width
    }

    Popup {
        id: popup
        modal: true
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: parent.width / 2
        height: parent.height / 2

        Rectangle {
            objectName: "shadow"
            parent: popup.background
            anchors.fill: parent
            anchors.margins: -20

            z: -1
            opacity: 0.5
            color: "silver"
        }
    }
}
