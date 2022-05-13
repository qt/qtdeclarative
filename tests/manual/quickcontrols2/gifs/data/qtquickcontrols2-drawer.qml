// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Window

ApplicationWindow {
    id: window
    width: 300
    height: 300
    visible: true

    property alias drawer: drawer

    Drawer {
        id: drawer
        width: window.width * 0.66
        height: window.height
        rightPadding: 0

        Rectangle {
            border.width: 1
            anchors.fill: parent

            Label {
                text: "Drawer"
                font.pixelSize: 32
                anchors.centerIn: parent
            }
        }
    }

    Rectangle {
        border.width: 1
        anchors.fill: parent

        Label {
            text: "Content"
            font.pixelSize: 32
            anchors.centerIn: parent
        }
    }

    Rectangle {
        z: 1
        color: "black"
        width: 1
        height: parent.height
        parent: window.overlay
    }
}
