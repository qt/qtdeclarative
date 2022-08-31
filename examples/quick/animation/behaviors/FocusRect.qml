// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: focusRect
    property string text

    x: 62
    y: 75
    width: 75
    height: 50
    radius: 6
    border.width: 4
    border.color: "white"
    color: "firebrick"

    // Set an 'elastic' behavior on the focusRect's x property.
    Behavior on x {
        NumberAnimation {
            easing.type: Easing.OutElastic
            easing.amplitude: 3.0
            easing.period: 2.0
            duration: 300
        }
    }

    //! [0]
    // Set an 'elastic' behavior on the focusRect's y property.
    Behavior on y {
        NumberAnimation {
            easing.type: Easing.OutElastic
            easing.amplitude: 3.0
            easing.period: 2.0
            duration: 300
        }
    }
    //! [0]

    Text {
        id: focusText
        text: focusRect.text
        anchors.centerIn: parent
        color: "white"
        font.pixelSize: 16
        font.bold: true

        // Set a behavior on the focusText's x property:
        // Set the opacity to 0, set the new text value, then set the opacity back to 1.
        Behavior on text {
            SequentialAnimation {
                NumberAnimation {
                    target: focusText
                    property: "opacity"
                    to: 0
                    duration: 150
                }
                NumberAnimation {
                    target: focusText
                    property: "opacity"
                    to: 1
                    duration: 150
                }
            }
        }
    }
}
