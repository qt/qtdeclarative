// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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

        swipe.left: null
        swipe.right: null
        swipe.behind: Rectangle {
            width: swipeDelegate.width
            height: swipeDelegate.height
            color: swipeDelegate.pressed ? "#333" : "#444"

            Label {
                text: "Behind Action"
                color: "#fff"
                anchors.centerIn: parent
            }
        }
    }
}
