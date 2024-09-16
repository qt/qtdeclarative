// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Controls

Rectangle {
    id: mainRectangle

    property string colorStringFormat: "#1CB669"

    signal onClicked()

    color: colorStringFormat

    Text {
        id: helloText

        text: "Second QML"
        color: "white"
        font.pixelSize: 72
        font.bold: true
        fontSizeMode: Text.VerticalFit
        horizontalAlignment: Text.AlignHCenter

        // Height is calculated based on display orientation
        // from Screen height, dividing numbers are based on what what seem
        // to look good on most displays
        height: Screen.width > Screen.height ? Screen.height / 8 : (Screen.height / 2) / 8

        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: 5
        }
    }

    Button {
        id: button

        anchors {
            horizontalCenter: parent.horizontalCenter
            top: helloText.bottom
            topMargin: height / 5
        }

        onClicked: mainRectangle.onClicked()

        background: Rectangle {
            id: buttonBackground

            radius: 14
            color: "#6200EE"
            opacity: button.down ? 0.6 : 1
            scale: button.down ? 0.9 : 1
        }

        contentItem: Text {
            id: buttonText

            text: "Change color"
            color: "white"
            font.pixelSize: 58
            minimumPixelSize: 10
            fontSizeMode: Text.Fit
            font.bold: true
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
