// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    id: button
    signal clicked
    property alias text: buttonLabel.text

    height: Math.max(Screen.pixelDensity * 7, buttonLabel.implicitHeight * 1.2)
    width: Math.max(Screen.pixelDensity * 11, buttonLabel.implicitWidth * 1.3)
    radius: 3
    property color dark: Qt.darker(palette.button, 1.3)
    gradient: Gradient {
        GradientStop { position: 0.0; color: tapHandler.pressed ? dark : palette.button }
        GradientStop { position: 1.0; color: dark }
    }

    TapHandler {
        id: tapHandler
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onTapped: button.clicked()
    }

    Text {
        id: buttonLabel
        text: "Click Me"
        color: palette.buttonText
        anchors.centerIn: parent
    }
}
//![0]
