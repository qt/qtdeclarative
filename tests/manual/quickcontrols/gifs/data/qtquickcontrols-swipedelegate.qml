// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: swipeDelegate.implicitWidth
    height: swipeDelegate.implicitHeight
    visible: true

    property alias swipeDelegate: swipeDelegate

    SwipeDelegate {
        id: swipeDelegate
        text: "SwipeDelegate"
        anchors.centerIn: parent

        swipe.left: Rectangle {
            width: swipeDelegate.width
            height: swipeDelegate.height
            color: swipeDelegate.pressed ? "#333" : "#444"

            Label {
                text: "Left Action"
                color: "#fff"
                anchors.centerIn: parent
            }
        }

        swipe.right: Rectangle {
            anchors.fill: parent
            width: swipeDelegate.width
            height: swipeDelegate.height
            color: swipeDelegate.pressed ? "#333" : "#444"

            Label {
                text: "Right Action"
                color: "#fff"
                anchors.centerIn: parent
            }
        }
    }
}
