// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 640
    height: 480

    Text {
        id: headerText

        anchors {
            top: parent.top
            topMargin: 20
            left: parent.left
            leftMargin: 20
        }
        width: 500
        height: 50
        font.pixelSize: 30
        text: "QML Accessibility Tester"
        Accessible.role: Accessible.StaticText
        Accessible.name: text
        Accessible.description: text
    }

    //Component which holds basic items of the qml for the purpose of accessibility testing
    Rectangle {

        width: 500
        height: 400
        anchors {
            left: parent.left
            leftMargin: 20
            top: headerText.bottom
            topMargin: 10
        }
        radius: 5
        color: "#f5f5f5"
        border.color: "grey"
        border.width: 3

        Text {
            id: textBox
            width: 200
            height: 30
            anchors
            {
                left: parent.left
                leftMargin: 10
                top: parent.top
                topMargin: 30
            }
            text: " Placeholder for WASM Accessibility"

            Accessible.role: Accessible.StaticText
            Accessible.name: text
            Accessible.description: text
        }
    }
}
