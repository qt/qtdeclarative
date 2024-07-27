// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls

Rectangle {
    id: secondaryRectangle

    property int gridRotation: 0

    color: "blue"

    Text {
        id: title

        text: "Second QML component"
        color: "white"
        font.pixelSize: 72
        fontSizeMode: Text.VerticalFit
        // Height is calculated based on display orientation
        // from Screen height, dividing numbers are based on what seem
        // to look good on most displays
        height: Screen.width > Screen.height ? Screen.height / 8 : (Screen.height / 2) / 8
        font.bold: true
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 5
        horizontalAlignment: Text.AlignHCenter
    }

    Text {
        id: gridText

        text: "QML Grid type"
        fontSizeMode: Text.VerticalFit
        font.pixelSize: 48
        color: "white"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: title.bottom
        anchors.topMargin: 100
    }

    Grid {
        id: grid

        columns: 3
        rows: 3
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: gridText.bottom
        anchors.topMargin: 50
        spacing: 50
        rotation: gridRotation

        Repeater {
            id: repeater

            model: [
                "green",
                "lightblue",
                "grey",
                "red",
                "black",
                "white",
                "pink",
                "yellow",
                "orange"
            ]

            Rectangle {
                required property string modelData

                height: 50
                width: 50
                color: modelData
            }
        }
    }
}
