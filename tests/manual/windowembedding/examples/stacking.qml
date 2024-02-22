// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: rootItem
    gradient: Gradient.TemptingAzure
    width: 500; height: 500

    property int windowZ: 0

    Window {
        id: redWindow
        color: "red"
        visible: true
        parent: rootItem

        MouseArea {
            anchors.fill: parent
            onClicked: redWindow.z = ++rootItem.windowZ
        }
    }

    Window {
        id: greenWindow
        color: "green"
        visible: true
        parent: rootItem
        x: 100; y: 100

        MouseArea {
            anchors.fill: parent
            onClicked: greenWindow.z = ++rootItem.windowZ
        }
    }

    Window {
        id: blueWindow
        color: "blue"
        visible: true
        parent: rootItem
        x: 200; y: 200

        MouseArea {
            anchors.fill: parent
            onClicked: blueWindow.z = ++rootItem.windowZ
        }
    }
}
