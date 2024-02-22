// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.12

Item {
    id: container

    property alias text: buttonLabel.text
    property alias label: buttonLabel
    signal clicked
    property alias containsMouse: mouseArea.containsMouse
    property alias pressed: mouseArea.pressed
    implicitHeight: Math.max(Screen.pixelDensity * 7, buttonLabel.implicitHeight * 1.2)
    implicitWidth: Math.max(Screen.pixelDensity * 11, buttonLabel.implicitWidth * 1.3)
    height: implicitHeight
    width: implicitWidth

    SystemPalette { id: palette }

    Rectangle {
        id: frame
        anchors.fill: parent
        color: palette.button
        gradient: Gradient {
            GradientStop { position: 0.0; color: mouseArea.pressed ? Qt.darker(palette.button, 1.3) : palette.button }
            GradientStop { position: 1.0; color: Qt.darker(palette.button, 1.3) }
        }
        antialiasing: true
        radius: height / 6
        border.color: Qt.darker(palette.button, 1.5)
        border.width: 1
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: container.clicked()
        hoverEnabled: true
    }

    Text {
        id: buttonLabel
        text: container.text
        color: palette.buttonText
        anchors.centerIn: parent
    }
}
