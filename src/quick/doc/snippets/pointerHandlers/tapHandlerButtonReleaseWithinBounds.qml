// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
    width: 120; height: 80

    component Button : Rectangle {
        id: button
        signal clicked
        property alias text: buttonLabel.text

        width: 80
        height: 40
        radius: 3
        property color dark: Qt.darker(palette.button, 1.3)
        gradient: Gradient {
            GradientStop { position: 0.0; color: tapHandler.pressed ? dark : palette.button }
            GradientStop { position: 1.0; color: dark }
        }

        SequentialAnimation on border.width {
            id: tapFlash
            running: false
            loops: 3
            PropertyAction { value: 2 }
            PauseAnimation { duration: 100 }
            PropertyAction { value: 0 }
            PauseAnimation { duration: 100 }
        }

        //![1]
        TapHandler {
            id: tapHandler
            gesturePolicy: TapHandler.ReleaseWithinBounds
            onTapped: tapFlash.start()
        }
        //![1]

        Text {
            id: buttonLabel
            text: "Click Me"
            color: palette.buttonText
            anchors.centerIn: parent
        }
    }

    Button { x: 10; y: 10 }
}
//![0]
