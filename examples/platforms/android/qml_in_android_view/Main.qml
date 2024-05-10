// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls

Rectangle {
    id: mainRectangle

    property string colorStringFormat: "#1CB669"

    signal onClicked()

    color: colorStringFormat

    Text {
        id: helloText

        text: "QML"
        color: "white"
        font.pixelSize: 72
        fontSizeMode: Text.VerticalFit
        // Height is calculated based on display orientation
        // from Screen height, dividing numbers are based on what what seem
        // to look good on most displays
        height: Screen.width > Screen.height ? Screen.height / 8 : (Screen.height / 2) / 8
        font.bold: true
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 5
        horizontalAlignment: Text.AlignHCenter
    }


    Text {
        id: changeColorText

        text: "Tap button to change Java view background color"
        wrapMode: Text.Wrap
        color: "white"
        font.pixelSize: 58
        fontSizeMode: Text.Fit
        // Height and width are calculated based on display orientation
        // from Screen height and width, dividing numbers are based on what seem to
        // look good on most displays
        height: Screen.width > Screen.height ? Screen.height / 8 : (Screen.height / 2) / 8
        width: Screen.width > Screen.height ? (Screen.width / 2) / 2 : Screen.width / 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: helloText.bottom
        anchors.topMargin: Screen.height / 10
        horizontalAlignment: Text.AlignHCenter
    }

    Button {
        id: button
        // Width is calculated from changeColorText which is calculated from Screen size
        // dividing numbers are base on what seems to look good on most displays
        width: changeColorText.width / 1.6
        height: changeColorText.height * 1.2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: changeColorText.bottom
        anchors.topMargin: height / 5

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

            text: "CHANGE COLOR"
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
