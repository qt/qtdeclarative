// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QmltcExample // application's own QML module

Rectangle {
    id: window

    width: 640
    height: 480
    focus: true
    color: "#F9F4EC"

    readonly property color textColor: "#601A4A"

    Row {
        id: row
        anchors.centerIn: window
        spacing: 10

        Column {
            id: column
            spacing: 5

            Text {
                text: "Hello, QML World!"
                font.pixelSize: slider.value
                color: textColor
            }

            MySlider {
                id: slider
                from: 20
                value: 20
                to: 30
            }
        }

        Column {
            spacing: 5

            Text {
                id: rndText
                font.pixelSize: 25
                color: textColor
                text: "0.00"
            }

            Rectangle {
                id: rndColorRect
                height: 20
                width: rndButton.width
                color: "black"

                MyColorPicker { // comes from C++
                    id: colorPicker
                    onEncodedColorChanged: rndColorRect.color = colorPicker.decodeColor()
                }
            }

            MyButton {
                id: rndButton
                text: "PICK"
                onClicked: function() {
                    var value = Math.random();
                    rndText.text = value.toFixed(rndButton.text.length - 2);
                    colorPicker.encodedColor = value;
                }
            }
        }
    }
}
