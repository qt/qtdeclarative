// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root
    property alias label: label
    property alias text: label.text
    property alias pressed: tap.pressed
    property alias hovered: hoverHandler.hovered
    property alias gesturePolicy: tap.gesturePolicy
    property alias margin: tap.margin
    property alias exclusiveSignals: tap.exclusiveSignals
    signal tapped

    implicitHeight: Math.max(Screen.pixelDensity * 7, label.implicitHeight * 2)
    implicitWidth: Math.max(Screen.pixelDensity * 11, label.implicitWidth * 1.3)
    height: implicitHeight
    width: implicitWidth
    radius: height / 6

    border { color: Qt.darker(palette.button, 1.5); width: 1 }
    gradient: Gradient {
            GradientStop { position: 0.0; color: tap.pressed ? Qt.darker(palette.button, 1.3) : palette.button }
            GradientStop { position: 1.0; color: Qt.darker(palette.button, 1.3) }
    }

    TapHandler {
        id: tap
        margin: 10 // the user can tap a little beyond the edges
        objectName: label.text + " Tap"
        onSingleTapped: {
            tapFlash.start()
            root.tapped()
        }
    }
    HoverHandler {
        id: hoverHandler
    }

    Text {
        id: label
        font.pointSize: 14
        color: palette.buttonText
        text: "Button"
        anchors.centerIn: parent
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: 2; radius: root.radius; antialiasing: true
        opacity: tapFlash.running ? 1 : 0
        FlashAnimation on visible {
            id: tapFlash
        }
    }
}
