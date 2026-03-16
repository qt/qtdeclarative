// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style: Style {

        //! [transition]
        comboBox {
            background.color: "lightgray"
            hovered.background.color: "plum"

            indicator.color: "white"
            hovered.indicator.color: "pink"
            hovered.indicator.border.width: 4

            transition: Transition {
                StyleAnimation {
                    animateBackgroundColors: true
                    animateIndicatorColors: true
                    animateIndicatorBorder: true
                    easing.type: Easing.OutQuad
                    duration: 500
                }
            }
        }
        //! [transition]

        //! [custom transition]
        checkBox {
            background.color: "lightgray"
            hovered.background.color: "plum"

            indicator.color: "white"
            hovered.indicator.color: "pink"
            hovered.indicator.border.width: 4

            transition: Transition {
                ColorAnimation {
                    properties: "background.color, background.border.color, background.image.color, background.shadow.color"
                    + ", indicator.color, indicator.border.color, indicator.image.color, indicator.shadow.color"
                    easing.type: Easing.OutQuad
                    duration: 500
                }
                NumberAnimation {
                    properties: "indicator.border.width"
                    easing.type: Easing.OutQuad
                    duration: 500
                }
            }
        }
        //! [custom transition]

        //! [mixed transition]
        slider {
            handle.color: "white"
            hovered.handle.color: "seagreen"
            hovered.handle.border.width: 8

            transition: Transition {
                StyleAnimation {
                    animateHandleColors: true
                    easing.type: Easing.OutQuad
                    duration: 500
                }
                NumberAnimation {
                    properties: "handle.border.width"
                    easing.type: Easing.OutBounce
                    duration: 1000
                }
            }
        }
        //! [mixed transition]
    }

    // The rest of the file is not a part of the docs. It just implements a small
    // UI to allow testing the style from the command line using the 'qml' app.

    ScrollView {
        anchors.fill: parent
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            ComboBox {
                model: ["apple", "banana", "orange"]
            }

            CheckBox {
                text: "CheckBox"
            }

            Slider {
                width: 200
            }
        }
    }
}
