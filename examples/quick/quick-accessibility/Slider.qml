// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

// Minimal slider implementation
Rectangle {
    id: slider

    property alias text: buttonText.text
    Accessible.role: Accessible.Slider

    property int value: 5         // required
    property int minimumValue: 0  // optional (default INT_MIN)
    property int maximumValue: 20 // optional (default INT_MAX)
    property int stepSize: 1      // optional (default 1)

    width: 100
    height: 30
    border.color: "black"
    border.width: 1

    Rectangle {
        id: indicator
        x: 1
        y: 1
        height: parent.height - 2
        width: ((parent.width - 2) / slider.maximumValue) * slider.value
        color: "lightgrey"
        Behavior on width {
            NumberAnimation { duration: 50 }
        }
    }

    Text {
        id: buttonText
        text: parent.value
        anchors.centerIn: parent
        font.pixelSize: parent.height * .5
    }

    MouseArea {
        anchors.fill: parent
        onClicked: (mouse) => {
            var pos = mouse.x / slider.width * (slider.maximumValue - slider.minimumValue)
                    + slider.minimumValue
            slider.value = pos
        }
    }

    Keys.onLeftPressed: value > minimumValue ? value = value - stepSize : minimumValue
    Keys.onRightPressed: value < maximumValue ? value = value + stepSize : maximumValue
}
