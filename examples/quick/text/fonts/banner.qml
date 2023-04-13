// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: screen

    property int pixelSize: screen.height * 1.25
    property color textColor: "lightsteelblue"
    readonly property string text: qsTr("Hello world! ")

    width: 320
    height: 480
    color: "steelblue"

    Row {
        y: -screen.height / 4.5

        NumberAnimation on x {
            from: 0
            to: -text.width
            duration: 6000
            loops: Animation.Infinite
        }
        Text {
            id: text
            font.pixelSize: screen.pixelSize
            color: screen.textColor
            text: screen.text
        }
        Text {
            font.pixelSize: screen.pixelSize
            color: screen.textColor
            text: screen.text
        }
        Text {
            font.pixelSize: screen.pixelSize
            color: screen.textColor
            text: screen.text
        }
    }
}
